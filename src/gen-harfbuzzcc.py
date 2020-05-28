#!/usr/bin/env python3

"usage: gen-harfbuzzcc.py harfbuzz.cc hb-blob.cc hb-buffer.cc ..."

import io, os, re, sys

if len (sys.argv) < 3:
	sys.exit (__doc__)

output_file = sys.argv[1]
source_paths = sys.argv[2:]

with open (output_file, "wb") as f:
	f.write ("".join ('#include "{}"\n'.format (x)
					  for x in source_paths
					  if x.endswith (".cc")).encode ())
