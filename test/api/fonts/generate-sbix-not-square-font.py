"""Generate sbix glyphs whose images are not square."""

import struct
import zlib
from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import newTable
from fontTools.ttLib.tables.sbixGlyph import Glyph
from fontTools.ttLib.tables.sbixStrike import Strike

RED = (255, 0, 0, 255)
BLUE = (0, 0, 255, 255)

# Four pixels to the em.  The rows of each image differ, so that it
# shows if one is left out or painted twice.
PPEM = 4
IMAGES = {
    # a: half an em wide and one tall; red above the middle, blue below
    "tall": (2, [RED, RED, BLUE, BLUE]),
    # b: one em wide and half an em tall; red above its middle, blue below
    "wide": (4, [RED, BLUE]),
}


def chunk(kind, data):
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data))
    )


def png(width, rows):
    # Stored, not compressed, so that the bytes are the same everywhere.
    raw = b"".join(b"\0" + bytes(row) * width for row in rows)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, len(rows), 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 0))
        + chunk(b"IEND", b"")
    )


def build(name):
    fb = FontBuilder(1000, isTTF=True)
    names = [".notdef"] + list(IMAGES)
    fb.setupGlyphOrder(names)
    fb.setupCharacterMap({0x61 + i: glyph for i, glyph in enumerate(names[1:])})
    fb.setupGlyf({glyph: TTGlyphPen(None).glyph() for glyph in names})
    fb.setupHorizontalMetrics({glyph: (1000, 0) for glyph in names})
    fb.setupHorizontalHeader(ascent=1000, descent=0)
    fb.setupNameTable({"familyName": "sbix Not Square", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender=1000, sTypoDescender=0, usWinAscent=1000, usWinDescent=0)
    fb.setupPost()

    sbix = newTable("sbix")
    sbix.version = 1
    sbix.flags = 1
    strike = Strike(ppem=PPEM, resolution=72)
    for glyph, (width, rows) in IMAGES.items():
        strike.glyphs[glyph] = Glyph(
            glyphName=glyph,
            graphicType="png ",
            imageData=png(width, rows),
            originOffsetX=0,
            originOffsetY=0,
        )
    sbix.strikes[PPEM] = strike
    fb.font["sbix"] = sbix

    fb.font.recalcTimestamp = False
    fb.font["head"].created = fb.font["head"].modified = 2082844800
    fb.save(Path(__file__).parent / name)


build("sbix-not-square.ttf")
