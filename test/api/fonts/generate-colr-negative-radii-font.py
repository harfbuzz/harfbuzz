"""Generate negative radial instances with all extend modes and stop orders."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.varLib.builder import buildVarRegionList, buildVarData, buildVarStore


def build(cases, name, wide_stops=False):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + [f"radial{i}" for i in range(len(cases))]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61 + i: glyph for i, glyph in enumerate(names[1:])})
    fb.setupGlyf({glyph: TTGlyphPen(None).glyph() for glyph in names})
    fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({"familyName": "COLR Negative Radii", "styleName": "Regular"})
    fb.setupOS2(
        sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200
    )
    fb.setupPost()
    fb.setupFvar([("wght", 0, 0, 1, "Weight"), ("wdth", 0, 0, 1, "Width")], [])
    paints, rows = {}, []
    for glyph, (r0, r1, extend, unordered, middle) in zip(names[1:], cases):
        stops = [
            dict(
                StopOffset=offset, PaletteIndex=0, Alpha=alpha, VarIndexBase=0xFFFFFFFF
            )
            for offset, alpha in [(0, 0.25), (middle, 0.5), (1, 1)]
        ]
        if unordered:
            stops = [stops[2], stops[0], stops[1]]
        paints[glyph] = dict(
            Format=7,
            ColorLine=dict(Extend=extend, ColorStop=stops),
            x0=10,
            y0=15,
            r0=100,
            x1=410,
            y1=215,
            r1=100,
            VarIndexBase=len(rows),
        )
        rows.extend([[0], [0], [r0 - 100], [0], [0], [r1 - 100]])
    if wide_stops:
        paints[names[1]] = dict(
            Format=7,
            x0=0,
            y0=0,
            r0=1,
            x1=0,
            y1=0,
            r1=1,
            VarIndexBase=0,
            ColorLine=dict(
                Extend="pad",
                ColorStop=[
                    dict(StopOffset=0, PaletteIndex=0, Alpha=0.25, VarIndexBase=6),
                    dict(StopOffset=1, PaletteIndex=0, Alpha=1, VarIndexBase=8),
                ],
            ),
        )
        rows = [
            [0],
            [0],
            [-2],
            [0],
            [0],
            [-1],
            [32769 * 16384],
            [0],
            [65534 * 16384],
            [0],
        ]
    regions = buildVarRegionList([{"wght": (0, 1, 1)}], ["wght", "wdth"])
    fb.font["COLR"] = buildCOLR(
        paints,
        version=1,
        glyphMap=fb.font.getReverseGlyphMap(),
        varStore=buildVarStore(regions, [buildVarData([0], rows)]),
        clipBoxes={glyph: (-2000, -2000, 2000, 2000) for glyph in names[1:]},
    )
    fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / name)


build(
    [
        (r0, r1, extend, unordered, 0.5)
        for unordered in (False, True)
        for extend in ("pad", "repeat", "reflect")
        for r0, r1 in [
            (-100, 500),
            (500, -100),
            (-100, -200),
            (-200, -100),
            (-100, -100),
        ]
    ],
    "colr-negative-radii.ttf",
)
build([(500, -100, "pad", False, 1 / 16384)], "colr-negative-radii-precision.ttf")

build(
    [(-1, 0, "pad", False, 0.5)], "colr-negative-radii-wide-stops.ttf", wide_stops=True
)
