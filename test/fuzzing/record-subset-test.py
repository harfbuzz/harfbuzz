#!/usr/bin/env python3

import math
import os
import sys

from hb_fuzzing_tools import (
    FLAG_BITS,
    HB_FUZZING_AXIS_PIN_TO_DEFAULT,
    HB_FUZZING_AXIS_SET_RANGE,
    HB_FUZZING_OP_SET_ADD_RANGES,
    HB_FUZZING_OP_TEXT_ADD,
    HB_FUZZING_OP_TEXT_DEL,
    HB_SUBSET_SETS_DROP_TABLE_TAG,
    HB_SUBSET_SETS_GLYPH_INDEX,
    HB_SUBSET_SETS_LAYOUT_FEATURE_TAG,
    HB_SUBSET_SETS_LAYOUT_SCRIPT_TAG,
    HB_SUBSET_SETS_NAME_ID,
    HB_SUBSET_SETS_NAME_LANG_ID,
    HB_SUBSET_SETS_UNICODE,
    add_set_option,
    emit_axis_records,
    emit_keep_everything,
    emit_pin_all_axes_to_default,
    emit_set_flags,
    emit_set_ranges,
    emit_text,
    fail,
    main_with_user_errors,
    parse_decimal_ranges,
    parse_hex_ranges,
    parse_output_arg,
    parse_tag_value,
    parse_tags,
    parse_text_codepoints,
    print_recorded_seed_path,
    read_range_file,
    read_text_file,
    write_seed_file,
)


KEEP_EVERYTHING_FLAGS = (
    0x00000040 |
    0x00000080 |
    0x00000008 |
    0x00000100 |
    0x00000020
)

SET_OPTION_TYPES = {
    "--gids": HB_SUBSET_SETS_GLYPH_INDEX,
    "--name-IDs": HB_SUBSET_SETS_NAME_ID,
    "--name-languages": HB_SUBSET_SETS_NAME_LANG_ID,
    "--layout-features": HB_SUBSET_SETS_LAYOUT_FEATURE_TAG,
    "--layout-scripts": HB_SUBSET_SETS_LAYOUT_SCRIPT_TAG,
    "--drop-tables": HB_SUBSET_SETS_DROP_TABLE_TAG,
    "--unicodes": HB_SUBSET_SETS_UNICODE,
}

VALUE_OPTIONS_TO_IGNORE = {
    "-o",
    "--output-file",
    "--num-iterations",
}

IGNORED_OPTIONS = {
    "--batch",
    "--preprocess",
}


def usage(prog: str) -> str:
    return f"""Usage: {prog} [-o OUTPUT_OR_DIR] HB_SUBSET_COMMAND [HB_SUBSET_ARGS...]

Generate an hb-subset-fuzzer seed by appending a versioned operation trailer to
the input font bytes. Existing hb-subset-fuzzer corpus files remain valid; this
script always emits the newer extended encoding.

Supported hb-subset inputs:
  - font file via positional argument or --font-file=FILE
  - boolean subset flags
  - --keep-everything
  - --text / --text-file and additive/subtractive forms
  - --unicodes / --unicodes-file and additive/subtractive forms
  - --gids / --gids-file
  - --name-IDs
  - --name-languages
  - --layout-features
  - --layout-scripts
  - --drop-tables
  - --instance / --variations with fixed values, ranges, and drop/default pins

Still unsupported:
  - --glyphs / --glyphs-file
  - --gid-map

Options:
  -o OUTPUT_OR_DIR
              Write the seed to this file, or into this directory using
              <sha1(font)>.susbset-seed.
              Default: test/fuzzing/fonts/<sha1(font)>.susbset-seed
  -h, --help  Show this help.
"""


def parse_axis_number(text: str) -> float:
    try:
        return float(text)
    except ValueError:
        fail(f"Failed parsing axis value '{text}'.")


