
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
	12u, 44u, 1u, 15u, 1u, 1u, 12u, 44u, 0u, 47u, 21u, 21u, 11u, 47u, 11u, 47u, 
	1u, 15u, 1u, 1u, 11u, 47u, 22u, 47u, 23u, 47u, 24u, 47u, 25u, 47u, 26u, 47u, 
	45u, 46u, 46u, 46u, 24u, 47u, 24u, 47u, 24u, 47u, 23u, 47u, 23u, 47u, 23u, 47u, 
	22u, 47u, 22u, 47u, 22u, 47u, 22u, 47u, 11u, 47u, 1u, 47u, 11u, 47u, 13u, 21u, 
	4u, 4u, 13u, 13u, 11u, 47u, 11u, 47u, 41u, 42u, 42u, 42u, 11u, 47u, 11u, 47u, 
	22u, 47u, 23u, 47u, 24u, 47u, 25u, 47u, 26u, 47u, 45u, 46u, 46u, 46u, 24u, 47u, 
	24u, 47u, 24u, 47u, 23u, 47u, 23u, 47u, 23u, 47u, 22u, 47u, 22u, 47u, 22u, 47u, 
	22u, 47u, 11u, 47u, 1u, 47u, 1u, 15u, 4u, 4u, 13u, 21u, 13u, 13u, 12u, 44u, 
	1u, 47u, 11u, 47u, 41u, 42u, 42u, 42u, 21u, 42u, 1u, 5u, 0
};

static const char _use_syllable_machine_key_spans[] = {
	33, 15, 1, 33, 48, 1, 37, 37, 
	15, 1, 37, 26, 25, 24, 23, 22, 
	2, 1, 24, 24, 24, 25, 25, 25, 
	26, 26, 26, 26, 37, 47, 37, 9, 
	1, 1, 37, 37, 2, 1, 37, 37, 
	26, 25, 24, 23, 22, 2, 1, 24, 
	24, 24, 25, 25, 25, 26, 26, 26, 
	26, 37, 47, 15, 1, 9, 1, 33, 
	47, 37, 2, 1, 22, 5
};

