"""Generate radial gradients whose color stops reach past the tip of the cone."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen

RED, BLUE = 0, 1

# Two circles, centered at (x0, 500) and (x1, 500), where the color line
# is at 0 and at 1; and the offsets of its two stops, red then blue.
# The radius is that of the first circle at 0, so a stop before 0 can
# lie where the radius would be negative: past the tip.
CASES = [
    # x0, r0, x1, r1, offsets, extend
    (500, 100, 500, 500, (-1, 1), "pad"),  # a: the tip is on the color line
    (500, 100, 500, 300, (-1.9999, -1), "pad"),  # b: all of the color line is past it
    (500, 100, 500, 500, (-1, 1), "repeat"),  # c
    (500, 100, 500, 500, (-1, 1), "reflect"),  # d
    (500, 500, 500, 100, (0, 1.9999), "pad"),  # e: the circles shrink to the tip
    (300, 100, 700, 400, (-1, 1), "pad"),  # f: a cone, its circles apart
]


def square():
    pen = TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((0, 1000))
    pen.lineTo((1000, 1000))
    pen.lineTo((1000, 0))
    pen.closePath()
    return pen.glyph()


def build(name):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + [f"radial{i}" for i in range(len(CASES))] + ["square"]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61 + i: glyph for i, glyph in enumerate(names[1:-1])})
    glyphs = {glyph: TTGlyphPen(None).glyph() for glyph in names}
    glyphs["square"] = square()
    fb.setupGlyf(glyphs)
    fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
    fb.setupHorizontalHeader(ascent=1000, descent=0)
    fb.setupNameTable({"familyName": "COLR Radial Past Tip", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender=1000, sTypoDescender=0, usWinAscent=1000, usWinDescent=0)
    fb.setupPost()
    paints = {}
    for glyph, (x0, r0, x1, r1, offsets, extend) in zip(names[1:], CASES):
        stops = [
            dict(StopOffset=offset, PaletteIndex=color, Alpha=1)
            for offset, color in zip(offsets, (RED, BLUE))
        ]
        paints[glyph] = dict(
            Format=10,
            Glyph="square",
            Paint=dict(
                Format=6,
                ColorLine=dict(Extend=extend, ColorStop=stops),
                x0=x0,
                y0=500,
                r0=r0,
                x1=x1,
                y1=500,
                r1=r1,
            ),
        )
    fb.font["COLR"] = buildCOLR(
        paints,
        version=1,
        glyphMap=fb.font.getReverseGlyphMap(),
        clipBoxes={glyph: (0, 0, 1000, 1000) for glyph in names[1:-1]},
    )
    fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1), (0, 0.3, 1, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / name)


build("colr-radial-past-tip.ttf")
