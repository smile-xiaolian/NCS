"""训练 / 评估 / 预测编排，并把结果写成大屏可读的 prediction.json。"""

from __future__ import annotations

import json
import sqlite3
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any

from dataio import export_from_db, load_hourly_csv, write_hourly_csv
from features import build_samples, feature_vector, format_time, is_peak_hour, load_holidays
from modeler import empty_metrics, evaluate, load_model, print_metrics, train

ML_ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ML_ROOT.parent
DEFAULT_DATASET = ML_ROOT / "dataset" / "hourly_load.csv"
DEFAULT_HOLIDAYS = ML_ROOT / "dataset" / "holidays.csv"
DEFAULT_MODEL = ML_ROOT / "models" / "load_rf.pkl"
DEFAULT_WEB_OUT = PROJECT_ROOT / "web" / "public" / "data" / "prediction.json"
CHINA_TZ = timezone(timedelta(hours=8))
SCHEMA = "ncs.load_prediction.v1"


def now_china() -> datetime:
    return datetime.now(CHINA_TZ).replace(tzinfo=None, minute=0, second=0, microsecond=0)


def empty_prediction(message: str, metrics: dict[str, Any] | None = None) -> dict[str, Any]:
    return {
        "schema": SCHEMA,
        "generated_at": format_time(datetime.now(CHINA_TZ).replace(tzinfo=None)),
        "timezone": "GMT+8",
        "status": "empty",
        "message": message,
        "metrics": metrics or empty_metrics(),
        "horizon_hours": 24,
        "series": [],
        "by_station": [],
        "history": [],
    }


def write_prediction(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"已写入大屏预测文件：{path}")


def resolve_rows(dataset: Path, db_path: str | None) -> tuple[list[dict[str, Any]], dict[int, str], dict[int, int]]:
    names: dict[int, str] = {}
    chargers: dict[int, int] = {}
    rows: list[dict[str, Any]] = []
    if db_path:
        rows, names, chargers = export_from_db(db_path)
    if not rows:
        rows = load_hourly_csv(dataset)
        for row in rows:
            names.setdefault(int(row["station_id"]), str(row.get("station_name") or ""))
    else:
        for row in rows:
            names.setdefault(int(row["station_id"]), str(row.get("station_name") or ""))
    return rows, names, chargers


def export_dataset(db_path: str, dataset: Path) -> int:
    rows, _, _ = export_from_db(db_path)
    write_hourly_csv(dataset, rows)
    print(f"已导出 {len(rows)} 条小时负荷到 {dataset}")
    return len(rows)


def run_train(rows: list[dict[str, Any]], holidays: set[str], model_path: Path):
    features, labels, _ = build_samples(rows, holidays)
    model, x, y = train(features, labels, model_path)
    print(f"训练完成，样本 {len(labels)} 条，模型写入 {model_path}")
    return model, features, labels


def run_evaluate(model, rows: list[dict[str, Any]], holidays: set[str]) -> dict[str, Any]:
    features, labels, _ = build_samples(rows, holidays)
    metrics = evaluate(model, features, labels)
    print_metrics(metrics)
    return metrics


def forecast(
    model,
    rows: list[dict[str, Any]],
    holidays: set[str],
    names: dict[int, str],
    chargers: dict[int, int],
    start: datetime | None = None,
) -> dict[str, Any]:
    if not rows:
        return empty_prediction("训练集为空，已预留 hourly_load.csv 格式。写入小时负荷后重新运行 --predict。")

    buckets = {(row["station_id"], row["datetime"]): float(row["energy"]) for row in rows}
    last_stamp = max(row["datetime"] for row in rows)
    origin = start or (last_stamp + timedelta(hours=1))
    station_ids = sorted({int(row["station_id"]) for row in rows})
    history_energy = [float(row["energy"]) for row in rows]
    peak_threshold = sorted(history_energy)[int(len(history_energy) * 0.75)] if history_energy else None

    by_station = []
    totals: dict[str, dict[str, Any]] = {}
    for station_id in station_ids:
        points = []
        for offset in range(24):
            target = origin + timedelta(hours=offset)
            vector = feature_vector(station_id, target, buckets, holidays)
            energy = max(0.0, float(model.predict([vector])[0]))
            buckets[(station_id, target)] = energy
            peak = is_peak_hour(target, energy, peak_threshold)
            online = chargers.get(station_id, 8)
            idle = max(0, int(online - round(energy / 15)))
            point = {
                "target_time": format_time(target),
                "predicted_energy": round(energy, 2),
                "predicted_idle": idle,
                "is_peak": peak,
            }
            points.append(point)
            slot = totals.setdefault(
                point["target_time"],
                {"target_time": point["target_time"], "predicted_energy": 0.0, "predicted_idle": 0, "is_peak": 0},
            )
            slot["predicted_energy"] = round(slot["predicted_energy"] + energy, 2)
            slot["predicted_idle"] += idle
            slot["is_peak"] = max(slot["is_peak"], peak)
        by_station.append(
            {
                "station_id": station_id,
                "station_name": names.get(station_id, ""),
                "points": points,
            }
        )

    series = [totals[key] for key in sorted(totals)]
    history = _history_tail(rows, names)
    return {
        "schema": SCHEMA,
        "generated_at": format_time(datetime.now(CHINA_TZ).replace(tzinfo=None)),
        "timezone": "GMT+8",
        "status": "ok",
        "message": "未来 24 小时负荷预测已生成",
        "metrics": empty_metrics(len(rows)),
        "horizon_hours": 24,
        "series": series,
        "by_station": by_station,
        "history": history,
    }


def _history_tail(rows: list[dict[str, Any]], names: dict[int, str], hours: int = 24) -> list[dict[str, Any]]:
    if not rows:
        return []
    last = max(row["datetime"] for row in rows)
    start = last - timedelta(hours=hours - 1)
    totals: dict[str, float] = {}
    for row in rows:
        if row["datetime"] < start:
            continue
        key = format_time(row["datetime"])
        totals[key] = round(totals.get(key, 0.0) + float(row["energy"]), 2)
    return [{"target_time": key, "actual_energy": totals[key]} for key in sorted(totals)]


def write_db(db_path: str, payload: dict[str, Any]) -> None:
    if payload.get("status") != "ok":
        print("预测为空，跳过回写 load_prediction 表")
        return
    conn = sqlite3.connect(db_path)
    try:
        conn.execute("DELETE FROM load_prediction")
        for station in payload.get("by_station") or []:
            for point in station.get("points") or []:
                conn.execute(
                    "INSERT INTO load_prediction(station_id,target_time,predicted_energy,predicted_idle,is_peak) VALUES(?,?,?,?,?)",
                    (
                        int(station["station_id"]),
                        point["target_time"],
                        float(point["predicted_energy"]),
                        int(point["predicted_idle"]),
                        int(point["is_peak"]),
                    ),
                )
        conn.commit()
        print("已回写 load_prediction 表。")
    finally:
        conn.close()
