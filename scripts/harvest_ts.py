#!/usr/bin/env python3
"""Harvest completed translations from each .ts file into JSON."""
import re
import json
from pathlib import Path

TRANS_DIR = Path(__file__).resolve().parents[2] / "UI" / "Common" / "Translations"
OUT_DIR = Path(__file__).parent / "harvested"
OUT_DIR.mkdir(exist_ok=True)


def harvest(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    result = {}
    for block in re.findall(r"<message>(.*?)</message>", text, re.DOTALL):
        src_m = re.search(r"<source>(.*?)</source>", block, re.DOTALL)
        tr_m = re.search(r"<translation([^>]*)>(.*?)</translation>", block, re.DOTALL)
        if not src_m or not tr_m:
            continue
        src = src_m.group(1)
        attrs = tr_m.group(1)
        tr = tr_m.group(2).strip()
        if 'type="unfinished"' in attrs:
            continue
        if tr and tr != src:
            result[src] = tr
        elif tr and src == tr:
            result[src] = tr
    return result


def main():
    for ts in sorted(TRANS_DIR.glob("SanYiCAD_*.ts")):
        lang = ts.stem.replace("SanYiCAD_", "")
        data = harvest(ts)
        out = OUT_DIR / f"{lang}.json"
        out.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"{lang}: {len(data)} harvested")


if __name__ == "__main__":
    main()
