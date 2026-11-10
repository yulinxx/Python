"""Example: create geometry through the PyBindCore Document facade."""

from __future__ import annotations

import apr


def main() -> None:
    if apr.Document is None:
        raise RuntimeError("PyBindCore extension (_apr_core) is not available")

    doc = apr.Document.create()
    doc.add_line(apr.Vec2(0, 0), apr.Vec2(100, 50))
    doc.add_circle(apr.Vec2(50, 25), 10)

    snap = doc.export_snapshot()
    print(f"entities={snap.entity_count}, bounds={snap.bounds}")
    for entity in snap.entities:
        print(f"  - {entity.type} id={entity.id}")


if __name__ == "__main__":
    main()
