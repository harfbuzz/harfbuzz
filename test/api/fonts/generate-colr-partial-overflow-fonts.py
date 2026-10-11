"""Generate COLRv1 default biases that cannot be stored in compact fields."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import (
    buildVarRegionList,
    buildVarData,
    buildVarStore,
    buildDeltaSetIndexMap,
)

fb = FontBuilder(1000, isTTF=True)
names = [".notdef", "a", "b", "c"]
fb.setupGlyphOrder(names)
fb.setupCharacterMap({97: "a", 98: "b", 99: "c"})
fb.setupGlyf({name: TTGlyphPen(None).glyph() for name in names})
fb.setupHorizontalMetrics({name: (500, 0) for name in names})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "COLR Partial Overflow", "styleName": "Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
regions = buildVarRegionList(
    [{"wght": (0, 1, 1)}, {"wdth": (0, 1, 1)}], ["wght", "wdth"]
)
store = buildVarStore(
    regions, [buildVarData([0, 1], [[32768, -49152], [0, 0], [49152, -49152], [0, 0]])]
)
solid = dict(Format=2, PaletteIndex=0, Alpha=1)
paints = {
    "a": dict(Format=3, PaletteIndex=0, Alpha=1, VarIndexBase=0),
    "b": dict(Format=17, Paint=solid, scaleX=1.5, scaleY=1, VarIndexBase=0),
    "c": dict(
        Format=5,
        ColorLine=dict(
            Extend="pad",
            ColorStop=[
                dict(StopOffset=0, PaletteIndex=0, Alpha=1, VarIndexBase=2),
                dict(StopOffset=1, PaletteIndex=0, Alpha=1, VarIndexBase=0xFFFFFFFF),
            ],
        ),
        x0=0,
        y0=0,
        x1=400,
        y1=0,
        x2=0,
        y2=700,
        VarIndexBase=0xFFFFFFFF,
    ),
}
fb.font["COLR"] = buildCOLR(
    paints,
    version=1,
    glyphMap=fb.font.getReverseGlyphMap(),
    varStore=store,
    clipBoxes={name: (0, 0, 400, 700) for name in paints},
)
fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
root = Path(__file__).parent
fb.save(root / "colr-partial-overflow.ttf")
fb.font["COLR"].table.VarIndexMap = buildDeltaSetIndexMap(list(range(4)))
fb.save(root / "colr-partial-overflow-mapped.ttf")
regions = buildVarRegionList([{}], ["wght", "wdth"])
store = buildVarStore(regions, [buildVarData([0], [[32768]])])
fb.font["COLR"] = buildCOLR(
    {"a": dict(Format=3, PaletteIndex=0, Alpha=0, VarIndexBase=0)},
    version=1,
    glyphMap=fb.font.getReverseGlyphMap(),
    varStore=store,
    clipBoxes={"a": (0, 0, 400, 700)},
)
fb.save(root / "colr-constant-bias.ttf")
