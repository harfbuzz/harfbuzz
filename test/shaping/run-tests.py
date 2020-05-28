#!/usr/bin/env python3

import sys, os, subprocess, hashlib, tempfile, shutil


args = sys.argv[1:]

reference = False
if len (args) and args[0] == "--reference":
	reference = True
	args = args[1:]

if not args or args[0].find('hb-shape') == -1 or not os.path.exists (args[0]):
	sys.exit ("""First argument does not seem to point to usable hb-shape.""")
hb_shape, args = args[0], args[1:]

def cmd(command):
	process = subprocess.Popen ([hb_shape, '--batch'],
			    stdin=subprocess.PIPE,
			    stdout=subprocess.PIPE,
			    stderr=subprocess.PIPE)
	process.stdin.write ((' '.join (command) + '\n').encode ("utf-8"))
	process.stdin.flush ()
	ret_stdout = None
	ret_stderr = None
	ret_stdout, ret_stderr = process.communicate ()
	if ret_stdout is not None:
		ret_stdout = ret_stdout.decode ("utf-8").strip ()
	if ret_stderr is not None:
		ret_stderr = ret_stderr.decode ("utf-8").strip ()
	return (ret_stdout, ret_stderr)

passes = 0
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
		f = open (filename, encoding='utf8')

	for line in f:
		comment = False
		if line.startswith ("#"):
			comment = True
			line = line[1:]

			if line.startswith (' '):
				if not reference:
					print ("#%s" % line)
				continue

		line = line.strip ()
		if not line:
			continue

		fontfile, options, unicodes, glyphs_expected = line.split (":")
		if fontfile.startswith ('/') or fontfile.startswith ('"/'):
			if os.name == 'nt': # Skip on Windows
				continue

			fontfile, expected_hash = fontfile.split('@')

			try:
				with open (fontfile, 'rb') as ff:
					actual_hash = hashlib.sha1 (ff.read()).hexdigest ().strip ()
					if actual_hash != expected_hash:
						print ('different version of %s found; Expected hash %s, got %s; skipping.' %
							   (fontfile, expected_hash, actual_hash))
						skips += 1
						continue
			except:
				print ('%s not found, skip.' % fontfile)
				skips += 1
				continue
		else:
			cwd = os.path.dirname(filename)
			fontfile = os.path.normpath (os.path.join (cwd, fontfile))

		extra_options = ["--shaper=ot"]
		if glyphs_expected != '*':
			extra_options.append("--verify")

		if comment:
			if not reference:
				print ('# %s "%s" --unicodes %s' % (hb_shape, fontfile, unicodes))
			continue

		if not reference:
			print ('%s "%s" %s %s --unicodes %s' %
					 (hb_shape, fontfile, ' '.join(extra_options), options, unicodes))

		# hack to support fonts with space on run-tests.py, after several other tries...
		if ' ' in fontfile:
			new_fontfile = os.path.join (tempfile.gettempdir (), 'tmpfile')
			shutil.copyfile(fontfile, new_fontfile)
			fontfile = new_fontfile

		glyphs1 = cmd ([hb_shape, "--font-funcs=ft",
			fontfile] + extra_options + ["--unicodes",
			unicodes] + (options.split (' ') if options else []))

		if glyphs1[1] is not None or glyphs1[1] != '':
			check_string = hb_shape[hb_shape.find(os.path.sep) + 1:] + \
			               ': Unknown font function implementation `ft\''
			if glyphs1[1].startswith(check_string):
				skips += 1
				print ("Skipping test due to lack of FreeType support")
				continue

		glyphs2 = cmd ([hb_shape, "--font-funcs=ot",
			fontfile] + extra_options + ["--unicodes",
			unicodes] + (options.split (' ') if options else []))

		if glyphs1[0] != glyphs2[0] and glyphs_expected != '*':
			print ("FT funcs: " + glyphs1[0], file=sys.stderr)
			print ("OT funcs: " + glyphs2[0], file=sys.stderr)
			fails += 1
		else:
			passes += 1

		if reference:
			print (":".join ([fontfile, options, unicodes, glyphs1[0]]))
			continue

		if glyphs1[0].strip() != glyphs_expected and glyphs_expected != '*':
			print ("Actual:   " + glyphs1[0], file=sys.stderr)
			print ("Expected: " + glyphs_expected, file=sys.stderr)
			fails += 1
		else:
			passes += 1

if not reference:
	print ("%d tests passed; %d failed; %d skipped." % (passes, fails, skips), file=sys.stderr)
	if not (fails + passes):
		print ("No tests ran.")
	elif not (fails + skips):
		print ("All tests passed.")

if fails:
	sys.exit (1)
elif passes:
	sys.exit (0)
else:
	sys.exit (77)
