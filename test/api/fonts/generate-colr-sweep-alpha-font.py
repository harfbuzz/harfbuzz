"""Generate sweep gradients between colors that are not opaque."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen

RED, BLUE = 0, 1


def sweep(end_angle, red_alpha, blue_alpha):
    # Around the center of the em, from red to blue.
    return dict(
        Format=8,
        ColorLine=dict(
            Extend="pad",
            ColorStop=[
                dict(StopOffset=0, PaletteIndex=RED, Alpha=red_alpha),
                dict(StopOffset=1, PaletteIndex=BLUE, Alpha=blue_alpha),
            ],
        ),
        centerX=500,
        centerY=500,
        startAngle=0,
        endAngle=end_angle,
    )


CASES = [
    sweep(360, 0.5, 0.5),  # a: once around, both at a half
    sweep(45, 1, 0),  # b: over 45 degrees, to transparent blue
    sweep(45, 1, 0.25),  # c: over 45 degrees, to blue at a quarter
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
    names = [".notdef"] + [f"sweep{i}" for i in range(len(CASES))] + ["square"]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61 + i: glyph for i, glyph in enumerate(names[1:-1])})
    glyphs = {glyph: TTGlyphPen(None).glyph() for glyph in names}
    glyphs["square"] = square()
    fb.setupGlyf(glyphs)
    fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
    fb.setupHorizontalHeader(ascent=1000, descent=0)
    fb.setupNameTable({"familyName": "COLR Sweep Alpha", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender=1000, sTypoDescender=0, usWinAscent=1000, usWinDescent=0)
    fb.setupPost()
    paints = {
        glyph: dict(Format=10, Glyph="square", Paint=paint)
        for glyph, paint in zip(names[1:], CASES)
    }
    fb.font["COLR"] = buildCOLR(
        paints,
        version=1,
        glyphMap=fb.font.getReverseGlyphMap(),
        clipBoxes={glyph: (0, 0, 1000, 1000) for glyph in names[1:-1]},
    )
    fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1), (0, 0, 1, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / name)


build("colr-sweep-alpha.ttf")
