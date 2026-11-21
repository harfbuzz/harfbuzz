"""Generate sbix glyphs whose PNG images are of kinds other than 8 bits a sample."""

import struct
import zlib
from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables.sbixGlyph import Glyph
from fontTools.ttLib.tables.sbixStrike import Strike

# Eight pixels to the em, and each image is 8 by 8.
SIZE = 8

RED, GREEN, BLUE, YELLOW = (255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)


def halves(x, y):
    return x * 2 // SIZE


def quadrants(x, y):
    return x * 2 // SIZE + 2 * (y * 2 // SIZE)


def columns(x, y):
    return x


# (color type, bits a sample, what a pixel is, palette, alpha of the
# palette's entries, interlaced).  No palette here has alpha: that takes
# zlib to embed, and the PDF of these is to be the same without it.
IMAGES = [
    # a: two colors, a bit a pixel: red on the left, blue on the right
    (3, 1, halves, [RED, BLUE], None, False),
    # b: four colors, two bits a pixel: a quadrant each
    (3, 2, quadrants, [RED, GREEN, BLUE, YELLOW], None, False),
    # c: sixteen colors, four bits a pixel: the same quadrants, out of
    # the entries 0, 5, 10 and 15
    (
        3,
        4,
        lambda x, y: 5 * quadrants(x, y),
        [(RED, GREEN, BLUE, YELLOW)[i // 5] if i % 5 == 0 else (0, 0, 0) for i in range(16)],
        None,
        False,
    ),
    # d: gray, a bit a pixel: black on the left, white on the right
    (0, 1, halves, None, None, False),
    # e: gray, two bits a pixel: four shades, a quadrant each
    (0, 2, quadrants, None, None, False),
    # f: gray, four bits a pixel: a darker shade a column, from white
    (0, 4, lambda x, y: 15 - 2 * columns(x, y), None, None, False),
    # g: 8 bits a sample, but interlaced: four colors, a quadrant each
    (2, 8, lambda x, y: (RED, GREEN, BLUE, YELLOW)[quadrants(x, y)], None, None, True),
    # h: the same in a palette, interlaced
    (3, 8, quadrants, [RED, GREEN, BLUE, YELLOW], None, True),
]

# Where the rows and columns of each of the seven passes of an
# interlaced image start, and how far apart they are.
ADAM7 = [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)]


def chunk(kind, data):
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data))
    )


def rows(color_type, bits, pixel, xs, ys):
    data = b""
    for y in ys:
        data += b"\0"  # no filter
        if color_type == 2:
            data += b"".join(bytes(pixel(x, y)) for x in xs)
            continue
        packed = 0
        count = 0
        for x in xs:
            packed = packed << bits | pixel(x, y)
            count += bits
        packed <<= -count % 8
        data += packed.to_bytes((count + 7) // 8, "big")
    return data


def png(color_type, bits, pixel, palette, alpha, interlaced):
    if interlaced:
        raw = b"".join(
            rows(color_type, bits, pixel, range(x0, SIZE, dx), range(y0, SIZE, dy))
            for x0, y0, dx, dy in ADAM7
            if x0 < SIZE and y0 < SIZE
        )
    else:
        raw = rows(color_type, bits, pixel, range(SIZE), range(SIZE))
    data = b"\x89PNG\r\n\x1a\n"
    data += chunk(
        b"IHDR", struct.pack(">IIBBBBB", SIZE, SIZE, bits, color_type, 0, 0, interlaced)
    )
    if palette:
        data += chunk(b"PLTE", b"".join(bytes(color) for color in palette))
    if alpha:
        data += chunk(b"tRNS", bytes(alpha))
    # Stored, not compressed, so that the bytes are the same everywhere.
    data += chunk(b"IDAT", zlib.compress(raw, 0))
    data += chunk(b"IEND", b"")
    return data


def build(name):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + [f"image{i}" for i in range(len(IMAGES))]
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61 + i: glyph for i, glyph in enumerate(names[1:])})
    fb.setupGlyf({glyph: TTGlyphPen(None).glyph() for glyph in names})
    fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
    fb.setupHorizontalHeader(ascent=1000, descent=0)
    fb.setupNameTable({"familyName": "sbix PNG Kinds", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender=1000, sTypoDescender=0, usWinAscent=1000, usWinDescent=0)
    fb.setupPost()

    sbix = newTable("sbix")
    sbix.version = 1
    sbix.flags = 1
    strike = Strike(ppem=SIZE, resolution=72)
    for glyph, image in zip(names[1:], IMAGES):
        strike.glyphs[glyph] = Glyph(
            glyphName=glyph,
            graphicType="png ",
            imageData=png(*image),
            originOffsetX=0,
            originOffsetY=0,
        )
    sbix.strikes[SIZE] = strike
    fb.font["sbix"] = sbix

    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / name)


build("sbix-png-kinds.ttf")
