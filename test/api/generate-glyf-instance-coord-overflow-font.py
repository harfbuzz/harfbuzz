#!/usr/bin/env python3
# Generate a tiny glyf variable font whose gvar deltas push instanced point
# coordinates, component offsets and phantom-point advances past the integer
# ranges the subsetter narrows them to.  This is a regression fixture for the
# float-to-int clamps in the glyf instancer: CompositeGlyphRecord::
# compile_with_point, SimpleGlyph::compile_bytes_with_deltas and
# Glyph::update_mtx.  Without the clamps, instancing this font with the axis
# pinned to its maximum is an out-of-range float-to-int conversion (undefined
# behavior, UBSan float-cast-overflow).
#
#   gid 1 "A"      base square, reused as a component
#   gid 2 "compW"  composite, 16-bit (WORD) component offset 30000 + delta
#                  10000 -> 40000, past int16 (CompositeGlyph.hh WORD branch)
#   gid 3 "compB"  composite, 8-bit component offset + a 32-bit (LONGS) delta,
#                  past int (CompositeGlyph.hh byte branch)
#   gid 4 "simp"   simple glyph, a contour point with a 32-bit delta, past int
#                  (SimpleGlyph.hh) which the sidebearing math also narrows
#   gid 5 "simpP"  simple glyph, a 32-bit delta on the advance phantom point,
#                  past int (Glyph::update_mtx)
import sys
from fontTools.fontBuilder import FontBuilder
from fontTools.ttLib.tables._g_l_y_f import Glyph, GlyphComponent, GlyphCoordinates
from fontTools.ttLib.tables.TupleVariation import TupleVariation
from fontTools.ttLib.tables._g_v_a_r import table__g_v_a_r
from fontTools.ttLib.tables import ttProgram

out = sys.argv[1] if len(sys.argv) > 1 else "fonts/glyf-instance-coord-overflow.ttf"

UPEM = 1000
BIG = 2147483647  # int32 max; roundf() of this rounds up to 2^31, past int
glyphs = [".notdef", "A", "compW", "compB", "simp", "simpP"]


def empty_program():
    p = ttProgram.Program()
    p.fromBytecode(b"")
    return p


def square():
    g = Glyph()
    g.numberOfContours = 1
    g.coordinates = GlyphCoordinates([(0, 0), (500, 0), (500, 500), (0, 500)])
    g.flags = [1, 1, 1, 1]
    g.endPtsOfContours = [3]
    g.program = empty_program()
    return g


def composite(offset_x, word):
    g = Glyph()
    g.numberOfContours = -1
    c = GlyphComponent()
    c.glyphName = "A"
    c.x = offset_x
    c.y = 0
    # ARGS_ARE_XY_VALUES, plus ARG_1_AND_2_ARE_WORDS for the 16-bit branch.
    c.flags = 0x0002 | (0x0001 if word else 0)
    c.transform = [[1, 0], [0, 1]]
    g.components = [c]
    g.program = empty_program()
    return g


notdef = Glyph()
notdef.numberOfContours = 0
notdef.program = empty_program()

fb = FontBuilder(UPEM, isTTF=True)
fb.setupGlyphOrder(glyphs)
fb.setupCharacterMap({0x41: "A", 0x42: "compW", 0x43: "compB",
                      0x44: "simp", 0x45: "simpP"})
fb.setupGlyf({
    ".notdef": notdef,
    "A": square(),
    "compW": composite(30000, True),
    "compB": composite(10, False),
    "simp": square(),
    "simpP": square(),
})
fb.setupHorizontalMetrics({g: (600, 0) for g in glyphs})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "GlyfInstanceCoordOverflow",
                   "styleName": "Regular"})
fb.setupOS2()
fb.setupPost()
fb.setupFvar(axes=[("wght", 0.0, 0.0, 1000.0, "Weight")], instances=[])

# A tuple that peaks at the axis maximum, so its scalar is 1.0 when the axis is
# pinned there.  Composite gvar data is one delta per component plus 4 phantom
# points; simple gvar data is one delta per contour point plus 4 phantoms.
peak = {"wght": (0.0, 1.0, 1.0)}
gv = table__g_v_a_r()
gv.version = 1
gv.reserved = 0
gv.variations = {
    ".notdef": [],
    "A": [TupleVariation(peak, [(0, 0)] * (4 + 4))],
    # component offset delta: +10000 (WORD) -> 40000 past int16
    "compW": [TupleVariation(peak, [(10000, 0), (0, 0), (0, 0), (0, 0), (0, 0)])],
    # component offset delta: +BIG (LONGS) past int
    "compB": [TupleVariation(peak, [(BIG, 0), (0, 0), (0, 0), (0, 0), (0, 0)])],
    # contour point 0 delta: +BIG (LONGS) past int
    "simp": [TupleVariation(peak, [(BIG, 0), (0, 0), (0, 0), (0, 0),
                                   (0, 0), (0, 0), (0, 0), (0, 0)])],
    # advance phantom point (index 5 = PHANTOM_RIGHT) delta: +BIG past int
    "simpP": [TupleVariation(peak, [(0, 0), (0, 0), (0, 0), (0, 0),
                                    (0, 0), (BIG, 0), (0, 0), (0, 0)])],
}
fb.font["gvar"] = gv

fb.font.save(out)
print("wrote", out)
