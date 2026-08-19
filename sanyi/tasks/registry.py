"""Task registry helpers for subprocess worker."""

from __future__ import annotations

import importlib
from typing import Any, Callable


TaskHandler = Callable[[dict[str, Any]], dict[str, Any]]


def resolve_entry(entry: str) -> TaskHandler:
    if ":" not in entry:
        raise ValueError(f"Invalid task entry '{entry}', expected module:callable")

    module_name, attr_name = entry.split(":", 1)
    module = importlib.import_module(module_name)
    task_fn = getattr(module, attr_name, None)
    if task_fn is None or not callable(task_fn):
        raise ValueError(f"Task entry '{entry}' is not callable")
    return task_fn
