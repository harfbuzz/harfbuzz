#!/usr/bin/env python3

import sys
import os
import subprocess
import tempfile

def run_command(command):
    with tempfile.TemporaryFile() as tempf:
        p = subprocess.Popen(command, stdout=tempf, stderr=tempf)
        p.wait()
        tempf.seek(0)
        output = tempf.read().decode('utf-8', errors='replace')
    return output, p.returncode

srcdir = os.getenv("srcdir", ".")
EXEEXT = os.getenv("EXEEXT", "")
top_builddir = os.getenv("top_builddir", ".")

hb_draw_fuzzer = os.path.join(top_builddir, "hb-draw-fuzzer" + EXEEXT)
# If not found automatically, try sys.argv[1]
if not os.path.exists(hb_draw_fuzzer):
    if len(sys.argv) < 2 or not os.path.exists(sys.argv[1]):
        sys.exit(
            "Failed to find hb-draw-fuzzer binary automatically.\n"
            "Please provide it as the first argument to the tool."
        )
    hb_draw_fuzzer = sys.argv[1]

print("Using hb_draw_fuzzer:", hb_draw_fuzzer)

# Collect all files from the fonts/ directory
parent_path = os.path.join(srcdir, "fonts")
if not os.path.isdir(parent_path):
    sys.exit(f"Directory {parent_path} not found or not a directory.")

files_to_check = [
    os.path.join(parent_path, f) for f in os.listdir(parent_path)
    if os.path.isfile(os.path.join(parent_path, f))
]

if not files_to_check:
    print(f"No files found in {parent_path}")
    sys.exit(1)

# Single invocation passing all files
cmd_line = [hb_draw_fuzzer] + files_to_check
output, returncode = run_command(cmd_line)

# Print output if not empty
if output.strip():
    print(output)

# If there's an error, print a message and exit non-zero
if returncode != 0:
    print("Failure while processing these files:", ", ".join(os.path.basename(f) for f in files_to_check))
    sys.exit(returncode)

print("All files processed successfully.")
