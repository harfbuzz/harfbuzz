"""Generate COLRv1 affine coefficients with low Fixed bits at ordinary magnitudes."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore


def build(cases, count, filename):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + [f"affine{i}" for i in range(len(cases))]
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
    fb.setupNameTable({"familyName": "COLR Affine Values", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
    rows, paints = [], {}
    for name, (field, base, delta) in zip(names[1:], cases):
        matrix = dict(xx=1, yx=0, xy=0, yy=1, dx=0, dy=0, VarIndexBase=len(rows))
        matrix[field] = base
        paints[name] = dict(Format=13, Paint=dict(
            Format=10, Glyph=name, Paint=dict(Format=2, PaletteIndex=0, Alpha=1)
        ), Transform=matrix)
        rows.extend([[delta if key == field else 0] for key in ("xx", "yx", "xy", "yy", "dx", "dy")])
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


fields = ("xx", "yx", "xy", "yy", "dx", "dy")
cases = [
    (field, sign * ((1 if i < 4 else 1000) + 1 / 65536), delta)
    for i, field in enumerate(fields)
    for sign in (1, -1)
    for delta in (1, -1)
]
cases += [
    ("dx", sign * (1000 + bits / 65536), 0)
    for sign in (1, -1)
    for bits in (1, 2)
]
build(cases, 1, "colr-affine-values.ttf")
