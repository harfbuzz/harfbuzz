
#line 1 "hb-ot-shape-complex-myanmar-machine.rl"
/*
 * Copyright © 2011,2012  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH
#define HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH

#include "hb.hh"


#line 36 "hb-ot-shape-complex-myanmar-machine.hh"
static const unsigned char _myanmar_syllable_machine_trans_keys[] = {
	3u, 3u, 3u, 3u, 1u, 32u, 3u, 30u, 5u, 29u, 5u, 8u, 5u, 29u, 3u, 25u, 
	5u, 25u, 5u, 25u, 3u, 25u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 29u, 1u, 16u, 
	3u, 29u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 30u, 3u, 29u, 3u, 29u, 
	3u, 29u, 3u, 29u, 3u, 29u, 5u, 29u, 5u, 8u, 5u, 29u, 3u, 25u, 5u, 25u, 
	5u, 25u, 3u, 25u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 29u, 1u, 16u, 3u, 30u, 
	3u, 29u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 30u, 3u, 29u, 3u, 29u, 
	3u, 29u, 3u, 29u, 3u, 29u, 3u, 30u, 3u, 29u, 1u, 32u, 1u, 32u, 8u, 8u, 
	0
};

static const char _myanmar_syllable_machine_key_spans[] = {
	1, 1, 32, 28, 25, 4, 25, 23, 
	21, 21, 23, 27, 27, 27, 27, 16, 
	27, 27, 27, 27, 27, 28, 27, 27, 
	27, 27, 27, 25, 4, 25, 23, 21, 
	21, 23, 27, 27, 27, 27, 16, 28, 
	27, 27, 27, 27, 27, 28, 27, 27, 
	27, 27, 27, 28, 27, 32, 32, 1
};

static const short _myanmar_syllable_machine_index_offsets[] = {
	0, 2, 4, 37, 66, 92, 97, 123, 
	147, 169, 191, 215, 243, 271, 299, 327, 
	344, 372, 400, 428, 456, 484, 513, 541, 
	569, 597, 625, 653, 679, 684, 710, 734, 
	756, 778, 802, 830, 858, 886, 914, 931, 
	960, 988, 1016, 1044, 1072, 1100, 1129, 1157, 
	1185, 1213, 1241, 1269, 1298, 1326, 1359, 1392
};

static const char _myanmar_syllable_machine_indicies[] = {
	1, 0, 3, 2, 5, 5, 6, 7, 
	8, 8, 4, 9, 4, 10, 5, 4, 
	4, 4, 4, 11, 4, 12, 13, 4, 
	14, 15, 16, 17, 18, 19, 20, 21, 
	22, 23, 24, 5, 4, 26, 27, 28, 
	28, 25, 29, 25, 30, 25, 25, 25, 
	25, 25, 25, 25, 31, 25, 25, 32, 
	33, 34, 35, 36, 37, 38, 39, 40, 
	41, 25, 28, 28, 25, 29, 25, 25, 
	25, 25, 25, 25, 25, 25, 25, 1, 
	25, 25, 25, 25, 25, 25, 36, 25, 
	25, 25, 40, 25, 28, 28, 25, 29, 
	25, 28, 28, 25, 29, 25, 25, 25, 
	25, 25, 25, 25, 25, 25, 25, 25, 
	25, 25, 25, 25, 25, 36, 25, 25, 
	25, 40, 25, 42, 25, 28, 28, 25, 
	29, 25, 36, 25, 25, 25, 25, 25, 
	25, 25, 43, 25, 25, 25, 25, 25, 
	25, 36, 25, 28, 28, 25, 29, 25, 
	25, 25, 25, 25, 25, 25, 25, 25, 
	44, 25, 25, 25, 25, 25, 25, 36, 
	25, 28, 28, 25, 29, 25, 25, 25, 
	25, 25, 25, 25, 25, 25, 25, 25, 
	25, 25, 25, 25, 25, 36, 25, 44, 
	25, 28, 28, 25, 29, 25, 25, 25, 
	25, 25, 25, 25, 25, 25, 25, 25, 
	25, 25, 25, 25, 25, 36, 25, 26, 
	25, 28, 28, 25, 29, 25, 30, 25, 
	25, 25, 25, 25, 25, 25, 45, 25, 
	25, 45, 25, 25, 25, 36, 46, 25, 
	25, 40, 25, 26, 25, 28, 28, 25, 
	29, 25, 30, 25, 25, 25, 25, 25, 
	25, 25, 47, 25, 25, 25, 25, 25, 
	25, 36, 25, 25, 25, 40, 25, 26, 
	25, 28, 28, 25, 29, 25, 30, 25, 
	25, 25, 25, 25, 25, 25, 45, 25, 
	25, 25, 25, 25, 25, 36, 46, 25, 
	25, 40, 25, 26, 25, 28, 28, 25, 
	29, 25, 30, 25, 25, 25, 25, 25, 
	25, 25, 47, 25, 25, 25, 25, 25, 
	25, 36, 46, 25, 25, 40, 25, 5, 
	5, 25, 25, 25, 25, 25, 25, 25, 
	25, 25, 25, 25, 25, 25, 5, 25, 
	26, 25, 28, 28, 25, 29, 25, 30, 
	25, 25, 25, 25, 25, 25, 25, 31, 
	25, 25, 32, 33, 34, 35, 36, 37, 
	38, 39, 40, 25, 26, 25, 28, 28, 
	25, 29, 25, 30, 25, 25, 25, 25, 
	25, 25, 25, 48, 25, 25, 25, 25, 
	25, 25, 36, 37, 38, 39, 40, 25, 
	26, 25, 28, 28, 25, 29, 25, 30, 
	25, 25, 25, 25, 25, 25, 25, 47, 
	25, 25, 25, 25, 25, 25, 36, 37, 
	38, 39, 40, 25, 26, 25, 28, 28, 
	25, 29, 25, 30, 25, 25, 25, 25, 
	25, 25, 25, 47, 25, 25, 25, 25, 
	25, 25, 36, 37, 38, 25, 40, 25, 
	26, 25, 28, 28, 25, 29, 25, 30, 
	25, 25, 25, 25, 25, 25, 25, 47, 
	25, 25, 25, 25, 25, 25, 36, 25, 
	38, 25, 40, 25, 26, 25, 28, 28, 
	25, 29, 25, 30, 25, 25, 25, 25, 
	25, 25, 25, 47, 25, 25, 25, 25, 
	25, 25, 36, 37, 38, 39, 40, 48, 
	25, 26, 25, 28, 28, 25, 29, 25, 
	30, 25, 25, 25, 25, 25, 25, 25, 
	47, 25, 25, 32, 25, 34, 25, 36, 
	37, 38, 39, 40, 25, 26, 25, 28, 
	28, 25, 29, 25, 30, 25, 25, 25, 
	25, 25, 25, 25, 48, 25, 25, 32, 
	25, 25, 25, 36, 37, 38, 39, 40, 
	25, 26, 25, 28, 28, 25, 29, 25, 
	30, 25, 25, 25, 25, 25, 25, 25, 
	49, 25, 25, 32, 33, 34, 25, 36, 
	37, 38, 39, 40, 25, 26, 25, 28, 
	28, 25, 29, 25, 30, 25, 25, 25, 
	25, 25, 25, 25, 47, 25, 25, 32, 
	33, 34, 25, 36, 37, 38, 39, 40, 
	25, 26, 27, 28, 28, 25, 29, 25, 
	30, 25, 25, 25, 25, 25, 25, 25, 
	31, 25, 25, 32, 33, 34, 35, 36, 
	37, 38, 39, 40, 25, 51, 51, 50, 
	9, 50, 50, 50, 50, 50, 50, 50, 
	50, 50, 3, 50, 50, 50, 50, 50, 
	50, 18, 50, 50, 50, 22, 50, 51, 
	51, 50, 9, 50, 51, 51, 50, 9, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	18, 50, 50, 50, 22, 50, 52, 50, 
	51, 51, 50, 9, 50, 18, 50, 50, 
	50, 50, 50, 50, 50, 53, 50, 50, 
	50, 50, 50, 50, 18, 50, 51, 51, 
	50, 9, 50, 50, 50, 50, 50, 50, 
	50, 50, 50, 54, 50, 50, 50, 50, 
	50, 50, 18, 50, 51, 51, 50, 9, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	18, 50, 54, 50, 51, 51, 50, 9, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	18, 50, 6, 50, 51, 51, 50, 9, 
	50, 10, 50, 50, 50, 50, 50, 50, 
	50, 55, 50, 50, 55, 50, 50, 50, 
	18, 56, 50, 50, 22, 50, 6, 50, 
	51, 51, 50, 9, 50, 10, 50, 50, 
	50, 50, 50, 50, 50, 57, 50, 50, 
	50, 50, 50, 50, 18, 50, 50, 50, 
	22, 50, 6, 50, 51, 51, 50, 9, 
	50, 10, 50, 50, 50, 50, 50, 50, 
	50, 55, 50, 50, 50, 50, 50, 50, 
	18, 56, 50, 50, 22, 50, 6, 50, 
	51, 51, 50, 9, 50, 10, 50, 50, 
	50, 50, 50, 50, 50, 57, 50, 50, 
	50, 50, 50, 50, 18, 56, 50, 50, 
	22, 50, 58, 58, 50, 50, 50, 50, 
	50, 50, 50, 50, 50, 50, 50, 50, 
	50, 58, 50, 6, 7, 51, 51, 50, 
	9, 50, 10, 50, 50, 50, 50, 50, 
	50, 50, 12, 50, 50, 14, 15, 16, 
	17, 18, 19, 20, 21, 22, 23, 50, 
	6, 50, 51, 51, 50, 9, 50, 10, 
	50, 50, 50, 50, 50, 50, 50, 12, 
	50, 50, 14, 15, 16, 17, 18, 19, 
	20, 21, 22, 50, 6, 50, 51, 51, 
	50, 9, 50, 10, 50, 50, 50, 50, 
	50, 50, 50, 59, 50, 50, 50, 50, 
	50, 50, 18, 19, 20, 21, 22, 50, 
	6, 50, 51, 51, 50, 9, 50, 10, 
	50, 50, 50, 50, 50, 50, 50, 57, 
	50, 50, 50, 50, 50, 50, 18, 19, 
	20, 21, 22, 50, 6, 50, 51, 51, 
	50, 9, 50, 10, 50, 50, 50, 50, 
	50, 50, 50, 57, 50, 50, 50, 50, 
	50, 50, 18, 19, 20, 50, 22, 50, 
	6, 50, 51, 51, 50, 9, 50, 10, 
	50, 50, 50, 50, 50, 50, 50, 57, 
	50, 50, 50, 50, 50, 50, 18, 50, 
	20, 50, 22, 50, 6, 50, 51, 51, 
	50, 9, 50, 10, 50, 50, 50, 50, 
	50, 50, 50, 57, 50, 50, 50, 50, 
	50, 50, 18, 19, 20, 21, 22, 59, 
	50, 6, 50, 51, 51, 50, 9, 50, 
	10, 50, 50, 50, 50, 50, 50, 50, 
	57, 50, 50, 14, 50, 16, 50, 18, 
	19, 20, 21, 22, 50, 6, 50, 51, 
	51, 50, 9, 50, 10, 50, 50, 50, 
	50, 50, 50, 50, 59, 50, 50, 14, 
	50, 50, 50, 18, 19, 20, 21, 22, 
	50, 6, 50, 51, 51, 50, 9, 50, 
	10, 50, 50, 50, 50, 50, 50, 50, 
	60, 50, 50, 14, 15, 16, 50, 18, 
	19, 20, 21, 22, 50, 6, 50, 51, 
	51, 50, 9, 50, 10, 50, 50, 50, 
	50, 50, 50, 50, 57, 50, 50, 14, 
	15, 16, 50, 18, 19, 20, 21, 22, 
	50, 6, 7, 51, 51, 50, 9, 50, 
	10, 50, 50, 50, 50, 50, 50, 50, 
	12, 50, 50, 14, 15, 16, 17, 18, 
	19, 20, 21, 22, 50, 26, 27, 28, 
	28, 25, 29, 25, 30, 25, 25, 25, 
	25, 25, 25, 25, 61, 25, 25, 32, 
	33, 34, 35, 36, 37, 38, 39, 40, 
	41, 25, 26, 62, 28, 28, 25, 29, 
	25, 30, 25, 25, 25, 25, 25, 25, 
	25, 31, 25, 25, 32, 33, 34, 35, 
	36, 37, 38, 39, 40, 25, 5, 5, 
	6, 7, 51, 51, 50, 9, 50, 10, 
	5, 50, 50, 50, 50, 5, 50, 12, 
	50, 50, 14, 15, 16, 17, 18, 19, 
	20, 21, 22, 23, 50, 5, 50, 5, 
	5, 63, 63, 63, 63, 63, 63, 63, 
	63, 5, 63, 63, 63, 63, 5, 63, 
	63, 63, 63, 63, 63, 63, 63, 63, 
	63, 63, 63, 63, 63, 63, 5, 63, 
	64, 63, 0
};

static const char _myanmar_syllable_machine_trans_targs[] = {
	2, 6, 2, 29, 2, 3, 27, 38, 
	2, 28, 35, 51, 40, 54, 41, 46, 
	47, 48, 30, 43, 44, 45, 34, 50, 
	55, 2, 4, 15, 2, 5, 12, 16, 
	17, 22, 23, 24, 7, 19, 20, 21, 
	11, 26, 8, 10, 9, 13, 14, 0, 
	18, 25, 2, 2, 31, 33, 32, 36, 
	37, 1, 39, 42, 49, 52, 53, 2, 
	2
};

static const char _myanmar_syllable_machine_trans_actions[] = {
	1, 0, 2, 0, 5, 0, 0, 0, 
	6, 0, 7, 0, 0, 0, 0, 7, 
	0, 0, 0, 7, 7, 7, 0, 0, 
	0, 8, 0, 0, 9, 0, 7, 0, 
	0, 7, 0, 0, 0, 7, 7, 7, 
	0, 0, 0, 0, 0, 0, 7, 0, 
	7, 7, 10, 11, 0, 0, 0, 0, 
	7, 0, 0, 7, 7, 0, 0, 12, 
	13
};

static const char _myanmar_syllable_machine_to_state_actions[] = {
	0, 0, 3, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _myanmar_syllable_machine_from_state_actions[] = {
	0, 0, 4, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const short _myanmar_syllable_machine_eof_trans[] = {
	1, 3, 0, 26, 26, 26, 26, 26, 
	26, 26, 26, 26, 26, 26, 26, 26, 
	26, 26, 26, 26, 26, 26, 26, 26, 
	26, 26, 26, 51, 51, 51, 51, 51, 
	51, 51, 51, 51, 51, 51, 51, 51, 
	51, 51, 51, 51, 51, 51, 51, 51, 
	51, 51, 51, 26, 26, 51, 64, 64
};

static const int myanmar_syllable_machine_start = 2;
static const int myanmar_syllable_machine_first_final = 2;
static const int myanmar_syllable_machine_error = -1;

static const int myanmar_syllable_machine_en_main = 2;


#line 36 "hb-ot-shape-complex-myanmar-machine.rl"



#line 94 "hb-ot-shape-complex-myanmar-machine.rl"


#define found_syllable(syllable_type) \
  HB_STMT_START { \
    if (0) fprintf (stderr, "syllable %d..%d %s\n", ts, te, #syllable_type); \
    for (unsigned int i = ts; i < te; i++) \
      info[i].syllable() = (syllable_serial << 4) | syllable_type; \
    syllable_serial++; \
    if (unlikely (syllable_serial == 16)) syllable_serial = 1; \
  } HB_STMT_END

static void
find_syllables (hb_buffer_t *buffer)
{
  unsigned int p, pe, eof, ts, te, act HB_UNUSED;
  int cs;
  hb_glyph_info_t *info = buffer->info;
  
#line 330 "hb-ot-shape-complex-myanmar-machine.hh"
	{
	cs = myanmar_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 114 "hb-ot-shape-complex-myanmar-machine.rl"


  p = 0;
  pe = eof = buffer->len;

  unsigned int syllable_serial = 1;
  
#line 346 "hb-ot-shape-complex-myanmar-machine.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
_resume:
	switch ( _myanmar_syllable_machine_from_state_actions[cs] ) {
	case 4:
#line 1 "NONE"
	{ts = p;}
	break;
#line 360 "hb-ot-shape-complex-myanmar-machine.hh"
	}

	_keys = _myanmar_syllable_machine_trans_keys + (cs<<1);
	_inds = _myanmar_syllable_machine_indicies + _myanmar_syllable_machine_index_offsets[cs];

	_slen = _myanmar_syllable_machine_key_spans[cs];
	_trans = _inds[ _slen > 0 && _keys[0] <=( info[p].myanmar_category()) &&
		( info[p].myanmar_category()) <= _keys[1] ?
		( info[p].myanmar_category()) - _keys[0] : _slen ];

_eof_trans:
	cs = _myanmar_syllable_machine_trans_targs[_trans];

	if ( _myanmar_syllable_machine_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _myanmar_syllable_machine_trans_actions[_trans] ) {
	case 7:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 9:
#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (consonant_syllable); }}
	break;
	case 6:
#line 87 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (non_myanmar_cluster); }}
	break;
	case 13:
#line 88 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (punctuation_cluster); }}
	break;
	case 11:
#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (broken_cluster); }}
	break;
	case 5:
#line 90 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (non_myanmar_cluster); }}
	break;
	case 8:
#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (consonant_syllable); }}
	break;
	case 10:
#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (broken_cluster); }}
	break;
	case 12:
#line 90 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (non_myanmar_cluster); }}
	break;
	case 1:
#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
	{{p = ((te))-1;}{ found_syllable (consonant_syllable); }}
	break;
	case 2:
#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
	{{p = ((te))-1;}{ found_syllable (broken_cluster); }}
	break;
#line 422 "hb-ot-shape-complex-myanmar-machine.hh"
	}

_again:
	switch ( _myanmar_syllable_machine_to_state_actions[cs] ) {
	case 3:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 431 "hb-ot-shape-complex-myanmar-machine.hh"
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _myanmar_syllable_machine_eof_trans[cs] > 0 ) {
		_trans = _myanmar_syllable_machine_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	}

#line 122 "hb-ot-shape-complex-myanmar-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH */
