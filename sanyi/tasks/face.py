"""Face detection task (scaffold implementation).

Replace the body with real OpenCV / insightface logic when dependencies are added.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any


def detect(input_data: dict[str, Any]) -> dict[str, Any]:
    image_path = str(input_data.get("image_path", "")).strip()
    if not image_path:
        raise ValueError("Missing required field: image_path")

    path = Path(image_path)
    if not path.is_file():
        raise FileNotFoundError(f"Image file not found: {image_path}")

    # Scaffold: return an empty result set with stable schema.
    return {
        "faces": [],
        "marked_image": "",
        "source_image": str(path),
        "message": "face.detect scaffold: no ML backend configured yet",
    }
