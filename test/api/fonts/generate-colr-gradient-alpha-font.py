"""Generate gradients between color stops that differ in alpha."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen

RED, BLUE = 0, 1

# COLR interpolates colors with alpha multiplied into them.  From opaque
# red to transparent blue that is red fading out, with no blue in it:
# a transparent stop has no color.  To blue at a quarter it is red
# fading into a little blue.
FADE_OUT = [(0, RED, 1), (1, BLUE, 0)]
FADE = [(0, RED, 1), (1, BLUE, 0.25)]


def linear(stops):
    # Across the em, left to right.
    return dict(
        Format=4,
        ColorLine=color_line(stops),
        x0=0,
        y0=500,
        x1=1000,
        y1=500,
        x2=0,
        y2=1500,
    )


def radial(stops):
    # From the center of the em to half of its width.
    return dict(
        Format=6,
        ColorLine=color_line(stops),
        x0=500,
        y0=500,
        r0=0,
        x1=500,
        y1=500,
        r1=500,
    )


def color_line(stops):
    return dict(
        Extend="pad",
        ColorStop=[
            dict(StopOffset=offset, PaletteIndex=color, Alpha=alpha)
            for offset, color, alpha in stops
        ],
    )


CASES = [
    linear(FADE_OUT),  # a
    radial(FADE_OUT),  # b
    linear(FADE),  # c
    radial(FADE),  # d
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
    fb.setupNameTable({"familyName": "COLR Gradient Alpha", "styleName": "Regular"})
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


build("colr-gradient-alpha.ttf")
