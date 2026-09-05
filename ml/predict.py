#!/usr/bin/env python3
"""NCS 负荷预测入口。支持 --train / --predict / --evaluate，数据集可为空。

示例：
  python ml/predict.py --predict
  python ml/predict.py --db path/to/charge_platform.db --export-dataset --train --evaluate --predict
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from features import load_holidays  # noqa: E402
from modeler import empty_metrics  # noqa: E402
from pipeline import (  # noqa: E402
    DEFAULT_DATASET,
    DEFAULT_HOLIDAYS,
    DEFAULT_MODEL,
    DEFAULT_WEB_OUT,
    empty_prediction,
    export_dataset,
    forecast,
    resolve_rows,
    run_evaluate,
    run_train,
    write_db,
    write_prediction,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--train", action="store_true", help="训练 RandomForest 并保存 ml/models/load_rf.pkl")
    parser.add_argument("--predict", action="store_true", help="预测未来 24 小时并写入大屏 JSON")
    parser.add_argument("--evaluate", action="store_true", help="输出 MAE/RMSE/MAPE 并与上周同刻基线对比")
    parser.add_argument("--export-dataset", action="store_true", help="从数据库导出 hourly_load.csv")
    parser.add_argument("--db", default=None, help="可选的 charge_platform.db 路径")
    parser.add_argument("--dataset", default=str(DEFAULT_DATASET), help="小时负荷 CSV，默认 ml/dataset/hourly_load.csv")
    parser.add_argument("--holidays", default=str(DEFAULT_HOLIDAYS), help="节假日 CSV")
    parser.add_argument("--model", default=str(DEFAULT_MODEL), help="模型输出路径")
    parser.add_argument("--web-out", default=str(DEFAULT_WEB_OUT), help="大屏读取的 prediction.json")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not (args.train or args.predict or args.evaluate or args.export_dataset):
        raise SystemExit("请至少指定 --train、--predict、--evaluate 或 --export-dataset")

    dataset = Path(args.dataset)
    holidays = load_holidays(Path(args.holidays))
    model_path = Path(args.model)
    web_out = Path(args.web_out)

    if args.export_dataset:
        if not args.db:
            raise SystemExit("--export-dataset 需要同时提供 --db")
        export_dataset(args.db, dataset)

    rows, names, chargers = resolve_rows(dataset, args.db)
    model = None
    metrics = empty_metrics(len(rows))

    if not rows:
        payload = empty_prediction(
            "训练集为空，已预留 hourly_load.csv 格式。写入小时负荷或从数据库导出后再训练/预测。",
            metrics,
        )
        if args.predict:
            write_prediction(web_out, payload)
        else:
            print(payload["message"])
        return 0

    if args.train or (args.evaluate and not model_path.exists()) or (args.predict and not model_path.exists()):
        try:
            model, _, _ = run_train(rows, holidays, model_path)
        except RuntimeError as exc:
            print(exc)
            if args.predict:
                write_prediction(web_out, empty_prediction(str(exc), metrics))
            return 0

    if model is None and model_path.exists():
        from modeler import load_model
        model = load_model(model_path)

    if args.evaluate:
        if model is None:
            print("没有可用模型，无法评估")
        else:
            metrics = run_evaluate(model, rows, holidays)

    if args.predict:
        if model is None:
            payload = empty_prediction("没有可用模型，已跳过预测。", metrics)
        else:
            payload = forecast(model, rows, holidays, names, chargers)
            payload["metrics"] = metrics
        write_prediction(web_out, payload)
        if args.db:
            write_db(args.db, payload)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
