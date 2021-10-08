
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

#include "hb-ot-shape-complex-syllabic.hh"

/* buffer var allocations */
#define use_category() complex_var_u8_category()

#define USE(Cat) use_syllable_machine_ex_##Cat

enum use_syllable_type_t {
  use_virama_terminated_cluster,
  use_sakot_terminated_cluster,
  use_standard_cluster,
  use_number_joiner_terminated_cluster,
  use_numeral_cluster,
  use_symbol_cluster,
  use_hieroglyph_cluster,
  use_broken_cluster,
  use_non_cluster,
};


#line 57 "hb-ot-shape-complex-use-machine.hh"
#define use_syllable_machine_ex_B 1u
#define use_syllable_machine_ex_CGJ 6u
#define use_syllable_machine_ex_CMAbv 31u
#define use_syllable_machine_ex_CMBlw 32u
#define use_syllable_machine_ex_CS 43u
#define use_syllable_machine_ex_FAbv 24u
#define use_syllable_machine_ex_FBlw 25u
#define use_syllable_machine_ex_FMAbv 45u
#define use_syllable_machine_ex_FMBlw 46u
#define use_syllable_machine_ex_FMPst 47u
#define use_syllable_machine_ex_FPst 26u
#define use_syllable_machine_ex_G 49u
#define use_syllable_machine_ex_GB 5u
#define use_syllable_machine_ex_H 12u
#define use_syllable_machine_ex_HN 13u
#define use_syllable_machine_ex_HVM 44u
#define use_syllable_machine_ex_J 50u
#define use_syllable_machine_ex_MAbv 27u
#define use_syllable_machine_ex_MBlw 28u
#define use_syllable_machine_ex_MPre 30u
#define use_syllable_machine_ex_MPst 29u
#define use_syllable_machine_ex_N 4u
#define use_syllable_machine_ex_O 0u
#define use_syllable_machine_ex_R 18u
#define use_syllable_machine_ex_SB 51u
#define use_syllable_machine_ex_SE 52u
#define use_syllable_machine_ex_SMAbv 41u
#define use_syllable_machine_ex_SMBlw 42u
#define use_syllable_machine_ex_SUB 11u
#define use_syllable_machine_ex_Sk 48u
#define use_syllable_machine_ex_VAbv 33u
#define use_syllable_machine_ex_VBlw 34u
#define use_syllable_machine_ex_VMAbv 37u
#define use_syllable_machine_ex_VMBlw 38u
#define use_syllable_machine_ex_VMPre 23u
#define use_syllable_machine_ex_VMPst 39u
#define use_syllable_machine_ex_VPre 22u
#define use_syllable_machine_ex_VPst 35u
#define use_syllable_machine_ex_ZWNJ 14u


#line 99 "hb-ot-shape-complex-use-machine.hh"
static const unsigned char _use_syllable_machine_trans_keys[] = {
	1u, 1u, 1u, 1u, 0u, 51u, 41u, 42u, 42u, 42u, 11u, 48u, 11u, 48u, 1u, 1u, 
	22u, 48u, 23u, 48u, 24u, 47u, 25u, 47u, 26u, 47u, 45u, 46u, 46u, 46u, 24u, 48u, 
	24u, 48u, 24u, 48u, 1u, 1u, 24u, 48u, 23u, 48u, 23u, 48u, 23u, 48u, 22u, 48u, 
	22u, 48u, 22u, 48u, 11u, 48u, 1u, 48u, 13u, 13u, 4u, 4u, 11u, 48u, 11u, 48u, 
	22u, 48u, 23u, 48u, 24u, 47u, 25u, 47u, 26u, 47u, 45u, 46u, 46u, 46u, 24u, 48u, 
	24u, 48u, 24u, 48u, 24u, 48u, 23u, 48u, 23u, 48u, 23u, 48u, 22u, 48u, 22u, 48u, 
	22u, 48u, 11u, 48u, 1u, 48u, 1u, 1u, 4u, 4u, 13u, 13u, 1u, 48u, 11u, 48u, 
	41u, 42u, 42u, 42u, 1u, 5u, 50u, 52u, 49u, 52u, 49u, 51u, 0
};

