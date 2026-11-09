"""Generate a COLRv1 font whose colour stops name variation rows that the
ItemVariationStore does not have."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore

fb = FontBuilder(1000, isTTF=True)
names = [".notdef", "linear"]
fb.setupGlyphOrder(names)
fb.setupCharacterMap({0x61: "linear"})
fb.setupGlyf({glyph: TTGlyphPen(None).glyph() for glyph in names})
fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "COLR Phantom Rows", "styleName": "Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupFvar([("wght", 0, 0, 1, "Weight")], [])
# The store has two rows (0 and 1).  Stop 0 uses them; the other stops name
# rows 2..5, which runtime evaluates as zero deltas.
paint = dict(
    Format=5,
    ColorLine=dict(
        Extend="pad",
        ColorStop=[
            dict(StopOffset=0, PaletteIndex=0, Alpha=1, VarIndexBase=0),
            dict(StopOffset=0.5, PaletteIndex=1, Alpha=1, VarIndexBase=2),
            dict(StopOffset=1, PaletteIndex=0, Alpha=1, VarIndexBase=4),
        ],
    ),
    x0=0, y0=0, x1=100, y1=0, x2=0, y2=100, VarIndexBase=0xFFFFFFFF,
)
regions = buildVarRegionList([{"wght": (0, 1, 1)}], ["wght"])
store = buildVarStore(regions, [buildVarData([0], [[1024], [-2048]], optimize=False)])
fb.font["COLR"] = buildCOLR(
    {"linear": paint}, version=1, glyphMap=fb.font.getReverseGlyphMap(),
    varStore=store, clipBoxes={"linear": (0, 0, 100, 100)},
)
fb.font["CPAL"] = buildCPAL([[(0, 0, 0, 1), (1, 1, 1, 1)]])
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
fb.save(Path(__file__).parent / "colr-phantom-rows.ttf")
