"""Generate a COLRv1 font whose ColorLine declares one more stop than it holds.

Glyph 1 is PaintGlyph -> PaintLinearGradient -> ColorLine. The ColorLine is the
last structure in COLR, has three ColorStops (offsets 0, 0.5, 1) but says
numStops = 4. FreeType reports four stops but refuses to iterate them; the
hb-ft color-line callback must then report zero stops written.
"""

import struct
from pathlib import Path

NUM_GLYPHS = 3
STOP_OFFSETS = (0x0000, 0x2000, 0x4000)  # F2DOT14: 0, 0.5, 1
NUM_STOPS_DECLARED = len(STOP_OFFSETS) + 1

# Minimal tables for an empty-outline TrueType font.
head = struct.pack(">IIIIHHQQhhhhHHhhh", 0x00010000, 0, 0, 0x5F0F3CF5, 0, 1000, 0, 0,
                   0, 0, 0, 0, 0, 8, 2, 0, 0)
hhea = struct.pack(">IhhhHhhhhhhhhhhhH", 0x00010000, 800, -200, 0, 1000, 0, 0, 1000,
                   1, 0, 0, 0, 0, 0, 0, 0, NUM_GLYPHS)
maxp = struct.pack(">IH13H", 0x00010000, NUM_GLYPHS, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0)
hmtx = struct.pack(">Hh", 500, 0) * NUM_GLYPHS
loca = struct.pack(">4H", 0, 0, 0, 0)
glyf = b""

# CPAL: one palette with one opaque red entry (B, G, R, A).
cpal = struct.pack(">HHHHIH", 0, 1, 1, 1, 14, 0) + bytes([0x00, 0x00, 0xFF, 0xFF])

# COLR v1: glyph 1 -> PaintGlyph(glyph 2) -> PaintLinearGradient -> ColorLine.
colr_header = struct.pack(">HHIIHIIIII",
                          1,           # version
                          0, 0, 0, 0,  # no v0 records
                          34,          # BaseGlyphList right after this header
                          0, 0, 0, 0)  # no LayerList, ClipList, VarIndexMap, VarStore
base_glyph_list = struct.pack(">IHI", 1, 1, 10)  # glyph 1, paint at +10
paint_glyph = struct.pack(">B3sH", 10, (6).to_bytes(3, "big"), 2)
paint_linear_gradient = struct.pack(">B3s6h", 4, (16).to_bytes(3, "big"), 0, 0, 500, 0, 0, 500)
color_line = struct.pack(">BH", 0, NUM_STOPS_DECLARED)
for stop_offset in STOP_OFFSETS:
    color_line += struct.pack(">hHh", stop_offset, 0, 0x4000)  # palette entry 0, alpha 1
colr = colr_header + base_glyph_list + paint_glyph + paint_linear_gradient + color_line

tables = sorted({b"head": head, b"hhea": hhea, b"maxp": maxp, b"hmtx": hmtx,
                 b"loca": loca, b"glyf": glyf, b"CPAL": cpal, b"COLR": colr}.items())
directory = struct.pack(">IHHHH", 0x00010000, len(tables), 128, 3, len(tables) * 16 - 128)
body = b""
for tag, data in tables:
    directory += struct.pack(">4sIII", tag, 0, 12 + 16 * len(tables) + len(body), len(data))
    body += data + b"\0" * (-len(data) % 4)

Path(__file__).with_name("colr-short-colorline.ttf").write_bytes(directory + body)
