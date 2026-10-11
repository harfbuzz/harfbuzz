"""Generate COLRv1 merged default and retained-axis deltas exceeding signed longs."""

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
    [{"wght": (0, 1, 1)}] * 2 + [{"wdth": (0, 1, 1)}] * 2,
    ["wght", "wdth"],
)
fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
root = Path(__file__).parent
for sign, label in [(1, "positive"), (-1, "negative")]:
    row = [sign * 2147483647] * 2 + [-sign * 2147483647] * 2
    store = buildVarStore(regions, [buildVarData(list(range(4)), [row])])
    for mapped in [False, True]:
        fb.font["COLR"] = buildCOLR(
            {
                "a": dict(
                    Format=21,
                    Paint=dict(Format=2, PaletteIndex=0, Alpha=1),
                    scale=1.5,
                    VarIndexBase=0,
                )
            },
            version=1,
            glyphMap=fb.font.getReverseGlyphMap(),
            varStore=store,
            varIndexMap=buildDeltaSetIndexMap([0]) if mapped else None,
            clipBoxes={"a": (0, 0, 400, 700)},
        )
        suffix = "-mapped" if mapped else ""
        fb.save(root / f"colr-long-sums-{label}{suffix}.ttf")
