"""Simple echo task for integration testing."""

from __future__ import annotations

from typing import Any


def run(input_data: dict[str, Any]) -> dict[str, Any]:
    return {
        "echo": input_data,
    }
