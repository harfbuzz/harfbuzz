#!/usr/bin/env python

from __future__ import print_function
import sys, os, subprocess, re

p = None
cm = None

def cmd(command, line):
	global p, cm
	if command != cm:
		if p:
			p.stdin.close()
		p = subprocess.Popen (command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		cm = command
	p.stdin.write(line+'\n')
	p.stdin.flush()
	p.poll()
	return p.stdout.readline ().decode ("utf-8").strip(), 0# p.returncode

def parse (s):
	s = re.sub (r"0[xX]", " ", s)
	s = re.sub (r"[<+>{},;&#\\xXuUnNiI\n\t]", " ", s)
	return [int (x, 16) for x in s.split ()]

def utf8 (s):
	s = u''.join (unichr (x) for x in parse (s))
	if sys.version_info[0] == 2: s = s.encode ('utf-8')
	return s

args = sys.argv[1:]
if not args or sys.argv[1].find('hb-shape') == -1 or not os.path.exists (sys.argv[1]):
	print ("""First argument does not seem to point to usable hb-shape.""")
	sys.exit (1)
hb_shape, args = args[0], args[1:]

extra_options = "--verify"

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

		if line.startswith ("#"):
			if not reference:
				print ("# %s %s --unicodes %s" % (hb_shape, fontfile, unicodes))
			continue

		if not reference:
			print ("%s %s %s %s --unicodes %s" %
					 (hb_shape, fontfile, extra_options, options, unicodes))

		glyphs1, returncode = cmd ([hb_shape, "--font-funcs=ft",
			fontfile, extra_options,
			] + (options.split (' ') if options else []), utf8(unicodes))

		if returncode:
			print ("hb-shape --font-funcs=ft failed.", file=sys.stderr)
			fails = fails + 1
			#continue

		#glyphs2, returncode = cmd ([hb_shape, "--font-funcs=ot",
		#	fontfile, extra_options, "--unicodes",
		#	unicodes] + (options.split (' ') if options else []))

		#if returncode:
		#	print ("ERROR: hb-shape --font-funcs=ot failed.", file=sys.stderr)
		#	fails = fails + 1
		#	#continue

		#if glyphs1 != glyphs2:
		#	print ("FT funcs: " + glyphs1, file=sys.stderr)
		#	print ("OT funcs: " + glyphs2, file=sys.stderr)
		#	fails = fails + 1

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
