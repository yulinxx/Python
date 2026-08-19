#!/usr/bin/env python3
"""Analyze and update SanYiCAD_zh.ts unfinished translations."""
import re
import sys
from pathlib import Path

TS_PATH = Path(__file__).resolve().parents[2] / "UI" / "Common" / "Translations" / "SanYiCAD_zh.ts"


def parse_messages(text: str):
    return re.findall(r"<message>(.*?)</message>", text, re.DOTALL)


def get_source_translation(block: str):
    src_m = re.search(r"<source>(.*?)</source>", block, re.DOTALL)
    tr_m = re.search(r"<translation([^>]*)>(.*?)</translation>", block, re.DOTALL)
    if not src_m:
        return None, None, None
    attrs = tr_m.group(1) if tr_m else ""
    translation = tr_m.group(2).strip() if tr_m else ""
    unfinished = 'type="unfinished"' in attrs
    return src_m.group(1), translation, unfinished


def main():
    text = TS_PATH.read_text(encoding="utf-8")
    need = []
    has_zh_unfinished = 0
    done = 0
    for block in parse_messages(text):
        src, tr, unfinished = get_source_translation(block)
        if not src:
            continue
        if unfinished:
            if tr and tr != src:
                has_zh_unfinished += 1
            else:
                need.append(src)
        elif tr and tr != src:
            done += 1
    print(f"File: {TS_PATH}")
    print(f"Already translated (finished): {done}")
    print(f"Unfinished but has zh text: {has_zh_unfinished}")
    print(f"Need translation: {len(need)}")
    for s in need:
        print(s)


if __name__ == "__main__":
    main()
