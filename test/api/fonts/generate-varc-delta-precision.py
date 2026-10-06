"""Generate the VARC delta-precision regression font using current FontTools."""

import sys

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables import otTables as ot
from fontTools.ttLib.tables.TupleVariation import TupleVariation
from fontTools.varLib.builder import (
    buildMultiVarData,
    buildMultiVarStore,
    buildSparseVarRegionList,
)


def build():
    order = [".notdef", "leaf", "conditional", "translated", "axes"]
    fb = FontBuilder(1000)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap({65: "conditional", 66: "translated", 67: "axes"})
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
    fb.setupNameTable({"familyName": "VARC delta precision", "styleName": "Regular"})
    fb.setupOS2()
    fb.setupPost()
    fb.setupFvar([("TEST", 0, 0, 1, "Test"), ("LEAF", 0, 0, 1, "Leaf")], [])
    fb.setupGvar(
        {
            "leaf": [
                TupleVariation(
                    {"LEAF": (0, 1, 1)}, [(16384, 0)] * 3 + [(0, 0)] * 4
                )
            ]
        }
    )

    varc = ot.VARC()
    varc.Version = 0x10000
    varc.Coverage = ot.Coverage()
    varc.Coverage.glyphs = order[2:]
    data = buildMultiVarData([0, 1], [])
    data.Item = [
        [16777217, -16777216],  # condition: +1 at TEST=1
        [2147483647, 0, 0, -2147483583, 0, 0],  # translation: +64
        [16777217, -16777216],  # axis override: +1 in F2DOT14 units
    ]
    varc.MultiVarStore = buildMultiVarStore(
        buildSparseVarRegionList([{"TEST": (0, 1, 1)}] * 2, ["TEST", "LEAF"]),
        [data],
    )
    condition = ot.ConditionTable()
    condition.Format, condition.DefaultValue, condition.VarIdx = 2, 0, 0
    varc.ConditionList = ot.ConditionList()
    varc.ConditionList.ConditionTable = [condition]
    varc.AxisIndicesList = ot.AxisIndicesList()
    varc.AxisIndicesList.Item = [(1,)]
    varc.VarCompositeGlyphs = ot.VarCompositeGlyphs()
    varc.VarCompositeGlyphs.VarCompositeGlyph = []
    for name in order[2:]:
        component = ot.VarComponent()
        component.glyphName = "leaf"
        if name == "axes":
            component.axisIndicesIndex = 0
            component.axisValues = (0.0,)
            component.axisValuesVarIndex = 2
        else:
            component.flags = int(
                ot.VarComponentFlags.HAVE_TRANSLATE_X
                | ot.VarComponentFlags.HAVE_SCALE_X
                | ot.VarComponentFlags.HAVE_SCALE_Y
            )
            component.transform.translateX = 400
            component.transform.scaleX = component.transform.scaleY = 2
            if name == "conditional":
                component.conditionIndex = 0
            else:
                component.transformVarIndex = 1
        glyph = ot.VarCompositeGlyph()
        glyph.components = [component]
        varc.VarCompositeGlyphs.VarCompositeGlyph.append(glyph)
    fb.font["VARC"] = newTable("VARC")
    fb.font["VARC"].table = varc
    return fb.font


if __name__ == "__main__":
    font = build()
    for path in sys.argv[1:]:
        font.save(path)
