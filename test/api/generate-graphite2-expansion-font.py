#!/usr/bin/env python3
# Generate the synthetic font used by test-graphite2-buffer.
# Based on Anthropic OSS Scanner finding ANT-2026-HEQW39B7.
# Usage: generate-graphite2-expansion-font.py FONT_OUT
import struct
import sys

GID_A, GID_X, NUM_GLYPHS = 1, 2, 3
X_PER_A = 40                            # Graphite2 refuses more than 64 glyphs per character

NEXT, INSERT, RET_ZERO, PUT_GLYPH = 0x19, 0x1F, 0x31, 0x3B     # Graphite action opcodes


def silf_pass(pass_offset):
    # one substitution rule:  a  >  a x x ... x
    insert_x = bytes([INSERT, PUT_GLYPH]) + struct.pack(">H", 0) + bytes([NEXT])     # PUT_GLYPH takes a class index
    action = bytes([NEXT]) + insert_x * X_PER_A + bytes([RET_ZERO])

    body = struct.pack(">4H", 1, 6, 0, 0)                       # numRange and its search header
    body += struct.pack(">3H", GID_A, GID_A, 0)                 # glyph range a..a -> FSM column 0
    body += struct.pack(">2H", 0, 1)                            # oRuleMap: success state 1 owns ruleMap[0:1]
    body += struct.pack(">H", 0)                                # ruleMap: rule 0
    body += struct.pack(">2B", 0, 0)                            # min/maxRulePreContext
    body += struct.pack(">H", 0)                                # start state
    body += struct.pack(">H", 1)                                # rule sort key: the rule matches 1 slot
    body += struct.pack(">B", 0)                                # rule pre-context
    body += struct.pack(">BH", 0, 0)                            # collision threshold, pass constraint length
    body += struct.pack(">2H", 0, 0)                            # oConstraints: no rule constraint
    body += struct.pack(">2H", 0, len(action))                  # oActions
    body += struct.pack(">H", 1)                                # transition: state 0, column 0 -> state 1
    body += b"\0"
    code_offset = pass_offset + 32 + len(body)
    header = struct.pack(">4B2H4L4H",
                         0, 1, 1, 0,                            # flags, maxRuleLoop, maxRuleContext, maxBackup
                         1, 0,                                  # numRules, fsmOffset
                         code_offset, code_offset, code_offset, 0,     # pcCode, rcCode, aCode, oDebug
                         2, 1, 1, 1)                            # numRows, numTransitional, numSuccess, numColumns
    return header + body + action


def silf_table():
    sub = struct.pack(">H2h", NUM_GLYPHS - 1, 0, 0)             # maxGlyphID, extraAscent, extraDescent
    sub += struct.pack(">5B", 1, 0, 1, 1, 0xFF)                 # numPasses, iSubst, iPos, iJust, iBidi
    sub += bytes(9)                                             # flags, maxPre/PostContext, five attribute numbers, numJLevels
    sub += struct.pack(">H4B", 0, 0, 0, 1, 0)                   # numLigComp, numUserDefn, maxCompPerLig, direction LTR, attCollisions
    sub += bytes(6)                                             # 3 reserved, numCritFeatures, reserved, numScriptTag
    sub += struct.pack(">H", 0xFFFF)                            # lbGID
    pseudos = struct.pack(">4H", 0, 0, 0, 0)                   # numPseudo and its search header
    classes = struct.pack(">4H", 1, 1, 8, 10)                  # numClass, numLinear, class offsets
    classes += struct.pack(">H", GID_X)                         # class 0 = [x]
    pass_offset = len(sub) + 8 + len(pseudos) + len(classes)
    the_pass = silf_pass(pass_offset)
    sub += struct.pack(">2L", pass_offset, pass_offset + len(the_pass)) + pseudos + classes + the_pass
    return struct.pack(">L2HL", 0x00020000, 1, 0, 12) + sub    # version 2, one sub-table at offset 12


def cmap_table():
    segments = struct.pack(">4H", 4, 4, 1, 0)                   # segCountX2 and its search header
    segments += struct.pack(">2H", 0x61, 0xFFFF) + b"\0\0"      # endCode
    segments += struct.pack(">2H", 0x61, 0xFFFF)                # startCode
    segments += struct.pack(">2h", GID_A - 0x61, 1)             # idDelta: 'a' -> GID_A
    segments += struct.pack(">2H", 0, 0)                        # idRangeOffset
    format_4 = struct.pack(">3H", 4, 6 + len(segments), 0) + segments
    return struct.pack(">2H2HL", 0, 1, 3, 1, 12) + format_4    # one Windows Unicode BMP subtable


tables = {
    b"head": struct.pack(">4L2H16x4h2H3h", 0x00010000, 0, 0, 0x5F0F3CF5, 0, 1000, 0, 0, 0, 0, 0, 8, 2, 0, 0),
    b"hhea": struct.pack(">L3hH11hH", 0x00010000, 800, -200, 0, 500, *[0] * 11, NUM_GLYPHS),
    b"maxp": struct.pack(">LH26x", 0x00010000, NUM_GLYPHS),
    b"hmtx": struct.pack(">Hh", 500, 0) * NUM_GLYPHS,
    b"cmap": cmap_table(),
    b"Silf": silf_table(),
    # Graphite2 wants an attribute run for every glyph: attribute 0, one value, 0
    b"Glat": struct.pack(">L", 0x00010000) + struct.pack(">2Bh", 0, 1, 0) * NUM_GLYPHS,
    b"Gloc": struct.pack(">L2H", 0x00010000, 0, 1) + b"".join(struct.pack(">H", 4 + 4 * gid) for gid in range(NUM_GLYPHS + 1)),
}

directory, data = b"", b""
for tag in sorted(tables):
    directory += struct.pack(">4s3L", tag, 0, 12 + 16 * len(tables) + len(data), len(tables[tag]))
    data += tables[tag] + b"\0" * (-len(tables[tag]) % 4)
font = struct.pack(">L4H", 0x00010000, len(tables), 128, 3, 0) + directory + data

open(sys.argv[1], "wb").write(font)
