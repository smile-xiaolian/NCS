"""UC-M-01 特征：时间、节假日、滞后负荷、滑动均值、电站编码、模拟天气。"""

from __future__ import annotations

import csv
import math
from datetime import datetime, timedelta
from pathlib import Path
from typing import Any

from dataio import TIME_FORMAT

FEATURE_NAMES = [
    "station_code",
    "hour",
    "weekday",
    "is_weekend",
    "is_holiday",
    "lag_1h",
    "lag_24h",
    "lag_168h",
    "rolling_mean",
    "temp_c",
    "is_rain",
]

PEAK_HOURS = {8, 9, 10, 17, 18, 19, 20}


def load_holidays(path: Path) -> set[str]:
    if not path.exists():
        return set()
    days: set[str] = set()
    with path.open(encoding="utf-8-sig", newline="") as handle:
        for raw in csv.DictReader(handle):
            date = str(raw.get("date") or "").strip()
            if date:
                days.add(date)
    return days


def simulate_weather(station_id: int, stamp: datetime) -> tuple[float, int]:
    day = stamp.timetuple().tm_yday
    seasonal = 16 + 12 * math.sin(2 * math.pi * (day - 80) / 365)
    diurnal = 3 * math.sin(2 * math.pi * (stamp.hour - 7) / 24)
    seed = (int(station_id) * 1000 + day * 24 + stamp.hour) % 97
    noise = (seed / 97 - 0.5) * 4
    temp = round(seasonal + diurnal + noise, 1)
    is_rain = 1 if seed % 8 == 0 else 0
    return temp, is_rain


def is_peak_hour(stamp: datetime, energy: float = 0.0, threshold: float | None = None) -> int:
    if stamp.hour in PEAK_HOURS:
        return 1
    if threshold is not None and energy >= threshold:
        return 1
    return 0


def build_samples(
    rows: list[dict[str, Any]], holidays: set[str]
) -> tuple[list[list[float]], list[float], list[dict[str, Any]]]:
    buckets = {(row["station_id"], row["datetime"]): float(row["energy"]) for row in rows}
    features: list[list[float]] = []
    labels: list[float] = []
    meta: list[dict[str, Any]] = []
    for row in rows:
        stamp: datetime = row["datetime"]
        station_id = int(row["station_id"])
        lag_1 = buckets.get((station_id, stamp - timedelta(hours=1)), 0.0)
        lag_24 = buckets.get((station_id, stamp - timedelta(hours=24)), 0.0)
        lag_168 = buckets.get((station_id, stamp - timedelta(hours=168)), 0.0)
        temp, rain = (
            (float(row["temp_c"]), int(row["is_rain"]))
            if "temp_c" in row and "is_rain" in row
            else simulate_weather(station_id, stamp)
        )
        vector = [
            float(station_id),
            float(stamp.hour),
            float(stamp.weekday()),
            float(int(stamp.weekday() >= 5)),
            float(int(stamp.strftime("%Y-%m-%d") in holidays)),
            lag_1,
            lag_24,
            lag_168,
            (lag_1 + lag_24 + lag_168) / 3,
            temp,
            float(rain),
        ]
        features.append(vector)
        labels.append(float(row["energy"]))
        meta.append(
            {
                "station_id": station_id,
                "station_name": row.get("station_name", ""),
                "datetime": stamp,
            }
        )
    return features, labels, meta


def feature_vector(
    station_id: int,
    stamp: datetime,
    buckets: dict[tuple[int, datetime], float],
    holidays: set[str],
    weather: tuple[float, int] | None = None,
) -> list[float]:
    temp, rain = weather or simulate_weather(station_id, stamp)
    lag_1 = buckets.get((station_id, stamp - timedelta(hours=1)), 0.0)
    lag_24 = buckets.get((station_id, stamp - timedelta(hours=24)), 0.0)
    lag_168 = buckets.get((station_id, stamp - timedelta(hours=168)), 0.0)
    return [
        float(station_id),
        float(stamp.hour),
        float(stamp.weekday()),
        float(int(stamp.weekday() >= 5)),
        float(int(stamp.strftime("%Y-%m-%d") in holidays)),
        lag_1,
        lag_24,
        lag_168,
        (lag_1 + lag_24 + lag_168) / 3,
        temp,
        float(rain),
    ]


def format_time(stamp: datetime) -> str:
    return stamp.strftime(TIME_FORMAT)
