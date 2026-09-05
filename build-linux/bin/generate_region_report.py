#!/usr/bin/env python3
"""根据 charge_platform.db 生成按省/市/区聚合的上报 JSON。

默认只使用 Python 标准库，不依赖第三方包。
"""

from __future__ import annotations

import argparse
import json
import re
import sqlite3
import sys
import time
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any


def find_project_root() -> Path:
    """根据脚本位置向上查找项目根目录，兼容源码运行和 build 目录运行。"""
    current = Path(__file__).resolve().parent
    for candidate in (current, *current.parents):
        if (candidate / "CMakeLists.txt").exists() and (candidate / "scripts").exists():
            return candidate
    return current.parent


PROJECT_ROOT = find_project_root()

DEFAULT_DB = str(Path.home() / ".local" / "share" / "NCS" / "charge_platform.db")
DEFAULT_OUT = str(PROJECT_ROOT / "charge_report.json")

CHINA_TZ = timezone(timedelta(hours=8))
TIME_FORMAT = "%Y-%m-%d %H:%M:%S"

# 数据库 charger.status 与上报字段的映射。
# 可根据实际业务调整：这里把 0 当作 online，1 当作 offline，2 当作 broken。
STATUS_LABELS = {
    0: "online",
    1: "offline",
    2: "broken",
}
STATUS_ORDER = ["online", "offline", "broken"]

# 当 station 表没有 province/city/district 列，且地址也解析不出省市时使用。
DEFAULT_PROVINCE = "上海市"
DEFAULT_CITY = "上海市"

