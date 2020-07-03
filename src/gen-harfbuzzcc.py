#!/usr/bin/env python3

"usage: gen-harfbuzzcc.py harfbuzz.cc hb-blob.cc hb-buffer.cc ..."

import os, sys

os.chdir (os.path.dirname (__file__))

if len (sys.argv) < 3:
	sys.exit (__doc__)

output_file = sys.argv[1]
source_paths = sys.argv[2:]

result = "".join ('#include "{}"\n'.format (x) for x in source_paths if x.endswith (".cc")).encode ()

with open (output_file, "rb") as f:
	current = f.read()

if result != current:
	with open (output_file, "wb") as f:
		f.write (result)
