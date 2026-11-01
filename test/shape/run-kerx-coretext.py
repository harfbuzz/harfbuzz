#!/usr/bin/env python3

import json
from pathlib import Path
import subprocess
import sys


if "ot" not in subprocess.check_output([sys.argv[2], "--list-shapers"], text=True).splitlines():
    print("1..0 # SKIP requires the ot shaper")
    sys.exit(0)

root = Path(__file__).resolve().parents[2]
data = root / "test/shape/data/in-house"
with (data / "kerx-coretext-cases.json").open() as source:
    cases = json.load(source)
with (data / "kerx-coretext-results.json").open() as source:
    reference = {case["case"]: case["glyphs"] for case in json.load(source)["cases"]}


def native_positions(glyphs):
    positions = [(g["gid"], g["cluster"], g["x"], g["y"]) for g in glyphs]
    return positions, glyphs[-1]["x"] + glyphs[-1]["advance"]


def hb_positions(glyphs):
    x = y = 0
    positions = []
    for glyph in glyphs:
        positions.append((glyph["g"], glyph["cl"], x + glyph["dx"], y + glyph["dy"]))
        x += glyph["ax"]
        y += glyph["ay"]
    return positions, x


print("TAP version 13")
print(f"1..{len(cases)}")
failed = False
for index, case in enumerate(cases, 1):
    try:
        font = str(root / case["font"])
        native = json.loads(subprocess.check_output([sys.argv[1], font, case["text"]]))
        shaped = json.loads(subprocess.check_output([
            sys.argv[2], font, case["text"], "--shapers=ot", "--cluster-level=1",
            "--no-glyph-names", "--output-format=json",
        ]))
        expected = native_positions(reference[case["name"]])
        assert native_positions(native) == expected, f"CoreText: {native!r}"
        assert hb_positions(shaped) == expected, f"HarfBuzz: {shaped!r}"
    except (AssertionError, OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"not ok {index} - {case['name']}")
        for line in str(error).splitlines():
            print(f"# {line}")
        failed = True
    else:
        print(f"ok {index} - {case['name']}")

sys.exit(1 if failed else 0)
