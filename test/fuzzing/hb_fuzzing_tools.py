#!/usr/bin/env python3

import hashlib
import os
import shlex
import struct
import sys


HB_FUZZING_EXTENDED_MAGIC = b"HBSUBFZ2"

HB_SUBSET_SETS_GLYPH_INDEX = 0
HB_SUBSET_SETS_UNICODE = 1
HB_SUBSET_SETS_DROP_TABLE_TAG = 3
HB_SUBSET_SETS_NAME_ID = 4
HB_SUBSET_SETS_NAME_LANG_ID = 5
HB_SUBSET_SETS_LAYOUT_FEATURE_TAG = 6
HB_SUBSET_SETS_LAYOUT_SCRIPT_TAG = 7

HB_FUZZING_OP_SET_FLAGS = 1
HB_FUZZING_OP_KEEP_EVERYTHING = 2
HB_FUZZING_OP_SET_CLEAR = 3
HB_FUZZING_OP_SET_INVERT = 4
HB_FUZZING_OP_SET_ADD_RANGES = 5
HB_FUZZING_OP_SET_DEL_RANGES = 6
HB_FUZZING_OP_TEXT_ADD = 7
HB_FUZZING_OP_TEXT_DEL = 8
HB_FUZZING_OP_AXIS_PIN_ALL_TO_DEFAULT = 9
HB_FUZZING_OP_AXIS_SET = 10

HB_FUZZING_AXIS_PIN_TO_DEFAULT = 0
HB_FUZZING_AXIS_SET_RANGE = 1

FLAG_BITS = {
    "--no-hinting": 0x00000001,
    "--retain-gids": 0x00000002,
    "--desubroutinize": 0x00000004,
    "--name-legacy": 0x00000008,
    "--set-overlaps-flag": 0x00000010,
    "--passthrough-tables": 0x00000020,
    "--notdef-outline": 0x00000040,
    "--glyph-names": 0x00000080,
    "--no-prune-unicode-ranges": 0x00000100,
    "--no-layout-closure": 0x00000200,
    "--optimize": 0x00000400,
    "--no-bidi-closure": 0x00000800,
    "--iftb-requirements": 0x00001000,
    "--retain-num-glyphs": 0x00002000,
    "--downgrade-cff2": 0x00004000,
}


class UserError(Exception):
    pass


def fail(msg: str) -> None:
    raise UserError(msg)


def pack_u32(value: int) -> bytes:
    return struct.pack("<I", value & 0xFFFFFFFF)


def pack_f32(value: float) -> bytes:
    return struct.pack("<f", value)


def emit_set_flags(ops: bytearray, flags: int) -> None:
    ops.append(HB_FUZZING_OP_SET_FLAGS)
    ops += pack_u32(flags)


def emit_keep_everything(ops: bytearray) -> None:
    ops.append(HB_FUZZING_OP_KEEP_EVERYTHING)


def emit_set_clear(ops: bytearray, set_type: int) -> None:
    ops += bytes((HB_FUZZING_OP_SET_CLEAR, set_type))


def emit_set_invert(ops: bytearray, set_type: int) -> None:
    ops += bytes((HB_FUZZING_OP_SET_INVERT, set_type))


def emit_set_ranges(ops: bytearray, opcode: int, set_type: int, ranges: list[tuple[int, int]]) -> None:
    ops += bytes((opcode, set_type))
    ops += pack_u32(len(ranges))
    for start, end in ranges:
        ops += pack_u32(start)
        ops += pack_u32(end)


def emit_text(ops: bytearray, opcode: int, codepoints: list[int]) -> None:
    ops.append(opcode)
    ops += pack_u32(len(codepoints))
    for cp in codepoints:
        ops += pack_u32(cp)


def emit_pin_all_axes_to_default(ops: bytearray) -> None:
    ops.append(HB_FUZZING_OP_AXIS_PIN_ALL_TO_DEFAULT)


def emit_axis_records(ops: bytearray, records: list[tuple[int, int, float, float, float]]) -> None:
    ops.append(HB_FUZZING_OP_AXIS_SET)
    ops += pack_u32(len(records))
    for tag, mode, minimum, middle, maximum in records:
        ops += pack_u32(tag)
        ops.append(mode)
        ops += pack_f32(minimum)
        ops += pack_f32(middle)
        ops += pack_f32(maximum)


def parse_decimal_ranges(value: str) -> list[tuple[int, int]]:
    ranges: list[tuple[int, int]] = []
    for chunk in value.replace(" ", ",").split(","):
        if not chunk:
            continue
        if chunk == "*":
            fail("Internal error: wildcard range should be handled before parsing.")
        if "-" in chunk:
            start_text, end_text = chunk.split("-", 1)
            try:
                start = int(start_text, 10)
                end = int(end_text, 10)
            except ValueError:
                fail(f"Failed parsing decimal range '{chunk}'.")
        else:
            try:
                start = end = int(chunk, 10)
            except ValueError:
                fail(f"Failed parsing decimal value '{chunk}'.")
        if start < 0 or end < start:
            fail(f"Invalid decimal range '{chunk}'.")
        ranges.append((start, end))
    return ranges


