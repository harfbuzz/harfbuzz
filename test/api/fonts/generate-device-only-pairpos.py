"""Generate PairPos fixtures whose later rows need instanced base fields."""

from pathlib import Path

from fontTools.feaLib.builder import addOpenTypeFeaturesFromString
from fontTools.fontBuilder import FontBuilder
from fontTools.otlLib import builder as layout_builder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.ttLib.tables.otBase import ValueRecord
from fontTools.varLib import builder

for format in (1, 2):
    fb = FontBuilder(1000)
    order = [".notdef", "A", "B", "C", "D"]
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap({ord(char): char for char in "ABCD"})
    fb.setupGlyf({name: TTGlyphPen(None).glyph() for name in order})
    fb.setupHorizontalMetrics({name: (600, 0) for name in order})
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupOS2()
    fb.setupNameTable({"familyName": "Device-only PairPos", "styleName": "Regular"})
    fb.setupPost()
    fb.setupFvar([(axis, -1, 0, 1, axis) for axis in ("TEST", "DUMY")], [])
    font = fb.font
    addOpenTypeFeaturesFromString(
        font, "feature kern { pos A B -1; pos C D -1; } kern;"
    )
    gdef = font["GDEF"] = newTable("GDEF")
    gdef.table = ot.GDEF()
    gdef.table.Version = 0x10003
    gdef.table.VarStore = builder.buildVarStore(
        builder.buildVarRegionList(
            [{"TEST": (0, 1, 1)}, {"DUMY": (0, 1, 1)}], ["TEST", "DUMY"]
        ),
        [builder.buildVarData([0, 1], [[0, 1], [100, 1]])],
    )
    pairs = {}
    for left, right, index in (("A", "B", 0), ("C", "D", 1)):
        first = ValueRecord(0x40)
        first.XAdvDevice = builder.buildVarDevTable(index)
        second = ValueRecord(0x10)
        second.XPlaDevice = builder.buildVarDevTable(index)
        key = (left, right) if format == 1 else ((left,), (right,))
        pairs[key] = (first, second)
    build = (
        layout_builder.buildPairPosGlyphsSubtable
        if format == 1
        else layout_builder.buildPairPosClassesSubtable
    )
    subtable = build(pairs, font.getReverseGlyphMap(), 0x40, 0x10)
    # Class zero is visited before the rows carrying the pinned delta.
    if format == 2:
        for row in subtable.Class1Record:
            for record in row.Class2Record:
                if record.Value1.XAdvDevice is None:
                    record.Value1.XAdvDevice = builder.buildVarDevTable(0)
                    record.Value2.XPlaDevice = builder.buildVarDevTable(0)
    font["GPOS"].table.LookupList.Lookup[0].SubTable = [subtable]
    font["head"].created = font["head"].modified = 3800000000
    font.recalcTimestamp = False
    font.save(Path(__file__).parent / f"device-only-pairpos{format}.ttf")
