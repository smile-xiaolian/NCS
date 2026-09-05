#!/usr/bin/env python3
"""训练入口，等价于 python ml/predict.py --train。"""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from predict import main as predict_main  # noqa: E402


def main() -> int:
    if "--train" not in sys.argv:
        sys.argv.insert(1, "--train")
    return predict_main()


if __name__ == "__main__":
    raise SystemExit(main())
