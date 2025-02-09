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


# Environment and binary location
srcdir = os.getenv("srcdir", ".")
EXEEXT = os.getenv("EXEEXT", "")
top_builddir = os.getenv("top_builddir", ".")

hb_repacker_fuzzer = os.path.join(top_builddir, "hb-repacker-fuzzer" + EXEEXT)
# If the binary isn't found, try sys.argv[1]
if not os.path.exists(hb_repacker_fuzzer):
    if len(sys.argv) < 2 or not os.path.exists(sys.argv[1]):
        sys.exit(
            "Failed to find hb-repacker-fuzzer binary automatically.\n"
            "Please provide it as the first argument to the tool."
        )
    hb_repacker_fuzzer = sys.argv[1]

print("hb_repacker_fuzzer:", hb_repacker_fuzzer)

# Collect all files from graphs/
graphs_path = os.path.join(srcdir, "graphs")
if not os.path.isdir(graphs_path):
    sys.exit(f"No 'graphs' directory found at {graphs_path}.")

files_to_check = [
    os.path.join(graphs_path, f)
    for f in os.listdir(graphs_path)
    if os.path.isfile(os.path.join(graphs_path, f))
]

if not files_to_check:
    print("No files found in the 'graphs' directory.")
    sys.exit(1)

# Single invocation passing all files
print(f"Running repacker fuzzer against {len(files_to_check)} file(s) in 'graphs'...")
cmd_line = [hb_repacker_fuzzer] + files_to_check
output, returncode = run_command(cmd_line)

# Print the output if present
if output.strip():
    print(output)

# Exit if there's an error
if returncode != 0:
    print("Failed for these files:")
    for f in files_to_check:
        print("  ", f)
    sys.exit("1 repacker fuzzer related test(s) failed.")

print("All repacker fuzzer tests passed successfully.")
