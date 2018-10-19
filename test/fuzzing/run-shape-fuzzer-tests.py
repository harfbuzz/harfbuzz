#!/usr/bin/env python

from __future__ import print_function, division, absolute_import

import sys, os, subprocess, tempfile, threading


def cmd(command):
	# https://stackoverflow.com/a/4408409
	# https://stackoverflow.com/a/10012262
	with tempfile.TemporaryFile() as tempf:
		p = subprocess.Popen (command, stderr=tempf)
		is_killed = {'value': False}

		def timeout(p, is_killed):
			is_killed['value'] = True
			p.kill()
		timer = threading.Timer (2, timeout, [p, is_killed])

		try:
			timer.start()
			p.wait ()
			tempf.seek (0)
			text = tempf.read().decode ("utf-8").strip ()
			returncode = p.returncode
		finally:
			timer.cancel()

		if is_killed['value']:
			text = 'error: timeout, ' + text
			returncode = 1

		return text, returncode


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

	text, returncode = cmd ([hb_shape_fuzzer, path])
	print (text)

	if returncode != 0 or 'error' in text:
		print ('failure on %s' % file)
		fails = fails + 1

if fails:
	print ("%i shape fuzzer related tests failed." % fails)
	sys.exit (1)
