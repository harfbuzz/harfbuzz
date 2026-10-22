"""Generate independently rounded child points and component offsets."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables.TupleVariation import TupleVariation

builder = FontBuilder(1000, isTTF=True)
builder.font.recalcTimestamp = False
builder.font["head"].created = builder.font["head"].modified = 2082844800
names = [".notdef", "child", "composite"]
builder.setupGlyphOrder(names)
builder.setupCharacterMap({65: "composite"})
pen = TTGlyphPen(None)
empty = pen.glyph()
pen = TTGlyphPen(None)
pen.moveTo((0, 0))
pen.lineTo((10, 0))
pen.lineTo((0, 10))
pen.closePath()
child = pen.glyph()
pen = TTGlyphPen({"child": child})
pen.addComponent("child", (1, 0, 0, 1, 0, 0))
builder.setupGlyf({".notdef": empty, "child": child, "composite": pen.glyph()})
builder.setupHorizontalMetrics({name: (500, 0) for name in names})
builder.setupHorizontalHeader(ascent=800, descent=-200)
builder.setupNameTable({"familyName": "Default instance bounds", "styleName": "Regular"})
builder.setupOS2()
builder.setupPost()
builder.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
gvar = newTable("gvar")
gvar.version = 1
gvar.reserved = 0
gvar.variations = {
    ".notdef": [],
    "child": [
        TupleVariation({"wght": (0, 1, 1)}, [(0, 0), (1, 0), (0, 0)] + [(0, 0)] * 4)
    ],
    "composite": [TupleVariation({"wght": (0, 1, 1)}, [(1, 0)] + [(0, 0)] * 4)],
}
builder.font["gvar"] = gvar
builder.save(Path(__file__).with_name("glyf-default-instance-bounds.ttf"))
