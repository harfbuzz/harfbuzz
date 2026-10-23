#!/usr/bin/env python3
"""Generate a minimal font with a GSUB ContextSubst format 6 lookup.

Format 6 is the beyond-64k coverage-based context format; its Coverage
offsets are 24-bit.  Lookup 0 (feature 'liga') matches glyphs 1 2 and
calls lookup 1 at sequence index 1.  Lookup 1 is a SingleSubst that
maps glyph 2 to glyph 3 and is not referenced by any feature, so the
2 -> 3 dependency is only found by recursing through the format 6 rule.
"""
from pathlib import Path
import struct

OUT = Path(__file__).parent / "fonts" / "context-format6.ttf"


def u16(v):
    return struct.pack(">H", v)


def u24(v):
    return struct.pack(">I", v)[1:]


def u32(v):
    return struct.pack(">I", v)


def coverage(*glyphs):
    return u16(1) + u16(len(glyphs)) + b"".join(u16(g) for g in glyphs)


# ContextFormat6: format, glyphCount, lookupCount, Offset24 coverages,
# then LookupRecords.
context_header_len = 6 + 2 * 3 + 4
cov0 = coverage(1)
cov1 = coverage(2)
context = (u16(6) + u16(2) + u16(1)
           + u24(context_header_len)
           + u24(context_header_len + len(cov0))
           + u16(1) + u16(1)  # sequenceIndex 1 -> lookup 1
           + cov0 + cov1)

# SingleSubstFormat1: 2 -> 3.
single = u16(1) + u16(6) + u16(1) + coverage(2)


def lookup(lookup_type, subtable):
    return u16(lookup_type) + u16(0) + u16(1) + u16(8) + subtable


lookups = [lookup(5, context), lookup(1, single)]
lookup_list = u16(len(lookups))
offset = 2 + 2 * len(lookups)
for l in lookups:
    lookup_list += u16(offset)
    offset += len(l)
lookup_list += b"".join(lookups)

feature_list = u16(1) + b"liga" + u16(8) + u16(0) + u16(1) + u16(0)
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
