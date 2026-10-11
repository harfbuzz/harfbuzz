"""Generate COLRv1 geometry defaults that overflow signed and unsigned fields."""

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
rows = [[0, 0] for _ in range(18)]
rows[0] = [10000, -40000]
rows[4] = [-200, 400]
rows[8] = [10000 * 65536, -30000 * 65536]
rows[16] = [10000, -40000]
store = buildVarStore(regions, [buildVarData([0, 1], rows)])
solid = dict(Format=2, PaletteIndex=0, Alpha=1)
line = dict(
    Extend="pad",
    ColorStop=[dict(StopOffset=0, PaletteIndex=0, Alpha=1, VarIndexBase=0xFFFFFFFF)],
)
paints = {
    "a": dict(Format=15, Paint=solid, dx=30000, dy=0, VarIndexBase=0),
    "b": dict(
        Format=7,
        ColorLine=line,
        x0=0,
        y0=0,
        r0=100,
        x1=400,
        y1=0,
        r1=500,
        VarIndexBase=2,
    ),
    "c": dict(
        Format=13,
        Paint=solid,
        Transform=dict(xx=30000, yx=0, xy=0, yy=1, dx=0, dy=0, VarIndexBase=8),
    ),
}
fb.font["COLR"] = buildCOLR(
    paints,
    version=1,
    glyphMap=fb.font.getReverseGlyphMap(),
    varStore=store,
    clipBoxes={name: (0, 0, 30000, 100, 14) for name in paints},
)
fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
root = Path(__file__).parent
fb.save(root / "colr-geometry-overflow.ttf")
fb.font["COLR"].table.VarIndexMap = buildDeltaSetIndexMap(list(range(18)))
fb.save(root / "colr-geometry-overflow-mapped.ttf")
