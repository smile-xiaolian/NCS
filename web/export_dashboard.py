#!/usr/bin/env python3
"""校验 webmodel 格式 JSON，并复制到大屏读取路径 public/data/dashboard.json。

大屏不连接数据库。管理员把上报 JSON 放到本目录即可，也可用本脚本做一次校验复制：

    python export_dashboard.py --src ../webmodel.json
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DEFAULT_OUT = ROOT / "public" / "data" / "dashboard.json"
REQUIRED_FIELDS = ("Province", "City", "District", "ChargerStatus", "24hChargedOrder", "24hTotalIncome", "24hRegisteredUser")


def validate(report: dict) -> list[str]:
    errors: list[str] = []
    if not isinstance(report, dict):
        return ["根节点必须是对象"]
    if "data" not in report or not isinstance(report["data"], list):
        errors.append("缺少 data 数组")
        return errors
    if "datetime" not in report:
        errors.append("缺少 datetime")
    for index, row in enumerate(report["data"]):
        for field in REQUIRED_FIELDS:
            if field not in row:
                errors.append(f"data[{index}] 缺少 {field}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--src", required=True, help="管理端提供的 webmodel 格式 JSON")
    parser.add_argument("--out", default=str(DEFAULT_OUT), help="大屏读取路径")
    args = parser.parse_args()

    src = Path(args.src)
    if not src.exists():
        print(f"找不到文件：{src}", file=sys.stderr)
        return 1

    report = json.loads(src.read_text(encoding="utf-8"))
    errors = validate(report)
    if errors:
        print("JSON 格式不符合 webmodel：", file=sys.stderr)
        for item in errors:
            print(f"- {item}", file=sys.stderr)
        return 1

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"已写入 {out}，共 {len(report['data'])} 个区域")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
