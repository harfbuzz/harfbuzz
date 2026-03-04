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


def usage(prog: str) -> str:
    return f"""Usage: {prog} [-o OUTPUT_OR_DIR] HB_SHAPE_COMMAND FONT_FILE [HB_SHAPE_ARGS...]

Generate an hb-shape-fuzzer seed using the legacy payload format:
an optional leading comment line containing the exact hb-shape command,
followed by the raw font bytes.

This recorder does not encode text, features, or variations into the payload.
Those are preserved only in the leading comment line for human recovery.

Options:
  -o OUTPUT_OR_DIR
              Write the seed to this file, or into this directory using
              <sha1(font)>.shape-seed.
              Default: test/fuzzing/fonts/<sha1(font)>.shape-seed
  -h, --help  Show this help.
"""


def find_font_file(args: list[str]) -> str:
    if len(args) < 2:
        fail("Expected at least HB_SHAPE_COMMAND and FONT_FILE.")

    hb_shape = args[0]
    if "hb-shape" not in os.path.basename(hb_shape):
        fail(f"First argument does not look like hb-shape: '{hb_shape}'.")

    font_file = args[1]
    if font_file.startswith("-"):
        fail("Expected FONT_FILE as the second positional argument.")

    return font_file


def record_shape_main(argv: list[str]) -> int:
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
        default_suffix=".shape-seed",
        leading_comment=format_leading_comment(args),
    )
    print_recorded_seed_path(seed_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main_with_user_errors(record_shape_main, sys.argv))
