#!/usr/bin/env python3
"""Generate a minimal font with a GSUB lookup of three SingleSubst subtables.

The font has four glyphs.  The subtables of its one lookup map, in order:

  1 -> 2             (format 1)
  0 -> 3 and 2 -> 1  (format 2)
  3 -> 1             (format 1)

So glyphs 0 and 2 are covered only by a subtable that follows a format 1
one, and glyph 3 only by a subtable that follows one of each format.
"""
from pathlib import Path
import struct

OUT = Path(__file__).with_name("single-subst-three-subtables.ttf")


def u16(v):
    return struct.pack(">H", v & 0xFFFF)


def u32(v):
    return struct.pack(">I", v)


def coverage(*glyphs):
    return u16(1) + u16(len(glyphs)) + b"".join(u16(g) for g in glyphs)


# SingleSubstFormat1: format, coverage offset, deltaGlyphID.
def single1(glyph, substitute):
    return u16(1) + u16(6) + u16(substitute - glyph) + coverage(glyph)


# SingleSubstFormat2: format, coverage offset, glyphCount, substitutes.
def single2(mapping):
    glyphs = sorted(mapping)
    return (u16(2) + u16(6 + 2 * len(glyphs)) + u16(len(glyphs))
            + b"".join(u16(mapping[g]) for g in glyphs)
            + coverage(*glyphs))


subtables = [single1(1, 2), single2({0: 3, 2: 1}), single1(3, 1)]

# Lookup: type, flag, subtable count, subtable offsets.
lookup = u16(1) + u16(0) + u16(len(subtables))
offset = 6 + 2 * len(subtables)
for subtable in subtables:
    lookup += u16(offset)
    offset += len(subtable)
lookup += b"".join(subtables)
lookup_list = u16(1) + u16(4) + lookup

feature_list = u16(1) + b"test" + u16(8) + u16(0) + u16(1) + u16(0)
lang_sys = u16(0) + u16(0xFFFF) + u16(1) + u16(0)
script_list = u16(1) + b"DFLT" + u16(8) + u16(4) + u16(0) + lang_sys

script_list_offset = 10
feature_list_offset = script_list_offset + len(script_list)
lookup_list_offset = feature_list_offset + len(feature_list)
gsub = (u16(1) + u16(0)
        + u16(script_list_offset)
        + u16(feature_list_offset)
        + u16(lookup_list_offset)
        + script_list + feature_list + lookup_list)

maxp = u32(0x00005000) + u16(4)

tables = sorted([(b"GSUB", gsub), (b"maxp", maxp)])
font = u32(0x00010000) + u16(len(tables)) + u16(32) + u16(1) + u16(0)
data = b""
offset = 12 + 16 * len(tables)
for tag, table in tables:
    font += tag + u32(0) + u32(offset + len(data)) + u32(len(table))
    data += table + b"\0" * (-len(table) % 4)
OUT.write_bytes(font + data)
