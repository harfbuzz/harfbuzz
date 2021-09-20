
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

enum myanmar_syllable_type_t {
  myanmar_consonant_syllable,
  myanmar_punctuation_cluster,
  myanmar_broken_cluster,
  myanmar_non_myanmar_cluster,
};


#line 43 "hb-ot-shape-complex-myanmar-machine.hh"
#define myanmar_syllable_machine_ex_A 10u
#define myanmar_syllable_machine_ex_As 18u
#define myanmar_syllable_machine_ex_C 1u
#define myanmar_syllable_machine_ex_CS 19u
#define myanmar_syllable_machine_ex_D 32u
#define myanmar_syllable_machine_ex_D0 20u
#define myanmar_syllable_machine_ex_DB 3u
#define myanmar_syllable_machine_ex_GB 11u
#define myanmar_syllable_machine_ex_H 4u
#define myanmar_syllable_machine_ex_IV 2u
#define myanmar_syllable_machine_ex_MH 21u
#define myanmar_syllable_machine_ex_ML 33u
#define myanmar_syllable_machine_ex_MR 22u
#define myanmar_syllable_machine_ex_MW 23u
#define myanmar_syllable_machine_ex_MY 24u
#define myanmar_syllable_machine_ex_P 31u
#define myanmar_syllable_machine_ex_PT 25u
#define myanmar_syllable_machine_ex_Ra 16u
#define myanmar_syllable_machine_ex_V 8u
#define myanmar_syllable_machine_ex_VAbv 26u
#define myanmar_syllable_machine_ex_VBlw 27u
#define myanmar_syllable_machine_ex_VPre 28u
#define myanmar_syllable_machine_ex_VPst 29u
#define myanmar_syllable_machine_ex_VS 30u
#define myanmar_syllable_machine_ex_ZWJ 6u
#define myanmar_syllable_machine_ex_ZWNJ 5u


#line 72 "hb-ot-shape-complex-myanmar-machine.hh"
static const unsigned char _myanmar_syllable_machine_trans_keys[] = {
	1u, 32u, 3u, 30u, 5u, 29u, 5u, 8u, 5u, 29u, 3u, 25u, 5u, 25u, 5u, 25u, 
	3u, 33u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 33u, 1u, 16u, 3u, 29u, 3u, 33u, 
	3u, 29u, 3u, 29u, 3u, 29u, 3u, 30u, 3u, 29u, 3u, 29u, 3u, 33u, 3u, 29u, 
	3u, 29u, 3u, 29u, 5u, 29u, 5u, 8u, 5u, 29u, 3u, 25u, 5u, 25u, 5u, 25u, 
	3u, 33u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 33u, 1u, 16u, 3u, 30u, 3u, 29u, 
	3u, 33u, 3u, 29u, 3u, 29u, 3u, 29u, 3u, 30u, 3u, 29u, 3u, 29u, 3u, 33u, 
	3u, 29u, 3u, 29u, 3u, 29u, 3u, 30u, 3u, 29u, 1u, 32u, 1u, 32u, 8u, 8u, 
	0
};

static const char _myanmar_syllable_machine_key_spans[] = {
	32, 28, 25, 4, 25, 23, 21, 21, 
	31, 27, 27, 27, 31, 16, 27, 31, 
	27, 27, 27, 28, 27, 27, 31, 27, 
	27, 27, 25, 4, 25, 23, 21, 21, 
	31, 27, 27, 27, 31, 16, 28, 27, 
	31, 27, 27, 27, 28, 27, 27, 31, 
	27, 27, 27, 28, 27, 32, 32, 1
};

static const short _myanmar_syllable_machine_index_offsets[] = {
	0, 33, 62, 88, 93, 119, 143, 165, 
	187, 219, 247, 275, 303, 335, 352, 380, 
	412, 440, 468, 496, 525, 553, 581, 613, 
	641, 669, 697, 723, 728, 754, 778, 800, 
	822, 854, 882, 910, 938, 970, 987, 1016, 
	1044, 1076, 1104, 1132, 1160, 1189, 1217, 1245, 
	1277, 1305, 1333, 1361, 1390, 1418, 1451, 1484
};

