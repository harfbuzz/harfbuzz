#!/usr/bin/env python3

"""usage: ./gen-indic-table.py IndicSyllabicCategory.txt IndicPositionalCategory.txt Blocks.txt

Input files:
* https://unicode.org/Public/UCD/latest/ucd/IndicSyllabicCategory.txt
* https://unicode.org/Public/UCD/latest/ucd/IndicPositionalCategory.txt
* https://unicode.org/Public/UCD/latest/ucd/Blocks.txt
"""

import sys

if len (sys.argv) != 4:
	sys.exit (__doc__)

ALLOWED_SINGLES = [0x00A0, 0x25CC]
ALLOWED_BLOCKS = [
	'Basic Latin',
	'Latin-1 Supplement',
	'Devanagari',
	'Bengali',
	'Gurmukhi',
	'Gujarati',
	'Oriya',
	'Tamil',
	'Telugu',
	'Kannada',
	'Malayalam',
	'Sinhala',
	'Myanmar',
	'Khmer',
	'Vedic Extensions',
	'General Punctuation',
	'Superscripts and Subscripts',
	'Devanagari Extended',
	'Myanmar Extended-B',
	'Myanmar Extended-A',
]

files = [open (x, encoding='utf-8') for x in sys.argv[1:]]

headers = [[f.readline () for i in range (2)] for f in files]

data = [{} for _ in files]
for i, f in enumerate (files):
	for line in f:

		j = line.find ('#')
		if j >= 0:
			line = line[:j]

		fields = [x.strip () for x in line.split (';')]
		if len (fields) == 1:
			continue

		uu = fields[0].split ('..')
		start = int (uu[0], 16)
		if len (uu) == 1:
			end = start
		else:
			end = int (uu[1], 16)

		t = fields[1]

		for u in range (start, end + 1):
			data[i][u] = t

# Merge data into one dict:
defaults = ('Other', 'Not_Applicable', 'No_Block')
combined = {}
for i,d in enumerate (data):
	for u,v in d.items ():
		if i == 2 and not u in combined:
			continue
		if not u in combined:
			combined[u] = list (defaults)
		combined[u][i] = v
combined = {k:v for k,v in combined.items() if k in ALLOWED_SINGLES or v[2] in ALLOWED_BLOCKS}
data = combined
del combined


# Convert categories & positions types

category_map = {
  'Other'			: 'X',
  'Avagraha'			: 'Symbol',
  'Bindu'			: 'SM',
  'Brahmi_Joining_Number'	: 'PLACEHOLDER', # Don't care.
  'Cantillation_Mark'		: 'A',
  'Consonant'			: 'C',
  'Consonant_Dead'		: 'C',
  'Consonant_Final'		: 'CM',
  'Consonant_Head_Letter'	: 'C',
  'Consonant_Initial_Postfixed'	: 'C', # TODO
  'Consonant_Killer'		: 'M', # U+17CD only.
  'Consonant_Medial'		: 'CM',
  'Consonant_Placeholder'	: 'PLACEHOLDER',
  'Consonant_Preceding_Repha'	: 'Repha',
  'Consonant_Prefixed'		: 'X', # Don't care.
  'Consonant_Subjoined'		: 'CM',
  'Consonant_Succeeding_Repha'	: 'CM',
  'Consonant_With_Stacker'	: 'CS',
  'Gemination_Mark'		: 'SM', # https://github.com/harfbuzz/harfbuzz/issues/552
  'Invisible_Stacker'		: 'Coeng',
  'Joiner'			: 'ZWJ',
  'Modifying_Letter'		: 'X',
  'Non_Joiner'			: 'ZWNJ',
  'Nukta'			: 'N',
  'Number'			: 'PLACEHOLDER',
  'Number_Joiner'		: 'PLACEHOLDER', # Don't care.
  'Pure_Killer'			: 'M', # Is like a vowel matra.
  'Register_Shifter'		: 'RS',
  'Syllable_Modifier'		: 'SM',
  'Tone_Letter'			: 'X',
  'Tone_Mark'			: 'N',
  'Virama'			: 'H',
  'Visarga'			: 'SM',
  'Vowel'			: 'V',
  'Vowel_Dependent'		: 'M',
  'Vowel_Independent'		: 'V',
  'Dotted_Circle'		: 'DOTTEDCIRCLE', # Ours, not Unicode's
  'Ra'				: 'Ra', # Ours, not Unicode's
}
position_map = {
  'Not_Applicable'		: 'END',

  'Left'			: 'PRE_C',
  'Top'				: 'ABOVE_C',
  'Bottom'			: 'BELOW_C',
  'Right'			: 'POST_C',

  # These should resolve to the position of the last part of the split sequence.
  'Bottom_And_Right'		: 'POST_C',
  'Left_And_Right'		: 'POST_C',
  'Top_And_Bottom'		: 'BELOW_C',
  'Top_And_Bottom_And_Left'	: 'BELOW_C',
  'Top_And_Bottom_And_Right'	: 'POST_C',
  'Top_And_Left'		: 'ABOVE_C',
  'Top_And_Left_And_Right'	: 'POST_C',
  'Top_And_Right'		: 'POST_C',

  'Overstruck'			: 'AFTER_MAIN',
  'Visual_order_left'		: 'PRE_M',
}

