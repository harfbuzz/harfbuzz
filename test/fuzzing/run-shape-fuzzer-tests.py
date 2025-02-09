#!/usr/bin/env python3

import sys
import os
import subprocess
import tempfile

def run_command(command):
    """Run a command, capturing potentially large output."""
    with tempfile.TemporaryFile() as tempf:
        p = subprocess.Popen(command, stdout=tempf, stderr=tempf)
        p.wait()
        tempf.seek(0)
        output = tempf.read().decode('utf-8', errors='replace')
    return output, p.returncode

srcdir = os.getenv("srcdir", ".")
EXEEXT = os.getenv("EXEEXT", "")
top_builddir = os.getenv("top_builddir", ".")

hb_shape_fuzzer = os.path.join(top_builddir, "hb-shape-fuzzer" + EXEEXT)
if not os.path.exists(hb_shape_fuzzer):
    # If not found automatically, fall back to the first CLI argument.
    if len(sys.argv) < 2 or not os.path.exists(sys.argv[1]):
        sys.exit(
            "Failed to find hb-shape-fuzzer binary automatically.\n"
            "Please provide it as the first argument to the tool."
        )
    hb_shape_fuzzer = sys.argv[1]

print("hb_shape_fuzzer:", hb_shape_fuzzer)

fonts_dir = os.path.join(srcdir, "fonts")
if not os.path.isdir(fonts_dir):
    sys.exit(f"Fonts directory not found at: {fonts_dir}")

# Gather all files in `fonts_dir`
files_to_test = [
    os.path.join(fonts_dir, f)
    for f in os.listdir(fonts_dir)
    if os.path.isfile(os.path.join(fonts_dir, f))
]

if not files_to_test:
    print(f"No files found in {fonts_dir}")
    sys.exit(1)

# Single invocation with all test files
cmd_line = [hb_shape_fuzzer] + files_to_test
output, returncode = run_command(cmd_line)

# Print output if any
if output.strip():
    print(output)

# Fail if return code is non-zero
if returncode != 0:
    print("Failure on the following file(s):")
    for f in files_to_test:
        print("  ", f)
    sys.exit("1 shape fuzzer test failed.")

print("All shape fuzzer tests passed successfully.")