static const char _myanmar_syllable_machine_indicies[] = {
	1, 1, 2, 3, 4, 4, 0, 5, 
	0, 6, 1, 0, 0, 0, 0, 7, 
	0, 8, 9, 0, 10, 11, 12, 13, 
	14, 15, 16, 17, 18, 19, 20, 1, 
	0, 22, 23, 24, 24, 21, 25, 21, 
	26, 21, 21, 21, 21, 21, 21, 21, 
	27, 21, 21, 28, 29, 30, 31, 32, 
	33, 34, 35, 36, 37, 21, 24, 24, 
	21, 25, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 38, 21, 21, 21, 21, 
	21, 21, 32, 21, 21, 21, 36, 21, 
	24, 24, 21, 25, 21, 24, 24, 21, 
	25, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 32, 21, 21, 21, 36, 21, 39, 
	21, 24, 24, 21, 25, 21, 32, 21, 
	21, 21, 21, 21, 21, 21, 40, 21, 
	21, 21, 21, 21, 21, 32, 21, 24, 
	24, 21, 25, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 40, 21, 21, 21, 
	21, 21, 21, 32, 21, 24, 24, 21, 
	25, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 32, 21, 22, 21, 24, 24, 21, 
	25, 21, 26, 21, 21, 21, 21, 21, 
	21, 21, 41, 21, 21, 42, 21, 21, 
	21, 32, 43, 21, 21, 36, 21, 21, 
	21, 41, 21, 22, 21, 24, 24, 21, 
	25, 21, 26, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 32, 21, 21, 21, 36, 21, 22, 
	21, 24, 24, 21, 25, 21, 26, 21, 
	21, 21, 21, 21, 21, 21, 41, 21, 
	21, 21, 21, 21, 21, 32, 43, 21, 
	21, 36, 21, 22, 21, 24, 24, 21, 
	25, 21, 26, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 32, 43, 21, 21, 36, 21, 22, 
	21, 24, 24, 21, 25, 21, 26, 21, 
	21, 21, 21, 21, 21, 21, 41, 21, 
	21, 21, 21, 21, 21, 32, 43, 21, 
	21, 36, 21, 21, 21, 41, 21, 1, 
	1, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 1, 21, 
	22, 21, 24, 24, 21, 25, 21, 26, 
	21, 21, 21, 21, 21, 21, 21, 27, 
	21, 21, 28, 29, 30, 31, 32, 33, 
	34, 35, 36, 21, 22, 21, 24, 24, 
	21, 25, 21, 26, 21, 21, 21, 21, 
	21, 21, 21, 44, 21, 21, 21, 21, 
	21, 21, 32, 33, 34, 35, 36, 21, 
	21, 21, 45, 21, 22, 21, 24, 24, 
	21, 25, 21, 26, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 32, 33, 34, 35, 36, 21, 
	22, 21, 24, 24, 21, 25, 21, 26, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 32, 33, 
	34, 21, 36, 21, 22, 21, 24, 24, 
	21, 25, 21, 26, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 32, 21, 34, 21, 36, 21, 
	22, 21, 24, 24, 21, 25, 21, 26, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 21, 21, 21, 32, 33, 
	34, 35, 36, 44, 21, 22, 21, 24, 
	24, 21, 25, 21, 26, 21, 21, 21, 
	21, 21, 21, 21, 44, 21, 21, 21, 
	21, 21, 21, 32, 33, 34, 35, 36, 
	21, 22, 21, 24, 24, 21, 25, 21, 
	26, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 28, 21, 30, 21, 32, 
	33, 34, 35, 36, 21, 22, 21, 24, 
	24, 21, 25, 21, 26, 21, 21, 21, 
	21, 21, 21, 21, 44, 21, 21, 28, 
	21, 21, 21, 32, 33, 34, 35, 36, 
	21, 21, 21, 45, 21, 22, 21, 24, 
	24, 21, 25, 21, 26, 21, 21, 21, 
	21, 21, 21, 21, 46, 21, 21, 28, 
	29, 30, 21, 32, 33, 34, 35, 36, 
	21, 22, 21, 24, 24, 21, 25, 21, 
	26, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 21, 28, 29, 30, 21, 32, 
	33, 34, 35, 36, 21, 22, 23, 24, 
	24, 21, 25, 21, 26, 21, 21, 21, 
	21, 21, 21, 21, 27, 21, 21, 28, 
	29, 30, 31, 32, 33, 34, 35, 36, 
	21, 48, 48, 47, 5, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 49, 47, 
	47, 47, 47, 47, 47, 14, 47, 47, 
	47, 18, 47, 48, 48, 47, 5, 47, 
	48, 48, 47, 5, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 47, 14, 47, 47, 47, 
	18, 47, 50, 47, 48, 48, 47, 5, 
	47, 14, 47, 47, 47, 47, 47, 47, 
	47, 51, 47, 47, 47, 47, 47, 47, 
	14, 47, 48, 48, 47, 5, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 51, 
	47, 47, 47, 47, 47, 47, 14, 47, 
	48, 48, 47, 5, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 47, 14, 47, 2, 47, 
	48, 48, 47, 5, 47, 6, 47, 47, 
	47, 47, 47, 47, 47, 52, 47, 47, 
	53, 47, 47, 47, 14, 54, 47, 47, 
	18, 47, 47, 47, 52, 47, 2, 47, 
	48, 48, 47, 5, 47, 6, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 47, 14, 47, 47, 47, 
	18, 47, 2, 47, 48, 48, 47, 5, 
	47, 6, 47, 47, 47, 47, 47, 47, 
	47, 52, 47, 47, 47, 47, 47, 47, 
	14, 54, 47, 47, 18, 47, 2, 47, 
	48, 48, 47, 5, 47, 6, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 47, 14, 54, 47, 47, 
	18, 47, 2, 47, 48, 48, 47, 5, 
	47, 6, 47, 47, 47, 47, 47, 47, 
	47, 52, 47, 47, 47, 47, 47, 47, 
	14, 54, 47, 47, 18, 47, 47, 47, 
	52, 47, 55, 55, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 55, 47, 2, 3, 48, 48, 47, 
	5, 47, 6, 47, 47, 47, 47, 47, 
	47, 47, 8, 47, 47, 10, 11, 12, 
	13, 14, 15, 16, 17, 18, 19, 47, 
	2, 47, 48, 48, 47, 5, 47, 6, 
	47, 47, 47, 47, 47, 47, 47, 8, 
	47, 47, 10, 11, 12, 13, 14, 15, 
	16, 17, 18, 47, 2, 47, 48, 48, 
	47, 5, 47, 6, 47, 47, 47, 47, 
	47, 47, 47, 56, 47, 47, 47, 47, 
	47, 47, 14, 15, 16, 17, 18, 47, 
	47, 47, 57, 47, 2, 47, 48, 48, 
	47, 5, 47, 6, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 14, 15, 16, 17, 18, 47, 
	2, 47, 48, 48, 47, 5, 47, 6, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 14, 15, 
	16, 47, 18, 47, 2, 47, 48, 48, 
	47, 5, 47, 6, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 14, 47, 16, 47, 18, 47, 
	2, 47, 48, 48, 47, 5, 47, 6, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 47, 14, 15, 
	16, 17, 18, 56, 47, 2, 47, 48, 
	48, 47, 5, 47, 6, 47, 47, 47, 
	47, 47, 47, 47, 56, 47, 47, 47, 
	47, 47, 47, 14, 15, 16, 17, 18, 
	47, 2, 47, 48, 48, 47, 5, 47, 
	6, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 10, 47, 12, 47, 14, 
	15, 16, 17, 18, 47, 2, 47, 48, 
	48, 47, 5, 47, 6, 47, 47, 47, 
	47, 47, 47, 47, 56, 47, 47, 10, 
	47, 47, 47, 14, 15, 16, 17, 18, 
	47, 47, 47, 57, 47, 2, 47, 48, 
	48, 47, 5, 47, 6, 47, 47, 47, 
	47, 47, 47, 47, 58, 47, 47, 10, 
	11, 12, 47, 14, 15, 16, 17, 18, 
	47, 2, 47, 48, 48, 47, 5, 47, 
	6, 47, 47, 47, 47, 47, 47, 47, 
	47, 47, 47, 10, 11, 12, 47, 14, 
	15, 16, 17, 18, 47, 2, 3, 48, 
	48, 47, 5, 47, 6, 47, 47, 47, 
	47, 47, 47, 47, 8, 47, 47, 10, 
	11, 12, 13, 14, 15, 16, 17, 18, 
	47, 22, 23, 24, 24, 21, 25, 21, 
	26, 21, 21, 21, 21, 21, 21, 21, 
	59, 21, 21, 28, 29, 30, 31, 32, 
	33, 34, 35, 36, 37, 21, 22, 60, 
	24, 24, 21, 25, 21, 26, 21, 21, 
	21, 21, 21, 21, 21, 27, 21, 21, 
	28, 29, 30, 31, 32, 33, 34, 35, 
	36, 21, 1, 1, 2, 3, 48, 48, 
	47, 5, 47, 6, 1, 47, 47, 47, 
	47, 1, 47, 8, 47, 47, 10, 11, 
	12, 13, 14, 15, 16, 17, 18, 19, 
	47, 1, 47, 1, 1, 61, 61, 61, 
	61, 61, 61, 61, 61, 1, 61, 61, 
	61, 61, 1, 61, 61, 61, 61, 61, 
	61, 61, 61, 61, 61, 61, 61, 61, 
	61, 61, 1, 61, 62, 61, 0
};

