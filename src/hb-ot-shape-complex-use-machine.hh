
#line 1 "hb-ot-shape-complex-use-machine.rl"
/*
 * Copyright © 2015  Mozilla Foundation.
 * Copyright © 2015  Google, Inc.
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
 * Mozilla Author(s): Jonathan Kew
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_SHAPE_COMPLEX_USE_MACHINE_HH
#define HB_OT_SHAPE_COMPLEX_USE_MACHINE_HH

#include "hb.hh"
#include "hb-ot-shape-complex-machine-index.hh"


#line 39 "hb-ot-shape-complex-use-machine.hh"
static const unsigned char _use_syllable_machine_trans_keys[] = {
	12u, 48u, 1u, 15u, 1u, 1u, 12u, 48u, 1u, 1u, 0u, 48u, 11u, 48u, 11u, 48u, 
	1u, 15u, 1u, 1u, 22u, 48u, 23u, 48u, 24u, 47u, 25u, 47u, 26u, 47u, 45u, 46u, 
	46u, 46u, 24u, 48u, 24u, 48u, 24u, 48u, 1u, 1u, 24u, 48u, 23u, 48u, 23u, 48u, 
	23u, 48u, 22u, 48u, 22u, 48u, 22u, 48u, 11u, 48u, 1u, 48u, 13u, 13u, 4u, 4u, 
	11u, 48u, 41u, 42u, 42u, 42u, 11u, 48u, 22u, 48u, 23u, 48u, 24u, 47u, 25u, 47u, 
	26u, 47u, 45u, 46u, 46u, 46u, 24u, 48u, 24u, 48u, 24u, 48u, 24u, 48u, 23u, 48u, 
	23u, 48u, 23u, 48u, 22u, 48u, 22u, 48u, 22u, 48u, 11u, 48u, 1u, 48u, 1u, 15u, 
	4u, 4u, 13u, 13u, 12u, 48u, 1u, 48u, 11u, 48u, 41u, 42u, 42u, 42u, 1u, 5u, 
	0
};

static const char _use_syllable_machine_key_spans[] = {
	37, 15, 1, 37, 1, 49, 38, 38, 
	15, 1, 27, 26, 24, 23, 22, 2, 
	1, 25, 25, 25, 1, 25, 26, 26, 
	26, 27, 27, 27, 38, 48, 1, 1, 
	38, 2, 1, 38, 27, 26, 24, 23, 
	22, 2, 1, 25, 25, 25, 25, 26, 
	26, 26, 27, 27, 27, 38, 48, 15, 
	1, 1, 37, 48, 38, 2, 1, 5
};

static const short _use_syllable_machine_index_offsets[] = {
	0, 38, 54, 56, 94, 96, 146, 185, 
	224, 240, 242, 270, 297, 322, 346, 369, 
	372, 374, 400, 426, 452, 454, 480, 507, 
	534, 561, 589, 617, 645, 684, 733, 735, 
	737, 776, 779, 781, 820, 848, 875, 900, 
	924, 947, 950, 952, 978, 1004, 1030, 1056, 
	1083, 1110, 1137, 1165, 1193, 1221, 1260, 1309, 
	1325, 1327, 1329, 1367, 1416, 1455, 1458, 1460
};

static const char _use_syllable_machine_indicies[] = {
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 0, 0, 1, 0, 3, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 4, 2, 3, 2, 
	6, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	6, 5, 5, 5, 6, 5, 7, 5, 
	8, 9, 10, 8, 11, 12, 10, 10, 
	10, 10, 10, 3, 13, 14, 10, 15, 
	8, 8, 16, 17, 10, 10, 18, 19, 
	20, 21, 22, 23, 24, 18, 25, 26, 
	27, 28, 29, 30, 10, 31, 32, 33, 
	10, 34, 35, 36, 37, 38, 39, 40, 
	13, 10, 42, 1, 41, 41, 43, 41, 
	41, 41, 41, 41, 41, 44, 45, 46, 
	47, 48, 49, 50, 44, 51, 9, 52, 
	53, 54, 55, 41, 56, 57, 58, 41, 
	41, 41, 41, 59, 60, 61, 62, 1, 
	41, 42, 1, 41, 41, 43, 41, 41, 
	41, 41, 41, 41, 44, 45, 46, 47, 
	48, 49, 50, 44, 51, 52, 52, 53, 
	54, 55, 41, 56, 57, 58, 41, 41, 
	41, 41, 59, 60, 61, 62, 1, 41, 
	42, 63, 63, 63, 63, 63, 63, 63, 
	63, 63, 63, 63, 63, 63, 64, 63, 
	42, 63, 44, 45, 46, 47, 48, 41, 
	41, 41, 41, 41, 41, 53, 54, 55, 
	41, 56, 57, 58, 41, 41, 41, 41, 
	45, 60, 61, 62, 65, 41, 45, 46, 
	47, 48, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 56, 57, 58, 41, 
	41, 41, 41, 41, 60, 61, 62, 65, 
	41, 46, 47, 48, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 60, 61, 
	62, 41, 47, 48, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 60, 61, 
	62, 41, 48, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 60, 61, 62, 
	41, 60, 61, 41, 61, 41, 46, 47, 
	48, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 56, 57, 58, 41, 41, 
	41, 41, 41, 60, 61, 62, 65, 41, 
	46, 47, 48, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 57, 58, 
	41, 41, 41, 41, 41, 60, 61, 62, 
	65, 41, 46, 47, 48, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 41, 41, 
	41, 58, 41, 41, 41, 41, 41, 60, 
	61, 62, 65, 41, 67, 66, 46, 47, 
	48, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 60, 61, 62, 65, 41, 
	45, 46, 47, 48, 41, 41, 41, 41, 
	41, 41, 53, 54, 55, 41, 56, 57, 
	58, 41, 41, 41, 41, 45, 60, 61, 
	62, 65, 41, 45, 46, 47, 48, 41, 
	41, 41, 41, 41, 41, 41, 54, 55, 
	41, 56, 57, 58, 41, 41, 41, 41, 
	45, 60, 61, 62, 65, 41, 45, 46, 
	47, 48, 41, 41, 41, 41, 41, 41, 
	41, 41, 55, 41, 56, 57, 58, 41, 
	41, 41, 41, 45, 60, 61, 62, 65, 
	41, 44, 45, 46, 47, 48, 41, 50, 
	44, 41, 41, 41, 53, 54, 55, 41, 
	56, 57, 58, 41, 41, 41, 41, 45, 
	60, 61, 62, 65, 41, 44, 45, 46, 
	47, 48, 41, 41, 44, 41, 41, 41, 
	53, 54, 55, 41, 56, 57, 58, 41, 
	41, 41, 41, 45, 60, 61, 62, 65, 
	41, 44, 45, 46, 47, 48, 49, 50, 
	44, 41, 41, 41, 53, 54, 55, 41, 
	56, 57, 58, 41, 41, 41, 41, 45, 
	60, 61, 62, 65, 41, 42, 1, 41, 
	41, 43, 41, 41, 41, 41, 41, 41, 
	44, 45, 46, 47, 48, 49, 50, 44, 
	51, 41, 52, 53, 54, 55, 41, 56, 
	57, 58, 41, 41, 41, 41, 59, 60, 
	61, 62, 1, 41, 42, 63, 63, 63, 
	63, 63, 63, 63, 63, 63, 63, 63, 
	63, 63, 64, 63, 63, 63, 63, 63, 
	63, 63, 45, 46, 47, 48, 63, 63, 
	63, 63, 63, 63, 63, 63, 63, 63, 
	56, 57, 58, 63, 63, 63, 63, 63, 
	60, 61, 62, 65, 63, 69, 68, 11, 
	70, 42, 1, 41, 41, 43, 41, 41, 
	41, 41, 41, 41, 44, 45, 46, 47, 
	48, 49, 50, 44, 51, 9, 52, 53, 
	54, 55, 41, 56, 57, 58, 41, 17, 
	71, 41, 59, 60, 61, 62, 1, 41, 
	17, 71, 72, 71, 72, 3, 6, 73, 
	73, 74, 73, 73, 73, 73, 73, 73, 
	18, 19, 20, 21, 22, 23, 24, 18, 
	25, 27, 27, 28, 29, 30, 73, 31, 
	32, 33, 73, 73, 73, 73, 37, 38, 
	39, 40, 6, 73, 18, 19, 20, 21, 
	22, 73, 73, 73, 73, 73, 73, 28, 
	29, 30, 73, 31, 32, 33, 73, 73, 
	73, 73, 19, 38, 39, 40, 75, 73, 
	19, 20, 21, 22, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 31, 32, 
	33, 73, 73, 73, 73, 73, 38, 39, 
	40, 75, 73, 20, 21, 22, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	38, 39, 40, 73, 21, 22, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	38, 39, 40, 73, 22, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 38, 
	39, 40, 73, 38, 39, 73, 39, 73, 
	20, 21, 22, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 31, 32, 33, 
	73, 73, 73, 73, 73, 38, 39, 40, 
	75, 73, 20, 21, 22, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	32, 33, 73, 73, 73, 73, 73, 38, 
	39, 40, 75, 73, 20, 21, 22, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 33, 73, 73, 73, 73, 
	73, 38, 39, 40, 75, 73, 20, 21, 
	22, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 38, 39, 40, 75, 73, 
	19, 20, 21, 22, 73, 73, 73, 73, 
	73, 73, 28, 29, 30, 73, 31, 32, 
	33, 73, 73, 73, 73, 19, 38, 39, 
	40, 75, 73, 19, 20, 21, 22, 73, 
	73, 73, 73, 73, 73, 73, 29, 30, 
	73, 31, 32, 33, 73, 73, 73, 73, 
	19, 38, 39, 40, 75, 73, 19, 20, 
	21, 22, 73, 73, 73, 73, 73, 73, 
	73, 73, 30, 73, 31, 32, 33, 73, 
	73, 73, 73, 19, 38, 39, 40, 75, 
	73, 18, 19, 20, 21, 22, 73, 24, 
	18, 73, 73, 73, 28, 29, 30, 73, 
	31, 32, 33, 73, 73, 73, 73, 19, 
	38, 39, 40, 75, 73, 18, 19, 20, 
	21, 22, 73, 73, 18, 73, 73, 73, 
	28, 29, 30, 73, 31, 32, 33, 73, 
	73, 73, 73, 19, 38, 39, 40, 75, 
	73, 18, 19, 20, 21, 22, 23, 24, 
	18, 73, 73, 73, 28, 29, 30, 73, 
	31, 32, 33, 73, 73, 73, 73, 19, 
	38, 39, 40, 75, 73, 3, 6, 73, 
	73, 74, 73, 73, 73, 73, 73, 73, 
	18, 19, 20, 21, 22, 23, 24, 18, 
	25, 73, 27, 28, 29, 30, 73, 31, 
	32, 33, 73, 73, 73, 73, 37, 38, 
	39, 40, 6, 73, 3, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 4, 73, 73, 73, 73, 73, 
	73, 73, 19, 20, 21, 22, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	31, 32, 33, 73, 73, 73, 73, 73, 
	38, 39, 40, 75, 73, 3, 76, 76, 
	76, 76, 76, 76, 76, 76, 76, 76, 
	76, 76, 76, 4, 76, 77, 73, 14, 
	73, 6, 76, 76, 76, 76, 76, 76, 
	76, 76, 76, 76, 76, 76, 76, 76, 
	76, 76, 76, 76, 76, 76, 76, 76, 
	76, 76, 76, 76, 76, 76, 76, 76, 
	76, 6, 76, 76, 76, 6, 76, 9, 
	73, 73, 73, 9, 73, 73, 73, 73, 
	73, 3, 6, 14, 73, 74, 73, 73, 
	73, 73, 73, 73, 18, 19, 20, 21, 
	22, 23, 24, 18, 25, 26, 27, 28, 
	29, 30, 73, 31, 32, 33, 73, 34, 
	35, 73, 37, 38, 39, 40, 6, 73, 
	3, 6, 73, 73, 74, 73, 73, 73, 
	73, 73, 73, 18, 19, 20, 21, 22, 
	23, 24, 18, 25, 26, 27, 28, 29, 
	30, 73, 31, 32, 33, 73, 73, 73, 
	73, 37, 38, 39, 40, 6, 73, 34, 
	35, 73, 35, 73, 9, 76, 76, 76, 
	9, 76, 0
};

static const char _use_syllable_machine_trans_targs[] = {
	5, 8, 5, 35, 2, 5, 1, 46, 
	5, 6, 5, 30, 32, 55, 56, 58, 
	59, 33, 36, 37, 38, 39, 40, 50, 
	51, 52, 60, 53, 47, 48, 49, 43, 
	44, 45, 61, 62, 63, 54, 41, 42, 
	5, 5, 7, 0, 10, 11, 12, 13, 
	14, 25, 26, 27, 28, 22, 23, 24, 
	17, 18, 19, 29, 15, 16, 5, 5, 
	9, 20, 5, 21, 5, 31, 5, 34, 
	5, 5, 3, 4, 5, 57
};

static const char _use_syllable_machine_trans_actions[] = {
	1, 0, 2, 3, 0, 4, 0, 5, 
	8, 5, 9, 0, 5, 10, 0, 10, 
	3, 0, 5, 5, 0, 0, 0, 5, 
	5, 5, 3, 3, 5, 5, 5, 5, 
	5, 5, 0, 0, 0, 3, 0, 0, 
	11, 12, 5, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 5, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 13, 14, 
	0, 0, 15, 0, 16, 0, 17, 0, 
	18, 19, 0, 0, 20, 0
};

static const char _use_syllable_machine_to_state_actions[] = {
	0, 0, 0, 0, 0, 6, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _use_syllable_machine_from_state_actions[] = {
	0, 0, 0, 0, 0, 7, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const short _use_syllable_machine_eof_trans[] = {
	1, 3, 3, 6, 6, 0, 42, 42, 
	64, 64, 42, 42, 42, 42, 42, 42, 
	42, 42, 42, 42, 67, 42, 42, 42, 
	42, 42, 42, 42, 42, 64, 69, 71, 
	42, 73, 73, 74, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 77, 
	74, 74, 77, 74, 74, 74, 74, 77
};

static const int use_syllable_machine_start = 5;
static const int use_syllable_machine_first_final = 5;
static const int use_syllable_machine_error = -1;

static const int use_syllable_machine_en_main = 5;


#line 39 "hb-ot-shape-complex-use-machine.rl"



#line 160 "hb-ot-shape-complex-use-machine.rl"


#define found_syllable(syllable_type) \
  HB_STMT_START { \
    if (0) fprintf (stderr, "syllable %d..%d %s\n", (*ts).second.first, (*te).second.first, #syllable_type); \
    for (unsigned i = (*ts).second.first; i < (*te).second.first; ++i) \
      info[i].syllable() = (syllable_serial << 4) | use_##syllable_type; \
    syllable_serial++; \
    if (unlikely (syllable_serial == 16)) syllable_serial = 1; \
  } HB_STMT_END

static bool
not_standard_default_ignorable (const hb_glyph_info_t &i)
{ return !((i.use_category() == USE_O || i.use_category() == USE_Rsv) && _hb_glyph_info_is_default_ignorable (&i)); }

static void
find_syllables_use (hb_buffer_t *buffer)
{
  hb_glyph_info_t *info = buffer->info;
  auto p =
    + hb_iter (info, buffer->len)
    | hb_enumerate
    | hb_filter (not_standard_default_ignorable, hb_second)
    | hb_filter ([&] (const hb_pair_t<unsigned, const hb_glyph_info_t &> p)
		 {
		   if (p.second.use_category() == USE_ZWNJ)
		     for (unsigned i = p.first + 1; i < buffer->len; ++i)
		       if (not_standard_default_ignorable (info[i]))
			 return !_hb_glyph_info_is_unicode_mark (&info[i]);
		   return true;
		 })
    | hb_enumerate
    | machine_index
    ;
  auto pe = p + p.len ();
  auto eof = +pe;
  auto ts = +p;
  auto te = +p;
  unsigned int act;
  int cs;
  
#line 373 "hb-ot-shape-complex-use-machine.hh"
	{
	cs = use_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 203 "hb-ot-shape-complex-use-machine.rl"


  unsigned int syllable_serial = 1;
  
#line 386 "hb-ot-shape-complex-use-machine.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
_resume:
	switch ( _use_syllable_machine_from_state_actions[cs] ) {
	case 7:
#line 1 "NONE"
	{ts = p;}
	break;
#line 400 "hb-ot-shape-complex-use-machine.hh"
	}

	_keys = _use_syllable_machine_trans_keys + (cs<<1);
	_inds = _use_syllable_machine_indicies + _use_syllable_machine_index_offsets[cs];

	_slen = _use_syllable_machine_key_spans[cs];
	_trans = _inds[ _slen > 0 && _keys[0] <=( (*p).second.second.use_category()) &&
		( (*p).second.second.use_category()) <= _keys[1] ?
		( (*p).second.second.use_category()) - _keys[0] : _slen ];

_eof_trans:
	cs = _use_syllable_machine_trans_targs[_trans];

	if ( _use_syllable_machine_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _use_syllable_machine_trans_actions[_trans] ) {
	case 5:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 8:
#line 148 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (independent_cluster); }}
	break;
	case 13:
#line 151 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (standard_cluster); }}
	break;
	case 11:
#line 155 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (broken_cluster); }}
	break;
	case 9:
#line 156 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (non_cluster); }}
	break;
	case 14:
#line 149 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (virama_terminated_cluster); }}
	break;
	case 15:
#line 150 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (sakot_terminated_cluster); }}
	break;
	case 12:
#line 151 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (standard_cluster); }}
	break;
	case 17:
#line 152 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (number_joiner_terminated_cluster); }}
	break;
	case 16:
#line 153 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (numeral_cluster); }}
	break;
	case 18:
#line 154 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (symbol_cluster); }}
	break;
	case 19:
#line 155 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (broken_cluster); }}
	break;
	case 20:
#line 156 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (non_cluster); }}
	break;
	case 1:
#line 151 "hb-ot-shape-complex-use-machine.rl"
	{{p = ((te))-1;}{ found_syllable (standard_cluster); }}
	break;
	case 4:
#line 155 "hb-ot-shape-complex-use-machine.rl"
	{{p = ((te))-1;}{ found_syllable (broken_cluster); }}
	break;
	case 2:
#line 1 "NONE"
	{	switch( act ) {
	case 8:
	{{p = ((te))-1;} found_syllable (broken_cluster); }
	break;
	case 9:
	{{p = ((te))-1;} found_syllable (non_cluster); }
	break;
	}
	}
	break;
	case 3:
#line 1 "NONE"
	{te = p+1;}
#line 155 "hb-ot-shape-complex-use-machine.rl"
	{act = 8;}
	break;
	case 10:
#line 1 "NONE"
	{te = p+1;}
#line 156 "hb-ot-shape-complex-use-machine.rl"
	{act = 9;}
	break;
#line 502 "hb-ot-shape-complex-use-machine.hh"
	}

_again:
	switch ( _use_syllable_machine_to_state_actions[cs] ) {
	case 6:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 511 "hb-ot-shape-complex-use-machine.hh"
	}

	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _use_syllable_machine_eof_trans[cs] > 0 ) {
		_trans = _use_syllable_machine_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	}

#line 208 "hb-ot-shape-complex-use-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_USE_MACHINE_HH */