static const short _use_syllable_machine_index_offsets[] = {
	0, 34, 50, 52, 86, 135, 137, 175, 
	213, 229, 231, 269, 296, 322, 347, 371, 
	394, 397, 399, 424, 449, 474, 500, 526, 
	552, 579, 606, 633, 660, 698, 746, 784, 
	794, 796, 798, 836, 874, 877, 879, 917, 
	955, 982, 1008, 1033, 1057, 1080, 1083, 1085, 
	1110, 1135, 1160, 1186, 1212, 1238, 1265, 1292, 
	1319, 1346, 1384, 1432, 1448, 1450, 1460, 1462, 
	1496, 1544, 1582, 1585, 1587, 1610
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
	9, 7, 10, 11, 9, 9, 9, 9, 
	9, 3, 12, 13, 9, 14, 7, 7, 
	15, 16, 9, 9, 17, 18, 19, 20, 
	21, 22, 23, 17, 24, 25, 26, 27, 
	28, 29, 9, 30, 31, 32, 9, 33, 
	34, 35, 36, 37, 38, 39, 9, 41, 
	40, 43, 1, 42, 42, 44, 42, 42, 
	42, 42, 42, 45, 46, 47, 48, 49, 
	50, 51, 52, 46, 53, 45, 54, 55, 
	56, 57, 42, 58, 59, 60, 42, 42, 
	42, 42, 61, 62, 63, 64, 42, 43, 
	1, 42, 42, 44, 42, 42, 42, 42, 
	42, 65, 46, 47, 48, 49, 50, 51, 
	52, 46, 53, 54, 54, 55, 56, 57, 
	42, 58, 59, 60, 42, 42, 42, 42, 
	61, 62, 63, 64, 42, 43, 66, 66, 
	66, 66, 66, 66, 66, 66, 66, 66, 
	66, 66, 66, 67, 66, 43, 66, 43, 
	1, 42, 42, 44, 42, 42, 42, 42, 
	42, 42, 46, 47, 48, 49, 50, 51, 
	52, 46, 53, 54, 54, 55, 56, 57, 
	42, 58, 59, 60, 42, 42, 42, 42, 
	61, 62, 63, 64, 42, 46, 47, 48, 
	49, 50, 42, 42, 42, 42, 42, 42, 
	55, 56, 57, 42, 58, 59, 60, 42, 
	42, 42, 42, 47, 62, 63, 64, 42, 
	47, 48, 49, 50, 42, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 58, 59, 
	60, 42, 42, 42, 42, 42, 62, 63, 
	64, 42, 48, 49, 50, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 42, 62, 
	63, 64, 42, 49, 50, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 42, 62, 
	63, 64, 42, 50, 42, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 62, 63, 
	64, 42, 62, 63, 42, 63, 42, 48, 
	49, 50, 42, 42, 42, 42, 42, 42, 
	42, 42, 42, 42, 58, 59, 60, 42, 
	42, 42, 42, 42, 62, 63, 64, 42, 
	48, 49, 50, 42, 42, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 59, 60, 
	42, 42, 42, 42, 42, 62, 63, 64, 
	42, 48, 49, 50, 42, 42, 42, 42, 
	42, 42, 42, 42, 42, 42, 42, 42, 
	60, 42, 42, 42, 42, 42, 62, 63, 
	64, 42, 47, 48, 49, 50, 42, 42, 
	42, 42, 42, 42, 55, 56, 57, 42, 
	58, 59, 60, 42, 42, 42, 42, 47, 
	62, 63, 64, 42, 47, 48, 49, 50, 
	42, 42, 42, 42, 42, 42, 42, 56, 
	57, 42, 58, 59, 60, 42, 42, 42, 
	42, 47, 62, 63, 64, 42, 47, 48, 
	49, 50, 42, 42, 42, 42, 42, 42, 
	42, 42, 57, 42, 58, 59, 60, 42, 
	42, 42, 42, 47, 62, 63, 64, 42, 
	46, 47, 48, 49, 50, 42, 52, 46, 
	42, 42, 42, 55, 56, 57, 42, 58, 
	59, 60, 42, 42, 42, 42, 47, 62, 
	63, 64, 42, 46, 47, 48, 49, 50, 
	42, 68, 46, 42, 42, 42, 55, 56, 
	57, 42, 58, 59, 60, 42, 42, 42, 
	42, 47, 62, 63, 64, 42, 46, 47, 
	48, 49, 50, 42, 42, 46, 42, 42, 
	42, 55, 56, 57, 42, 58, 59, 60, 
	42, 42, 42, 42, 47, 62, 63, 64, 
	42, 46, 47, 48, 49, 50, 51, 52, 
	46, 42, 42, 42, 55, 56, 57, 42, 
	58, 59, 60, 42, 42, 42, 42, 47, 
	62, 63, 64, 42, 43, 1, 42, 42, 
	44, 42, 42, 42, 42, 42, 42, 46, 
	47, 48, 49, 50, 51, 52, 46, 53, 
	42, 54, 55, 56, 57, 42, 58, 59, 
	60, 42, 42, 42, 42, 61, 62, 63, 
	64, 42, 43, 66, 66, 66, 66, 66, 
	66, 66, 66, 66, 66, 66, 66, 66, 
	67, 66, 66, 66, 66, 66, 66, 66, 
	47, 48, 49, 50, 66, 66, 66, 66, 
	66, 66, 66, 66, 66, 66, 58, 59, 
	60, 66, 66, 66, 66, 66, 62, 63, 
	64, 66, 43, 1, 42, 42, 44, 42, 
	42, 42, 42, 42, 42, 46, 47, 48, 
	49, 50, 51, 52, 46, 53, 45, 54, 
	55, 56, 57, 42, 58, 59, 60, 42, 
	42, 42, 42, 61, 62, 63, 64, 42, 
	70, 69, 69, 69, 69, 69, 69, 69, 
	71, 69, 10, 72, 70, 69, 43, 1, 
	42, 42, 44, 42, 42, 42, 42, 42, 
	73, 46, 47, 48, 49, 50, 51, 52, 
	46, 53, 45, 54, 55, 56, 57, 42, 
	58, 59, 60, 42, 74, 75, 42, 61, 
	62, 63, 64, 42, 43, 1, 42, 42, 
	44, 42, 42, 42, 42, 42, 42, 46, 
	47, 48, 49, 50, 51, 52, 46, 53, 
	45, 54, 55, 56, 57, 42, 58, 59, 
	60, 42, 74, 75, 42, 61, 62, 63, 
	64, 42, 74, 75, 76, 75, 76, 3, 
	6, 77, 77, 78, 77, 77, 77, 77, 
	77, 79, 17, 18, 19, 20, 21, 22, 
	23, 17, 24, 26, 26, 27, 28, 29, 
	77, 30, 31, 32, 77, 77, 77, 77, 
	36, 37, 38, 39, 77, 3, 6, 77, 
	77, 78, 77, 77, 77, 77, 77, 77, 
	17, 18, 19, 20, 21, 22, 23, 17, 
	24, 26, 26, 27, 28, 29, 77, 30, 
	31, 32, 77, 77, 77, 77, 36, 37, 
	38, 39, 77, 17, 18, 19, 20, 21, 
	77, 77, 77, 77, 77, 77, 27, 28, 
	29, 77, 30, 31, 32, 77, 77, 77, 
	77, 18, 37, 38, 39, 77, 18, 19, 
	20, 21, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 30, 31, 32, 77, 
	77, 77, 77, 77, 37, 38, 39, 77, 
	19, 20, 21, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 37, 38, 39, 
	77, 20, 21, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 37, 38, 39, 
	77, 21, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 37, 38, 39, 77, 
	37, 38, 77, 38, 77, 19, 20, 21, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 30, 31, 32, 77, 77, 77, 
	77, 77, 37, 38, 39, 77, 19, 20, 
	21, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 31, 32, 77, 77, 
	77, 77, 77, 37, 38, 39, 77, 19, 
	20, 21, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 32, 77, 
	77, 77, 77, 77, 37, 38, 39, 77, 
	18, 19, 20, 21, 77, 77, 77, 77, 
	77, 77, 27, 28, 29, 77, 30, 31, 
	32, 77, 77, 77, 77, 18, 37, 38, 
	39, 77, 18, 19, 20, 21, 77, 77, 
	77, 77, 77, 77, 77, 28, 29, 77, 
	30, 31, 32, 77, 77, 77, 77, 18, 
	37, 38, 39, 77, 18, 19, 20, 21, 
	77, 77, 77, 77, 77, 77, 77, 77, 
	29, 77, 30, 31, 32, 77, 77, 77, 
	77, 18, 37, 38, 39, 77, 17, 18, 
	19, 20, 21, 77, 23, 17, 77, 77, 
	77, 27, 28, 29, 77, 30, 31, 32, 
	77, 77, 77, 77, 18, 37, 38, 39, 
	77, 17, 18, 19, 20, 21, 77, 80, 
	17, 77, 77, 77, 27, 28, 29, 77, 
	30, 31, 32, 77, 77, 77, 77, 18, 
	37, 38, 39, 77, 17, 18, 19, 20, 
	21, 77, 77, 17, 77, 77, 77, 27, 
	28, 29, 77, 30, 31, 32, 77, 77, 
	77, 77, 18, 37, 38, 39, 77, 17, 
	18, 19, 20, 21, 22, 23, 17, 77, 
	77, 77, 27, 28, 29, 77, 30, 31, 
	32, 77, 77, 77, 77, 18, 37, 38, 
	39, 77, 3, 6, 77, 77, 78, 77, 
	77, 77, 77, 77, 77, 17, 18, 19, 
	20, 21, 22, 23, 17, 24, 77, 26, 
	27, 28, 29, 77, 30, 31, 32, 77, 
	77, 77, 77, 36, 37, 38, 39, 77, 
	3, 77, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 77, 4, 77, 
	77, 77, 77, 77, 77, 77, 18, 19, 
	20, 21, 77, 77, 77, 77, 77, 77, 
	77, 77, 77, 77, 30, 31, 32, 77, 
	77, 77, 77, 77, 37, 38, 39, 77, 
	3, 81, 81, 81, 81, 81, 81, 81, 
	81, 81, 81, 81, 81, 81, 4, 81, 
	82, 77, 13, 77, 77, 77, 77, 77, 
	77, 77, 83, 77, 13, 77, 6, 81, 
	81, 81, 81, 81, 81, 81, 81, 81, 
	81, 81, 81, 81, 81, 81, 81, 81, 
	81, 81, 81, 81, 81, 81, 81, 81, 
	81, 81, 81, 81, 81, 81, 6, 81, 
	8, 77, 77, 77, 8, 77, 77, 77, 
	77, 77, 3, 6, 13, 77, 78, 77, 
	77, 77, 77, 77, 77, 17, 18, 19, 
	20, 21, 22, 23, 17, 24, 25, 26, 
	27, 28, 29, 77, 30, 31, 32, 77, 
	33, 34, 77, 36, 37, 38, 39, 77, 
	3, 6, 77, 77, 78, 77, 77, 77, 
	77, 77, 77, 17, 18, 19, 20, 21, 
	22, 23, 17, 24, 25, 26, 27, 28, 
	29, 77, 30, 31, 32, 77, 77, 77, 
	77, 36, 37, 38, 39, 77, 33, 34, 
	77, 34, 77, 74, 76, 76, 76, 76, 
	76, 76, 76, 76, 76, 76, 76, 76, 
	76, 76, 76, 76, 76, 76, 76, 74, 
	75, 76, 8, 81, 81, 81, 8, 81, 
	0
};

