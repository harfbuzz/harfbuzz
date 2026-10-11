"""Generate COLRv1 centered scales requiring all fractional bits in Fixed translations."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore


def build(cases, count, filename):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + [f"center{i}" for i in range(len(cases))]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61 + i: name for i, name in enumerate(names[1:])})
    pen = TTGlyphPen(None)
    pen.moveTo((50, 50))
    pen.lineTo((500, 50))
    pen.lineTo((500, 500))
    pen.lineTo((50, 500))
    pen.closePath()
    fb.setupGlyf({name: pen.glyph() for name in names})
    fb.setupHorizontalMetrics({name: (1000, 0) for name in names})
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({"familyName": "COLR Affine Precision", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
    rows, paints = [], {}
    for name, (fmt, bx, by, dx, dy) in zip(names[1:], cases):
        paint = dict(Format=fmt, Paint=dict(
            Format=10, Glyph=name, Paint=dict(Format=2, PaletteIndex=0, Alpha=1)
        ), centerX=bx, centerY=by, VarIndexBase=len(rows))
        if fmt == 19:
            paint.update(scaleX=1.50006103515625, scaleY=1.50006103515625)
            deltas = [2048, 2048, dx, dy]
        elif fmt == 23:
            paint.update(scale=1.50006103515625)
            deltas = [2048, dx, dy]
        elif fmt == 27:
            paint.update(angle=45)
            deltas = [2048, dx, dy]
        else:
            paint.update(xSkewAngle=22.5, ySkewAngle=-22.5)
            deltas = [512, -512, dx, dy]
        paints[name] = paint
        rows.extend([[delta] * count for delta in deltas])
    regions = buildVarRegionList([{"wght": (0, 1, 1)}], ["wght", "wdth"])
    fb.font["COLR"] = buildCOLR(
        paints,
        version=1,
        glyphMap=fb.font.getReverseGlyphMap(),
        varStore=buildVarStore(regions, [buildVarData([0] * count, rows)]),
    )
    fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / filename)


cases = [(fmt, cx, cy, 0, 0) for fmt in (19, 23) for cx, cy in ((20001, 20003), (-20001, -20003))]
build(cases, 4, "colr-affine-precision.ttf")
