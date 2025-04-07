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
import io

from repack_test import RepackTest

try:
    from fontTools.ttLib import TTFont
except ImportError:
    print("# fonttools is not present, skipping test.")
    sys.exit()

ots_sanitize = shutil.which("ots-sanitize")

EXE_WRAPPER = os.environ.get("MESON_EXE_WRAPPER")


def subset_cmd(command):
    global hb_subset, process
    print("# " + hb_subset + " " + " ".join(command))
    process.stdin.write((";".join(command) + "\n").encode("utf-8"))
    process.stdin.flush()
    return process.stdout.readline().decode("utf-8").strip()


def cmd(command):
    p = subprocess.Popen(
        command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True
    )
    (stdoutdata, stderrdata) = p.communicate()
    if stderrdata:
        print(stderrdata, file=sys.stderr)
    return stdoutdata, p.returncode


def fail_test(test, cli_args, message):
    global fails
    fails += 1

    print("ERROR: %s" % message)
    print("Test State:")
    print("  test.font_name    %s" % test.font_name)
    print("  test.test_path %s" % os.path.abspath(test.test_path))
    print("not ok -", test)
    print("   ---", file=sys.stderr)
    print('   message: "%s"' % message, file=sys.stderr)
    print('   test.font_name: "%s"' % test.font_name, file=sys.stderr)
    print('   test.test_path: "%s"' % os.path.abspath(test.test_path), file=sys.stderr)
    print("   ...", file=sys.stderr)
    return False


def run_test(test, should_check_ots):
    out_file = os.path.join(out_dir, test.font_name + "-subset.ttf")
    cli_args = [
        "--font-file=" + test.font_path(),
        "--output-file=" + out_file,
        "--unicodes=%s" % test.codepoints_string(),
        "--drop-tables-=GPOS,GSUB,GDEF",
    ]
    print("#", " ".join(cli_args))
    ret = subset_cmd(cli_args)

    if ret != "success":
        return fail_test(test, cli_args, "%s failed" % " ".join(cli_args))

    try:
        with TTFont(out_file) as font:
            pass
    except Exception as e:
        print(e)
        return fail_test(test, cli_args, "ttx failed to parse the result")

    if should_check_ots:
        print("# Checking output with ots-sanitize.")
        if not check_ots(out_file):
            return fail_test(test, cli_args, "ots for subsetted file fails.")

    return True


def has_ots():
    if not ots_sanitize:
        print("# OTS is not present, skipping all ots checks.")
        return False
    return True


def check_ots(path):
    ots_report, returncode = cmd([ots_sanitize, path])
    if returncode:
        print("# OTS Failure: %s" % ots_report)
        return False
    return True


args = sys.argv[1:]
if not args or sys.argv[1].find("hb-subset") == -1 or not os.path.exists(sys.argv[1]):
    sys.exit("First argument does not seem to point to usable hb-subset.")
hb_subset, args = args[0], args[1:]

if len(args) != 1:
    sys.exit("No tests supplied.")

batch_cmd = [hb_subset, "--batch"]
if EXE_WRAPPER:
    batch_cmd = [EXE_WRAPPER] + batch_cmd
process = subprocess.Popen(
    batch_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=sys.stdout
)

print("TAP version 14")

has_ots = has_ots()

fails = 0

path = args[0]
if not path.endswith(".tests"):
    sys.exit("Not a valid test case path.")

out_dir = tempfile.mkdtemp()

number = 0
with open(path, mode="r", encoding="utf-8") as f:
    # TODO(garretrieger): re-enable OTS checking.
    if run_test(RepackTest(path, f.read()), False):
        number += 1
        print("ok %d - %s" % (number, os.path.basename(path)))

print("1..%d" % number)

if fails != 0:
    print("# %d test(s) failed; output left in %s" % (fails, out_dir))
else:
    print("# All tests passed.")
    shutil.rmtree(out_dir)
