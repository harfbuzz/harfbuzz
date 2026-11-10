#!/usr/bin/env python3
"""Regenerate the checksum vectors embedded in src/test-checksum.cc.

The expected checksums are computed independently of the C++ implementation
using Python's int.from_bytes().
"""
from pathlib import Path
import random

OUTPUT = Path(__file__).with_name("test-checksum.cc")
BEGIN = "/* BEGIN GENERATED CHECKSUM VECTORS */"
END = "/* END GENERATED CHECKSUM VECTORS */"

random.seed(0x6588)
data = bytes(random.randrange(256) for _ in range(40))

def checksum(data):
    data += b"\0" * (-len(data) % 4)
    total = 0
    for i in range(0, len(data), 4):
        total = (total + int.from_bytes(data[i:i + 4], "big")) & 0xFFFFFFFF
    return total

cases = []
for offset in range(4):
    for length in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 16, 19, 23, 27, 31, 32]:
        if offset + length <= len(data):
            cases.append((offset, length, checksum(data[offset:offset + length])))

lines = [BEGIN]
lines.append("  static const uint8_t data[] = {")
for i in range(0, len(data), 10):
    lines.append("    " + ", ".join(f"0x{b:02X}" for b in data[i:i + 10]) + ",")
lines.append("  };")
lines.append("  static const struct")
lines.append("  {")
lines.append("    unsigned offset;")
lines.append("    unsigned length;")
lines.append("    uint32_t checksum;")
lines.append("  } cases[] = {")
for offset, length, value in cases:
    lines.append(f"    {{{offset}, {length}, 0x{value:08X}u}},")
lines.append("  };")
lines.append(END)

text = OUTPUT.read_text()
start = text.index(BEGIN)
end = text.index(END) + len(END)
OUTPUT.write_text(text[:start] + "\n".join(lines) + text[end:])
print(f"Updated {OUTPUT} with {len(cases)} checksum vectors")
