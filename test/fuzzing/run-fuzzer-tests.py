#!/usr/bin/env python3

import sys
import os
from hb_fuzzer_tools import run_command, chunkify, gather_files


def main():
    # Find the fuzzer binary
    assert len(sys.argv) > 2, "Please provide fuzzer binary and fonts directory paths."
    assert os.path.exists(sys.argv[1]), "The fuzzer binary does not exist."
    assert os.path.exists(sys.argv[2]), "The fonts directory does not exist."

    fuzzer = sys.argv[1]
    fonts_dir = sys.argv[2]

    print("Using fuzzer:", fuzzer)

    # Gather all files from fonts/
    files_to_test = gather_files(fonts_dir)

    if not files_to_test:
        print("No files found in", fonts_dir)
        sys.exit(0)

    fails = 0
    batch_index = 0

    # Run in batches of up to 64 files
    for chunk in chunkify(files_to_test, 64):
        batch_index += 1
        cmd_line = [fuzzer] + chunk
        output, returncode = run_command(cmd_line)

        if output.strip():
            print(output)

        if returncode != 0:
            print(f"Failure in batch #{batch_index}")
            fails += 1

    if fails > 0:
        sys.exit(f"{fails} fuzzer batch(es) failed.")

    print("All fuzzer tests passed successfully.")


if __name__ == "__main__":
    main()