category_overrides = {

  0x0930: 'Ra', # Devanagari
  0x09B0: 'Ra', # Bengali
  0x09F0: 'Ra', # Bengali
  0x0A30: 'Ra', # Gurmukhi 	No Reph
  0x0AB0: 'Ra', # Gujarati
  0x0B30: 'Ra', # Oriya
  0x0BB0: 'Ra', # Tamil 	No Reph
  0x0C30: 'Ra', # Telugu 	Reph formed only with ZWJ
  0x0CB0: 'Ra', # Kannada
  0x0D30: 'Ra', # Malayalam 	No Reph, Logical Repha
  0x0DBB: 'Ra', # Sinhala 	Reph formed only with ZWJ

  # The following act more like the Bindus.
  0x0953: 'SM',
  0x0954: 'SM',

  # The following act like consonants.
  0x0A72: 'C',
  0x0A73: 'C',
  0x1CF5: 'C',
  0x1CF6: 'C',

  # TODO: The following should only be allowed after a Visarga.
  # For now, just treat them like regular tone marks.
  0x1CE2: 'A',
  0x1CE3: 'A',
  0x1CE4: 'A',
  0x1CE5: 'A',
  0x1CE6: 'A',
  0x1CE7: 'A',
  0x1CE8: 'A',

  # TODO: The following should only be allowed after some of
  # the nasalization marks, maybe only for U+1CE9..U+1CF1.
  # For now, just treat them like tone marks.
  0x1CED: 'A',

  # The following take marks in standalone clusters, similar to Avagraha.
  0xA8F2: 'Symbol',
  0xA8F3: 'Symbol',
  0xA8F4: 'Symbol',
  0xA8F5: 'Symbol',
  0xA8F6: 'Symbol',
  0xA8F7: 'Symbol',
  0x1CE9: 'Symbol',
  0x1CEA: 'Symbol',
  0x1CEB: 'Symbol',
  0x1CEC: 'Symbol',
  0x1CEE: 'Symbol',
  0x1CEF: 'Symbol',
  0x1CF0: 'Symbol',
  0x1CF1: 'Symbol',

  0x0A51: 'M', # https://github.com/harfbuzz/harfbuzz/issues/524

  # According to ScriptExtensions.txt, these Grantha marks may also be used in Tamil,
  # so the Indic shaper needs to know their categories.
  0x11301: 'SM',
  0x11302: 'SM',
  0x11303: 'SM',
  0x1133B: 'N',
  0x1133C: 'N',

  0x0AFB: 'N', # https://github.com/harfbuzz/harfbuzz/issues/552
  0x0B55: 'N', # https://github.com/harfbuzz/harfbuzz/issues/2849

  0x0980: 'PLACEHOLDER', # https://github.com/harfbuzz/harfbuzz/issues/538
  0x09FC: 'PLACEHOLDER', # https://github.com/harfbuzz/harfbuzz/pull/1613
  0x0C80: 'PLACEHOLDER', # https://github.com/harfbuzz/harfbuzz/pull/623
  0x0D04: 'PLACEHOLDER', # https://github.com/harfbuzz/harfbuzz/pull/3511

  0x2010: 'PLACEHOLDER',
  0x2011: 'PLACEHOLDER',

  0x25CC: 'DOTTEDCIRCLE',
}

defaults = (category_map[defaults[0]], position_map[defaults[1]], defaults[2])

new_data = {}
for key, (cat, pos, block) in data.items():
  cat = category_map[cat]
  pos = position_map[pos]
  new_data[key] = (cat, pos, block)
data = new_data

for k,new_cat in category_overrides.items():
  (cat, pos, block) = data.get(k, defaults)
  data[k] = (new_cat, pos, block)

values = [{_: 1} for _ in defaults]
for vv in data.values():
  for i,v in enumerate(vv):
    values[i][v] = values[i].get (v, 0) + 1


# We only expect position for certain types
for key, (cat, pos, block) in data.items():
  if cat not in ('CM', 'SM', 'RS', 'H', 'M'):
    pos = 'END'
    data[key] = (cat, pos, block)



# Move the outliers NO-BREAK SPACE and DOTTED CIRCLE out
singles = {}
for u in ALLOWED_SINGLES:
	singles[u] = data[u]
	del data[u]

print ("/* == Start of generated table == */")
print ("/*")
print (" * The following table is generated by running:")
print (" *")
print (" *   ./gen-indic-table.py IndicSyllabicCategory.txt IndicPositionalCategory.txt Blocks.txt")
print (" *")
print (" * on files with these headers:")
print (" *")
for h in headers:
	for l in h:
		print (" * %s" % (l.strip()))
print (" */")
print ()
print ('#include "hb.hh"')
print ()
print ('#ifndef HB_NO_OT_SHAPE')
print ()
print ('#include "hb-ot-shaper-indic.hh"')
print ()

