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

    default_bin = os.path.join(top_builddir, "hb-shape-fuzzer" + EXEEXT)
    hb_shape_fuzzer = find_fuzzer_binary(default_bin, sys.argv)

    print("Using hb_shape_fuzzer:", hb_shape_fuzzer)

    # Gather all files from fonts/
    fonts_dir = os.path.join(srcdir, "fonts")
    files_to_test = gather_files(fonts_dir)

    if not files_to_test:
        print("No files found in", fonts_dir)
        sys.exit(0)

    fails = 0
    batch_index = 0

    # Batch up to 64 files at a time
    for chunk in chunkify(files_to_test, 64):
        batch_index += 1
        cmd_line = [hb_shape_fuzzer] + chunk
        output, returncode = run_command(cmd_line)

        if output.strip():
            print(output)

        if returncode != 0:
            print(f"Failure in batch #{batch_index}")
            fails += 1

    if fails > 0:
        sys.exit(f"{fails} shape fuzzer batch(es) failed.")

    print("All shape fuzzer tests passed successfully.")

if __name__ == "__main__":
    main()