def parse_axis_range(value: str) -> tuple[int, float, float, float]:
    if value == "drop":
        return HB_FUZZING_AXIS_PIN_TO_DEFAULT, 0.0, 0.0, 0.0

    parts = value.split(":")
    if len(parts) == 1:
        v = parse_axis_number(parts[0])
        return HB_FUZZING_AXIS_SET_RANGE, v, v, v
    if len(parts) == 2:
        minimum = math.nan if parts[0] == "" else parse_axis_number(parts[0])
        maximum = math.nan if parts[1] == "" else parse_axis_number(parts[1])
        return HB_FUZZING_AXIS_SET_RANGE, minimum, math.nan, maximum
    if len(parts) == 3:
        minimum = math.nan if parts[0] == "" else parse_axis_number(parts[0])
        middle = math.nan if parts[1] == "" else parse_axis_number(parts[1])
        maximum = math.nan if parts[2] == "" else parse_axis_number(parts[2])
        return HB_FUZZING_AXIS_SET_RANGE, minimum, middle, maximum
    fail(f"Failed parsing axis range '{value}'.")


def parse_variations(value: str) -> tuple[bool, list[tuple[int, int, float, float, float]]]:
    pin_all = False
    records: list[tuple[int, int, float, float, float]] = []
    for item in value.replace(" ", ",").split(","):
        if not item:
            continue
        if "=" not in item:
            fail(f"Unsupported variation item '{item}'. Expected tag=value.")
        tag_text, axis_value_text = item.split("=", 1)
        if tag_text == "*":
            if axis_value_text != "drop":
                fail("Only *=drop is supported for wildcard axis pinning.")
            pin_all = True
            continue
        tag = parse_tag_value(tag_text, "Axis tag")
        mode, minimum, middle, maximum = parse_axis_range(axis_value_text)
        records.append((tag, mode, minimum, middle, maximum))
    return pin_all, records


def parser_for_set_option(base_name: str):
    if base_name == "--unicodes":
        return parse_hex_ranges
    if base_name in ("--layout-features", "--layout-scripts", "--drop-tables"):
        return parse_tags
    return parse_decimal_ranges


def handle_file_backed_set_option(ops: bytearray, name: str, path: str) -> None:
    if name == "--gids-file":
        emit_set_ranges(ops, HB_FUZZING_OP_SET_ADD_RANGES, HB_SUBSET_SETS_GLYPH_INDEX, read_range_file(path, parse_decimal_ranges))
        return
    if name == "--unicodes-file":
        emit_set_ranges(ops, HB_FUZZING_OP_SET_ADD_RANGES, HB_SUBSET_SETS_UNICODE, read_range_file(path, parse_hex_ranges))
        return
    fail(f"Unsupported file-backed option '{name}'.")