static const char _use_syllable_machine_key_spans[] = {
	1, 1, 52, 2, 1, 38, 38, 1, 
	27, 26, 24, 23, 22, 2, 1, 25, 
	25, 25, 1, 25, 26, 26, 26, 27, 
	27, 27, 38, 48, 1, 1, 38, 38, 
	27, 26, 24, 23, 22, 2, 1, 25, 
	25, 25, 25, 26, 26, 26, 27, 27, 
	27, 38, 48, 1, 1, 1, 48, 38, 
	2, 1, 5, 3, 4, 3
};

static const short _use_syllable_machine_index_offsets[] = {
	0, 2, 4, 57, 60, 62, 101, 140, 
	142, 170, 197, 222, 246, 269, 272, 274, 
	300, 326, 352, 354, 380, 407, 434, 461, 
	489, 517, 545, 584, 633, 635, 637, 676, 
	715, 743, 770, 795, 819, 842, 845, 847, 
	873, 899, 925, 951, 978, 1005, 1032, 1060, 
	1088, 1116, 1155, 1204, 1206, 1208, 1210, 1259, 
	1298, 1301, 1303, 1309, 1313, 1318
};

static const char _use_syllable_machine_indicies[] = {
	1, 0, 2, 0, 3, 4, 5, 5, 
	6, 7, 5, 5, 5, 5, 5, 1, 
	8, 9, 5, 5, 5, 5, 10, 5, 
	5, 5, 11, 12, 13, 14, 15, 16, 
	17, 11, 18, 19, 20, 21, 22, 23, 
	5, 24, 25, 26, 5, 27, 28, 29, 
	30, 31, 32, 33, 8, 34, 5, 35, 
	5, 3, 37, 36, 37, 36, 39, 40, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 41, 42, 43, 44, 45, 46, 47, 
	41, 48, 4, 49, 50, 51, 52, 38, 
	53, 54, 55, 38, 38, 38, 38, 56, 
	57, 58, 59, 40, 38, 39, 40, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	41, 42, 43, 44, 45, 46, 47, 41, 
	48, 49, 49, 50, 51, 52, 38, 53, 
	54, 55, 38, 38, 38, 38, 56, 57, 
	58, 59, 40, 38, 39, 60, 41, 42, 
	43, 44, 45, 38, 38, 38, 38, 38, 
	38, 50, 51, 52, 38, 53, 54, 55, 
	38, 38, 38, 38, 42, 57, 58, 59, 
	61, 38, 42, 43, 44, 45, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	53, 54, 55, 38, 38, 38, 38, 38, 
	57, 58, 59, 61, 38, 43, 44, 45, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 57, 58, 59, 38, 44, 45, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 57, 58, 59, 38, 45, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 57, 58, 59, 38, 57, 58, 38, 
	58, 38, 43, 44, 45, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 53, 
	54, 55, 38, 38, 38, 38, 38, 57, 
	58, 59, 61, 38, 43, 44, 45, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 54, 55, 38, 38, 38, 38, 
	38, 57, 58, 59, 61, 38, 43, 44, 
	45, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 55, 38, 38, 
	38, 38, 38, 57, 58, 59, 61, 38, 
	63, 62, 43, 44, 45, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 38, 38, 38, 57, 
	58, 59, 61, 38, 42, 43, 44, 45, 
	38, 38, 38, 38, 38, 38, 50, 51, 
	52, 38, 53, 54, 55, 38, 38, 38, 
	38, 42, 57, 58, 59, 61, 38, 42, 
	43, 44, 45, 38, 38, 38, 38, 38, 
	38, 38, 51, 52, 38, 53, 54, 55, 
	38, 38, 38, 38, 42, 57, 58, 59, 
	61, 38, 42, 43, 44, 45, 38, 38, 
	38, 38, 38, 38, 38, 38, 52, 38, 
	53, 54, 55, 38, 38, 38, 38, 42, 
	57, 58, 59, 61, 38, 41, 42, 43, 
	44, 45, 38, 47, 41, 38, 38, 38, 
	50, 51, 52, 38, 53, 54, 55, 38, 
	38, 38, 38, 42, 57, 58, 59, 61, 
	38, 41, 42, 43, 44, 45, 38, 38, 
	41, 38, 38, 38, 50, 51, 52, 38, 
	53, 54, 55, 38, 38, 38, 38, 42, 
	57, 58, 59, 61, 38, 41, 42, 43, 
	44, 45, 46, 47, 41, 38, 38, 38, 
	50, 51, 52, 38, 53, 54, 55, 38, 
	38, 38, 38, 42, 57, 58, 59, 61, 
	38, 39, 40, 38, 38, 38, 38, 38, 
	38, 38, 38, 38, 41, 42, 43, 44, 
	45, 46, 47, 41, 48, 38, 49, 50, 
	51, 52, 38, 53, 54, 55, 38, 38, 
	38, 38, 56, 57, 58, 59, 40, 38, 
	39, 60, 60, 60, 60, 60, 60, 60, 
	60, 60, 60, 60, 60, 60, 60, 60, 
	60, 60, 60, 60, 60, 60, 42, 43, 
	44, 45, 60, 60, 60, 60, 60, 60, 
	60, 60, 60, 60, 53, 54, 55, 60, 
	60, 60, 60, 60, 57, 58, 59, 61, 
	60, 65, 64, 6, 66, 39, 40, 38, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	41, 42, 43, 44, 45, 46, 47, 41, 
	48, 4, 49, 50, 51, 52, 38, 53, 
	54, 55, 38, 3, 37, 38, 56, 57, 
	58, 59, 40, 38, 1, 68, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 11, 
	12, 13, 14, 15, 16, 17, 11, 18, 
	20, 20, 21, 22, 23, 67, 24, 25, 
	26, 67, 67, 67, 67, 30, 31, 32, 
	33, 68, 67, 11, 12, 13, 14, 15, 
	67, 67, 67, 67, 67, 67, 21, 22, 
	23, 67, 24, 25, 26, 67, 67, 67, 
	67, 12, 31, 32, 33, 69, 67, 12, 
	13, 14, 15, 67, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 24, 25, 26, 
	67, 67, 67, 67, 67, 31, 32, 33, 
	69, 67, 13, 14, 15, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 31, 
	32, 33, 67, 14, 15, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 31, 
	32, 33, 67, 15, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 31, 32, 
	33, 67, 31, 32, 67, 32, 67, 13, 
	14, 15, 67, 67, 67, 67, 67, 67, 
	67, 67, 67, 67, 24, 25, 26, 67, 
	67, 67, 67, 67, 31, 32, 33, 69, 
	67, 13, 14, 15, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 25, 
	26, 67, 67, 67, 67, 67, 31, 32, 
	33, 69, 67, 13, 14, 15, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 26, 67, 67, 67, 67, 67, 
	31, 32, 33, 69, 67, 13, 14, 15, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 31, 32, 33, 69, 67, 12, 
	13, 14, 15, 67, 67, 67, 67, 67, 
	67, 21, 22, 23, 67, 24, 25, 26, 
	67, 67, 67, 67, 12, 31, 32, 33, 
	69, 67, 12, 13, 14, 15, 67, 67, 
	67, 67, 67, 67, 67, 22, 23, 67, 
	24, 25, 26, 67, 67, 67, 67, 12, 
	31, 32, 33, 69, 67, 12, 13, 14, 
	15, 67, 67, 67, 67, 67, 67, 67, 
	67, 23, 67, 24, 25, 26, 67, 67, 
	67, 67, 12, 31, 32, 33, 69, 67, 
	11, 12, 13, 14, 15, 67, 17, 11, 
	67, 67, 67, 21, 22, 23, 67, 24, 
	25, 26, 67, 67, 67, 67, 12, 31, 
	32, 33, 69, 67, 11, 12, 13, 14, 
	15, 67, 67, 11, 67, 67, 67, 21, 
	22, 23, 67, 24, 25, 26, 67, 67, 
	67, 67, 12, 31, 32, 33, 69, 67, 
	11, 12, 13, 14, 15, 16, 17, 11, 
	67, 67, 67, 21, 22, 23, 67, 24, 
	25, 26, 67, 67, 67, 67, 12, 31, 
	32, 33, 69, 67, 1, 68, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 11, 
	12, 13, 14, 15, 16, 17, 11, 18, 
	67, 20, 21, 22, 23, 67, 24, 25, 
	26, 67, 67, 67, 67, 30, 31, 32, 
	33, 68, 67, 1, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 67, 
	67, 12, 13, 14, 15, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 67, 24, 
	25, 26, 67, 67, 67, 67, 67, 31, 
	32, 33, 69, 67, 1, 70, 71, 67, 
	9, 67, 4, 67, 67, 67, 4, 67, 
	67, 67, 67, 67, 1, 68, 9, 67, 
	67, 67, 67, 67, 67, 67, 67, 11, 
	12, 13, 14, 15, 16, 17, 11, 18, 
	19, 20, 21, 22, 23, 67, 24, 25, 
	26, 67, 27, 28, 67, 30, 31, 32, 
	33, 68, 67, 1, 68, 67, 67, 67, 
	67, 67, 67, 67, 67, 67, 11, 12, 
	13, 14, 15, 16, 17, 11, 18, 19, 
	20, 21, 22, 23, 67, 24, 25, 26, 
	67, 67, 67, 67, 30, 31, 32, 33, 
	68, 67, 27, 28, 67, 28, 67, 4, 
	70, 70, 70, 4, 70, 73, 72, 34, 
	72, 34, 73, 72, 73, 72, 34, 72, 
	35, 72, 0
};

