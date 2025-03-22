#!/usr/bin/env python3

import pathlib
import subprocess
import os
import sys
import tempfile

EXE_WRAPPER = os.environ.get("MESON_EXE_WRAPPER")


def run_command(command):
    """
    Run a command, capturing potentially large output in a temp file.
    Returns (output_string, exit_code).
    """
    global EXE_WRAPPER
    if EXE_WRAPPER:
        command = [EXE_WRAPPER] + command

    with tempfile.TemporaryFile() as tempf:
        p = subprocess.Popen(command, stdout=tempf, stderr=tempf)
        p.wait()
        tempf.seek(0)
        output = tempf.read().decode("utf-8", errors="replace").strip()
    return output, p.returncode


def chunkify(lst, chunk_size=64):
    """
    Yield successive chunk_size-sized slices from lst.
    """
    for i in range(0, len(lst), chunk_size):
        yield lst[i : i + chunk_size]


def main():
    assert len(sys.argv) > 2, "Please provide fuzzer binary and fonts directory paths."

    fuzzer = pathlib.Path(sys.argv[1])
    assert fuzzer.is_file(), f"Fuzzer binary not found: {fuzzer}"
    print("Using fuzzer:", fuzzer)

    # Gather all test files
    files_to_test = []
    for fonts_dir in sys.argv[2:]:
        fonts_dir = pathlib.Path(fonts_dir)
        assert fonts_dir.is_dir(), f"Fonts directory not found: {fonts_dir}"
        test_files = [str(f) for f in fonts_dir.iterdir() if f.is_file()]
        assert test_files, f"No files found in {fonts_dir}"
        files_to_test += test_files

    if not files_to_test:
        print("No test files found")
        sys.exit(0)

    fails = 0
    batch_index = 0

    # Run in batches of up to 64 files
    for chunk in chunkify(files_to_test, 64):
        batch_index += 1
        cmd_line = [fuzzer] + chunk
        output, returncode = run_command(cmd_line)

        if output:
            print(output)

        if returncode != 0:
            print(f"Failure in batch #{batch_index}")
            fails += 1

    if fails > 0:
        sys.exit(f"{fails} fuzzer batch(es) failed.")

    print("All fuzzer tests passed successfully.")


if __name__ == "__main__":
    main()
