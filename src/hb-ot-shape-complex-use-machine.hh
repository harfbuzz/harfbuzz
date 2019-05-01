
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


#line 38 "hb-ot-shape-complex-use-machine.hh"
static const unsigned char _use_syllable_machine_trans_keys[] = {
	12u, 44u, 1u, 15u, 1u, 1u, 12u, 44u, 0u, 44u, 21u, 21u, 8u, 44u, 8u, 44u, 
	1u, 15u, 1u, 1u, 8u, 44u, 8u, 44u, 8u, 39u, 8u, 26u, 8u, 26u, 8u, 26u, 
	8u, 39u, 8u, 39u, 8u, 39u, 8u, 44u, 8u, 44u, 8u, 44u, 8u, 44u, 8u, 44u, 
	8u, 44u, 8u, 44u, 8u, 44u, 1u, 39u, 8u, 44u, 13u, 21u, 4u, 4u, 13u, 13u, 
	8u, 44u, 8u, 44u, 41u, 42u, 42u, 42u, 8u, 44u, 8u, 44u, 8u, 44u, 8u, 39u, 
	8u, 26u, 8u, 26u, 8u, 26u, 8u, 39u, 8u, 39u, 8u, 39u, 8u, 44u, 8u, 44u, 
	8u, 44u, 8u, 44u, 8u, 44u, 8u, 44u, 8u, 44u, 8u, 44u, 1u, 39u, 1u, 15u, 
	4u, 4u, 13u, 21u, 13u, 13u, 12u, 44u, 1u, 44u, 8u, 44u, 41u, 42u, 42u, 42u, 
	21u, 42u, 1u, 5u, 0
};

static const char _use_syllable_machine_key_spans[] = {
	33, 15, 1, 33, 45, 1, 37, 37, 
	15, 1, 37, 37, 32, 19, 19, 19, 
	32, 32, 32, 37, 37, 37, 37, 37, 
	37, 37, 37, 39, 37, 9, 1, 1, 
	37, 37, 2, 1, 37, 37, 37, 32, 
	19, 19, 19, 32, 32, 32, 37, 37, 
	37, 37, 37, 37, 37, 37, 39, 15, 
	1, 9, 1, 33, 44, 37, 2, 1, 
	22, 5
};

static const short _use_syllable_machine_index_offsets[] = {
	0, 34, 50, 52, 86, 132, 134, 172, 
	210, 226, 228, 266, 304, 337, 357, 377, 
	397, 430, 463, 496, 534, 572, 610, 648, 
	686, 724, 762, 800, 840, 878, 888, 890, 
	892, 930, 968, 971, 973, 1011, 1049, 1087, 
	1120, 1140, 1160, 1180, 1213, 1246, 1279, 1317, 
	1355, 1393, 1431, 1469, 1507, 1545, 1583, 1623, 
	1639, 1641, 1651, 1653, 1687, 1732, 1770, 1773, 
	1775, 1798
};

