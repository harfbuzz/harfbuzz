#!/usr/bin/env python3
"""Generate original shaping parity fixtures (repository MIT license)."""

from pathlib import Path
from struct import pack
import shutil
import subprocess
import sys

from fontTools.feaLib.builder import addOpenTypeFeaturesFromString
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
from fontTools.ttLib.tables.DefaultTable import DefaultTable


fonts = Path(__file__).parent / "data/in-house/fonts"


def empty_font(names, cmap, family):
    builder = FontBuilder(1000, isTTF=True)
    builder.setupGlyphOrder(names)
    builder.setupCharacterMap(cmap)
    builder.setupGlyf({name: TTGlyphPen(None).glyph() for name in names})
    builder.setupHorizontalMetrics({name: (500, 0) for name in names})
    builder.setupHorizontalHeader(ascent=800, descent=-200)
    builder.setupNameTable({"familyName": family, "styleName": "Regular"})
    builder.setupOS2(sTypoAscender=800, sTypoDescender=-200,
                     usWinAscent=800, usWinDescent=200)
    builder.setupPost()
    builder.font["head"].created = builder.font["head"].modified = 3400000000
    builder.font.recalcTimestamp = False
    return builder.font


font = empty_font([".notdef", "A", "B"], {65: "A", 66: "B"}, "Cursive rounding")
addOpenTypeFeaturesFromString(font, """
    languagesystem DFLT dflt;
    feature curs {
        pos cursive A <anchor 1 1> <anchor 1 1>;
        pos cursive B <anchor 2 2> <anchor 2 2>;
    } curs;
""")
font.save(fonts / "cursive-anchor-rounding.ttf")


def kerx4(mark_class):
    # Two states, eight classes. Mark one glyph and attach later marks to it.
    classes = pack(">HHHHHHH", 8, 1, 4, 4, 5, 6, 7)
    states = [0] * 8 + [3] * 8
    states[mark_class] = states[8 + mark_class] = 1
    states[8 + 7] = 2
    states = pack(">16H", *states)
    entries = b"".join(pack(">HHH", *entry) for entry in [
        (0, 0, 0xFFFF), (1, 0x8000, 0xFFFF),
        (1, 0, 0), (1, 0, 0xFFFF),
    ])
    class_offset = 20
    state_offset = class_offset + len(classes)
    entry_offset = state_offset + len(states)
    action_offset = entry_offset + len(entries)
    data = pack(">5L", 8, class_offset, state_offset, entry_offset,
                0x80000000 | action_offset)
    data += classes + states + entries + pack(">4h", 0, 100, 0, 0)
    return pack(">3L", 12 + len(data), 4, 0) + data


font = empty_font(
    [".notdef", "A", "gap", "x", "acute"],
    {65: "A", 0xAD: "gap", 120: "x", 0x301: "acute"},
    "Kerx attachment overflow",
)
# A morx insertion doubles the hidden gap without unhiding it. Fifteen
# passes cross the attachment boundary, but leave only three visible glyphs.
classes = pack(">4H", 8, 2, 1, 4)
states = pack(">10H", 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
entries = pack(">8H", 0, 0, 0xFFFF, 0xFFFF, 0, 0x20, 0, 0xFFFF)
insert = pack(">5L", 5, 20, 28, 48, 64) + classes + states + entries + pack(">H", 2)
subtable = pack(">3L", 12 + len(insert), 5, 1) + insert
morx = DefaultTable("morx")
morx.data = pack(">2L", 0x00020000, 1)
morx.data += pack(">4L", 1, 16 + 15 * len(subtable), 0, 15) + 15 * subtable
font["morx"] = morx
kerx = DefaultTable("kerx")
# First attach to the nearby x, then try attaching to the distant A.
kerx.data = pack(">2L", 0x00020000, 2) + kerx4(6) + kerx4(4)
font["kerx"] = kerx
font.save(fonts / "kerx-attachment-overflow.ttf")


for name, tags in [
    ("morx-language-canonicalization.ttf", [b"SR.UTF-8", b"ZH_hant\xFF"]),
    ("morx-long-language-tag.ttf", [b"s" * 64, b"zh-Hant"]),
]:
    font = TTFont(fonts / "882a555639995ed746c7575416acb689ecb9549d.ttf")
    font.recalcTimestamp = False
    data = pack(">3L", 1, 0, len(tags))
    offset = 12 + 4 * len(tags)
    for tag in tags:
        data += pack(">2H", offset, len(tag))
        offset += len(tag)
    table = DefaultTable("ltag")
    table.data = data + b"".join(tags)
    font["ltag"] = table
    font.save(fonts / name)


shutil.copyfile(
    Path(__file__).parent.parent / "api/fonts/mvar-huge-ascender.ttf",
    fonts / "mvar-huge-ascender.ttf",
)

hb_shape = sys.argv[1] if len(sys.argv) > 1 else "build/util/hb-shape"
cases = []
for direction in ["ltr", "rtl"]:
    for scale in [600, -600]:
        cases.append(("cursive-anchor-rounding.ttf", "41,42",
                      [f"--direction={direction}", f"--font-size={scale}"]))
for language in ["sr", "SR_Latn_RS", "zh-Hant", "zh-Hant-TW", "zh"]:
    cases.append(("morx-language-canonicalization.ttf", "41", [f"--language={language}"]))
cases.append(("morx-long-language-tag.ttf", "41", ["--language=" + "s" * 63 + "-sub"]))
cases.append(("kerx-attachment-overflow.ttf", "41,78,301", ["--no-clusters"]))
# Hidden prefix characters fund the expansion within the shaping work budget.
cases.append(("kerx-attachment-overflow.ttf", "2060," * 64 + "41,AD,78,301", ["--no-clusters"]))
cases.append(("mvar-huge-ascender.ttf", "41",
              ["--direction=ttb", "--variations=TEST=1", "--font-funcs=ot"]))

tests = fonts.parent / "tests/shaping-parity.tests"
with tests.open("w") as output:
    output.write("# Generated by test/shape/generate-shaping-parity-fonts.py\n")
    for name, unicodes, options in cases:
        options = ["--no-glyph-names", *options]
        result = subprocess.check_output(
            [hb_shape, str(fonts / name), f"--unicodes={unicodes}",
             *options, "--font-funcs=ot"], text=True,
        ).strip()
        output.write(f"../fonts/{name};{' '.join(options)};{unicodes};{result}\n")
