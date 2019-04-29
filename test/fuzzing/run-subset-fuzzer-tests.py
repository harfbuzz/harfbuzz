#!/usr/bin/env python

from __future__ import print_function, division, absolute_import

import sys, os, subprocess, tempfile, threading


def which(program):
	# https://stackoverflow.com/a/377028
	def is_exe(fpath):
		return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

	fpath, _ = os.path.split(program)
	if fpath:
		if is_exe(program):
			return program
	else:
		for path in os.environ["PATH"].split(os.pathsep):
			exe_file = os.path.join(path, program)
			if is_exe(exe_file):
				return exe_file

	return None


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
hb_subset_fuzzer = os.path.join (top_builddir, "hb-subset-fuzzer" + EXEEXT)

if not os.path.exists (hb_subset_fuzzer):
        if len (sys.argv) < 2 or not os.path.exists (sys.argv[1]):
                print ("""Failed to find hb-subset-fuzzer binary automatically,
please provide it as the first argument to the tool""")
                sys.exit (1)

        hb_subset_fuzzer = sys.argv[1]

print ('hb_subset_fuzzer:', hb_subset_fuzzer)
fails = 0

libtool = os.environ.get('LIBTOOL')
valgrind = None
if os.environ.get('RUN_VALGRIND', ''):
	valgrind = which ('valgrind')
	if valgrind is None:
		print ("""Valgrind requested but not found.""")
		sys.exit (1)
	if libtool is None:
		print ("""Valgrind support is currently autotools only and needs libtool but not found.""")


def run_dir (parent_path):
	global fails
	for file in os.listdir (parent_path):
		path = os.path.join(parent_path, file)

		print ("running subset fuzzer against %s" % path)
		if valgrind:
			text, returncode = cmd (libtool.split(' ') + ['--mode=execute', valgrind + ' --leak-check=full --show-leak-kinds=all --error-exitcode=1', '--', hb_subset_fuzzer, path])
		else:
			text, returncode = cmd ([hb_subset_fuzzer, path])
			if 'error' in text:
				returncode = 1

		if not valgrind and text.strip ():
			print (text)

		if returncode != 0:
			print ("failed for %s" % path)
			fails = fails + 1


run_dir (os.path.join (srcdir, "..", "subset", "data", "fonts"))
# TODO running these tests very slow tests.  Fix and re-enable
#run_dir (os.path.join (srcdir, "fonts"))

if fails:
        print ("%i subset fuzzer related tests failed." % fails)
        sys.exit (1)
