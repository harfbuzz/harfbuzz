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
	print (p.stderr.read (), file=sys.stderr)
	return p.stdout.read ().decode ("utf-8"), p.returncode


srcdir = os.environ.get ("srcdir", ".")
builddir = os.environ.get ("builddir", ".")
top_builddir = os.environ.get ("top_builddir",
	os.path.normpath (os.path.join (os.getcwd (), "..", "..")))
EXEEXT = os.environ.get ("EXEEXT", "")

extra_options = "--verify"
hb_shape = os.path.join (top_builddir, "util", "hb-shape" + EXEEXT)

fails = 0
args = sys.argv[1:]

reference = False
if len (args) and args[0] == "--reference":
	reference = True
	args = args[1:]

if not len (args):
	args = [sys.stdin]

for f in args:
	if not reference:
		if f == sys.stdin:
			print ("Running tests from standard input")
		else:
			print ("Running tests in " + f)

	if f == sys.stdin:
		def f():
			while True:
				try:
					line = input ()
				except EOFError:
					break

				if len (line):
					yield line
				else:
					break
		f = f()
	else:
		f = open (f)

	for line in f:
		fontfile, options, unicodes, glyphs_expected = line.split (":")

		if line.startswith ("#"):
			if not reference:
				print ("# hb-shape %s --unicodes %s" % (fontfile, unicodes))
			continue

		if not reference:
			print ("hb-shape %s %s %s --unicodes %s" %
					 (fontfile, extra_options, options, unicodes))

		glyphs1, returncode = cmd ([hb_shape, "--font-funcs=ft",
			os.path.join (srcdir, fontfile), extra_options, "--unicodes",
			unicodes] + (options.split (' ') if len(options) else []))

		if returncode:
			print ("hb-shape --font-funcs=ft failed.", file=sys.stderr)
			fails = fails + 1
			#continue

		glyphs2, returncode = cmd ([hb_shape, "--font-funcs=ot",
			os.path.join (srcdir, fontfile), extra_options, "--unicodes",
			unicodes] + (options.split (' ') if len(options) else []))

		if returncode:
			print ("hb-shape --font-funcs=ot failed.", file=sys.stderr)
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
