#!/usr/bin/env python

from __future__ import print_function, division, absolute_import

import sys, os, subprocess, tempfile


def cmd(command):
	# https://stackoverflow.com/a/4408409
	with tempfile.TemporaryFile() as tempf:
		p = subprocess.Popen (command, stdout=tempf, stderr=sys.stdout)
		p.wait ()
		tempf.seek(0)
		return tempf.read().decode ("utf-8").strip (), p.returncode

args = sys.argv[1:]
if not args or sys.argv[1].find('hb-shape') == -1 or not os.path.exists (sys.argv[1]):
	print ("""First argument does not seem to point to usable hb-shape.""")
	sys.exit (1)
hb_shape, args = args[0], args[1:]

fails = 0

reference = False
if len (args) and args[0] == "--reference":
	reference = True
	args = args[1:]

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

		extra_options = ["--shaper=ot"]
		glyphs_expected = glyphs_expected.strip()
		if glyphs_expected != '*':
			extra_options.append("--verify")

		if line.startswith ("#"):
			if not reference:
				print ("# %s %s --unicodes %s" % (hb_shape, fontfile, unicodes))
			continue

		if not reference:
			print ("%s %s %s %s --unicodes %s" %
					 (hb_shape, fontfile, ' '.join(extra_options), options, unicodes))

		glyphs1, returncode = cmd ([hb_shape, "--font-funcs=ft",
			fontfile] + extra_options + ["--unicodes",
			unicodes] + (options.split (' ') if options else []))

		if returncode:
			print ("ERROR: hb-shape --font-funcs=ft failed.") # file=sys.stderr
			fails = fails + 1
			#continue

		glyphs2, returncode = cmd ([hb_shape, "--font-funcs=ot",
			fontfile] + extra_options + ["--unicodes",
			unicodes] + (options.split (' ') if options else []))

		if returncode:
			print ("ERROR: hb-shape --font-funcs=ot failed.") # file=sys.stderr
			fails = fails + 1
			#continue

		if glyphs1 != glyphs2 and glyphs_expected != '*':
			print ("FT funcs: " + glyphs1) # file=sys.stderr
			print ("OT funcs: " + glyphs2) # file=sys.stderr
			fails = fails + 1

		if reference:
			print (":".join ([fontfile, options, unicodes, glyphs1]))
			continue

		if glyphs1.strip() != glyphs_expected and glyphs_expected != '*':
			print ("Actual:   " + glyphs1) # file=sys.stderr
			print ("Expected: " + glyphs_expected) # file=sys.stderr
			fails = fails + 1

if fails != 0:
	if not reference:
		print (str (fails) + " tests failed.") # file=sys.stderr
	sys.exit (1)

else:
	if not reference:
		print ("All tests passed.")
