#!/usr/bin/env python

from __future__ import print_function
import sys, os, subprocess

srcdir = os.environ.get ("srcdir", ".")
EXEEXT = os.environ.get ("EXEEXT", "")
top_builddir = os.environ.get ("top_builddir", ".")
hb_fuzzer = os.path.join (top_builddir, "hb-fuzzer" + EXEEXT)

if not os.path.exists (hb_fuzzer):
	if len (sys.argv) == 1 or not os.path.exists (sys.argv[1]):
		print ("""Failed to find hb-fuzzer binary automatically,
please provide it as the first argument to the tool""")
		sys.exit (1)

	hb_fuzzer = sys.argv[1]

print ('hb_fuzzer:', hb_fuzzer)
fails = 0

for line in open (os.path.join (srcdir, "..", "shaping", "data", "in-house", "tests", "fuzzed.tests")):
	font = line.split (":")[0]

	p = subprocess.Popen ([hb_fuzzer, os.path.join (srcdir, "..", "shaping", font)])

	if p.wait () != 0:
		fails = fails + 1

if fails:
	print ("%i fuzzer related tests failed." % fails)
	sys.exit (1)
