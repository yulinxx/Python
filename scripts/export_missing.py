#!/usr/bin/env python3
import re
import json
from pathlib import Path

TRANS_DIR = Path(__file__).resolve().parents[2] / "UI" / "Common" / "Translations"
OUT = Path(__file__).parent / "missing_by_lang"


def missing_sources(path: Path):
    text = path.read_text(encoding="utf-8")
    missing = []
    for block in re.findall(r"<message>(.*?)</message>", text, re.DOTALL):
        if 'type="unfinished"' not in block:
            continue
        src = re.search(r"<source>(.*?)</source>", block, re.DOTALL).group(1)
        tr = re.search(r"<translation[^>]*>(.*?)</translation>", block, re.DOTALL)
        t = tr.group(1).strip() if tr else ""
        if not t or t == src:
            missing.append(src)
    return missing


def main():
    OUT.mkdir(exist_ok=True)
    all_missing = None
    for ts in sorted(TRANS_DIR.glob("SanYiCAD_*.ts")):
        lang = ts.stem.replace("SanYiCAD_", "")
        miss = missing_sources(ts)
        (OUT / f"{lang}.json").write_text(json.dumps(miss, ensure_ascii=False, indent=2), encoding="utf-8")
        s = set(miss)
        all_missing = s if all_missing is None else all_missing & s
        print(f"{lang}: {len(miss)} missing")
    common = sorted(all_missing)
    (OUT / "_common.json").write_text(json.dumps(common, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"common across all non-complete langs: {len(common)}")


if __name__ == "__main__":
    main()
