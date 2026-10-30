#!/usr/bin/env python3
"""Generate small synthetic CFF regression fonts (requires fontTools)."""
from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2CharString
from fontTools.cffLib import SubrsIndex, FDSelect, FDArrayIndex, FontDict
from fontTools.ttLib import TTFont

OUT = Path(__file__).parent / "fonts"


def builder():
    f = FontBuilder(1000, isTTF=False)
    f.font.recalcBBoxes = False
    f.font.recalcTimestamp = False
    f.font["head"].created = f.font["head"].modified = 2082844800
    gs = [".notdef", "A", "B"]
    f.setupGlyphOrder(gs)
    f.setupCharacterMap({65: "A", 66: "B"})
    f.setupHorizontalMetrics({g: (500, 0) for g in gs})
    f.setupHorizontalHeader(ascent=800, descent=-200)
    f.setupNameTable({"familyName": "CFF regression", "styleName": "Regular"})
    f.setupOS2()
    f.setupPost()
    return f


for cff2 in [False, True]:
    for kind in ["caller-dependent"]:
        f = builder()
        end = [] if cff2 else ["endchar"]
        ret = [] if cff2 else ["return"]
        # An unused first subroutine forces renumbering of caller-supplied IDs.
        if kind == "caller-dependent":
            progs = [
                ret,
                [10, 0, "rlineto"] + ret,
                [20, 0, "rlineto"] + ret,
                ["callsubr"] + ret,
            ]
            glyphs = {
                ".notdef": end,
                "A": [0, 0, "rmoveto", -106, -104, "callsubr"] + end,
                "B": [0, 0, "rmoveto", -105, -104, "callsubr"] + end,
            }
        elif kind == "shared-hints":
            progs = [[0, 10] + ret]
            glyphs = {
                ".notdef": end,
                "A": [-107, "callsubr", "hstem", 0, 0, "rmoveto", 10, 0, "rlineto"]
                + end,
                "B": [-107, "callsubr", "rmoveto", 20, 0, "rlineto"] + end,
            }
        cs = {g: T2CharString(program=p) for g, p in glyphs.items()}
        if cff2:
            f.setupCFF2(cs)
            top = f.font["CFF2"].cff.topDictIndex[0]
            private = top.FDArray[0].Private
        else:
            f.setupCFF("CFFRegression", {}, cs, {})
            top = f.font["CFF "].cff.topDictIndex[0]
            private = top.Private
        private.Subrs = SubrsIndex()
        for p in progs:
            private.Subrs.append(
                T2CharString(program=p, private=private, globalSubrs=top.GlobalSubrs)
            )
        f.save(
            OUT
            / (
                ("cff2" if cff2 else "cff1")
                + (
                    "_caller_subr.otf"
                    if kind == "caller-dependent"
                    else "_shared_hint_operands.otf"
                )
            )
        )
# Multiple blend groups accumulate fourteen live results, not 574 operands.
f = builder()
f.setupFvar([("wght", 100, 400, 900, "Weight")], [])
f.setupCFF2(
    {
        g: T2CharString(
            program=[0, 0, "rmoveto", 10, 0, "rlineto"] if g != ".notdef" else []
        )
        for g in [".notdef", "A", "B"]
    },
    regions=[{"wght": (0, 1, 1)}] * 40,
)
private = f.font["CFF2"].cff.topDictIndex[0].FDArray[0].Private
private.BlueValues = [[i] + [0] * 40 for i in range(14)]
f.save(OUT / "cff2_long_private_blends.otf")

# A local subroutine can leave seac operands on the shared stack before the
# glyph's own endchar.
f = TTFont(OUT / "cff1_seac_subr_endchar.otf")
top = f["CFF "].cff.topDictIndex[0]
top.Private.Subrs[0].setProgram([0, 0, 65, 194, "return"])
top.CharStrings["Aacute"].setProgram([-107, "callsubr", "endchar"])
f.save(OUT / "cff1_seac_subr_operands.otf")

# A CID charset contains CIDs, not the standard SIDs needed by seac. Deliberately
# use CIDs 34 and 125, which collide with the SIDs for A and acute, so interpreting
# the invalid seac in CID 1000 would incorrectly retain those unrelated glyphs.
f = builder()
glyphs = [".notdef", "cid00034", "cid00125", "cid01000"]
f.setupGlyphOrder(glyphs)
f.setupCharacterMap({0x41: glyphs[1], 0x301: glyphs[2], 0xC1: glyphs[3]})
f.setupHorizontalMetrics({g: (500, 0) for g in glyphs})
cs = {g: T2CharString(program=["endchar"]) for g in glyphs}
cs["cid01000"].setProgram([0, 0, 65, 194, "endchar"])
f.setupCFF("CIDSeacRegression", {"ROS": ("Adobe", "Identity", 0), "CIDCount": 1001}, cs, {})
top = f.font["CFF "].cff.topDictIndex[0]
fd = FontDict()
fd.Private = top.Private
top.FDArray = FDArrayIndex()
top.FDArray.append(fd)
top.FDSelect = FDSelect(format=0)
top.FDSelect.gidArray = [0] * len(glyphs)
del top.Private
f.save(OUT / "cff1_cid_seac.otf")
