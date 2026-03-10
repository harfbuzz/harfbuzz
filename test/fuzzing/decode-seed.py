#!/usr/bin/env python3

import math
import os
import struct
import sys

from hb_fuzzing_tools import (
    HB_FUZZING_AXIS_PIN_TO_DEFAULT,
    HB_FUZZING_AXIS_SET_RANGE,
    HB_FUZZING_EXTENDED_MAGIC,
    HB_FUZZING_OP_AXIS_PIN_ALL_TO_DEFAULT,
    HB_FUZZING_OP_AXIS_SET,
    HB_FUZZING_OP_KEEP_EVERYTHING,
    HB_FUZZING_OP_SET_ADD_RANGES,
    HB_FUZZING_OP_SET_CLEAR,
    HB_FUZZING_OP_SET_DEL_RANGES,
    HB_FUZZING_OP_SET_FLAGS,
    HB_FUZZING_OP_SET_INVERT,
    HB_FUZZING_OP_TEXT_ADD,
    HB_FUZZING_OP_TEXT_DEL,
    UserError,
    fail,
    main_with_user_errors,
    parse_output_arg,
)


SET_TYPE_NAMES = {
    0: "glyph-index",
    1: "unicode",
    3: "drop-table",
    4: "name-id",
    5: "name-lang-id",
    6: "layout-feature",
    7: "layout-script",
}


def usage(prog: str) -> str:
    return f"""Usage: {prog} SEED_FILE...

Decode a shared HarfBuzz fuzzing seed in plain text.

Supports the extended HBSUBFZ2 trailer format used by the shape, raster,
vector, and subset fuzzers.
"""


def format_tag(tag: int) -> str:
    raw = tag.to_bytes(4, byteorder="big")
    try:
        text = raw.decode("ascii")
        if all(32 <= b <= 126 for b in raw):
            return f"{text!r}"
    except UnicodeDecodeError:
        pass
    return "0x" + raw.hex()


def format_float(value: float) -> str:
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:g}"


def format_codepoints(codepoints: list[int]) -> str:
    text = "".join(chr(cp) for cp in codepoints)
    cps = " ".join(f"U+{cp:04X}" for cp in codepoints)
    return f"text={text!r} cps=[{cps}]"


def split_leading_comment(data: bytes) -> tuple[str | None, bytes]:
    if not data.startswith(b"$"):
        return None, data
    newline = data.find(b"\n")
    if newline < 0:
        return None, data
    return data[:newline].decode("utf-8", errors="replace"), data[newline + 1 :]


def split_extended_payload(data: bytes) -> tuple[bytes, bytes | None]:
    if len(data) < len(HB_FUZZING_EXTENDED_MAGIC) + 4:
        return data, None
    if not data.endswith(HB_FUZZING_EXTENDED_MAGIC):
        return data, None

    ops_len = struct.unpack("<I", data[-12:-8])[0]
    prefix_len = len(data) - len(HB_FUZZING_EXTENDED_MAGIC) - 4 - ops_len
    if prefix_len < 0:
        fail("Malformed extended trailer: negative prefix length.")
    return data[:prefix_len], data[prefix_len:-12]


def read_u8(ops: bytes, offset: int) -> tuple[int, int]:
    if offset + 1 > len(ops):
        fail("Malformed ops payload: truncated u8.")
    return ops[offset], offset + 1


def read_u32(ops: bytes, offset: int) -> tuple[int, int]:
    if offset + 4 > len(ops):
        fail("Malformed ops payload: truncated u32.")
    return struct.unpack("<I", ops[offset : offset + 4])[0], offset + 4


def read_f32(ops: bytes, offset: int) -> tuple[float, int]:
    if offset + 4 > len(ops):
        fail("Malformed ops payload: truncated f32.")
    return struct.unpack("<f", ops[offset : offset + 4])[0], offset + 4


