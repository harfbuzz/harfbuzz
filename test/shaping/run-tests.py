#!/usr/bin/env python

from __future__ import print_function
import sys, os, subprocess


try:
	input = raw_input
except NameError:
	pass


def cmd(command):
	p = subprocess.Popen (
		command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	p.wait ()
	print (p.stderr.read (), file=sys.stderr, end='')
	return p.stdout.read ().decode ("utf-8"), p.returncode


builddir = os.environ.get ("builddir", ".")
top_builddir = os.environ.get ("top_builddir",
	os.path.normpath (os.path.join (builddir, "..", "..")))
utildir = os.environ.get ("utildir", "util")
EXEEXT = os.environ.get ("EXEEXT", "")

extra_options = "--verify"
hb_shape = os.path.join (top_builddir, utildir, "hb-shape" + EXEEXT)

args = sys.argv[1:]

if not os.path.exists (hb_shape):
	if len (sys.argv) == 1 or sys.argv[1].find('hb-shape') == -1 or not os.path.exists (sys.argv[1]):
		print ("""Failed to find hb-shape binary automatically,
please provide it as the first argument to the tool""")
		sys.exit (1)

	hb_shape = args[0]
	args = args[1:]

fails = 0

reference = False
if len (args) and args[0] == "--reference":
	reference = True
	args = args[1:]

if not reference:
	print ('hb_shape:', hb_shape)

if not len (args):
	args = ['-']

for filename in args:
	if not reference:
		if filename == '-':
			print ("Running tests from standard input")
		else:
			print ("Running tests in " + filename)

	if filename == '-':
		f = sys.stdin
	else:
		f = open (filename)

	for line in f:
		fontfile, options, unicodes, glyphs_expected = line.split (":")
		cwd = os.path.dirname(filename)
		fontfile = os.path.normpath (os.path.join (cwd, fontfile))

		if line.startswith ("#"):
			if not reference:
				print ("# %s %s --unicodes %s" % (hb_shape, fontfile, unicodes))
			continue

		if not reference:
			print ("%s %s %s %s --unicodes %s" %
					 (hb_shape, fontfile, extra_options, options, unicodes))

		glyphs1, returncode = cmd ([hb_shape, "--font-funcs=ft",
			fontfile, extra_options, "--unicodes",
			unicodes] + (options.split (' ') if options else []))

		if returncode:
			print ("hb-shape --font-funcs=ft failed.", file=sys.stderr)
			fails = fails + 1
			#continue

		glyphs2, returncode = cmd ([hb_shape, "--font-funcs=ot",
			fontfile, extra_options, "--unicodes",
			unicodes] + (options.split (' ') if options else []))

		if returncode:
			print ("ERROR: hb-shape --font-funcs=ot failed.", file=sys.stderr)
			fails = fails + 1
			#continue

		if glyphs1 != glyphs2:
			print ("FT funcs: " + glyphs1, file=sys.stderr)
			print ("OT funcs: " + glyphs2, file=sys.stderr)
			fails = fails + 1

		if reference:
			print (":".join ([fontfile, options, unicodes, glyphs1]))
			continue

		if glyphs1.strip() != glyphs_expected.strip():
			print ("Actual:   " + glyphs1, file=sys.stderr)
			print ("Expected: " + glyphs_expected, file=sys.stderr)
			fails = fails + 1

if fails != 0:
	if not reference:
		print (str (fails) + " tests failed.")
	sys.exit (1)

else:
	if not reference:
		print ("All tests passed.")