def record_subset_main(argv: list[str]) -> int:
    usage_text = usage(argv[0])
    out, args = parse_output_arg(argv, usage_text)

    if len(args) < 2:
        print(usage_text, end="", file=sys.stderr)
        return 1

    hb_subset = args[0]
    if "hb-subset" not in os.path.basename(hb_subset):
        fail(f"First argument does not look like hb-subset: '{hb_subset}'.")

    font_file = None
    flags = 0
    ops = bytearray()
    positionals: list[str] = []

    i = 1
    while i < len(args):
        arg = args[i]

        if arg in FLAG_BITS:
            flags |= FLAG_BITS[arg]
            emit_set_flags(ops, flags)
            i += 1
            continue

        if arg == "--keep-everything":
            emit_keep_everything(ops)
            flags = KEEP_EVERYTHING_FLAGS
            i += 1
            continue

        if arg in IGNORED_OPTIONS:
            i += 1
            continue

        ignored_value = False
        for prefix in VALUE_OPTIONS_TO_IGNORE:
            if arg == prefix:
                if i + 1 >= len(args):
                    fail(f"Missing value for '{arg}'.")
                i += 2
                ignored_value = True
                break
            if arg.startswith(prefix + "="):
                i += 1
                ignored_value = True
                break
        if ignored_value:
            continue

        if arg == "--font-file":
            if i + 1 >= len(args):
                fail("Missing value for '--font-file'.")
            font_file = args[i + 1]
            i += 2
            continue
        if arg.startswith("--font-file="):
            font_file = arg.split("=", 1)[1]
            i += 1
            continue

        if arg in ("--text", "--text+", "--text-"):
            if i + 1 >= len(args):
                fail(f"Missing value for '{arg}'.")
            emit_text(ops, HB_FUZZING_OP_TEXT_DEL if arg.endswith("-") else HB_FUZZING_OP_TEXT_ADD, parse_text_codepoints(args[i + 1]))
            i += 2
            continue
        if arg.startswith("--text=") or arg.startswith("--text+=") or arg.startswith("--text-="):
            name, value = arg.split("=", 1)
            emit_text(ops, HB_FUZZING_OP_TEXT_DEL if name.endswith("-") else HB_FUZZING_OP_TEXT_ADD, parse_text_codepoints(value))
            i += 1
            continue
        if arg == "--text-file":
            if i + 1 >= len(args):
                fail("Missing value for '--text-file'.")
            emit_text(ops, HB_FUZZING_OP_TEXT_ADD, read_text_file(args[i + 1]))
            i += 2
            continue
        if arg.startswith("--text-file="):
            emit_text(ops, HB_FUZZING_OP_TEXT_ADD, read_text_file(arg.split("=", 1)[1]))
            i += 1
            continue

        handled_set_option = False
        for base_name, set_type in SET_OPTION_TYPES.items():
            for modifier in ("", "+", "-"):
                full_name = base_name + modifier
                if arg == full_name:
                    if i + 1 >= len(args):
                        fail(f"Missing value for '{arg}'.")
                    add_set_option(ops, set_type, modifier, args[i + 1], parser_for_set_option(base_name))
                    i += 2
                    handled_set_option = True
                    break
                if arg.startswith(full_name + "="):
                    add_set_option(ops, set_type, modifier, arg.split("=", 1)[1], parser_for_set_option(base_name))
                    i += 1
                    handled_set_option = True
                    break
            if handled_set_option:
                break
        if handled_set_option:
            continue

        if arg == "--gids-file":
            if i + 1 >= len(args):
                fail("Missing value for '--gids-file'.")
            handle_file_backed_set_option(ops, arg, args[i + 1])
            i += 2
            continue
        if arg == "--unicodes-file":
            if i + 1 >= len(args):
                fail("Missing value for '--unicodes-file'.")
            handle_file_backed_set_option(ops, arg, args[i + 1])
            i += 2
            continue
        if arg.startswith("--gids-file=") or arg.startswith("--unicodes-file="):
            name, value = arg.split("=", 1)
            handle_file_backed_set_option(ops, name, value)
            i += 1
            continue

        if arg in ("--instance", "--variations"):
            if i + 1 >= len(args):
                fail(f"Missing value for '{arg}'.")
            pin_all, records = parse_variations(args[i + 1])
            if pin_all:
                emit_pin_all_axes_to_default(ops)
            if records:
                emit_axis_records(ops, records)
            i += 2
            continue
        if arg.startswith("--instance=") or arg.startswith("--variations="):
            pin_all, records = parse_variations(arg.split("=", 1)[1])
            if pin_all:
                emit_pin_all_axes_to_default(ops)
            if records:
                emit_axis_records(ops, records)
            i += 1
            continue

        if arg.startswith("--glyphs") or arg.startswith("--gid-map"):
            fail(f"Unsupported hb-subset option '{arg}'.")

        if arg.startswith("-"):
            fail(f"Unsupported option '{arg}'.")

        positionals.append(arg)
        i += 1

    if font_file is None:
        if not positionals:
            fail("No font file found in hb-subset invocation.")
        font_file = positionals[0]
        positionals = positionals[1:]

    for text_arg in positionals:
        emit_text(ops, HB_FUZZING_OP_TEXT_ADD, parse_text_codepoints(text_arg))

    print_recorded_seed_path(write_seed_file(font_file, out, ops))
    return 0


if __name__ == "__main__":
    raise SystemExit(main_with_user_errors(record_subset_main, sys.argv))
