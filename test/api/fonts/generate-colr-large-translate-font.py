"""Generate COLRv1 translations that exceed FWORD and Fixed storage."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore


def build(cases, count, filename):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + [f"translate{i}" for i in range(len(cases))]
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
    fb.setupNameTable({"familyName": "COLR Large Translate", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
    rows, paints = [], {}
    for name, (bx, by, dx, dy) in zip(names[1:], cases):
        paints[name] = dict(
            Format=15,
            Paint=dict(
                Format=10, Glyph=name, Paint=dict(Format=2, PaletteIndex=0, Alpha=1)
            ),
            dx=bx,
            dy=by,
            VarIndexBase=len(rows),
        )
        for delta in (dx, dy):
            rows.append([delta] * count)
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


build(
    [
        (30000, -30000, 2500, -2500),
        (-30000, 30000, -2500, 2500),
        (0, 0, 2**30, -(2**30)),
        (0, 0, -(2**30), 2**30),
        (0, 0, 2**30, 0),
        (0, 0, 0, -(2**30)),
    ],
    4,
    "colr-large-translate.ttf",
)
build([(0, 0, 2**30, -(2**30))], 32766, "colr-maximum-translate.ttf")
