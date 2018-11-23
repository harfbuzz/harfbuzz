#!/usr/bin/env python

from __future__ import print_function, division, absolute_import

import sys, os, subprocess, hashlib, tempfile, shutil

def cmd(command):
	global process
	process.stdin.write ((' '.join (command) + '\n').encode ("utf-8"))
	process.stdin.flush ()
	return process.stdout.readline().decode ("utf-8").strip ()

args = sys.argv[1:]

reference = False
if len (args) and args[0] == "--reference":
	reference = True
	args = args[1:]

if not args or args[0].find('hb-shape') == -1 or not os.path.exists (args[0]):
	print ("""First argument does not seem to point to usable hb-shape.""")
	sys.exit (1)
hb_shape, args = args[0], args[1:]

process = subprocess.Popen ([hb_shape, '--batch'],
			    stdin=subprocess.PIPE,
			    stdout=subprocess.PIPE,
			    stderr=sys.stdout)

fails = 0
skips = 0

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
		if fontfile.startswith ('/') or fontfile.startswith ('"/'):
			fontfile, expected_hash = fontfile.split('@')

			try:
				with open (fontfile, 'rb') as ff:
					actual_hash = hashlib.sha1 (ff.read()).hexdigest ().strip ()
					if actual_hash != expected_hash:
						print ('different versions of the font is found, expected %s hash was %s but got %s, skip' %
							   (fontfile, expected_hash, actual_hash))
						skips = skips + 1
						continue
			except:
				print ('%s is not found, skip.' % fontfile)
				skips = skips + 1
				continue
		else:
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

		# hack to support fonts with space on run-tests.py, after several other tries...
		if ' ' in fontfile:
			new_fontfile = os.path.join (tempfile.gettempdir (), 'tmpfile')
			shutil.copyfile(fontfile, new_fontfile)
			fontfile = new_fontfile

		glyphs1 = cmd ([hb_shape, "--font-funcs=ft",
			fontfile] + extra_options + ["--unicodes",
			unicodes] + (options.split (' ') if options else []))

		glyphs2 = cmd ([hb_shape, "--font-funcs=ot",
			fontfile] + extra_options + ["--unicodes",
			unicodes] + (options.split (' ') if options else []))

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

if fails != 0 or skips != 0:
	if not reference:
		print ("%d tests are failed and %d tests are skipped." % (fails, skips)) # file=sys.stderr
	if fails != 0:
		sys.exit (1)
	sys.exit (77)
else:
	if not reference:
		print ("All tests passed.")
