#!/usr/bin/env python3
"""Generate CFF2 VariationStores exercising OFF 2026 section 5.3.3.11.2."""
from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2CharString
from fontTools.varLib.builder import buildVarData

OUT = Path(__file__).parent


def builder(glyphs):
    f = FontBuilder(1000, isTTF=False)
    f.font.recalcBBoxes = f.font.recalcTimestamp = False
    f.font["head"].created = f.font["head"].modified = 2082844800
    f.setupGlyphOrder(glyphs)
    f.setupCharacterMap({65 + i: g for i, g in enumerate(glyphs[1:])})
    f.setupHorizontalMetrics({g: (500, 0) for g in glyphs})
    f.setupHorizontalHeader(ascent=800, descent=-200)
    f.setupNameTable({"familyName": "CFF2 store length", "styleName": "Regular"})
    f.setupOS2()
    f.setupPost()
    return f


# Each glyph uses a separate one-region VarData. vsindex occurs once per glyph.
glyphs = [".notdef"] + [f"g{i}" for i in range(200)]
f = builder(glyphs)
f.setupFvar(
    [("wght", 0, 0, 1, "Weight")]
    + [(f"X{i:03}", 0, 0, 1, f"Axis{i}") for i in range(1, 60)],
    [],
)
chars = {".notdef": T2CharString(program=[])}
for i, g in enumerate(glyphs[1:]):
    chars[g] = T2CharString(
        program=[
            i,
            "vsindex",
            0,
            0,
            "rmoveto",
            1,
            10,
            1,
            "blend",
            0,
            "rlineto",
            0,
            10,
            "rlineto",
        ]
    )
f.setupCFF2(chars, regions=[{"wght": (0, (i + 1) / 256, 1)} for i in range(200)])
store = f.font["CFF2"].cff.topDictIndex[0].VarStore.otVarStore
store.VarData = [buildVarData([i], [], optimize=False) for i in range(200)]
store.VarDataCount = 200
f.save(OUT / "cff2-large-store.otf")

