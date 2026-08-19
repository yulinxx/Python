"""Subprocess worker entry point for PythonHost mode A tasks."""

from __future__ import annotations

import argparse
import json
import sys
import traceback
from typing import Any

from sanyi.tasks.registry import resolve_entry


def _configure_import_path(root: str) -> None:
    if root and root not in sys.path:
        sys.path.insert(0, root)


def _run_payload(payload: dict[str, Any]) -> dict[str, Any]:
    entry = payload.get("entry")
    if not entry:
        raise ValueError("Missing task entry")

    task_fn = resolve_entry(str(entry))
    input_data = payload.get("input") or {}
    if not isinstance(input_data, dict):
        raise ValueError("Task input must be a JSON object")

    result = task_fn(input_data)
    if not isinstance(result, dict):
        raise TypeError("Task handler must return a JSON object")

    return {
        "protocol": payload.get("protocol", 1),
        "request_id": payload.get("request_id"),
        "ok": True,
        "result": result,
        "error": None,
        "metrics": {},
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m sanyi.runtime")
    parser.add_argument("--root", default="", help="Python package root directory")
    args = parser.parse_args(argv)

    _configure_import_path(args.root)

    try:
        payload = json.load(sys.stdin)
        response = _run_payload(payload)
    except Exception as exc:  # noqa: BLE001 - worker boundary
        response = {
            "protocol": 1,
            "request_id": None,
            "ok": False,
            "result": {},
            "error": str(exc),
            "metrics": {"traceback": traceback.format_exc()},
        }

    json.dump(response, sys.stdout, ensure_ascii=False)
    sys.stdout.write("\n")
    sys.stdout.flush()
    return 0 if response.get("ok") else 1


if __name__ == "__main__":
    raise SystemExit(main())