static const char _use_syllable_machine_trans_targs[] = {
	2, 31, 42, 3, 5, 2, 28, 30, 
	51, 52, 54, 32, 33, 34, 35, 36, 
	46, 47, 48, 55, 49, 43, 44, 45, 
	39, 40, 41, 56, 57, 58, 50, 37, 
	38, 2, 59, 61, 2, 4, 2, 6, 
	7, 8, 9, 10, 11, 12, 23, 24, 
	25, 26, 20, 21, 22, 15, 16, 17, 
	27, 13, 14, 2, 2, 18, 2, 19, 
	2, 29, 2, 2, 0, 1, 2, 53, 
	2, 60
};

static const char _use_syllable_machine_trans_actions[] = {
	1, 2, 2, 0, 0, 5, 0, 0, 
	0, 0, 2, 2, 2, 0, 0, 0, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 0, 0, 0, 2, 0, 
	0, 6, 0, 0, 7, 0, 8, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 9, 10, 0, 11, 0, 
	12, 0, 13, 14, 0, 0, 15, 0, 
	16, 0
};

static const char _use_syllable_machine_to_state_actions[] = {
	0, 0, 3, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0
};

static const char _use_syllable_machine_from_state_actions[] = {
	0, 0, 4, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0
};

static const short _use_syllable_machine_eof_trans[] = {
	1, 1, 0, 37, 37, 39, 39, 61, 
	39, 39, 39, 39, 39, 39, 39, 39, 
	39, 39, 63, 39, 39, 39, 39, 39, 
	39, 39, 39, 61, 65, 67, 39, 68, 
	68, 68, 68, 68, 68, 68, 68, 68, 
	68, 68, 68, 68, 68, 68, 68, 68, 
	68, 68, 68, 71, 68, 68, 68, 68, 
	68, 68, 71, 73, 73, 73
};

