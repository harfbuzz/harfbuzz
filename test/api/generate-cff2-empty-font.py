#!/usr/bin/env python3
"""Generate the empty-outline CFF2 instancing regression font (fontTools)."""
from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2CharString

builder = FontBuilder(1000, isTTF=False)
builder.font.recalcBBoxes = False
builder.font.recalcTimestamp = False
builder.font["head"].created = builder.font["head"].modified = 2082844800
names = [".notdef", "space"]
builder.setupGlyphOrder(names)
builder.setupCharacterMap({32: "space"})
builder.setupHorizontalMetrics({name: (500, 50) for name in names})
builder.setupHorizontalHeader(ascent=800, descent=-200)
builder.setupNameTable({"familyName": "Empty CFF2", "styleName": "Regular"})
builder.setupOS2()
builder.setupPost()
builder.setupFvar([("wght", 0, 0, 1, "Weight")], [])
builder.setupCFF2({name: T2CharString(program=[]) for name in names})
builder.save(Path(__file__).parent / "fonts" / "cff2-empty.otf")
