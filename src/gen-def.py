#!/usr/bin/env python3

"usage: gen-def.py harfbuzz.def hb.h [hb-blob.h hb-buffer.h ...]"

import os, re, sys

if len (sys.argv) < 3:
	sys.exit(__doc__)

output_file = sys.argv[1]
header_paths = sys.argv[2:]

headers_content = []
for h in header_paths:
	if h.endswith (".h"):
		with open (h, encoding='utf-8') as f: headers_content.append (f.read ())

symbols = sorted (re.findall (r"^hb_\w+(?= \()", "\n".join (headers_content), re.M))
if '--experimental-api' not in sys.argv:
	# Move these to harfbuzz-sections.txt when got stable
	experimental_symbols = \
"""hb_shape_justify
hb_subset_input_override_name_table
hb_subset_cff_get_charstring_data
hb_subset_cff_get_charstrings_index
hb_subset_cff2_get_charstring_data
hb_subset_cff2_get_charstrings_index
""".splitlines ()
	symbols = [x for x in symbols if x not in experimental_symbols]

if '--depend-api' not in sys.argv:
	# Depend API symbols - only included when depend_api=true
	depend_symbols = \
"""hb_depend_from_face_or_fail
hb_depend_get_glyph_entry
hb_depend_get_set_from_index
hb_depend_print
hb_depend_destroy
""".splitlines ()
	symbols = [x for x in symbols if x not in depend_symbols]
symbols = "\n".join (symbols)

result = symbols if os.getenv ('PLAIN_LIST', '') else """EXPORTS
%s
LIBRARY lib%s-0.dll""" % (symbols, output_file.replace ('src/', '').replace ('.def', ''))

with open (output_file, "w") as f: f.write (result)
