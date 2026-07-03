"""Example: create geometry through the PyBindCore Document facade."""

from __future__ import annotations

import sanyi


def main() -> None:
    if sanyi.Document is None:
        raise RuntimeError("PyBindCore extension (_sanyi_core) is not available")

    doc = sanyi.Document.create()
    doc.add_line(sanyi.Vec2(0, 0), sanyi.Vec2(100, 50))
    doc.add_circle(sanyi.Vec2(50, 25), 10)

    snap = doc.export_snapshot()
    print(f"entities={snap.entity_count}, bounds={snap.bounds}")
    for entity in snap.entities:
        print(f"  - {entity.type} id={entity.id}")


if __name__ == "__main__":
    main()
