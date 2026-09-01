#!/usr/bin/env python3
"""NCS 负荷预测：固定随机种子、RandomForestRegressor、SQLite 回写。"""
import argparse
import os
import sqlite3
from datetime import datetime, timedelta

import joblib
import numpy as np
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error, mean_squared_error

RANDOM_SEED = 42
MODEL_PATH = os.path.join(os.path.dirname(__file__), "models", "load_rf.pkl")

def load_rows(conn):
    sql = """SELECT c.station_id, o.end_time, o.energy
             FROM charging_order o JOIN charger c ON c.id=o.charger_id
             WHERE o.status=2 AND o.end_time IS NOT NULL ORDER BY o.end_time"""
    return conn.execute(sql).fetchall()

def samples(rows):
    buckets = {}
    for station, text, energy in rows:
        dt = datetime.strptime(text, "%Y-%m-%d %H:%M:%S").replace(minute=0, second=0)
        buckets[(station, dt)] = buckets.get((station, dt), 0.0) + float(energy or 0)
    result = []
    for (station, dt), value in buckets.items():
        def lag(hours): return buckets.get((station, dt - timedelta(hours=hours)), 0.0)
        result.append(([station, dt.hour, dt.weekday(), int(dt.weekday() >= 5), lag(1), lag(24), lag(168), (lag(1)+lag(24)+lag(168))/3], value))
    return sorted(result, key=lambda x: x[0][0])

def train(data):
    if len(data) < 10:
        raise RuntimeError("历史订单不足，至少需要 10 条已完成订单")
    x = np.array([item[0] for item in data]); y = np.array([item[1] for item in data])
    model = RandomForestRegressor(n_estimators=180, max_depth=12, random_state=RANDOM_SEED, n_jobs=-1)
    model.fit(x, y)
    os.makedirs(os.path.dirname(MODEL_PATH), exist_ok=True)
    joblib.dump(model, MODEL_PATH)
    return model, x, y

def evaluate(model, x, y):
    split = max(1, int(len(x) * .8))
    actual = y[split:]; predicted = model.predict(x[split:])
    if len(actual) == 0: actual, predicted = y, model.predict(x)
    mae = mean_absolute_error(actual, predicted)
    rmse = mean_squared_error(actual, predicted) ** .5
    mape = np.mean(np.abs((actual-predicted) / np.maximum(actual, .01))) * 100
    baseline = x[split:, 5] if len(x[split:]) else x[:, 5]
    base_mae = mean_absolute_error(actual, baseline)
    print(f"MAE={mae:.3f}, RMSE={rmse:.3f}, MAPE={mape:.2f}%, 上周同刻基线 MAE={base_mae:.3f}")

def predict(conn, model, data):
    station_ids = [row[0] for row in conn.execute("SELECT id FROM station")]
    latest = {(features[0],): features for features, _ in data}
    now = datetime.now().replace(minute=0, second=0, microsecond=0)
    conn.execute("DELETE FROM load_prediction")
    for station in station_ids:
        base = latest.get((station,), [station, now.hour, now.weekday(), 0, 0, 0, 0, 0])
        for offset in range(1, 25):
            target = now + timedelta(hours=offset)
            features = [station, target.hour, target.weekday(), int(target.weekday() >= 5), base[4], base[5], base[6], base[7]]
            energy = max(0.0, float(model.predict(np.array([features]))[0]))
            peak = int(target.hour in (8, 9, 10, 17, 18, 19, 20))
            idle = max(0, 8 - int(round(energy / 15)))
            conn.execute("INSERT INTO load_prediction(station_id,target_time,predicted_energy,predicted_idle,is_peak) VALUES(?,?,?,?,?)", (station, target.strftime("%Y-%m-%d %H:%M:%S"), round(energy, 2), idle, peak))
    conn.commit()
    print("已回写未来 24 小时预测结果。")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--db", required=True, help="charge_platform.db 的绝对路径")
    parser.add_argument("--train", action="store_true")
    parser.add_argument("--predict", action="store_true")
    parser.add_argument("--evaluate", action="store_true")
    args = parser.parse_args()
    if not (args.train or args.predict or args.evaluate): parser.error("请至少指定 --train、--predict 或 --evaluate")
    conn = sqlite3.connect(args.db); data = samples(load_rows(conn))
    if args.train or args.evaluate or not os.path.exists(MODEL_PATH): model, x, y = train(data)
    else: model = joblib.load(MODEL_PATH); x = y = None
    if args.evaluate: evaluate(model, x, y)
    if args.predict: predict(conn, model, data)

if __name__ == "__main__": main()
