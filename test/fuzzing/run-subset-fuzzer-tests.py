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
    EXEEXT = os.getenv("EXEEXT", "")
    top_builddir = os.getenv("top_builddir", ".")

    # Locate the binary
    default_bin = os.path.join(top_builddir, "hb-subset-fuzzer" + EXEEXT)
    hb_subset_fuzzer = find_fuzzer_binary(default_bin, sys.argv)

    print("Using hb_subset_fuzzer:", hb_subset_fuzzer)

    # Gather from two directories, then combine
    dir1 = os.path.join(srcdir, "..", "subset", "data", "fonts")
    dir2 = os.path.join(srcdir, "fonts")

    files_to_test = gather_files(dir1) + gather_files(dir2)

    if not files_to_test:
        print("No files found in either directory.")
        sys.exit(0)

    fails = 0
    batch_index = 0

    # Batch the tests in up to 64 files per run
    for chunk in chunkify(files_to_test, 64):
        batch_index += 1
        cmd_line = [hb_subset_fuzzer] + chunk
        output, returncode = run_command(cmd_line)

        if output.strip():
            print(output)

        if returncode != 0:
            print(f"Failure in batch #{batch_index}")
            fails += 1

    if fails > 0:
        sys.exit(f"{fails} subset fuzzer batch(es) failed.")

    print("All subset fuzzer tests passed successfully.")

if __name__ == "__main__":
    main()
