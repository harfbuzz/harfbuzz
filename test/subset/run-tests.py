#!/usr/bin/env python3

# Runs a subsetting test suite. Compares the results of subsetting via harfbuzz
# to subsetting via fonttools.

from difflib import unified_diff
import os
import re
import subprocess
import sys
import tempfile
import shutil

from subset_test_suite import SubsetTestSuite

fonttools = shutil.which ("fonttools")
ots_sanitize = shutil.which ("ots-sanitize")

if not fonttools:
	print ("fonttools is not present, skipping test.")
	sys.exit (77)

def cmd (command):
	p = subprocess.Popen (
		command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
		universal_newlines=True)
	(stdoutdata, stderrdata) = p.communicate ()
	print (stderrdata, end="", file=sys.stderr)
	return stdoutdata, p.returncode

def read_binary (file_path):
	with open (file_path, 'rb') as f:
		return f.read ()

def fail_test (test, cli_args, message):
	print ('ERROR: %s' % message)
	print ('Test State:')
	print ('  test.font_path    %s' % os.path.abspath (test.font_path))
	print ('  test.profile_path %s' % os.path.abspath (test.profile_path))
	print ('  test.unicodes	    %s' % test.unicodes ())
	expected_file = os.path.join(test_suite.get_output_directory (),
				     test.get_font_name ())
	print ('  expected_file	    %s' % os.path.abspath (expected_file))
	return 1

def run_test (test, should_check_ots):
	out_file = os.path.join (tempfile.mkdtemp (), test.get_font_name () + '-subset' + test.get_font_extension ())
	cli_args = [hb_subset,
		    "--font-file=" + test.font_path,
		    "--output-file=" + out_file,
		    "--unicodes=%s" % test.unicodes (),
		    "--drop-tables+=DSIG,GPOS,GSUB,GDEF",
                    "--drop-tables-=sbix"]
	cli_args.extend (test.get_profile_flags ())
	print (' '.join (cli_args))
	_, return_code = cmd (cli_args)

	if return_code:
		return fail_test (test, cli_args, "%s returned %d" % (' '.join (cli_args), return_code))

	expected_ttx = tempfile.mktemp ()
	_, return_code = run_ttx (os.path.join (test_suite.get_output_directory (),
					    					test.get_font_name ()),
							  expected_ttx)
	if return_code:
		if os.path.exists (expected_ttx): os.remove (expected_ttx)
		return fail_test (test, cli_args, "ttx (expected) returned %d" % (return_code))

	actual_ttx = tempfile.mktemp ()
	_, return_code = run_ttx (out_file, actual_ttx)
	if return_code:
		if os.path.exists (expected_ttx): os.remove (expected_ttx)
		if os.path.exists (actual_ttx): os.remove (actual_ttx)
		return fail_test (test, cli_args, "ttx (actual) returned %d" % (return_code))

	with open (expected_ttx, encoding='utf-8') as f:
		expected_ttx_text = f.read ()
	with open (actual_ttx, encoding='utf-8') as f:
		actual_ttx_text = f.read ()

	# cleanup
	try:
		os.remove (expected_ttx)
		os.remove (actual_ttx)
	except:
		pass

	print ("stripping checksums.")
	expected_ttx_text = strip_check_sum (expected_ttx_text)
	actual_ttx_text = strip_check_sum (actual_ttx_text)

	if not actual_ttx_text == expected_ttx_text:
		for line in unified_diff (expected_ttx_text.splitlines (1), actual_ttx_text.splitlines (1)):
			sys.stdout.write (line)
		sys.stdout.flush ()
		return fail_test (test, cli_args, 'ttx for expected and actual does not match.')

	if should_check_ots:
		print ("Checking output with ots-sanitize.")
		if not check_ots (out_file):
			return fail_test (test, cli_args, 'ots for subsetted file fails.')

	return 0

def run_ttx (font_path, ttx_output_path):
	print ("fonttools ttx %s" % font_path)
	return cmd ([fonttools, "ttx", "-q", "-o", ttx_output_path, font_path])

def strip_check_sum (ttx_string):
	return re.sub ('checkSumAdjustment value=["]0x([0-9a-fA-F])+["]',
		       'checkSumAdjustment value="0x00000000"',
		       ttx_string, count=1)

def has_ots ():
	if not ots_sanitize:
		print("OTS is not present, skipping all ots checks.")
		return False
	return True

def check_ots (path):
	ots_report, returncode = cmd ([ots_sanitize, path])
	if returncode:
		print ("OTS Failure: %s" % ots_report)
		return False
	return True

args = sys.argv[1:]
if not args or sys.argv[1].find ('hb-subset') == -1 or not os.path.exists (sys.argv[1]):
	sys.exit ("First argument does not seem to point to usable hb-subset.")
hb_subset, args = args[0], args[1:]

if not len (args):
	sys.exit ("No tests supplied.")

has_ots = has_ots()

fails = 0
for path in args:
	with open (path, mode="r", encoding="utf-8") as f:
		print ("Running tests in " + path)
		test_suite = SubsetTestSuite (path, f.read ())
		for test in test_suite.tests ():
			fails += run_test (test, has_ots)

if fails != 0:
	sys.exit ("%d test(s) failed." % fails)
else:
	print ("All tests passed.")
