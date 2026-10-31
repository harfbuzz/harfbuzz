"""Generate a COLRv0 font whose base glyphs all share the same layer range."""

from pathlib import Path
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib.tables.DefaultTable import DefaultTable
import struct

# Every base glyph claims all the layers, so retaining 256 of them expands
# to 65536 layer records, one more than the COLR header can count.
count = 256
bases = [f"base{i}" for i in range(count)]
layers = [f"layer{i}" for i in range(count)]
names = [".notdef"] + bases + layers
fb = FontBuilder(1000, isTTF=True)
fb.setupGlyphOrder(names)
fb.setupCharacterMap({0xE000 + i: name for i, name in enumerate(bases)})
fb.setupGlyf({name: TTGlyphPen(None).glyph() for name in names})
fb.setupHorizontalMetrics({name: (1000, 0) for name in names})
fb.setupHorizontalHeader(ascent=800, descent=-200)
fb.setupNameTable({"familyName": "COLR Shared Layers", "styleName": "Regular"})
fb.setupOS2(sTypoAscender=800, sTypoDescender=-200, usWinAscent=800, usWinDescent=200)
fb.setupPost()
fb.setupCPAL([[(0, 0, 0, 1), (1, 1, 1, 1)]])

# fontTools writes each base glyph its own layers; build the table by hand.
header = struct.pack(">HHIIH", 0, count, 14, 14 + count * 6, count)
base_records = b"".join(struct.pack(">HHH", 1 + i, 0, count) for i in range(count))
layer_records = b"".join(struct.pack(">HH", 1 + count + i, i % 2) for i in range(count))
colr = DefaultTable("COLR")
colr.data = header + base_records + layer_records
fb.font["COLR"] = colr

fb.save(Path(__file__).with_name("colr-shared-layers.ttf"))
