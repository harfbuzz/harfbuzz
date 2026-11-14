#!/usr/bin/env python3

import subprocess
import sys


hb_shape, font = sys.argv[1:]
options = ["--no-glyph-names", "--no-positions", "--no-clusters"]
expected = subprocess.check_output([hb_shape, font, "--unicodes=61", *options], text=True)
short = ";".join([font, "--unicodes=61", *options])
long = ";".join([font, "--unicodes=" + "0" * 12000 + "61", *options])

for records, count in [(long + "\n", 1), (long, 1), (long + "\n" + short + "\n", 2)]:
    result = subprocess.run(
        [hb_shape, "--batch"], input=records, text=True, capture_output=True
    )
    assert result.returncode == 0, result.stderr
    assert result.stdout == expected * count, (result.stdout, expected * count)
