"""Generate COLRv1 scale variations that exceed F2Dot14 after instancing."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore

fb = FontBuilder(1000, isTTF=True)
names = [".notdef"] + [f"scale{i}" for i in range(16)]
fb.setupGlyphOrder(names)
fb.setupCharacterMap({0x61 + i: name for i, name in enumerate(names[1:])})
glyphs = {}
for name in names:
    pen = TTGlyphPen(None)
    glyphs[name] = pen.glyph()
fb.setupGlyf(glyphs)
fb.setupHorizontalMetrics({name: (1000, 0) for name in names})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "COLR Wide Scale", "styleName": "Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
rows = []
paints = {}
for i, name in enumerate(names[1:]):
    format = (17, 19, 21, 23)[i % 4]
    sign = 1 if i % 8 < 4 else -1
    paint = dict(
        Format=format,
        Paint=dict(Format=2, PaletteIndex=0, Alpha=1),
        VarIndexBase=len(rows),
    )
    if format in (17, 19):
        paint.update(scaleX=sign * 1.5, scaleY=-sign * 1.5)
        deltas = [sign * 16384, -sign * 16384, 3, -5]
    else:
        paint.update(scale=sign * 1.5)
        deltas = [sign * 16384, 3, -5, 0]
    if format in (19, 23):
        paint.update(centerX=37 if i < 8 else 30000, centerY=-29 if i < 8 else -30000)
    rows.extend([delta] for delta in deltas)
    paints[name] = paint
regions = buildVarRegionList([{"wght": (0, 1, 1)}], ["wght", "wdth"])
store = buildVarStore(regions, [buildVarData([0], rows)])
fb.font["COLR"] = buildCOLR(
    paints,
    version=1,
    glyphMap=fb.font.getReverseGlyphMap(),
    varStore=store,
    clipBoxes={name: (0, 0, 1000, 1000) for name in names[1:]},
)
fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
fb.font.recalcTimestamp = False
fb.font["head"].created = fb.font["head"].modified = 2082844800
fb.save(Path(__file__).parent / "colr-wide-scale.ttf")
