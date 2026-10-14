#!/usr/bin/env python3
# Generate a tiny TrueType variable font whose HVAR item-variation store uses
# long-word (int32) advance deltas across several regions, all peaking at the
# axis maximum.  At the axis extreme the summed advance delta is about 6e9, so
# the advance plus that delta exceeds the range of a 32-bit value.  This is a
# regression fixture for the float-to-unsigned clamp in
# hmtx*_accelerator_t::get_advance_with_var_unscaled.
import sys
from fontTools.fontBuilder import FontBuilder
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.ttLib.tables._g_l_y_f import Glyph
from fontTools.varLib import builder

out = sys.argv[1] if len(sys.argv) > 1 else "fonts/hvar-long-deltas.ttf"

glyphs = [".notdef", "A"]

fb = FontBuilder(1000, isTTF=True)
fb.setupGlyphOrder(glyphs)
fb.setupCharacterMap({0x41: "A"})
fb.setupGlyf({g: Glyph() for g in glyphs})
fb.setupHorizontalMetrics({g: (500, 0) for g in glyphs})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "HVARLongDeltas", "styleName": "Regular"})
fb.setupOS2()
fb.setupPost()
fb.setupFvar(axes=[("TEST", 0.0, 0.0, 1.0, "Test")], instances=[])
font = fb.font

# Three regions all peaking at normalized coord 1.0 so their scalars are all
# 1.0 there; each per-glyph delta set is ~2e9 across the three regions.
supports = [{"TEST": (0.0, 1.0, 1.0)},
            {"TEST": (0.25, 1.0, 1.0)},
            {"TEST": (0.5, 1.0, 1.0)}]
regionList = builder.buildVarRegionList(supports, ["TEST"])
BIG = 2000000000
items = [[BIG, BIG, BIG] for _ in glyphs]
varData = builder.buildVarData([0, 1, 2], items, optimize=False)
store = builder.buildVarStore(regionList, [varData])

hvar = ot.HVAR()
hvar.Version = 0x00010000
hvar.VarStore = store
hvar.AdvWidthMap = None  # implicit mapping: outer=0, inner=glyph id
hvar.LsbMap = None
hvar.RsbMap = None
t = newTable("HVAR")
t.table = hvar
font["HVAR"] = t

font.save(out)
print("wrote", out)
