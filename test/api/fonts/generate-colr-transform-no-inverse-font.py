"""Generate layers under transforms that have no inverse."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.colorLib.builder import buildCOLR, buildCPAL
from fontTools.pens.ttGlyphPen import TTGlyphPen

RED, GREEN, BLUE = 0, 1, 2

# Each glyph is a red bar on the left, then the square in green under a
# transform, then a blue bar on the right.  Where the transform has no
# inverse, what it paints has no area, and the glyph is the two bars.
ZERO = (0, 0, 0, 0, 0, 0)
ON_A_LINE = (1, 1, 1, 1, 0, 0)  # everything onto the line y = x


def solid(color):
    return dict(Format=2, PaletteIndex=color, Alpha=1)


def color_line():
    return dict(
        Extend="pad",
        ColorStop=[
            dict(StopOffset=0, PaletteIndex=GREEN, Alpha=1),
            dict(StopOffset=1, PaletteIndex=BLUE, Alpha=1),
        ],
    )


LINEAR = dict(
    Format=4, ColorLine=color_line(), x0=200, y0=500, x1=800, y1=500, x2=200, y2=1100
)
RADIAL = dict(
    Format=6, ColorLine=color_line(), x0=500, y0=500, r0=100, x1=500, y1=500, r1=500
)
SWEEP = dict(
    Format=8, ColorLine=color_line(), centerX=500, centerY=500, startAngle=0, endAngle=360
)


def square(paint):
    return dict(Format=10, Glyph="square", Paint=paint)


def transform(matrix, paint):
    return dict(Format=12, Transform=matrix, Paint=paint)


def scale(x, y, paint):
    return dict(Format=16, scaleX=x, scaleY=y, Paint=paint)


def nested(count, matrix, paint):
    for _ in range(count):
        paint = transform(matrix, paint)
    return paint


CASES = [
    transform(ZERO, square(solid(GREEN))),  # a
    scale(0, 1, square(LINEAR)),  # b: everything onto the line x = 0
    transform(ON_A_LINE, square(RADIAL)),  # c
    transform(ZERO, square(SWEEP)),  # d
    # e: a group under such a transform
    transform(
        ZERO,
        dict(
            Format=32,
            SourcePaint=square(solid(GREEN)),
            CompositeMode="multiply",
            BackdropPaint=square(solid(RED)),
        ),
    ),
    # f, g: transforms that each have an inverse, one in another, to
    # where the numbers of the whole run out: too small, and too large
    nested(40, (1 / 65536, 0, 0, 1 / 65536, 0, 0), square(solid(GREEN))),
    nested(40, (32767, 0, 0, 32767, 0, 0), square(solid(GREEN))),
    # h: a transform that has an inverse, which does paint: the green
    # square at half its size, around the middle
    dict(Format=22, scale=0.5, centerX=500, centerY=500, Paint=square(solid(GREEN))),
]


def rectangle(x0, x1):
    pen = TTGlyphPen(None)
    pen.moveTo((x0, 0))
    pen.lineTo((x0, 1000))
    pen.lineTo((x1, 1000))
    pen.lineTo((x1, 0))
    pen.closePath()
    return pen.glyph()


def build(name):
    fb = FontBuilder(1000, isTTF=True)
    shapes = ["left", "square", "right"]
    names = [".notdef"] + [f"layers{i}" for i in range(len(CASES))] + shapes
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap(
        {0x61 + i: glyph for i, glyph in enumerate(names[1 : 1 + len(CASES)])}
    )
    glyphs = {glyph: TTGlyphPen(None).glyph() for glyph in names}
    glyphs["left"] = rectangle(0, 250)
    glyphs["square"] = rectangle(0, 1000)
    glyphs["right"] = rectangle(750, 1000)
    fb.setupGlyf(glyphs)
    # The left side bearing is where the outline starts.
    fb.setupHorizontalMetrics(
        {glyph: (1000, 750 if glyph == "right" else 0) for glyph in names}
    )
    fb.setupHorizontalHeader(ascent=1000, descent=0)
    fb.setupNameTable(
        {"familyName": "COLR Transform No Inverse", "styleName": "Regular"}
    )
    fb.setupOS2(sTypoAscender=1000, sTypoDescender=0, usWinAscent=1000, usWinDescent=0)
    fb.setupPost()
    paints = {
        glyph: dict(
            Format=1,
            Layers=[
                dict(Format=10, Glyph="left", Paint=solid(RED)),
                paint,
                dict(Format=10, Glyph="right", Paint=solid(BLUE)),
            ],
        )
        for glyph, paint in zip(names[1:], CASES)
    }
    fb.font["COLR"] = buildCOLR(
        paints,
        version=1,
        glyphMap=fb.font.getReverseGlyphMap(),
        clipBoxes={glyph: (0, 0, 1000, 1000) for glyph in names[1 : 1 + len(CASES)]},
    )
    fb.font["CPAL"] = buildCPAL([[(1, 0, 0, 1), (0, 1, 0, 1), (0, 0, 1, 1)]])
    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / name)


build("colr-transform-no-inverse.ttf")