static const char _use_syllable_machine_trans_targs[] = {
	4, 8, 4, 38, 2, 4, 1, 5, 
	6, 4, 31, 34, 59, 60, 63, 64, 
	68, 40, 41, 42, 43, 44, 53, 54, 
	56, 65, 57, 50, 51, 52, 47, 48, 
	49, 66, 67, 69, 58, 45, 46, 4, 
	4, 4, 4, 7, 0, 30, 11, 12, 
	13, 14, 15, 24, 25, 27, 28, 21, 
	22, 23, 18, 19, 20, 29, 16, 17, 
	4, 10, 4, 9, 26, 4, 32, 33, 
	4, 35, 36, 37, 4, 4, 3, 39, 
	55, 4, 61, 62
};

static const char _use_syllable_machine_trans_actions[] = {
	1, 0, 2, 3, 0, 4, 0, 0, 
	7, 8, 0, 7, 9, 0, 9, 3, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 3, 3, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 3, 0, 0, 10, 
	11, 12, 13, 7, 0, 7, 0, 0, 
	0, 0, 0, 0, 0, 0, 7, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	14, 7, 15, 0, 0, 16, 0, 0, 
	17, 7, 0, 0, 18, 19, 0, 3, 
	0, 20, 0, 0
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
	0, 0, 0, 0, 0, 0
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
	0, 0, 0, 0, 0, 0
};

