#!/usr/bin/env python3
"""Analyze unfinished entries in all SanYiCAD_*.ts files."""
import re
from pathlib import Path

TRANS_DIR = Path(__file__).resolve().parents[2] / "UI" / "Common" / "Translations"


def analyze(path: Path):
    text = path.read_text(encoding="utf-8")
    blocks = re.findall(r"<message>(.*?)</message>", text, re.DOTALL)
    total = len(blocks)
    unfinished = 0
    empty = 0
    done = 0
    for b in blocks:
        if 'type="unfinished"' in b:
            unfinished += 1
            tr = re.search(r"<translation[^>]*>(.*?)</translation>", b, re.DOTALL)
            t = tr.group(1).strip() if tr else ""
            src = re.search(r"<source>(.*?)</source>", b, re.DOTALL).group(1)
            if not t or t == src:
                empty += 1
        else:
            tr = re.search(r"<translation>(.*?)</translation>", b, re.DOTALL)
            if tr and tr.group(1).strip():
                done += 1
    return total, done, unfinished, empty


def main():
    for ts in sorted(TRANS_DIR.glob("SanYiCAD_*.ts")):
        total, done, unfinished, empty = analyze(ts)
        print(f"{ts.name:20s} total={total:4d} done={done:4d} unfinished={unfinished:4d} need_fill={empty:4d}")


if __name__ == "__main__":
    main()
