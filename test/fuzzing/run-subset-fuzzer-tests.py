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
        output = tempf.read().decode("utf-8", errors="replace")
    return output, p.returncode

# Environment variables and binary location
srcdir = os.getenv("srcdir", ".")
EXEEXT = os.getenv("EXEEXT", "")
top_builddir = os.getenv("top_builddir", ".")

hb_subset_fuzzer = os.path.join(top_builddir, "hb-subset-fuzzer" + EXEEXT)
# If not found automatically, fall back to the first CLI argument
if not os.path.exists(hb_subset_fuzzer):
    if len(sys.argv) < 2 or not os.path.exists(sys.argv[1]):
        sys.exit(
            "Failed to find hb-subset-fuzzer binary automatically.\n"
            "Please provide it as the first argument to the tool."
        )
    hb_subset_fuzzer = sys.argv[1]

print("hb_subset_fuzzer:", hb_subset_fuzzer)

# Gather all files from both directories
dir1 = os.path.join(srcdir, "..", "subset", "data", "fonts")
dir2 = os.path.join(srcdir, "fonts")

files_to_test = []

for d in [dir1, dir2]:
    if not os.path.isdir(d):
        # Skip if the directory doesn't exist
        continue
    for f in os.listdir(d):
        file_path = os.path.join(d, f)
        if os.path.isfile(file_path):
            files_to_test.append(file_path)

if not files_to_test:
    print("No fonts found in either directory.")
    sys.exit(1)

# Run the fuzzer once, passing all collected files
print(f"Running subset fuzzer on {len(files_to_test)} file(s).")
cmd_line = [hb_subset_fuzzer] + files_to_test
output, returncode = run_command(cmd_line)

# Print any output
if output.strip():
    print(output)

# If there's an error, exit non-zero
if returncode != 0:
    print("Failure while processing these files:")
    for f in files_to_test:
        print(" ", f)
    sys.exit("1 subset fuzzer test failed.")

print("All subset fuzzer tests passed successfully.")