static const char _use_syllable_machine_indicies[] = {
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 3, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	4, 2, 3, 2, 6, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 6, 5, 7, 8, 
	9, 7, 10, 11, 9, 9, 12, 9, 
	9, 3, 13, 14, 9, 15, 7, 7, 
	16, 17, 9, 9, 18, 19, 20, 21, 
	22, 23, 24, 18, 25, 26, 27, 28, 
	29, 30, 9, 31, 32, 33, 9, 34, 
	35, 36, 37, 9, 39, 38, 41, 40, 
	40, 42, 1, 40, 40, 43, 40, 40, 
	40, 40, 40, 44, 45, 46, 47, 48, 
	49, 50, 51, 45, 52, 44, 53, 54, 
	55, 56, 40, 57, 58, 59, 40, 40, 
	40, 40, 60, 40, 41, 40, 40, 42, 
	1, 40, 40, 43, 40, 40, 40, 40, 
	40, 61, 45, 46, 47, 48, 49, 50, 
	51, 45, 52, 53, 53, 54, 55, 56, 
	40, 57, 58, 59, 40, 40, 40, 40, 
	60, 40, 42, 62, 62, 62, 62, 62, 
	62, 62, 62, 62, 62, 62, 62, 62, 
	63, 62, 42, 62, 41, 40, 40, 42, 
	1, 40, 40, 43, 40, 40, 40, 40, 
	40, 40, 45, 46, 47, 48, 49, 50, 
	51, 45, 52, 53, 53, 54, 55, 56, 
	40, 57, 58, 59, 40, 40, 40, 40, 
	60, 40, 41, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	45, 46, 47, 48, 49, 40, 40, 40, 
	40, 40, 40, 54, 55, 56, 40, 57, 
	58, 59, 40, 40, 40, 40, 46, 40, 
	41, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 46, 
	47, 48, 49, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 57, 58, 59, 
	40, 41, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 47, 48, 49, 40, 41, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 48, 49, 
	40, 41, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 49, 40, 41, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 47, 48, 49, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 57, 58, 59, 40, 41, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 47, 48, 
	49, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 58, 59, 40, 41, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 47, 
	48, 49, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 59, 40, 
	41, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 46, 
	47, 48, 49, 40, 40, 40, 40, 40, 
	40, 54, 55, 56, 40, 57, 58, 59, 
	40, 40, 40, 40, 46, 40, 41, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 46, 47, 48, 
	49, 40, 40, 40, 40, 40, 40, 40, 
	55, 56, 40, 57, 58, 59, 40, 40, 
	40, 40, 46, 40, 41, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 46, 47, 48, 49, 40, 
	40, 40, 40, 40, 40, 40, 40, 56, 
	40, 57, 58, 59, 40, 40, 40, 40, 
	46, 40, 41, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	45, 46, 47, 48, 49, 40, 51, 45, 
	40, 40, 40, 54, 55, 56, 40, 57, 
	58, 59, 40, 40, 40, 40, 46, 40, 
	41, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 45, 46, 
	47, 48, 49, 40, 64, 45, 40, 40, 
	40, 54, 55, 56, 40, 57, 58, 59, 
	40, 40, 40, 40, 46, 40, 41, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 45, 46, 47, 48, 
	49, 40, 40, 45, 40, 40, 40, 54, 
	55, 56, 40, 57, 58, 59, 40, 40, 
	40, 40, 46, 40, 41, 40, 40, 40, 
	40, 40, 40, 40, 40, 40, 40, 40, 
	40, 40, 45, 46, 47, 48, 49, 50, 
	51, 45, 40, 40, 40, 54, 55, 56, 
	40, 57, 58, 59, 40, 40, 40, 40, 
	46, 40, 41, 40, 40, 42, 1, 40, 
	40, 43, 40, 40, 40, 40, 40, 40, 
	45, 46, 47, 48, 49, 50, 51, 45, 
	52, 40, 53, 54, 55, 56, 40, 57, 
	58, 59, 40, 40, 40, 40, 60, 40, 
	42, 62, 62, 62, 62, 62, 62, 41, 
	62, 62, 62, 62, 62, 62, 63, 62, 
	62, 62, 62, 62, 62, 62, 46, 47, 
	48, 49, 62, 62, 62, 62, 62, 62, 
	62, 62, 62, 62, 57, 58, 59, 62, 
	41, 40, 40, 42, 1, 40, 40, 43, 
	40, 40, 40, 40, 40, 40, 45, 46, 
	47, 48, 49, 50, 51, 45, 52, 44, 
	53, 54, 55, 56, 40, 57, 58, 59, 
	40, 40, 40, 40, 60, 40, 66, 65, 
	65, 65, 65, 65, 65, 65, 67, 65, 
	10, 68, 66, 65, 41, 40, 40, 42, 
	1, 40, 40, 43, 40, 40, 40, 40, 
	40, 69, 45, 46, 47, 48, 49, 50, 
	51, 45, 52, 44, 53, 54, 55, 56, 
	40, 57, 58, 59, 40, 70, 71, 40, 
	60, 40, 41, 40, 40, 42, 1, 40, 
	40, 43, 40, 40, 40, 40, 40, 40, 
	45, 46, 47, 48, 49, 50, 51, 45, 
	52, 44, 53, 54, 55, 56, 40, 57, 
	58, 59, 40, 70, 71, 40, 60, 40, 
	70, 71, 72, 71, 72, 12, 73, 73, 
	3, 6, 73, 73, 74, 73, 73, 73, 
	73, 73, 75, 18, 19, 20, 21, 22, 
	23, 24, 18, 25, 27, 27, 28, 29, 
	30, 73, 31, 32, 33, 73, 73, 73, 
	73, 37, 73, 12, 73, 73, 3, 6, 
	73, 73, 74, 73, 73, 73, 73, 73, 
	73, 18, 19, 20, 21, 22, 23, 24, 
	18, 25, 27, 27, 28, 29, 30, 73, 
	31, 32, 33, 73, 73, 73, 73, 37, 
	73, 12, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 18, 
	19, 20, 21, 22, 73, 73, 73, 73, 
	73, 73, 28, 29, 30, 73, 31, 32, 
	33, 73, 73, 73, 73, 19, 73, 12, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 19, 20, 
	21, 22, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 31, 32, 33, 73, 
	12, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	20, 21, 22, 73, 12, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 21, 22, 73, 
	12, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 22, 73, 12, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 20, 21, 22, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 31, 32, 33, 73, 12, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 20, 21, 22, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 32, 33, 73, 12, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 20, 21, 
	22, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 33, 73, 12, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 19, 20, 
	21, 22, 73, 73, 73, 73, 73, 73, 
	28, 29, 30, 73, 31, 32, 33, 73, 
	73, 73, 73, 19, 73, 12, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 19, 20, 21, 22, 
	73, 73, 73, 73, 73, 73, 73, 29, 
	30, 73, 31, 32, 33, 73, 73, 73, 
	73, 19, 73, 12, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 19, 20, 21, 22, 73, 73, 
	73, 73, 73, 73, 73, 73, 30, 73, 
	31, 32, 33, 73, 73, 73, 73, 19, 
	73, 12, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 18, 
	19, 20, 21, 22, 73, 24, 18, 73, 
	73, 73, 28, 29, 30, 73, 31, 32, 
	33, 73, 73, 73, 73, 19, 73, 12, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 18, 19, 20, 
	21, 22, 73, 76, 18, 73, 73, 73, 
	28, 29, 30, 73, 31, 32, 33, 73, 
	73, 73, 73, 19, 73, 12, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 18, 19, 20, 21, 22, 
	73, 73, 18, 73, 73, 73, 28, 29, 
	30, 73, 31, 32, 33, 73, 73, 73, 
	73, 19, 73, 12, 73, 73, 73, 73, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 18, 19, 20, 21, 22, 23, 24, 
	18, 73, 73, 73, 28, 29, 30, 73, 
	31, 32, 33, 73, 73, 73, 73, 19, 
	73, 12, 73, 73, 3, 6, 73, 73, 
	74, 73, 73, 73, 73, 73, 73, 18, 
	19, 20, 21, 22, 23, 24, 18, 25, 
	73, 27, 28, 29, 30, 73, 31, 32, 
	33, 73, 73, 73, 73, 37, 73, 3, 
	73, 73, 73, 73, 73, 73, 12, 73, 
	73, 73, 73, 73, 73, 4, 73, 73, 
	73, 73, 73, 73, 73, 19, 20, 21, 
	22, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 73, 31, 32, 33, 73, 3, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 4, 77, 78, 
	73, 14, 73, 73, 73, 73, 73, 73, 
	73, 79, 73, 14, 73, 6, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 6, 77, 8, 
	73, 73, 73, 8, 73, 73, 12, 73, 
	73, 3, 6, 14, 73, 74, 73, 73, 
	73, 73, 73, 73, 18, 19, 20, 21, 
	22, 23, 24, 18, 25, 26, 27, 28, 
	29, 30, 73, 31, 32, 33, 73, 34, 
	35, 73, 37, 73, 12, 73, 73, 3, 
	6, 73, 73, 74, 73, 73, 73, 73, 
	73, 73, 18, 19, 20, 21, 22, 23, 
	24, 18, 25, 26, 27, 28, 29, 30, 
	73, 31, 32, 33, 73, 73, 73, 73, 
	37, 73, 34, 35, 73, 35, 73, 70, 
	72, 72, 72, 72, 72, 72, 72, 72, 
	72, 72, 72, 72, 72, 72, 72, 72, 
	72, 72, 72, 70, 71, 72, 8, 77, 
	77, 77, 8, 77, 0
};

