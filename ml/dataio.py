"""小时负荷数据集读写。CSV 可为空，仅保留表头。"""

from __future__ import annotations

import csv
import sqlite3
from datetime import datetime, timedelta
from pathlib import Path
from typing import Any

TIME_FORMAT = "%Y-%m-%d %H:%M:%S"
CSV_FIELDS = ("station_id", "station_name", "datetime", "energy")


def parse_time(text: str) -> datetime:
    return datetime.strptime(str(text).strip().replace("T", " "), TIME_FORMAT)


def floor_hour(stamp: datetime) -> datetime:
    return stamp.replace(minute=0, second=0, microsecond=0)


def load_hourly_csv(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    rows: list[dict[str, Any]] = []
    with path.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames:
            return []
        for raw in reader:
            if not raw or not any((value or "").strip() for value in raw.values()):
                continue
            station = str(raw.get("station_id", "")).strip()
            stamp = str(raw.get("datetime", "")).strip()
            if not station or not stamp:
                continue
            item: dict[str, Any] = {
                "station_id": int(station),
                "station_name": str(raw.get("station_name") or "").strip(),
                "datetime": floor_hour(parse_time(stamp)),
                "energy": float(raw.get("energy") or 0),
            }
            if raw.get("temp_c") not in (None, ""):
                item["temp_c"] = float(raw["temp_c"])
            if raw.get("is_rain") not in (None, ""):
                item["is_rain"] = int(raw["is_rain"])
            rows.append(item)
    rows.sort(key=lambda item: (item["station_id"], item["datetime"]))
    return rows


def write_hourly_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = list(CSV_FIELDS)
    if any("temp_c" in row for row in rows):
        fieldnames.append("temp_c")
    if any("is_rain" in row for row in rows):
        fieldnames.append("is_rain")
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            payload = {
                "station_id": row["station_id"],
                "station_name": row.get("station_name", ""),
                "datetime": row["datetime"].strftime(TIME_FORMAT) if isinstance(row["datetime"], datetime) else row["datetime"],
                "energy": row.get("energy", 0),
            }
            if "temp_c" in row:
                payload["temp_c"] = row["temp_c"]
            if "is_rain" in row:
                payload["is_rain"] = row["is_rain"]
            writer.writerow(payload)


def export_from_db(db_path: str) -> tuple[list[dict[str, Any]], dict[int, str], dict[int, int]]:
    """从 charging_order 按 (电站, 小时) 聚合，并补齐中间缺测小时。"""
    conn = sqlite3.connect(db_path)
    try:
        names = {
            int(row[0]): str(row[1] or "")
            for row in conn.execute("SELECT id, name FROM station")
        }
        chargers = {
            int(row[0]): int(row[1])
            for row in conn.execute(
                "SELECT station_id, COUNT(*) FROM charger WHERE status != 2 GROUP BY station_id"
            )
        }
        buckets: dict[tuple[int, datetime], float] = {}
        for station_id, name, end_time, energy in conn.execute(
            """
            SELECT c.station_id, s.name, o.end_time, o.energy
            FROM charging_order AS o
            JOIN charger AS c ON c.id = o.charger_id
            JOIN station AS s ON s.id = c.station_id
            WHERE o.status = 2 AND o.end_time IS NOT NULL
            """
        ):
            stamp = floor_hour(parse_time(str(end_time)))
            key = (int(station_id), stamp)
            buckets[key] = buckets.get(key, 0.0) + float(energy or 0)
            names[int(station_id)] = str(name or names.get(int(station_id), ""))
    finally:
        conn.close()

    rows: list[dict[str, Any]] = []
    stations = sorted({key[0] for key in buckets})
    for station_id in stations:
        hours = sorted(stamp for sid, stamp in buckets if sid == station_id)
        if not hours:
            continue
        cursor = hours[0]
        last = hours[-1]
        while cursor <= last:
            rows.append(
                {
                    "station_id": station_id,
                    "station_name": names.get(station_id, ""),
                    "datetime": cursor,
                    "energy": round(buckets.get((station_id, cursor), 0.0), 4),
                }
            )
            cursor += timedelta(hours=1)
    return rows, names, chargers