static const int use_syllable_machine_start = 2;
static const int use_syllable_machine_first_final = 2;
static const int use_syllable_machine_error = -1;

static const int use_syllable_machine_en_main = 2;


#line 58 "hb-ot-shape-complex-use-machine.rl"



#line 173 "hb-ot-shape-complex-use-machine.rl"


#define found_syllable(syllable_type) \
  HB_STMT_START { \
    if (0) fprintf (stderr, "syllable %d..%d %s\n", (*ts).second.first, (*te).second.first, #syllable_type); \
    for (unsigned i = (*ts).second.first; i < (*te).second.first; ++i) \
      info[i].syllable() = (syllable_serial << 4) | syllable_type; \
    syllable_serial++; \
    if (unlikely (syllable_serial == 16)) syllable_serial = 1; \
  } HB_STMT_END


template <typename Iter>
struct machine_index_t :
  hb_iter_with_fallback_t<machine_index_t<Iter>,
			  typename Iter::item_t>
{
  machine_index_t (const Iter& it) : it (it) {}
  machine_index_t (const machine_index_t& o) : it (o.it) {}

  static constexpr bool is_random_access_iterator = Iter::is_random_access_iterator;
  static constexpr bool is_sorted_iterator = Iter::is_sorted_iterator;

  typename Iter::item_t __item__ () const { return *it; }
  typename Iter::item_t __item_at__ (unsigned i) const { return it[i]; }
  unsigned __len__ () const { return it.len (); }
  void __next__ () { ++it; }
  void __forward__ (unsigned n) { it += n; }
  void __prev__ () { --it; }
  void __rewind__ (unsigned n) { it -= n; }
  void operator = (unsigned n)
  { unsigned index = (*it).first; if (index < n) it += n - index; else if (index > n) it -= index - n; }
  void operator = (const machine_index_t& o) { *this = (*o.it).first; }
  bool operator == (const machine_index_t& o) const { return (*it).first == (*o.it).first; }
  bool operator != (const machine_index_t& o) const { return !(*this == o); }

  private:
  Iter it;
};
struct
{
  template <typename Iter,
	    hb_requires (hb_is_iterable (Iter))>
  machine_index_t<hb_iter_type<Iter>>
  operator () (Iter&& it) const
  { return machine_index_t<hb_iter_type<Iter>> (hb_iter (it)); }
}
HB_FUNCOBJ (machine_index);



static bool
not_ccs_default_ignorable (const hb_glyph_info_t &i)
{ return !(i.use_category() == USE(CGJ) && _hb_glyph_info_is_default_ignorable (&i)); }

static inline void
find_syllables_use (hb_buffer_t *buffer)
{
  hb_glyph_info_t *info = buffer->info;
  auto p =
    + hb_iter (info, buffer->len)
    | hb_enumerate
    | hb_filter ([] (const hb_glyph_info_t &i) { return not_ccs_default_ignorable (i); },
		 hb_second)
    | hb_filter ([&] (const hb_pair_t<unsigned, const hb_glyph_info_t &> p)
		 {
		   if (p.second.use_category() == USE(ZWNJ))
		     for (unsigned i = p.first + 1; i < buffer->len; ++i)
		       if (not_ccs_default_ignorable (info[i]))
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
  unsigned int act HB_UNUSED;
  int cs;
  
#line 455 "hb-ot-shape-complex-use-machine.hh"
	{
	cs = use_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 257 "hb-ot-shape-complex-use-machine.rl"


  unsigned int syllable_serial = 1;
  
#line 468 "hb-ot-shape-complex-use-machine.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
_resume:
	switch ( _use_syllable_machine_from_state_actions[cs] ) {
	case 4:
#line 1 "NONE"
	{ts = p;}
	break;
#line 482 "hb-ot-shape-complex-use-machine.hh"
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
	case 2:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 9:
#line 163 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (use_standard_cluster); }}
	break;
	case 6:
#line 168 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (use_broken_cluster); }}
	break;
	case 5:
#line 169 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (use_non_cluster); }}
	break;
	case 10:
#line 161 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_virama_terminated_cluster); }}
	break;
	case 11:
#line 162 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_sakot_terminated_cluster); }}
	break;
	case 8:
#line 163 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_standard_cluster); }}
	break;
	case 13:
#line 164 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_number_joiner_terminated_cluster); }}
	break;
	case 12:
#line 165 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_numeral_cluster); }}
	break;
	case 7:
#line 166 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_symbol_cluster); }}
	break;
	case 16:
#line 167 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_hieroglyph_cluster); }}
	break;
	case 14:
#line 168 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_broken_cluster); }}
	break;
	case 15:
#line 169 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (use_non_cluster); }}
	break;
	case 1:
#line 168 "hb-ot-shape-complex-use-machine.rl"
	{{p = ((te))-1;}{ found_syllable (use_broken_cluster); }}
	break;
#line 556 "hb-ot-shape-complex-use-machine.hh"
	}

_again:
	switch ( _use_syllable_machine_to_state_actions[cs] ) {
	case 3:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 565 "hb-ot-shape-complex-use-machine.hh"
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

#line 262 "hb-ot-shape-complex-use-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_USE_MACHINE_HH */
