"""SanYi CAD Python integration package."""

from __future__ import annotations

__all__ = ["__version__", "core"]
__version__ = "0.1.0"
_core_import_error = None

try:
    from . import _sanyi_core as core
except ImportError as exc:  # pragma: no cover - extension not built or ABI mismatch
    core = None
    _core_import_error = exc

__all__ += [
    "Document",
    "Vec2",
    "BBox2",
    "EntityRef",
    "EntitySnapshot",
    "SceneSnapshot",
    "ApplyChanges",
]

if core is not None:
    Document = core.Document
    Vec2 = core.Vec2
    BBox2 = core.BBox2
    EntityRef = core.EntityRef
    EntitySnapshot = core.EntitySnapshot
    SceneSnapshot = core.SceneSnapshot
    ApplyChanges = core.ApplyChanges
else:
    Document = None
    Vec2 = None
    BBox2 = None
    EntityRef = None
    EntitySnapshot = None
    SceneSnapshot = None
    ApplyChanges = None


def core_import_error() -> str | None:
    """Return the import error message when _sanyi_core is unavailable."""
    return str(_core_import_error) if _core_import_error is not None else None
