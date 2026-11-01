#!/usr/bin/env python3
"""Generate CFF encodings with and without excess codes (requires fontTools)."""

from pathlib import Path
from struct import pack

from fontTools.fontBuilder import FontBuilder
from fontTools.ttLib.tables.DefaultTable import DefaultTable

OUT = Path(__file__).parent / "fonts"


def index(items):
    if not items:
        return b"\0\0"
    offsets = [1]
    for item in items:
        offsets.append(offsets[-1] + len(item))
    assert offsets[-1] < 256
    return pack(">H", len(items)) + b"\1" + bytes(offsets) + b"".join(items)


for fmt in (0, 1):
    for excess in (False, True):
        f = FontBuilder(1000, isTTF=False)
        f.font.sfntVersion = "OTTO"
        f.font.recalcBBoxes = False
        f.font.recalcTimestamp = False
        f.font["head"].created = f.font["head"].modified = 2082844800
        glyphs = [".notdef", "A", "B"]
        f.setupGlyphOrder(glyphs)
        f.setupCharacterMap({65: "A", 66: "B"})
        f.setupHorizontalMetrics({g: (500, 0) for g in glyphs})
        f.setupHorizontalHeader(ascent=800, descent=-200)
        f.setupNameTable({"familyName": "CFF encoding regression", "styleName": "Regular"})
        f.setupOS2()
        f.setupPost()

        # Write the CFF directly so the encoding can describe nonexistent glyphs.
        name = index([b"CFFEncodingRegression"])
        charset = b"\0\0\x22\0\x23"  # Format 0: SIDs 34 (A), 35 (B).
        if fmt == 0:
            encoding = bytes([0, 4, 65, 66, 67, 68] if excess else [0, 2, 65, 66])
        else:
            encoding = bytes([1, 1, 65, 3 if excess else 1])
        charstrings = index([b"\x0e"] * 3)  # Three empty glyphs (endchar).

        # Three long-integer operands and their operators make an 18-byte DICT.
        charset_offset = 4 + len(name) + len(index([bytes(18)])) + 4
        encoding_offset = charset_offset + len(charset)
        charstrings_offset = encoding_offset + len(encoding)
        top_dict = b"".join(
            b"\x1d" + pack(">I", offset) + bytes([op])
            for offset, op in (
                (charset_offset, 15),
                (encoding_offset, 16),
                (charstrings_offset, 17),
            )
        )
        cff = DefaultTable("CFF ")
        cff.data = (
            b"\1\0\4\4"
            + name
            + index([top_dict])
            + index([])  # String INDEX.
            + index([])  # Global Subr INDEX.
            + charset
            + encoding
            + charstrings
        )
        f.font["CFF "] = cff
        suffix = "excess" if excess else "bounded"
        f.save(OUT / f"cff1_encoding{fmt}_{suffix}.otf")