static const char _myanmar_syllable_machine_trans_targs[] = {
	0, 1, 26, 37, 0, 27, 33, 51, 
	39, 54, 40, 46, 47, 48, 29, 42, 
	43, 44, 32, 50, 55, 0, 2, 13, 
	0, 3, 9, 14, 15, 21, 22, 23, 
	5, 17, 18, 19, 8, 25, 4, 6, 
	7, 10, 12, 11, 16, 20, 24, 0, 
	0, 28, 30, 31, 34, 36, 35, 38, 
	41, 45, 49, 52, 53, 0, 0
};

static const char _myanmar_syllable_machine_trans_actions[] = {
	3, 0, 0, 0, 4, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 5, 0, 0, 
	6, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 7, 
	8, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 9, 10
};

static const char _myanmar_syllable_machine_to_state_actions[] = {
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _myanmar_syllable_machine_from_state_actions[] = {
	2, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const short _myanmar_syllable_machine_eof_trans[] = {
	0, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 22, 22, 48, 62, 62
};

static const int myanmar_syllable_machine_start = 0;
static const int myanmar_syllable_machine_first_final = 0;
static const int myanmar_syllable_machine_error = -1;

static const int myanmar_syllable_machine_en_main = 0;


#line 44 "hb-ot-shape-complex-myanmar-machine.rl"



#line 102 "hb-ot-shape-complex-myanmar-machine.rl"


#define found_syllable(syllable_type) \
  HB_STMT_START { \
    if (0) fprintf (stderr, "syllable %d..%d %s\n", ts, te, #syllable_type); \
    for (unsigned int i = ts; i < te; i++) \
      info[i].syllable() = (syllable_serial << 4) | syllable_type; \
    syllable_serial++; \
    if (unlikely (syllable_serial == 16)) syllable_serial = 1; \
  } HB_STMT_END

static void
find_syllables_myanmar (hb_buffer_t *buffer)
{
  unsigned int p, pe, eof, ts, te, act HB_UNUSED;
  int cs;
  hb_glyph_info_t *info = buffer->info;
  
#line 375 "hb-ot-shape-complex-myanmar-machine.hh"
	{
	cs = myanmar_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 122 "hb-ot-shape-complex-myanmar-machine.rl"


  p = 0;
  pe = eof = buffer->len;

  unsigned int syllable_serial = 1;
  
#line 391 "hb-ot-shape-complex-myanmar-machine.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
_resume:
	switch ( _myanmar_syllable_machine_from_state_actions[cs] ) {
	case 2:
#line 1 "NONE"
	{ts = p;}
	break;
#line 405 "hb-ot-shape-complex-myanmar-machine.hh"
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
	case 6:
#line 94 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (myanmar_consonant_syllable); }}
	break;
	case 4:
#line 95 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (myanmar_non_myanmar_cluster); }}
	break;
	case 10:
#line 96 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (myanmar_punctuation_cluster); }}
	break;
	case 8:
#line 97 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (myanmar_broken_cluster); }}
	break;
	case 3:
#line 98 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p+1;{ found_syllable (myanmar_non_myanmar_cluster); }}
	break;
	case 5:
#line 94 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (myanmar_consonant_syllable); }}
	break;
	case 7:
#line 97 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (myanmar_broken_cluster); }}
	break;
	case 9:
#line 98 "hb-ot-shape-complex-myanmar-machine.rl"
	{te = p;p--;{ found_syllable (myanmar_non_myanmar_cluster); }}
	break;
#line 455 "hb-ot-shape-complex-myanmar-machine.hh"
	}

_again:
	switch ( _myanmar_syllable_machine_to_state_actions[cs] ) {
	case 1:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 464 "hb-ot-shape-complex-myanmar-machine.hh"
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

#line 130 "hb-ot-shape-complex-myanmar-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH */
