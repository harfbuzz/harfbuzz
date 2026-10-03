"""Generate the static VARC constant-delta regression font using FontTools."""

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
    fb.setupNameTable({"familyName": "VARC static deltas", "styleName": "Regular"})
    fb.setupOS2()
    fb.setupPost()
    fb.setupGvar(
        {"leaf": [TupleVariation({0: (0, 1, 1)}, [(1000, 0)] * 3 + [(0, 0)] * 4)]}
    )
    fb.font["gvar"].axisCount = 1

    varc = ot.VARC()
    varc.Version = 0x10000
    varc.Coverage = ot.Coverage()
    varc.Coverage.glyphs = order[2:]
    # The first region is constant; the second is inactive at the default.
    varc.MultiVarStore = buildMultiVarStore(
        buildSparseVarRegionList([{}, {0: (0, 1, 1)}], [0]),
        [buildMultiVarData([0, 1], [[1, -1000], [64, 200], [8192, 16384]])],
    )
    condition = ot.ConditionTable()
    condition.Format, condition.DefaultValue, condition.VarIdx = 2, 0, 0
    varc.ConditionList = ot.ConditionList()
    varc.ConditionList.ConditionTable = [condition]
    varc.AxisIndicesList = ot.AxisIndicesList()
    varc.AxisIndicesList.Item = [(0,)]
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
            component.flags = int(ot.VarComponentFlags.HAVE_TRANSLATE_X)
            component.transform.translateX = 400
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
