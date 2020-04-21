#!/usr/bin/env python3

import io, os, re, sys

if len (sys.argv) < 4:
	sys.exit("usage: gen-hb-version.py 1.0.0 hb-version.h.in hb-version.h")

version = sys.argv[1]
major, minor, micro = version.split (".")
input = sys.argv[2]
output = sys.argv[3]

with open (output, "wb") as output_file, open (input, "r") as input_file:
	output_file.write (input_file.read ()
		.replace ("@HB_VERSION_MAJOR@", major)
		.replace ("@HB_VERSION_MINOR@", minor)
		.replace ("@HB_VERSION_MICRO@", micro)
		.replace ("@HB_VERSION@", version)
		.encode ())
