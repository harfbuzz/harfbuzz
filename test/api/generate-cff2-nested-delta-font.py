#!/usr/bin/env python3
"""Generate nested-default and nested-delta CFF2 regression charstrings."""
from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2CharString

builder = FontBuilder(1000, isTTF=False)
builder.font.recalcBBoxes = builder.font.recalcTimestamp = False
builder.font["head"].created = builder.font["head"].modified = 2082844800
glyphs = [".notdef", "linear", "product"]
builder.setupGlyphOrder(glyphs)
builder.setupCharacterMap({65: "linear", 66: "product"})
builder.setupHorizontalMetrics({glyph: (500, 0) for glyph in glyphs})
builder.setupHorizontalHeader(ascent=800, descent=-200)
builder.setupNameTable({"familyName": "Nested CFF2 delta", "styleName": "Regular"})
builder.setupOS2()
builder.setupPost()
builder.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
move = [0, 0, "rmoveto"]
finish = ["hlineto", 100, "vlineto", -100, "hlineto", -100, "vlineto"]
programs = {
    ".notdef": [],
    # 100 + 20*wght + 30*wdth: a nested default remains linear.
    "linear": move + [100, 20, 0, 1, "blend", 0, 30, 1, "blend"] + finish,
    # 100 + (10 + 20*wght)*wdth: a nested delta requires a product.
    "product": move + [100, 0, 10, 20, 0, 1, "blend", 1, "blend"] + finish,
}
builder.setupCFF2(
    {glyph: T2CharString(program=program) for glyph, program in programs.items()},
    regions=[{"wght": (0, 1, 1)}, {"wdth": (0, 1, 1)}],
)
builder.save(Path(__file__).parent / "fonts" / "cff2-nested-delta.otf")
