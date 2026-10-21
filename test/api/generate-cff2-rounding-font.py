#!/usr/bin/env python3
"""Generate CFF2 instancing regressions for fonttools/fonttools#4338.

Run from the repository root. Each ring contour closes explicitly in the
default and in every variation region, so relative rounding must not open it.
The other glyphs exercise compact path operators, flexes, and hint masks.
"""

import math
from pathlib import Path
import subprocess
import sys

from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2CharString


def ring(radius, wobble):
    commands = []
    previous = (0, 0)
    for cx, cy, direction in ((400, 400, 1), (1100, 400, -1)):
        points = [
            (
                round(cx + (radius + wobble * (i % 3)) * math.cos(i * math.pi / 8)),
                round(cy + (radius - wobble * (i % 2)) * math.sin(i * math.pi / 8)),
            )
            for i in range(16)
        ][::direction]
        commands.append(("rmoveto", [points[0][j] - previous[j] for j in (0, 1)]))
        previous = points[0]
        for start, end in zip(points, points[1:] + points[:1]):
            controls = [
                tuple(start[j] + (end[j] - start[j]) // 3 for j in (0, 1)),
                tuple(end[j] - (end[j] - start[j]) // 3 for j in (0, 1)),
                end,
            ]
            args = []
            for point in controls:
                args.extend(point[j] - previous[j] for j in (0, 1))
                previous = point
            commands.append(("rrcurveto", args))
    return commands


def blended(default, deltas):
    return list(default) + [d[i] for i in range(len(default)) for d in deltas] + [len(default), "blend"]


program = []
masters = [ring(r, w) for r, w in ((300, 7), (337, 23), (305, 11), (343, 29), (321, 13))]
for i, (op, args) in enumerate(masters[0]):
    # Independent wght, slnt, interaction, and intermediate-wght regions.
    deltas = [
        [masters[1][i][1][j] - v for j, v in enumerate(args)],
        [masters[2][i][1][j] - v for j, v in enumerate(args)],
        [masters[3][i][1][j] - masters[1][i][1][j] - masters[2][i][1][j] + v for j, v in enumerate(args)],
        [masters[4][i][1][j] - (masters[1][i][1][j] + v) / 2 for j, v in enumerate(args)],
    ]
    program.extend(blended(args, deltas) + [op])

programs = {".notdef": [], "ring": program}
operators = {
    "hlineto": [23, 31, -19, 41, 37],
    "vlineto": [23, 31, -19, 41, 37],
    "rcurveline": [17, 23, 31, -19, 41, 37, 29, -11],
    "rlinecurve": [17, 23, 31, -19, 41, 37, 29, -11],
    "hhcurveto": [13, 17, 23, 31, -19, 41, 37, 29, -11],
    "vvcurveto": [13, 17, 23, 31, -19, 41, 37, 29, -11],
    "hvcurveto": [17, 23, 31, -19, 41, 37, 29, -11, 13],
    "vhcurveto": [17, 23, 31, -19, 41, 37, 29, -11, 13],
    "flex": [17, 23, 31, -19, 41, 37, 29, -11, 13, 47, 19, -23, 50],
    "hflex": [17, 23, 31, -19, 41, 37, 29],
    "hflex1": [17, 23, 31, -19, 41, 37, 29, -11, 13],
    # Its direction changes over wght: exercise the conservative fallback.
    "flex1": [10, 20] * 5 + [17],
}
for op in ("hhcurveto", "vvcurveto", "hvcurveto", "vhcurveto"):
    operators[op + ".one"] = [17, 23, 31, -19]
    operators[op + ".two"] = [17, 23, 31, -19, 41, 37, 29, -11]
for op, args in operators.items():
    deltas = [[(j % 7) - 3 for j in range(len(args))]] + [[0] * len(args)] * 3
    if op == "flex":
        deltas[0][-1] = 0
    if op == "flex1":
        deltas[0] = [30, -10] * 5 + [0]
    p = blended([101, 103], [[1, -1], [0, 0], [0, 0], [0, 0]]) + ["rmoveto"]
    # Repeat to expose cumulative errors, including flex's implicit y reset.
    for _ in range(3):
        p += blended(args, deltas) + [op.split(".")[0]]
    p += [23, 29, "rlineto", 31, "hmoveto", 37, "vmoveto", 41, 43, "rlineto"]
    programs[op] = p

stem = blended([101, 23, 29, 31], [[1, 1, 1, 1], [0] * 4, [0] * 4, [0] * 4])
programs["hinted"] = stem + ["hstemhm"] + stem + ["hintmask", b"\xf0"] + program
programs["fractional"] = [10.25, 20.75, "rmoveto", 100.5, 0, 0, 100.5, -100.5, 0, "rlineto"]

order = list(programs)
fb = FontBuilder(1000, isTTF=False)
fb.setupGlyphOrder(order)
fb.setupCharacterMap({0x41 + i: name for i, name in enumerate(order[1:])})
fb.setupHorizontalMetrics({name: (2000, 0) for name in order})
fb.setupHorizontalHeader(ascent=1600, descent=-400)
fb.setupNameTable({"familyName": "CFF2 Rounding", "styleName": "Regular"})
fb.setupOS2()
fb.setupFvar([("wght", 300, 300, 900, "Weight"), ("slnt", -11, 0, 0, "Slant")], [])
fb.setupCFF2(
    {name: T2CharString(program=p) for name, p in programs.items()},
    regions=[
        {"wght": (0, 1, 1)},
        {"slnt": (-1, -1, 0)},
        {"wght": (0, 1, 1), "slnt": (-1, -1, 0)},
        {"wght": (0, 0.5, 1)},
    ],
)
fb.setupPost(keepGlyphNames=True)
fb.font["head"].created = fb.font["head"].modified = 2082844800
fb.font.recalcTimestamp = False
fb.save(Path(__file__).parent / "fonts" / "cff2-rounding.otf")

# The existing API test also compares this table's serialized bytes.
subprocess.run(
    [
        sys.argv[1] if len(sys.argv) > 1 else "build/util/hb-subset",
        str(Path(__file__).parent / "fonts" / "AdobeVFPrototype.abc.otf"),
        "--unicodes=61,62,63",
        "--variations=wght=250,CNTR=0",
        "--output-file=" + str(Path(__file__).parent / "fonts" / "AdobeVFPrototype.abc.static.otf"),
    ],
    check=True,
)
