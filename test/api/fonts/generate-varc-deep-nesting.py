"""Generate the VARC nesting-limit regression font using FontTools.

nest0 holds nest1, which holds nest2, and so on for seventy glyphs; the
last one holds the leaf.  HarfBuzz stops at 64 levels.

  A: nest0, then the leaf     (the limit is met before the leaf)
  B: the leaf, then nest0     (the limit is met after the leaf)
  C: nest20, then the leaf    (fifty levels, within the limit)
"""

import sys

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot

NEST = 70


def build():
    nest = ["nest%d" % i for i in range(NEST)]
    order = [".notdef", "leaf", "deep", "leafdeep", "shallow"] + nest
    fb = FontBuilder(1000)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap({65: "deep", 66: "leafdeep", 67: "shallow"})
    glyphs = {name: TTGlyphPen(None).glyph() for name in order}
    pen = TTGlyphPen(None)
    pen.moveTo((100, 0))
    pen.lineTo((200, 0))
    pen.lineTo((100, 100))
    pen.closePath()
    glyphs["leaf"] = pen.glyph()
    fb.setupGlyf(glyphs)
    fb.setupHorizontalMetrics(
        {name: (500, 100 if name == "leaf" else 0) for name in order}
    )
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({"familyName": "VARC deep nesting", "styleName": "Regular"})
    fb.setupOS2()
    fb.setupPost()

    components = {
        "deep": ["nest0", "leaf"],
        "leafdeep": ["leaf", "nest0"],
        "shallow": ["nest20", "leaf"],
    }
    for i, name in enumerate(nest):
        components[name] = [nest[i + 1] if i + 1 < NEST else "leaf"]

    varc = ot.VARC()
    varc.Version = 0x10000
    varc.Coverage = ot.Coverage()
    varc.Coverage.glyphs = order[2:]
    varc.MultiVarStore = None
    varc.ConditionList = None
    varc.AxisIndicesList = None
    varc.VarCompositeGlyphs = ot.VarCompositeGlyphs()
    varc.VarCompositeGlyphs.VarCompositeGlyph = []
    for name in order[2:]:
        glyph = ot.VarCompositeGlyph()
        glyph.components = []
        for target in components[name]:
            component = ot.VarComponent()
            component.glyphName = target
            glyph.components.append(component)
        varc.VarCompositeGlyphs.VarCompositeGlyph.append(glyph)
    fb.font["VARC"] = newTable("VARC")
    fb.font["VARC"].table = varc
    return fb.font


if __name__ == "__main__":
    font = build()
    for path in sys.argv[1:]:
        font.save(path)
