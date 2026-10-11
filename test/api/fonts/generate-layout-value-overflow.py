"""Generate signed-16 positioning limits and overflowing variation deltas."""

from pathlib import Path

from fontTools.feaLib.builder import addOpenTypeFeaturesFromString
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.varLib.builder import (
    buildVarData,
    buildVarDevTable,
    buildVarRegionList,
    buildVarStore,
)


fb = FontBuilder(1000, isTTF=True)
names = [".notdef"] + ["g" + str(i) for i in range(16)]
fb.setupGlyphOrder(names)
fb.setupCharacterMap({0x61 + i: name for i, name in enumerate(names[1:])})
pen = TTGlyphPen(None)
pen.moveTo((0, 0))
pen.lineTo((500, 0))
pen.lineTo((500, 500))
pen.closePath()
fb.setupGlyf({name: pen.glyph() for name in names})
fb.setupHorizontalMetrics({name: (600, 0) for name in names})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "Layout Value Limits", "styleName": "Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
font = fb.font
rules = []
rows = []
for i in range(16):
    sign = 1 if i % 2 == 0 else -1
    values = [0] * 4
    values[(i % 8) // 2] = sign * (30000 if i < 8 else 32760)
    rules.append(f"pos g{i} <{' '.join(map(str, values))}>;")
    rows.append([sign * 10000 if i < 8 else (7 if sign > 0 else -8), sign])
addOpenTypeFeaturesFromString(font, "feature kern {\n" + "\n".join(rules) + "\n} kern;")
fields = ["XPlaDevice", "YPlaDevice", "XAdvDevice", "YAdvDevice"]
for lookup in font["GPOS"].table.LookupList.Lookup:
    for subtable in lookup.SubTable:
        records = (
            subtable.Value
            if subtable.Format == 2
            else [subtable.Value] * len(subtable.Coverage.glyphs)
        )
        for name, record in zip(subtable.Coverage.glyphs, records):
            index = int(name[1:])
            field = (index % 8) // 2
            setattr(record, fields[field], buildVarDevTable(index))
            subtable.ValueFormat |= 0x10 << field
font["GDEF"] = newTable("GDEF")
gdef = font["GDEF"].table = ot.GDEF()
gdef.Version = 0x00010003
gdef.populateDefaults()
gdef.VarStore = buildVarStore(
    buildVarRegionList([{"wght": (0, 1, 1)}, {"wdth": (0, 1, 1)}], ["wght", "wdth"]),
    [buildVarData([0, 1], rows)],
)
font["head"].created = font["head"].modified = 2082844800
font.recalcTimestamp = False
font.save(Path(__file__).with_name("layout-value-overflow.ttf"))