static const char _use_syllable_machine_trans_targs[] = {
	4, 8, 4, 36, 2, 4, 1, 5, 
	6, 4, 29, 32, 4, 55, 56, 59, 
	60, 64, 38, 39, 40, 41, 42, 49, 
	50, 52, 61, 53, 46, 47, 48, 43, 
	44, 45, 62, 63, 65, 54, 4, 4, 
	4, 4, 7, 0, 28, 11, 12, 13, 
	14, 15, 22, 23, 25, 26, 19, 20, 
	21, 16, 17, 18, 27, 10, 4, 9, 
	24, 4, 30, 31, 4, 33, 34, 35, 
	4, 4, 3, 37, 51, 4, 57, 58
};

static const char _use_syllable_machine_trans_actions[] = {
	1, 0, 2, 3, 0, 4, 0, 0, 
	7, 8, 0, 7, 9, 10, 0, 10, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 3, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 3, 11, 12, 
	13, 14, 7, 0, 7, 0, 0, 0, 
	0, 0, 0, 0, 0, 7, 0, 0, 
	0, 0, 0, 0, 0, 7, 15, 0, 
	0, 16, 0, 0, 17, 7, 0, 0, 
	18, 19, 0, 3, 0, 20, 0, 0
};

static const char _use_syllable_machine_to_state_actions[] = {
	0, 0, 0, 0, 5, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const char _use_syllable_machine_from_state_actions[] = {
	0, 0, 0, 0, 6, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const short _use_syllable_machine_eof_trans[] = {
	1, 3, 3, 6, 0, 39, 41, 41, 
	63, 63, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 41, 63, 41, 66, 69, 66, 
	41, 41, 73, 73, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 78, 
	74, 74, 74, 78, 74, 74, 74, 74, 
	73, 78
};

static const int use_syllable_machine_start = 4;
static const int use_syllable_machine_first_final = 4;
static const int use_syllable_machine_error = -1;

static const int use_syllable_machine_en_main = 4;


#line 38 "hb-ot-shape-complex-use-machine.rl"



#line 146 "hb-ot-shape-complex-use-machine.rl"


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
  unsigned int p, pe, eof, ts, te, act;
  int cs;
  hb_glyph_info_t *info = buffer->info;
  
#line 396 "hb-ot-shape-complex-use-machine.hh"
	{
	cs = use_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 166 "hb-ot-shape-complex-use-machine.rl"


  p = 0;
  pe = eof = buffer->len;

  unsigned int syllable_serial = 1;
  
#line 412 "hb-ot-shape-complex-use-machine.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
_resume:
	switch ( _use_syllable_machine_from_state_actions[cs] ) {
	case 6:
#line 1 "NONE"
	{ts = p;}
	break;
#line 426 "hb-ot-shape-complex-use-machine.hh"
	}

	_keys = _use_syllable_machine_trans_keys + (cs<<1);
	_inds = _use_syllable_machine_indicies + _use_syllable_machine_index_offsets[cs];

	_slen = _use_syllable_machine_key_spans[cs];
	_trans = _inds[ _slen > 0 && _keys[0] <=( info[p].use_category()) &&
		( info[p].use_category()) <= _keys[1] ?
		( info[p].use_category()) - _keys[0] : _slen ];

_eof_trans:
	cs = _use_syllable_machine_trans_targs[_trans];

	if ( _use_syllable_machine_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _use_syllable_machine_trans_actions[_trans] ) {
	case 7:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 12:
#line 135 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (independent_cluster); }}
	break;
	case 14:
#line 137 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (standard_cluster); }}
	break;
	case 9:
#line 141 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (broken_cluster); }}
	break;
	case 8:
#line 142 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (non_cluster); }}
	break;
	case 11:
#line 135 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (independent_cluster); }}
	break;
	case 15:
#line 136 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (virama_terminated_cluster); }}
	break;
	case 13:
#line 137 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (standard_cluster); }}
	break;
	case 17:
#line 138 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (number_joiner_terminated_cluster); }}
	break;
	case 16:
#line 139 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (numeral_cluster); }}
	break;
	case 18:
#line 140 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (symbol_cluster); }}
	break;
	case 19:
#line 141 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (broken_cluster); }}
	break;
	case 20:
#line 142 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (non_cluster); }}
	break;
	case 1:
#line 137 "hb-ot-shape-complex-use-machine.rl"
	{{p = ((te))-1;}{ found_syllable (standard_cluster); }}
	break;
	case 4:
#line 141 "hb-ot-shape-complex-use-machine.rl"
	{{p = ((te))-1;}{ found_syllable (broken_cluster); }}
	break;
	case 2:
#line 1 "NONE"
	{	switch( act ) {
	case 7:
	{{p = ((te))-1;} found_syllable (broken_cluster); }
	break;
	case 8:
	{{p = ((te))-1;} found_syllable (non_cluster); }
	break;
	}
	}
	break;
	case 3:
#line 1 "NONE"
	{te = p+1;}
#line 141 "hb-ot-shape-complex-use-machine.rl"
	{act = 7;}
	break;
	case 10:
#line 1 "NONE"
	{te = p+1;}
#line 142 "hb-ot-shape-complex-use-machine.rl"
	{act = 8;}
	break;
#line 528 "hb-ot-shape-complex-use-machine.hh"
	}

_again:
	switch ( _use_syllable_machine_to_state_actions[cs] ) {
	case 5:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 537 "hb-ot-shape-complex-use-machine.hh"
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

#line 174 "hb-ot-shape-complex-use-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_USE_MACHINE_HH */
