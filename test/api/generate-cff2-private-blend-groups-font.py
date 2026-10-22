#!/usr/bin/env python3
"""Generate a CFF2 font with multiple valid Private DICT blend groups."""
from pathlib import Path

from fontTools.cffLib import PrivateDictCompiler, encodeNumber
from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2CharString


class BlendGroupsCompiler(PrivateDictCompiler):
    def compile(self, reason):
        # Emit two seven-result groups directly, independently of FontTools'
        # split-blend compiler. Live results keep the stack below 513 entries.
        data = bytearray()
        for group in range(2):
            defaults = ([-40] + [2] * 6) if group == 0 else [2] * 7
            for value in defaults + [0] * (7 * 40) + [7]:
                data.extend(encodeNumber(value))
            data.append(23)  # blend
        data.append(6)  # BlueValues
        return bytes(data)


builder = FontBuilder(1000, isTTF=False)
builder.font.recalcBBoxes = False
builder.font.recalcTimestamp = False
builder.font["head"].created = builder.font["head"].modified = 2082844800
builder.setupGlyphOrder([".notdef", "A"])
builder.setupCharacterMap({65: "A"})
builder.setupHorizontalMetrics({name: (500, 0) for name in [".notdef", "A"]})
builder.setupHorizontalHeader(ascent=800, descent=-200)
builder.setupNameTable({"familyName": "Private blend groups", "styleName": "Regular"})
builder.setupOS2()
builder.setupPost()
builder.setupFvar([("wght", 0, 0, 1, "Weight")], [])
builder.setupCFF2(
    {
        ".notdef": T2CharString(program=[]),
        "A": T2CharString(program=[10, 20, "rmoveto", 40, 30, -40, 60, "rlineto"]),
    },
    regions=[{"wght": (0, 1, 1)}] * 40,
)
builder.font["CFF2"].cff.topDictIndex[0].FDArray[0].Private.compilerClass = BlendGroupsCompiler
builder.save(Path(__file__).parent / "fonts" / "cff2-private-blend-groups.otf")
