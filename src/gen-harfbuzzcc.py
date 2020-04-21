#!/usr/bin/env python3

import io, os, re, sys

if len (sys.argv) < 3:
	sys.exit("usage: gen-harfbuzzcc.py harfbuzz.def hb-blob.cc hb-buffer.cc ...")

output_file = sys.argv[1]
source_paths = sys.argv[2:]

with open (output_file, "wb") as f:
	f.write ("".join ('#include "{}"\n'.format (x)
					  for x in source_paths
					  if x.endswith (".cc")).encode ())
