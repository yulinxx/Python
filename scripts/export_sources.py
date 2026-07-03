#!/usr/bin/env python3
"""Export all English source strings from SanYiCAD_en.ts."""
import re
from pathlib import Path

TS = Path(__file__).resolve().parents[2] / "UI" / "Common" / "Translations" / "SanYiCAD_en.ts"

text = TS.read_text(encoding="utf-8")
sources = []
for block in re.findall(r"<message>(.*?)</message>", text, re.DOTALL):
    m = re.search(r"<source>(.*?)</source>", block, re.DOTALL)
    if m:
        sources.append(m.group(1))

out = Path(__file__).parent / "all_sources.txt"
out.write_text("\n".join(sources), encoding="utf-8")
print(len(sources), "sources written to", out)
