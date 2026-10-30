#!/usr/bin/env python3
"""Generate the missing-component Arabic fallback regression font."""

from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen


glyphs = [
    (None, ".notdef"),
    (0x0644, "lam"),
    (0x0645, "meem"),
    (0x0647, "heh"),
    (0xFEDF, "lam.init"),
    (0xFEE0, "lam.medi"),
    (0xFEE4, "meem.medi"),
    (0xFEEA, "heh.fina"),
    (0xFD88, "lam_meem_hah.init"),
    (0xF201, "lellah"),
]
# The first ligature needs meem.medi and the missing U+FEA4 hah.medi.
# The later lellah ligature needs lam.medi and heh.fina instead.
names = [name for _, name in glyphs]
font = FontBuilder(1000, isTTF=True)
font.font.recalcTimestamp = False
font.font["head"].created = font.font["head"].modified = 2082844800
font.setupGlyphOrder(names)
font.setupCharacterMap({cp: name for cp, name in glyphs if cp is not None})
font.setupGlyf({name: TTGlyphPen(None).glyph() for name in names})
font.setupHorizontalMetrics({name: (600, 0) for name in names})
font.setupHorizontalHeader(ascent=800, descent=-200)
font.setupNameTable({"familyName": "Arabic fallback regression", "styleName": "Regular"})
font.setupOS2()
font.setupPost()
font.save(
    Path(__file__).parent
    / "data/in-house/fonts/arabic-fallback-missing-component.ttf"
)
