#!/usr/bin/env python3

import os
import sys

from hb_fuzzing_tools import (
    fail,
    format_leading_comment,
    main_with_user_errors,
    parse_output_arg,
    print_recorded_seed_path,
    write_seed_file,
)


COMMAND_NAMES = ("hb-shape", "hb-raster", "hb-vector")


def usage(prog: str) -> str:
    return f"""Usage: {prog} [-o OUTPUT_OR_DIR] HB_{'{SHAPE|RASTER|VECTOR}'}_COMMAND FONT_FILE [HB_TOOL_ARGS...]

Generate a shared text/render seed using the hb-shape-style payload format:
an optional leading comment line containing the exact CLI invocation,
followed by the raw font bytes.

This recorder does not encode text, features, or variations into the payload.
Those are preserved only in the leading comment line for human recovery.

Options:
  -o OUTPUT_OR_DIR
              Write the seed to this file, or into this directory using
              <sha1(font)>.seed.
              Default: test/fuzzing/fonts/<sha1(font)>.seed
  -h, --help  Show this help.
"""


def find_font_file(args: list[str]) -> str:
    if len(args) < 2:
        fail("Expected at least a HarfBuzz text/render command and FONT_FILE.")

    command = args[0]
    base = os.path.basename(command)
    if not any(name in base for name in COMMAND_NAMES):
        fail(f"First argument does not look like one of {', '.join(COMMAND_NAMES)}: '{command}'.")

    font_file = args[1]
    if font_file.startswith("-"):
        fail("Expected FONT_FILE as the second positional argument.")

    return font_file


def record_test_main(argv: list[str]) -> int:
    usage_text = usage(argv[0])
    out, args = parse_output_arg(argv, usage_text)

    if len(args) < 2:
        print(usage_text, end="", file=sys.stderr)
        return 1

    font_file = find_font_file(args)
    seed_path = write_seed_file(
        font_file,
        out,
        bytearray(),
        default_suffix=".seed",
        leading_comment=format_leading_comment(args),
    )
    print_recorded_seed_path(seed_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main_with_user_errors(record_test_main, sys.argv))
