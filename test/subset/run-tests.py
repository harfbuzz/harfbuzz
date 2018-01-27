#!/usr/bin/env python

# Runs a subsetting test suite. Compares the results of subsetting via harfbuz
# to subsetting via fonttools.

from __future__ import print_function

import io
import os
import subprocess
import sys

from subset_test_suite import SubsetTestSuite


def cmd(command):
	p = subprocess.Popen (
		command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	p.wait ()
	print (p.stderr.read (), end="") # file=sys.stderr
	return p.stdout.read (), p.returncode


def run_test(test):
	result, return_code = cmd([hb_subset,
														 test.font_path,
														 "--unicodes=%s" % test.unicodes()])

	if return_code:
		print ("ERROR: hb-subset failed for %s, %s, %s" % (test.font_path, test.profile_path, test.unicodes()))
		return 1

	with open(os.path.join(test_suite.get_output_directory(),
												 test.get_font_name())) as expected:
		if not result == expected.read():
			print ("ERROR: hb-subset %s, %s, %s does not match expected value." % (test.font_path, test.profile_path, test.unicodes()))
			return 1

	return 0


args = sys.argv[1:]
if not args or sys.argv[1].find('hb-subset') == -1 or not os.path.exists (sys.argv[1]):
	print ("First argument does not seem to point to usable hb-subset.")
	sys.exit (1)
hb_subset, args = args[0], args[1:]

if not len(args):
	print ("No tests supplied.")
	sys.exit (1)

fails = 0
for path in args:
	with io.open(path, mode="r", encoding="utf-8") as f:
		print ("Running tests in " + path)
		test_suite = SubsetTestSuite(path, f.read())
		for test in test_suite.tests():
			fails += run_test(test)

if fails != 0:
	print (str (fails) + " test(s) failed.")
	sys.exit(1)
else:
	print ("All tests passed.")