# Shorten values
short = [{
	"Repha":		'Rf',
	"Coeng":		'Co',
	"PLACEHOLDER":		'GB',
	"DOTTEDCIRCLE":		'DC',
},{
	"END":			'X',
	"ABOVE_C":		'T',
	"BELOW_C":		'B',
	"POST_C":		'R',
	"PRE_C":		'L',
	"AFTER_MAIN":		'A',
}]
all_shorts = [{},{}]

# Add some of the values, to make them more readable, and to avoid duplicates

for i in range (2):
	for v,s in short[i].items ():
		all_shorts[i][s] = v

what = ["OT", "POS"]
what_short = ["ISC", "IMC"]
print ('#pragma GCC diagnostic push')
print ('#pragma GCC diagnostic ignored "-Wunused-macros"')
cat_defs = []
for i in range (2):
	vv = sorted (values[i].keys ())
	for v in vv:
		v_no_and = v.replace ('_And_', '_')
		if v in short[i]:
			s = short[i][v]
		else:
			s = ''.join ([c for c in v_no_and if ord ('A') <= ord (c) <= ord ('Z')])
			if s in all_shorts[i]:
				raise Exception ("Duplicate short value alias", v, all_shorts[i][s])
			all_shorts[i][s] = v
			short[i][v] = s
		cat_defs.append ((what_short[i] + '_' + s, what[i] + '_' + (v.upper () if i else v), str (values[i][v]), v))

maxlen_s = max ([len (c[0]) for c in cat_defs])
maxlen_l = max ([len (c[1]) for c in cat_defs])
maxlen_n = max ([len (c[2]) for c in cat_defs])
for s in what_short:
	print ()
	for c in [c for c in cat_defs if s in c[0]]:
		print ("#define %s %s /* %s chars; %s */" %
			(c[0].ljust (maxlen_s), c[1].ljust (maxlen_l), c[2].rjust (maxlen_n), c[3]))
print ()
print ('#pragma GCC diagnostic pop')
print ()
print ("#define _(S,M) INDIC_COMBINE_CATEGORIES (ISC_##S, IMC_##M)")
print ()
print ()

total = 0
used = 0
last_block = None
def print_block (block, start, end, data):
	global total, used, last_block
	if block and block != last_block:
		print ()
		print ()
		print ("  /* %s */" % block)
	num = 0
	assert start % 8 == 0
	assert (end+1) % 8 == 0
	for u in range (start, end+1):
		if u % 8 == 0:
			print ()
			print ("  /* %04X */" % u, end="")
		if u in data:
			num += 1
		d = data.get (u, defaults)
		print ("%9s" % ("_(%s,%s)," % (short[0][d[0]], short[1][d[1]])), end="")

	total += end - start + 1
	used += num
	if block:
		last_block = block

uu = sorted (data.keys ())

last = -100000
num = 0
offset = 0
starts = []
ends = []
print ("static const uint16_t indic_table[] = {")
for u in uu:
	if u <= last:
		continue
	block = data[u][2]

	start = u//8*8
	end = start+1
	while end in uu and block == data[end][2]:
		end += 1
	end = (end-1)//8*8 + 7

	if start != last + 1:
		if start - last <= 1+16*3:
			print_block (None, last+1, start-1, data)
		else:
			if last >= 0:
				ends.append (last + 1)
				offset += ends[-1] - starts[-1]
			print ()
			print ()
			print ("#define indic_offset_0x%04xu %d" % (start, offset))
			starts.append (start)

	print_block (block, start, end, data)
	last = end
ends.append (last + 1)
offset += ends[-1] - starts[-1]
print ()
print ()
occupancy = used * 100. / total
page_bits = 12
print ("}; /* Table items: %d; occupancy: %d%% */" % (offset, occupancy))
print ()
print ("uint16_t")
print ("hb_indic_get_categories (hb_codepoint_t u)")
print ("{")
print ("  switch (u >> %d)" % page_bits)
print ("  {")
pages = set ([u>>page_bits for u in starts+ends+list (singles.keys ())])
for p in sorted(pages):
	print ("    case 0x%0Xu:" % p)
	for u,d in singles.items ():
		if p != u>>page_bits: continue
		print ("      if (unlikely (u == 0x%04Xu)) return _(%s,%s);" % (u, short[0][d[0]], short[1][d[1]]))
	for (start,end) in zip (starts, ends):
		if p not in [start>>page_bits, end>>page_bits]: continue
		offset = "indic_offset_0x%04xu" % start
		print ("      if (hb_in_range<hb_codepoint_t> (u, 0x%04Xu, 0x%04Xu)) return indic_table[u - 0x%04Xu + %s];" % (start, end-1, start, offset))
	print ("      break;")
	print ("")
print ("    default:")
print ("      break;")
print ("  }")
print ("  return _(X,X);")
print ("}")
print ()
print ("#undef _")
for i in range (2):
	print ()
	vv = sorted (values[i].keys ())
	for v in vv:
		print ("#undef %s_%s" %
			(what_short[i], short[i][v]))
print ()
print ('#endif')
print ()
print ("/* == End of generated table == */")

# Maintain at least 30% occupancy in the table */
if occupancy < 30:
	raise Exception ("Table too sparse, please investigate: ", occupancy)