def parse_hex_ranges(value: str) -> list[tuple[int, int]]:
    ranges: list[tuple[int, int]] = []
    for chunk in value.replace(" ", ",").split(","):
        if not chunk:
            continue
        if chunk == "*":
            fail("Internal error: wildcard range should be handled before parsing.")
        if "-" in chunk:
            start_text, end_text = chunk.split("-", 1)
            try:
                start = int(start_text, 16)
                end = int(end_text, 16)
            except ValueError:
                fail(f"Failed parsing unicode range '{chunk}'.")
        else:
            try:
                start = end = int(chunk, 16)
            except ValueError:
                fail(f"Failed parsing unicode value '{chunk}'.")
        if start < 0 or end < start or end > 0x10FFFF:
            fail(f"Invalid unicode range '{chunk}'.")
        ranges.append((start, end))
    return ranges


def parse_tags(value: str) -> list[tuple[int, int]]:
    tags: list[tuple[int, int]] = []
    for chunk in value.replace(",", " ").split():
        if chunk == "*":
            fail("Internal error: wildcard tag set should be handled before parsing.")
        if len(chunk) > 4:
            fail(f"Tag '{chunk}' is longer than 4 bytes.")
        try:
            tag_value = int.from_bytes(chunk.ljust(4, " ").encode("ascii"), byteorder="big")
        except UnicodeEncodeError:
            fail(f"Tag '{chunk}' is not ASCII.")
        tags.append((tag_value, tag_value))
    return tags


def parse_text_codepoints(value: str) -> list[int]:
    codepoints = [ord(ch) for ch in value]
    for cp in codepoints:
        if cp < 0 or cp > 0x10FFFF:
            fail(f"Invalid codepoint value {cp}.")
    return codepoints


def parse_tag_value(tag_text: str, kind: str) -> int:
    if len(tag_text) > 4:
        fail(f"{kind} '{tag_text}' is longer than 4 bytes.")
    try:
        return int.from_bytes(tag_text.ljust(4, " ").encode("ascii"), byteorder="big")
    except UnicodeEncodeError:
        fail(f"{kind} '{tag_text}' is not ASCII.")


def add_set_option(ops: bytearray, set_type: int, modifier: str, value: str, parser) -> None:
    if modifier == "":
        emit_set_clear(ops, set_type)

    if value == "*":
        if modifier == "-":
            emit_set_clear(ops, set_type)
        else:
            emit_set_clear(ops, set_type)
            emit_set_invert(ops, set_type)
        return

    ranges = parser(value)
    if ranges:
        emit_set_ranges(
            ops,
            HB_FUZZING_OP_SET_DEL_RANGES if modifier == "-" else HB_FUZZING_OP_SET_ADD_RANGES,
            set_type,
            ranges,
        )


def parse_output_arg(argv: list[str], usage_text: str) -> tuple[str, list[str]]:
    out = ""
    args = argv[1:]

    while args:
        arg = args[0]
        if arg == "-o":
            if len(args) < 2:
                print(usage_text, end="", file=sys.stderr)
                raise SystemExit(1)
            out = args[1]
            args = args[2:]
            continue
        if arg in ("-h", "--help"):
            print(usage_text, end="")
            raise SystemExit(0)
        if arg == "--":
            args = args[1:]
            break
        break

    return out, args


def read_text_file(path: str) -> list[int]:
    with open(path, "r", encoding="utf-8") as fp:
        return parse_text_codepoints(fp.read())


def read_range_file(path: str, parser) -> list[tuple[int, int]]:
    ranges = []
    with open(path, "r", encoding="utf-8") as fp:
        for line in fp:
            line = line.split("#", 1)[0].strip()
            if line:
                ranges.extend(parser(line))
    return ranges


def resolve_seed_output_path(font_path: str, font_bytes: bytes, out_path: str, default_suffix: str) -> tuple[str, str]:
    digest = hashlib.sha1(font_bytes).hexdigest()
    default_name = digest + default_suffix
    default_dir = os.path.join(os.path.dirname(__file__), "fonts")

    if not out_path:
        display_path = os.path.join(default_dir, default_name)
        return display_path, os.path.abspath(display_path)

    if out_path.endswith(os.sep):
        display_path = os.path.join(out_path, default_name)
        return display_path, os.path.abspath(display_path)

    abs_out_path = os.path.abspath(out_path)
    if os.path.isdir(abs_out_path):
        if os.path.isabs(out_path):
            display_path = os.path.join(out_path, default_name)
        else:
            display_path = os.path.join(out_path, default_name)
        return display_path, os.path.join(abs_out_path, default_name)

    return out_path, abs_out_path

def format_leading_comment(argv: list[str]) -> bytes:
    return ("$ " + " ".join(shlex.quote(arg) for arg in argv) + "\n").encode("utf-8")


def write_seed_file(font_file: str,
                    out_path: str,
                    ops: bytearray,
                    default_suffix: str = ".susbset-seed",
                    leading_comment: bytes = b"") -> str:
    font_path = os.path.abspath(font_file)
    if not os.path.isfile(font_path):
        fail(f"Font file not found: '{font_path}'.")

    with open(font_path, "rb") as fp:
        font_bytes = fp.read()

    display_path, seed_path = resolve_seed_output_path(font_path, font_bytes, out_path, default_suffix)
    parent = os.path.dirname(seed_path)
    if parent:
        os.makedirs(parent, exist_ok=True)

    with open(seed_path, "wb") as fp:
        fp.write(leading_comment)
        fp.write(font_bytes)
        fp.write(ops)
        fp.write(pack_u32(len(ops)))
        fp.write(HB_FUZZING_EXTENDED_MAGIC)

    return display_path


def print_recorded_seed_path(seed_path: str) -> None:
    print(seed_path)


def main_with_user_errors(main_fn, argv: list[str]) -> int:
    try:
        return main_fn(argv)
    except UserError as e:
        print(e, file=sys.stderr)
        return 1
