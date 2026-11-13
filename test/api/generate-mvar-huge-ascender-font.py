#!/usr/bin/env python3
# Generate a tiny TrueType variable font without a vmtx table whose MVAR
# moves the ascender ('hasc') and descender ('hdsc') by extreme (but valid)
# 32-bit deltas at the axis maximum.  The scaled ascender and descender then
# each fit in hb_position_t, but their difference does not.  This is a
# regression fixture for the vertical advance / origin fallbacks in
# hb-ot-font.cc, which subtract the two.
import sys
from fontTools.fontBuilder import FontBuilder
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.ttLib.tables._g_l_y_f import Glyph
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib import builder

out = sys.argv[1] if len(sys.argv) > 1 else "fonts/mvar-huge-ascender.ttf"

glyphs = [".notdef", "A"]

pen = TTGlyphPen(None)
pen.moveTo((100, 0))
pen.lineTo((100, 500))
pen.lineTo((400, 500))
pen.lineTo((400, 0))
pen.closePath()

fb = FontBuilder(1000, isTTF=True)
fb.setupGlyphOrder(glyphs)
fb.setupCharacterMap({0x41: "A"})
fb.setupGlyf({".notdef": Glyph(), "A": pen.glyph()})
fb.setupHorizontalMetrics({g: (500, 0) for g in glyphs})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "MVARHugeAscender", "styleName": "Regular"})
fb.setupOS2(version=4, sTypoAscender=800, sTypoDescender=-200, fsSelection=0x80)
fb.setupPost()
fb.setupFvar(axes=[("TEST", 0.0, 0.0, 1.0, "Test")], instances=[])
font = fb.font

supports = [{"TEST": (0.0, 1.0, 1.0)}]
regionList = builder.buildVarRegionList(supports, ["TEST"])
items = [[2147483647], [-2147483648]]
varData = builder.buildVarData([0], items, optimize=False)
store = builder.buildVarStore(regionList, [varData])

mvar = ot.MVAR()
mvar.Version = 0x00010000
mvar.Reserved = 0
mvar.ValueRecordSize = 8
mvar.VarStore = store
records = []
for i, tag in enumerate(["hasc", "hdsc"]):
    rec = ot.MetricsValueRecord()
    rec.ValueTag = tag
    rec.VarIdx = i
    records.append(rec)
mvar.ValueRecord = records
mvar.ValueRecordCount = len(records)
t = newTable("MVAR")
t.table = mvar
font["MVAR"] = t

font.save(out)
print("wrote", out)
