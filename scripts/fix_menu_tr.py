#!/usr/bin/env python3
"""Convert menu tr() calls in BaseMenu helpers to QT_TR_NOOP for runtime retranslate."""
import re
from pathlib import Path

UI_ROOT = Path(__file__).resolve().parents[2] / "UI"
MENU_DIRS = [
    UI_ROOT / "2D" / "Src" / "Ui" / "MenuManager",
    UI_ROOT / "3D" / "Src" / "Ui" / "MenuManager",
    UI_ROOT / "Common" / "Src" / "Menu",
]

PATTERNS = [
    (re.compile(r'createRootMenu\(\s*([^,]+),\s*tr\("([^"]*)"\)\s*\)'), r'createRootMenu(\1, QT_TR_NOOP("\2"))'),
    (re.compile(r'addSubMenu\(\s*tr\("([^"]*)"\)'), r'addSubMenu(QT_TR_NOOP("\1")'),
    (re.compile(r'registerAction\(\s*"([^"]+)",\s*tr\("([^"]*)"\)'), r'registerAction("\1", QT_TR_NOOP("\2")'),
]


def ensure_qt_global(content: str) -> str:
    if "QT_TR_NOOP" not in content:
        return content
    if "#include <QtGlobal>" in content or "QtGlobal" in content:
        return content
    if "#include \"UI/Menu/BaseMenu.h\"" in content:
        return content.replace(
            '#include "UI/Menu/BaseMenu.h"',
            '#include "UI/Menu/BaseMenu.h"\n#include <QtGlobal>',
            1,
        )
    first_include = content.find("#include")
    if first_include == -1:
        return "#include <QtGlobal>\n" + content
    line_end = content.find("\n", first_include)
    return content[: line_end + 1] + "#include <QtGlobal>\n" + content[line_end + 1 :]


def main():
    changed_files = 0
    for menu_dir in MENU_DIRS:
        if not menu_dir.exists():
            continue
        for path in menu_dir.glob("*.cpp"):
            original = path.read_text(encoding="utf-8")
            updated = original
            for pattern, repl in PATTERNS:
                updated = pattern.sub(repl, updated)
            if updated != original:
                updated = ensure_qt_global(updated)
                path.write_text(updated, encoding="utf-8")
                changed_files += 1
                print(f"Updated {path}")
    print(f"Done. {changed_files} files changed.")


if __name__ == "__main__":
    main()
