"""Generate COLRv1 clip boxes with fractional edge deltas."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore


def build(cases, count, filename):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + [f"clip{i}" for i in range(len(cases))]
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
    fb.setupNameTable({"familyName": "COLR Clip Rounding", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
    rows, paints = [], {}
    clips = {}
    for name, deltas in zip(names[1:], cases):
        paints[name] = dict(Format=10, Glyph=name, Paint=dict(Format=2, PaletteIndex=0, Alpha=1))
        clips[name] = (50, 50, 500, 500, len(rows))
        rows.extend([[delta] for delta in deltas])
    regions = buildVarRegionList([{"wght": (0, 1, 1)}], ["wght", "wdth"])
    fb.font["COLR"] = buildCOLR(
        paints,
        version=1,
        glyphMap=fb.font.getReverseGlyphMap(),
        varStore=buildVarStore(regions, [buildVarData([0], rows)]),
        clipBoxes=clips,
    )
    fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / filename)


build([(-1, -1, 1, 1), (1, 1, -1, -1), (1, -1, 1, -1), (-1, 1, -1, 1)], 1, "colr-clip-rounding.ttf")