def decode_ops(ops: bytes) -> list[str]:
    out: list[str] = []
    offset = 0
    while offset < len(ops):
      op, offset = read_u8(ops, offset)

      if op in (HB_FUZZING_OP_TEXT_ADD, HB_FUZZING_OP_TEXT_DEL):
          count, offset = read_u32(ops, offset)
          cps: list[int] = []
          for _ in range(count):
              cp, offset = read_u32(ops, offset)
              cps.append(cp)
          name = "TEXT_ADD" if op == HB_FUZZING_OP_TEXT_ADD else "TEXT_DEL"
          out.append(f"{name} {format_codepoints(cps)}")
          continue

      if op == HB_FUZZING_OP_AXIS_PIN_ALL_TO_DEFAULT:
          out.append("AXIS_PIN_ALL_TO_DEFAULT")
          continue

      if op == HB_FUZZING_OP_AXIS_SET:
          count, offset = read_u32(ops, offset)
          records: list[str] = []
          for _ in range(count):
              tag, offset = read_u32(ops, offset)
              mode, offset = read_u8(ops, offset)
              minimum, offset = read_f32(ops, offset)
              middle, offset = read_f32(ops, offset)
              maximum, offset = read_f32(ops, offset)
              if mode == HB_FUZZING_AXIS_PIN_TO_DEFAULT:
                  mode_text = "pin-to-default"
              elif mode == HB_FUZZING_AXIS_SET_RANGE:
                  mode_text = (
                      f"set-range min={format_float(minimum)} "
                      f"mid={format_float(middle)} max={format_float(maximum)}"
                  )
              else:
                  mode_text = f"unknown-mode({mode})"
              records.append(f"tag={format_tag(tag)} {mode_text}")
          out.append("AXIS_SET " + "; ".join(records))
          continue

      if op == HB_FUZZING_OP_SET_FLAGS:
          flags, offset = read_u32(ops, offset)
          out.append(f"SET_FLAGS 0x{flags:08X}")
          continue

      if op == HB_FUZZING_OP_KEEP_EVERYTHING:
          out.append("KEEP_EVERYTHING")
          continue

      if op in (HB_FUZZING_OP_SET_CLEAR, HB_FUZZING_OP_SET_INVERT):
          set_type, offset = read_u8(ops, offset)
          name = "SET_CLEAR" if op == HB_FUZZING_OP_SET_CLEAR else "SET_INVERT"
          out.append(f"{name} {SET_TYPE_NAMES.get(set_type, f'unknown({set_type})')}")
          continue

      if op in (HB_FUZZING_OP_SET_ADD_RANGES, HB_FUZZING_OP_SET_DEL_RANGES):
          set_type, offset = read_u8(ops, offset)
          count, offset = read_u32(ops, offset)
          ranges: list[str] = []
          for _ in range(count):
              start, offset = read_u32(ops, offset)
              end, offset = read_u32(ops, offset)
              if start == end:
                  ranges.append(str(start))
              else:
                  ranges.append(f"{start}-{end}")
          name = "SET_ADD_RANGES" if op == HB_FUZZING_OP_SET_ADD_RANGES else "SET_DEL_RANGES"
          out.append(f"{name} {SET_TYPE_NAMES.get(set_type, f'unknown({set_type})')} [{', '.join(ranges)}]")
          continue

      fail(f"Unknown opcode {op} at byte offset {offset - 1}.")

    return out


def decode_file(path: str) -> None:
    with open(path, "rb") as fp:
        data = fp.read()

    comment, payload = split_leading_comment(data)
    prefix, ops = split_extended_payload(payload)

    print(f"file: {path}")
    if comment is not None:
        print(f"leading comment: {comment}")
    print(f"extended: {'yes' if ops is not None else 'no'}")
    print(f"prefix bytes: {len(prefix)}")

    if ops is not None:
        print(f"ops bytes: {len(ops)}")
        decoded = decode_ops(ops)
        if decoded:
            print("ops:")
            for line in decoded:
                print(f"  {line}")
        else:
            print("ops: <empty>")
    print()


def decode_seed_main(argv: list[str]) -> int:
    usage_text = usage(argv[0])
    _, args = parse_output_arg(argv, usage_text)
    if not args:
        print(usage_text, end="", file=sys.stderr)
        return 1

    for path in args:
        if not os.path.isfile(path):
            fail(f"Seed file not found: '{path}'.")
        decode_file(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main_with_user_errors(decode_seed_main, sys.argv))