static const short _use_syllable_machine_eof_trans[] = {
	1, 3, 3, 6, 0, 41, 43, 43, 
	67, 67, 43, 43, 43, 43, 43, 43, 
	43, 43, 43, 43, 43, 43, 43, 43, 
	43, 43, 43, 43, 43, 67, 43, 70, 
	73, 70, 43, 43, 77, 77, 78, 78, 
	78, 78, 78, 78, 78, 78, 78, 78, 
	78, 78, 78, 78, 78, 78, 78, 78, 
	78, 78, 78, 82, 78, 78, 78, 82, 
	78, 78, 78, 78, 77, 82
};

static const int use_syllable_machine_start = 4;
static const int use_syllable_machine_first_final = 4;
static const int use_syllable_machine_error = -1;

static const int use_syllable_machine_en_main = 4;


#line 38 "hb-ot-shape-complex-use-machine.rl"



#line 150 "hb-ot-shape-complex-use-machine.rl"


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
  
#line 375 "hb-ot-shape-complex-use-machine.hh"
	{
	cs = use_syllable_machine_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 170 "hb-ot-shape-complex-use-machine.rl"


  p = 0;
  pe = eof = buffer->len;

  unsigned int syllable_serial = 1;
  
#line 391 "hb-ot-shape-complex-use-machine.hh"
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
#line 405 "hb-ot-shape-complex-use-machine.hh"
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
#line 139 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (independent_cluster); }}
	break;
	case 14:
#line 141 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (standard_cluster); }}
	break;
	case 10:
#line 145 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (broken_cluster); }}
	break;
	case 8:
#line 146 "hb-ot-shape-complex-use-machine.rl"
	{te = p+1;{ found_syllable (non_cluster); }}
	break;
	case 11:
#line 139 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (independent_cluster); }}
	break;
	case 15:
#line 140 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (virama_terminated_cluster); }}
	break;
	case 13:
#line 141 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (standard_cluster); }}
	break;
	case 17:
#line 142 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (number_joiner_terminated_cluster); }}
	break;
	case 16:
#line 143 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (numeral_cluster); }}
	break;
	case 18:
#line 144 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (symbol_cluster); }}
	break;
	case 19:
#line 145 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (broken_cluster); }}
	break;
	case 20:
#line 146 "hb-ot-shape-complex-use-machine.rl"
	{te = p;p--;{ found_syllable (non_cluster); }}
	break;
	case 1:
#line 141 "hb-ot-shape-complex-use-machine.rl"
	{{p = ((te))-1;}{ found_syllable (standard_cluster); }}
	break;
	case 4:
#line 145 "hb-ot-shape-complex-use-machine.rl"
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
#line 145 "hb-ot-shape-complex-use-machine.rl"
	{act = 7;}
	break;
	case 9:
#line 1 "NONE"
	{te = p+1;}
#line 146 "hb-ot-shape-complex-use-machine.rl"
	{act = 8;}
	break;
#line 507 "hb-ot-shape-complex-use-machine.hh"
	}

_again:
	switch ( _use_syllable_machine_to_state_actions[cs] ) {
	case 5:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 516 "hb-ot-shape-complex-use-machine.hh"
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

#line 178 "hb-ot-shape-complex-use-machine.rl"

}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_USE_MACHINE_HH */
