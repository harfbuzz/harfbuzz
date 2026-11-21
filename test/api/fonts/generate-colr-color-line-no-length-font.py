"""Generate gradients whose color line has no length, and ill-formed ones."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen

RED, BLUE, GREEN = 0, 1, 2

# A color line has no length if it has one stop, or if all its stops are
# at one offset.  If it pads, it is the first stop's color before that
# offset and the last stop's from it on, and stops in between are not
# used; if it repeats or reflects, nothing is painted.
ONE = [(0.25, RED)]
TWO = [(0.25, RED), (0.25, BLUE)]
THREE = [(0.25, RED), (0.25, GREEN), (0.25, BLUE)]
NEAR = [(0.125, RED), (0.125, BLUE)]
FULL = [(0, RED), (1, BLUE)]


def linear(stops, extend="pad", x1=800):
    # From x = 200 to x1, so that 0.25 is at x = 350 for x1 = 800.
    return dict(
        Format=4,
        ColorLine=color_line(stops, extend),
        x0=200,
        y0=500,
        x1=x1,
        y1=500,
        x2=200,
        y2=1100,
    )


def radial(stops, extend="pad", r1=500):
    # Around the center, from a radius of 100 to r1, so that 0.125 is at
    # a radius of 150 for r1 = 500: nearer the center than half of what
    # the color line covers from 0 to 1.
    return dict(
        Format=6,
        ColorLine=color_line(stops, extend),
        x0=500,
        y0=500,
        r0=100,
        x1=500,
        y1=500,
        r1=r1,
    )


def color_line(stops, extend):
    return dict(
        Extend=extend,
        ColorStop=[
            dict(StopOffset=offset, PaletteIndex=color, Alpha=1)
            for offset, color in stops
        ],
    )


CASES = [
    linear(ONE),  # a: red
    linear(TWO),  # b: red left of x = 350, blue right of it
    radial(NEAR),  # c: red within a radius of 150, blue outside
    linear(TWO, "repeat"),  # d: nothing
    radial(NEAR, "reflect"),  # e: nothing
    radial(ONE),  # f: red
    linear(FULL, x1=200),  # g: ill-formed, along no line: nothing
    radial(FULL, r1=100),  # h: ill-formed, one circle: nothing
    linear(THREE),  # i: as b, with no green
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
    names = [".notdef"] + [f"gradient{i}" for i in range(len(CASES))] + ["square"]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61 + i: glyph for i, glyph in enumerate(names[1:-1])})
    glyphs = {glyph: TTGlyphPen(None).glyph() for glyph in names}
    glyphs["square"] = square()
    fb.setupGlyf(glyphs)
    fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
    fb.setupHorizontalHeader(ascent=1000, descent=0)
    fb.setupNameTable(
        {"familyName": "COLR Color Line No Length", "styleName": "Regular"}
    )
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
    fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1), (0, 0.3, 1, 1), (0, 1, 0, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / name)


build("colr-color-line-no-length.ttf")
