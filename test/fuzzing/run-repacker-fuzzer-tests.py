#!/usr/bin/env python3

import sys
import os
from hb_fuzzer_tools import (
    run_command,
    chunkify,
    find_fuzzer_binary,
    gather_files
)

def main():
    srcdir = os.getenv("srcdir", ".")
    top_builddir = os.getenv("top_builddir", ".")

    # Find the fuzzer binary
    default_bin = os.path.join(top_builddir, "hb-repacker-fuzzer")
    hb_repacker_fuzzer = find_fuzzer_binary(default_bin, sys.argv)

    print("Using hb_repacker_fuzzer:", hb_repacker_fuzzer)

    # Gather all files from graphs/
    graphs_dir = os.path.join(srcdir, "graphs")
    files_to_test = gather_files(graphs_dir)

    if not files_to_test:
        print("No files found in", graphs_dir)
        sys.exit(0)

    fails = 0
    batch_index = 0

    # Run in batches of up to 64 files
    for chunk in chunkify(files_to_test, 64):
        batch_index += 1
        cmd_line = [hb_repacker_fuzzer] + chunk
        output, returncode = run_command(cmd_line)

        if output.strip():
            print(output)

        if returncode != 0:
            print(f"Failure in batch #{batch_index}")
            fails += 1

    if fails > 0:
        sys.exit(f"{fails} repacker fuzzer batch(es) failed.")

    print("All repacker fuzzer tests passed successfully.")

if __name__ == "__main__":
    main()
