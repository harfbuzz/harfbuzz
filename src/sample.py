#!/usr/bin/python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys
from gi.repository import HarfBuzz as hb

def nothing(data):
	print(data)

fontdata = open (sys.argv[1], 'rb').read ()

blob = hb.blob_create (fontdata, hb.memory_mode_t.DUPLICATE, 1234, None)
buf = hb.buffer_create ()
hb.buffer_add_utf8 (buf, "Hello بهداد", 0, -1)
hb.buffer_guess_segment_properties (buf)

face = hb.face_create (blob, 0)
font = hb.font_create (face)
upem = hb.face_get_upem (face)
hb.font_set_scale (font, upem, upem)
#hb.ft_font_set_funcs (font)
hb.ot_font_set_funcs (font)

hb.shape (font, buf, [])

infos = hb.buffer_get_glyph_infos (buf)
positions = hb.buffer_get_glyph_positions (buf)

for info,pos in zip(infos, positions):
	gid = info.codepoint
	cluster = info.cluster
	advance = pos.x_advance

	print(gid, cluster, advance)