MUNICIPALITIES = {"北京市", "上海市", "天津市", "重庆市"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", default=DEFAULT_DB, help="SQLite 数据库路径")
    parser.add_argument("--out", default=DEFAULT_OUT, help="输出 JSON 文件路径")
    parser.add_argument("--report-id", default=None, help="ReportID，默认使用 10 位 Unix 秒级时间戳")
    parser.add_argument("--datetime", dest="report_datetime", default=None,
                        help="报表时间，格式 yyyy-MM-dd HH:mm:ss；默认取当前 GMT+8 时间")
    parser.add_argument("--hours", type=int, default=24, help="统计最近多少小时，默认 24")
    parser.add_argument("--region-map", default=None,
                        help="可选的地区映射 JSON 文件，按 station id 覆盖 Province/City/District")
    parser.add_argument("--interval", type=int, default=30,
                        help="循环生成时的间隔秒数，默认 30")
    parser.add_argument("--once", action="store_true",
                        help="只执行一次，不进入循环")
    return parser.parse_args()


def get_table_columns(conn: sqlite3.Connection, table: str) -> set[str]:
    rows = conn.execute(f"PRAGMA table_info({table})").fetchall()
    return {row[1] for row in rows}


def parse_region(address: str | None) -> dict[str, str]:
    """从 station.address 中尽量解析出省/市/区。

    项目里 station 表目前没有 province/city/district 字段，因此这个函数是一种
    兼容策略；如果地址里带“省/市/区”，直接拆开，否则使用默认上海地区。
    """
    text = re.sub(r"\s+", "", address or "")
    province = city = district = ""

    # 示例：
    # “广东省广州市天河区xx路” -> 广东省 / 广州市 / 天河区
    # “上海市黄浦区人民大道”  -> 上海市 / 上海市 / 黄浦区
    # “黄浦区人民大道”        -> 默认省 / 默认市 / 黄浦区
    match = re.match(
        r"^(?:(?P<province>[^省]+省))?(?:(?P<city>[^市]+市))?(?P<district>[^区县]+[区县])?",
        text,
    )
    if match:
        province = match.group("province") or ""
        city = match.group("city") or ""
        district = match.group("district") or ""

    if not province and city in MUNICIPALITIES:
        province = city
    if not city and not province:
        province, city = DEFAULT_PROVINCE, DEFAULT_CITY
    if not district:
        district = "未知区域"

    return {
        "Province": province or DEFAULT_PROVINCE,
        "City": city or DEFAULT_CITY,
        "District": district,
    }


def load_region_overrides(path: str | None) -> dict[int, dict[str, str]]:
    if not path:
        return {}
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    result: dict[int, dict[str, str]] = {}
    for station_id, region in data.items():
        result[int(station_id)] = {
            "Province": str(region.get("Province", "")),
            "City": str(region.get("City", "")),
            "District": str(region.get("District", "")),
        }
    return result


def normalize_status(value: Any) -> int:
    if isinstance(value, str):
        value = value.strip().lower()
        status_by_text = {v: k for k, v in STATUS_LABELS.items()}
        return status_by_text.get(value, 0)
    return int(value)


def fetch_stations(
    conn: sqlite3.Connection, region_overrides: dict[int, dict[str, str]]
) -> list[dict[str, Any]]:
    station_columns = get_table_columns(conn, "station")
    has_region_columns = {"province", "city", "district"}.issubset(
        {c.lower() for c in station_columns}
    )

    select_columns = ["id", "name", "address"]
    if has_region_columns:
        select_columns = ["id", "name", "address", "province", "city", "district"]
    sql = f"SELECT {', '.join(select_columns)} FROM station ORDER BY id"

    stations: list[dict[str, Any]] = []
    for row in conn.execute(sql):
        record = dict(zip(select_columns, row))
        station_id = int(record["id"])
        if station_id in region_overrides:
            region = region_overrides[station_id]
        elif has_region_columns:
            region = {
                "Province": str(record.get("province") or DEFAULT_PROVINCE),
                "City": str(record.get("city") or DEFAULT_CITY),
                "District": str(record.get("district") or "未知区域"),
            }
        else:
            region = parse_region(record.get("address"))

        stations.append({"id": station_id, "region": region})
    return stations


def fetch_charger_counts(conn: sqlite3.Connection) -> dict[tuple[int, int], int]:
    counts: dict[tuple[int, int], int] = {}
    for station_id, raw_status, num in conn.execute(
        "SELECT station_id, status, COUNT(*) FROM charger GROUP BY station_id, status"
    ):
        status = normalize_status(raw_status)
        counts[(int(station_id), status)] = int(num)
    return counts


def fetch_orders_by_station(conn: sqlite3.Connection, cutoff: str) -> dict[int, dict[str, float | int]]:
    result: dict[int, dict[str, float | int]] = {}
    for station_id, orders, income in conn.execute(
        """
        SELECT c.station_id, COUNT(*), COALESCE(SUM(o.amount), 0)
        FROM charging_order AS o
        JOIN charger AS c ON c.id = o.charger_id
        WHERE o.status = 2 AND o.end_time >= ?
        GROUP BY c.station_id
        """,
        (cutoff,),
    ):
        result[int(station_id)] = {"orders": int(orders), "income": float(income)}
    return result


def fetch_registered_users_by_region(
    conn: sqlite3.Connection, cutoff: str
) -> tuple[dict[tuple[str, str, str], int], bool]:
    """统计最近 24 小时新增用户。

    当前 user 表没有地区字段，所以只有用户表确实含 province/city/district 时，
    才能精确按区统计；否则会回退为“无法按区归属”，每个区显示 0。
    """
    user_columns = {c.lower() for c in get_table_columns(conn, "user")}
    if {"province", "city", "district"}.issubset(user_columns):
        result: dict[tuple[str, str, str], int] = {}
        for province, city, district, num in conn.execute(
            """
            SELECT province, city, district, COUNT(*)
            FROM user
            WHERE created_at >= ?
            GROUP BY province, city, district
            """,
            (cutoff,),
        ):
            result[(str(province), str(city), str(district))] = int(num)
        return result, True

    # user 表没有 region 字段时，不把全局新增用户错误地塞到某个区。
    return {}, False


def build_report(args: argparse.Namespace) -> dict[str, Any]:
    conn = sqlite3.connect(args.db)
    conn.row_factory = sqlite3.Row
    try:
        region_overrides = load_region_overrides(args.region_map)
        stations = fetch_stations(conn, region_overrides)
        charger_counts = fetch_charger_counts(conn)

        report_time = datetime.now(CHINA_TZ)
        if args.report_datetime:
            report_time = datetime.strptime(args.report_datetime, TIME_FORMAT).replace(
                tzinfo=CHINA_TZ
            )
        cutoff = report_time - timedelta(hours=args.hours)
        cutoff_str = cutoff.strftime(TIME_FORMAT)

        order_by_station = fetch_orders_by_station(conn, cutoff_str)
        users_by_region, users_can_be_grouped = fetch_registered_users_by_region(
            conn, cutoff_str
        )

        # 同一个省/市/区下可能有多个电站，这里按地区聚合。
        region_order: list[tuple[str, str, str]] = []
        region_chargers: dict[tuple[str, str, str], dict[str, int]] = {}
        region_orders: dict[tuple[str, str, str], int] = {}
        region_income: dict[tuple[str, str, str], float] = {}

        for station in stations:
            station_id = station["id"]
            region = station["region"]
            region_key = (region["Province"], region["City"], region["District"])

            if region_key not in region_orders:
                region_order.append(region_key)
                region_chargers[region_key] = {label: 0 for label in STATUS_ORDER}
                region_orders[region_key] = 0
                region_income[region_key] = 0.0

            for label in STATUS_ORDER:
                status_value = next(k for k, v in STATUS_LABELS.items() if v == label)
                region_chargers[region_key][label] += charger_counts.get(
                    (station_id, status_value), 0
                )

            orders = order_by_station.get(station_id, {"orders": 0, "income": 0.0})
            region_orders[region_key] += int(orders["orders"])
            region_income[region_key] += float(orders["income"])

        data: list[dict[str, Any]] = []
        for region_key in region_order:
            province, city, district = region_key
            registered_users = users_by_region.get(region_key, 0) if users_can_be_grouped else 0
            data.append(
                {
                    "Province": province,
                    "City": city,
                    "District": district,
                    "ChargerStatus": [
                        {"Status": label, "ChargerNum": region_chargers[region_key][label]}
                        for label in STATUS_ORDER
                    ],
                    "24hChargedOrder": region_orders[region_key],
                    "24hTotalIncome": round(region_income[region_key], 2),
                    "24hRegisteredUser": registered_users,
                }
            )

        report_id = args.report_id
        if report_id is None:
            report_id = str(int(time.time()))

        return {
            "ReportID": report_id,
            "timezone": "GMT+8",
            "datetime": report_time.strftime(TIME_FORMAT),
            "data": data,
        }
    finally:
        conn.close()


def write_once(args: argparse.Namespace) -> None:
    report = build_report(args)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(
        json.dumps(report, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    stamp = datetime.now(CHINA_TZ).strftime(TIME_FORMAT)
    print(f"[{stamp}] report written to {out_path}")


def main() -> None:
    args = parse_args()
    if args.once:
        write_once(args)
        return

    interval = max(1, args.interval)
    while True:
        try:
            write_once(args)
        except KeyboardInterrupt:
            break
        except Exception as exc:
            stamp = datetime.now(CHINA_TZ).strftime(TIME_FORMAT)
            print(f"[{stamp}] report generation failed: {exc}", file=sys.stderr)
        time.sleep(interval)


if __name__ == "__main__":
    main()
