"""RandomForest 训练、评估与落盘。随机种子固定为 42。"""

from __future__ import annotations

from pathlib import Path

import numpy as np

RANDOM_SEED = 42
MIN_SAMPLES = 10


def _require_sklearn():
    try:
        from sklearn.ensemble import RandomForestRegressor
        from sklearn.metrics import mean_absolute_error, mean_squared_error
        import joblib
    except ImportError as exc:
        raise SystemExit("缺少依赖，请先执行：pip install -r ml/requirements.txt") from exc
    return RandomForestRegressor, mean_absolute_error, mean_squared_error, joblib


def train(features: list[list[float]], labels: list[float], model_path: Path):
    if len(labels) < MIN_SAMPLES:
        raise RuntimeError(f"历史样本不足 {MIN_SAMPLES} 条，当前 {len(labels)} 条，已跳过训练")
    RandomForestRegressor, _, _, joblib = _require_sklearn()
    x = np.array(features, dtype=float)
    y = np.array(labels, dtype=float)
    model = RandomForestRegressor(
        n_estimators=180,
        max_depth=12,
        random_state=RANDOM_SEED,
        n_jobs=-1,
    )
    model.fit(x, y)
    model_path.parent.mkdir(parents=True, exist_ok=True)
    joblib.dump({"model": model, "seed": RANDOM_SEED}, model_path)
    return model, x, y


def load_model(model_path: Path):
    _, _, _, joblib = _require_sklearn()
    payload = joblib.load(model_path)
    if isinstance(payload, dict) and "model" in payload:
        return payload["model"]
    return payload


def evaluate(model, features: list[list[float]], labels: list[float]) -> dict[str, float | int | bool | None]:
    _, mean_absolute_error, mean_squared_error, _ = _require_sklearn()
    if len(labels) < 2:
        return empty_metrics(len(labels))
    x = np.array(features, dtype=float)
    y = np.array(labels, dtype=float)
    split = max(1, int(len(x) * 0.8))
    actual, test_x = y[split:], x[split:]
    if len(actual) == 0:
        actual, test_x = y, x
    predicted = model.predict(test_x)
    mae = float(mean_absolute_error(actual, predicted))
    rmse = float(mean_squared_error(actual, predicted) ** 0.5)
    mape = float(np.mean(np.abs((actual - predicted) / np.maximum(actual, 0.01))) * 100)
    # 特征第 8 列（0-based 7）是 lag_168h，即上周同一时刻。
    baseline = test_x[:, 7]
    baseline_mae = float(mean_absolute_error(actual, baseline))
    return {
        "mae": round(mae, 3),
        "rmse": round(rmse, 3),
        "mape": round(mape, 2),
        "baseline_mae": round(baseline_mae, 3),
        "beats_baseline": bool(mae < baseline_mae),
        "sample_count": int(len(labels)),
    }


def empty_metrics(sample_count: int = 0) -> dict[str, float | int | bool | None]:
    return {
        "mae": None,
        "rmse": None,
        "mape": None,
        "baseline_mae": None,
        "beats_baseline": None,
        "sample_count": sample_count,
    }


def print_metrics(metrics: dict[str, float | int | bool | None]) -> None:
    if metrics.get("mae") is None:
        print(f"评估跳过：有效样本 {metrics.get('sample_count', 0)} 条")
        return
    flag = "优于基线" if metrics["beats_baseline"] else "未优于基线"
    print(
        f"MAE={metrics['mae']:.3f}, RMSE={metrics['rmse']:.3f}, "
        f"MAPE={metrics['mape']:.2f}%, 上周同刻基线 MAE={metrics['baseline_mae']:.3f}（{flag}）"
    )
