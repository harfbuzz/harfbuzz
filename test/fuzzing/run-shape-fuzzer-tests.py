#!/usr/bin/env python

from __future__ import print_function, division, absolute_import

import sys, os, subprocess

srcdir = os.environ.get ("srcdir", ".")
EXEEXT = os.environ.get ("EXEEXT", "")
top_builddir = os.environ.get ("top_builddir", ".")
hb_shape_fuzzer = os.path.join (top_builddir, "hb-shape-fuzzer" + EXEEXT)

if not os.path.exists (hb_shape_fuzzer):
	if len (sys.argv) == 1 or not os.path.exists (sys.argv[1]):
		print ("""Failed to find hb-shape-fuzzer binary automatically,
please provide it as the first argument to the tool""")
		sys.exit (1)

	hb_shape_fuzzer = sys.argv[1]

print ('hb_shape_fuzzer:', hb_shape_fuzzer)
fails = 0

parent_path = os.path.join (srcdir, "fonts")
for file in os.listdir (parent_path):
	path = os.path.join(parent_path, file)

	p = subprocess.Popen ([hb_shape_fuzzer, path])

	if p.wait () != 0:
		print ('failure on %s', font)
		fails = fails + 1

if fails:
	print ("%i shape fuzzer related tests failed." % fails)
	sys.exit (1)
