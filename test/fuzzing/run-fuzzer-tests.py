#!/usr/bin/env python

from __future__ import print_function
import sys, os, subprocess

srcdir = os.environ.get ("srcdir", ".")
EXEEXT = os.environ.get ("EXEEXT", "")
top_builddir = os.environ.get ("top_builddir", ".")
hb_fuzzer = os.path.join (top_builddir, "hb-fuzzer" + EXEEXT)

if hb_fuzzer == ".\\hb-fuzzer":
	hb_fuzzer = "./hb-fuzzer.exe"

if not os.path.exists (hb_fuzzer):
	hb_fuzzer = sys.argv[1]

print ('hb_fuzzer:', hb_fuzzer)
fails = 0

for line in open (os.path.join (srcdir, "..", "shaping", "tests", "fuzzed.tests")):
	font = line.split (":")[0]

	p = subprocess.Popen (
		[hb_fuzzer, os.path.join (srcdir, "..", "shaping", font)],
		stdout=subprocess.PIPE, stderr=subprocess.PIPE)

	if p.wait () != 0:
		fails = fails + 1

	print ((p.stdout.read () + p.stderr.read ()).decode ("utf-8").strip ())

if fails:
	print ("%i fuzzer related tests failed." % fails)
	sys.exit (1)
