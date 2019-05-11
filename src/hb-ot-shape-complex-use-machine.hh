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


static const unsigned char _use_syllable_machine_trans_keys[] = {
	7u, 33u, 1u, 9u, 1u, 1u, 7u, 33u,
	0u, 33u, 12u, 12u, 5u, 33u, 5u, 33u,
	1u, 9u, 1u, 1u, 5u, 33u, 5u, 33u,
	5u, 29u, 5u, 17u, 5u, 17u, 5u, 17u,
	5u, 29u, 5u, 29u, 5u, 29u, 5u, 33u,
	5u, 33u, 5u, 33u, 5u, 33u, 5u, 33u,
	5u, 33u, 5u, 33u, 5u, 33u, 1u, 29u,
	5u, 33u, 8u, 12u, 3u, 3u, 8u, 8u,
	5u, 33u, 5u, 33u, 30u, 31u, 31u, 31u,
	5u, 33u, 5u, 33u, 5u, 33u, 5u, 29u,
	5u, 17u, 5u, 17u, 5u, 17u, 5u, 29u,
	5u, 29u, 5u, 29u, 5u, 33u, 5u, 33u,
	5u, 33u, 5u, 33u, 5u, 33u, 5u, 33u,
	5u, 33u, 5u, 33u, 1u, 29u, 1u, 9u,
	3u, 3u, 8u, 12u, 8u, 8u, 7u, 33u,
	1u, 33u, 5u, 33u, 30u, 31u, 31u, 31u,
	12u, 31u, 1u, 4u, 0u
};

static const char _use_syllable_machine_char_class[] = {
	0, 1, 2, 0, 3, 4, 2, 2,
	5, 2, 2, 6, 7, 8, 2, 9,
	0, 0, 10, 11, 2, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22,
	23, 24, 25, 26, 2, 27, 28, 29,
	2, 30, 31, 32, 33, 0
};

static const short _use_syllable_machine_index_offsets[] = {
	0, 27, 36, 37, 64, 98, 99, 128,
	157, 166, 167, 196, 225, 250, 263, 276,
	289, 314, 339, 364, 393, 422, 451, 480,
	509, 538, 567, 596, 625, 654, 659, 660,
	661, 690, 719, 721, 722, 751, 780, 809,
	834, 847, 860, 873, 898, 923, 948, 977,
	1006, 1035, 1064, 1093, 1122, 1151, 1180, 1209,
	1218, 1219, 1224, 1225, 1252, 1285, 1314, 1316,
	1317, 1337, 0
};

static const char _use_syllable_machine_indicies[] = {
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 3, 2, 2, 2, 2,
	2, 2, 2, 4, 3, 6, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 6,
	7, 8, 9, 10, 11, 12, 3, 13,
	14, 15, 16, 17, 9, 18, 19, 20,
	21, 22, 23, 24, 18, 25, 26, 27,
	28, 29, 30, 31, 32, 33, 34, 35,
	36, 37, 39, 41, 42, 1, 40, 43,
	40, 40, 44, 45, 46, 47, 48, 49,
	50, 51, 45, 52, 44, 53, 54, 55,
	56, 57, 58, 59, 40, 40, 40, 60,
	41, 42, 1, 40, 43, 40, 40, 61,
	45, 46, 47, 48, 49, 50, 51, 45,
	52, 53, 53, 54, 55, 56, 57, 58,
	59, 40, 40, 40, 60, 42, 62, 62,
	62, 62, 62, 62, 62, 63, 42, 41,
	42, 1, 40, 43, 40, 40, 40, 45,
	46, 47, 48, 49, 50, 51, 45, 52,
	53, 53, 54, 55, 56, 57, 58, 59,
	40, 40, 40, 60, 41, 40, 40, 40,
	40, 40, 40, 40, 45, 46, 47, 48,
	49, 40, 40, 40, 40, 40, 40, 54,
	55, 56, 57, 58, 59, 40, 40, 40,
	46, 41, 40, 40, 40, 40, 40, 40,
	40, 40, 46, 47, 48, 49, 40, 40,
	40, 40, 40, 40, 40, 40, 40, 57,
	58, 59, 41, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 47, 48, 49, 41,
	40, 40, 40, 40, 40, 40, 40, 40,
	40, 40, 48, 49, 41, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40, 40,
	49, 41, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 47, 48, 49, 40, 40,
	40, 40, 40, 40, 40, 40, 40, 57,
	58, 59, 41, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 47, 48, 49, 40,
	40, 40, 40, 40, 40, 40, 40, 40,
	40, 58, 59, 41, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 47, 48, 49,
	40, 40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 59, 41, 40, 40, 40,
	40, 40, 40, 40, 40, 46, 47, 48,
	49, 40, 40, 40, 40, 40, 40, 54,
	55, 56, 57, 58, 59, 40, 40, 40,
	46, 41, 40, 40, 40, 40, 40, 40,
	40, 40, 46, 47, 48, 49, 40, 40,
	40, 40, 40, 40, 40, 55, 56, 57,
	58, 59, 40, 40, 40, 46, 41, 40,
	40, 40, 40, 40, 40, 40, 40, 46,
	47, 48, 49, 40, 40, 40, 40, 40,
	40, 40, 40, 56, 57, 58, 59, 40,
	40, 40, 46, 41, 40, 40, 40, 40,
	40, 40, 40, 45, 46, 47, 48, 49,
	40, 51, 45, 40, 40, 40, 54, 55,
	56, 57, 58, 59, 40, 40, 40, 46,
	41, 40, 40, 40, 40, 40, 40, 40,
	45, 46, 47, 48, 49, 40, 64, 45,
	40, 40, 40, 54, 55, 56, 57, 58,
	59, 40, 40, 40, 46, 41, 40, 40,
	40, 40, 40, 40, 40, 45, 46, 47,
	48, 49, 40, 40, 45, 40, 40, 40,
	54, 55, 56, 57, 58, 59, 40, 40,
	40, 46, 41, 40, 40, 40, 40, 40,
	40, 40, 45, 46, 47, 48, 49, 50,
	51, 45, 40, 40, 40, 54, 55, 56,
	57, 58, 59, 40, 40, 40, 46, 41,
	42, 1, 40, 43, 40, 40, 40, 45,
	46, 47, 48, 49, 50, 51, 45, 52,
	40, 53, 54, 55, 56, 57, 58, 59,
	40, 40, 40, 60, 42, 62, 62, 62,
	41, 62, 62, 62, 63, 62, 62, 62,
	62, 46, 47, 48, 49, 62, 62, 62,
	62, 62, 62, 62, 62, 62, 57, 58,
	59, 41, 42, 1, 40, 43, 40, 40,
	40, 45, 46, 47, 48, 49, 50, 51,
	45, 52, 44, 53, 54, 55, 56, 57,
	58, 59, 40, 40, 40, 60, 66, 65,
	65, 65, 67, 10, 66, 41, 42, 1,
	40, 43, 40, 40, 69, 45, 46, 47,
	48, 49, 50, 51, 45, 52, 44, 53,
	54, 55, 56, 57, 58, 59, 70, 71,
	40, 60, 41, 42, 1, 40, 43, 40,
	40, 40, 45, 46, 47, 48, 49, 50,
	51, 45, 52, 44, 53, 54, 55, 56,
	57, 58, 59, 70, 71, 40, 60, 70,
	71, 71, 12, 3, 6, 73, 74, 73,
	73, 75, 18, 19, 20, 21, 22, 23,
	24, 18, 25, 27, 27, 28, 29, 30,
	31, 32, 33, 73, 73, 73, 37, 12,
	3, 6, 73, 74, 73, 73, 73, 18,
	19, 20, 21, 22, 23, 24, 18, 25,
	27, 27, 28, 29, 30, 31, 32, 33,
	73, 73, 73, 37, 12, 73, 73, 73,
	73, 73, 73, 73, 18, 19, 20, 21,
	22, 73, 73, 73, 73, 73, 73, 28,
	29, 30, 31, 32, 33, 73, 73, 73,
	19, 12, 73, 73, 73, 73, 73, 73,
	73, 73, 19, 20, 21, 22, 73, 73,
	73, 73, 73, 73, 73, 73, 73, 31,
	32, 33, 12, 73, 73, 73, 73, 73,
	73, 73, 73, 73, 20, 21, 22, 12,
	73, 73, 73, 73, 73, 73, 73, 73,
	73, 73, 21, 22, 12, 73, 73, 73,
	73, 73, 73, 73, 73, 73, 73, 73,
	22, 12, 73, 73, 73, 73, 73, 73,
	73, 73, 73, 20, 21, 22, 73, 73,
	73, 73, 73, 73, 73, 73, 73, 31,
	32, 33, 12, 73, 73, 73, 73, 73,
	73, 73, 73, 73, 20, 21, 22, 73,
	73, 73, 73, 73, 73, 73, 73, 73,
	73, 32, 33, 12, 73, 73, 73, 73,
	73, 73, 73, 73, 73, 20, 21, 22,
	73, 73, 73, 73, 73, 73, 73, 73,
	73, 73, 73, 33, 12, 73, 73, 73,
	73, 73, 73, 73, 73, 19, 20, 21,
	22, 73, 73, 73, 73, 73, 73, 28,
	29, 30, 31, 32, 33, 73, 73, 73,
	19, 12, 73, 73, 73, 73, 73, 73,
	73, 73, 19, 20, 21, 22, 73, 73,
	73, 73, 73, 73, 73, 29, 30, 31,
	32, 33, 73, 73, 73, 19, 12, 73,
	73, 73, 73, 73, 73, 73, 73, 19,
	20, 21, 22, 73, 73, 73, 73, 73,
	73, 73, 73, 30, 31, 32, 33, 73,
	73, 73, 19, 12, 73, 73, 73, 73,
	73, 73, 73, 18, 19, 20, 21, 22,
	73, 24, 18, 73, 73, 73, 28, 29,
	30, 31, 32, 33, 73, 73, 73, 19,
	12, 73, 73, 73, 73, 73, 73, 73,
	18, 19, 20, 21, 22, 73, 76, 18,
	73, 73, 73, 28, 29, 30, 31, 32,
	33, 73, 73, 73, 19, 12, 73, 73,
	73, 73, 73, 73, 73, 18, 19, 20,
	21, 22, 73, 73, 18, 73, 73, 73,
	28, 29, 30, 31, 32, 33, 73, 73,
	73, 19, 12, 73, 73, 73, 73, 73,
	73, 73, 18, 19, 20, 21, 22, 23,
	24, 18, 73, 73, 73, 28, 29, 30,
	31, 32, 33, 73, 73, 73, 19, 12,
	3, 6, 73, 74, 73, 73, 73, 18,
	19, 20, 21, 22, 23, 24, 18, 25,
	73, 27, 28, 29, 30, 31, 32, 33,
	73, 73, 73, 37, 3, 73, 73, 73,
	12, 73, 73, 73, 4, 73, 73, 73,
	73, 19, 20, 21, 22, 73, 73, 73,
	73, 73, 73, 73, 73, 73, 31, 32,
	33, 3, 77, 77, 77, 77, 77, 77,
	77, 4, 78, 14, 73, 73, 73, 79,
	14, 6, 77, 77, 77, 77, 77, 77,
	77, 77, 77, 77, 77, 77, 77, 77,
	77, 77, 77, 77, 77, 77, 77, 77,
	77, 77, 77, 6, 8, 73, 73, 8,
	12, 3, 6, 14, 74, 73, 73, 73,
	18, 19, 20, 21, 22, 23, 24, 18,
	25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 73, 37, 12, 3, 6,
	73, 74, 73, 73, 73, 18, 19, 20,
	21, 22, 23, 24, 18, 25, 26, 27,
	28, 29, 30, 31, 32, 33, 73, 73,
	73, 37, 34, 35, 35, 70, 72, 72,
	72, 72, 72, 72, 72, 72, 72, 72,
	72, 72, 72, 72, 72, 72, 72, 70,
	71, 8, 77, 77, 8, 0
};

static const char _use_syllable_machine_index_defaults[] = {
	0, 2, 2, 5, 9, 38, 40, 40,
	62, 62, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 62, 40, 65, 68, 65,
	40, 40, 72, 72, 73, 73, 73, 73,
	73, 73, 73, 73, 73, 73, 73, 73,
	73, 73, 73, 73, 73, 73, 73, 77,
	73, 73, 73, 77, 73, 73, 73, 73,
	72, 77, 0
};

static const char _use_syllable_machine_trans_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	0
};

static const char _use_syllable_machine_cond_targs[] = {
	4, 8, 4, 36, 2, 4, 1, 5,
	6, 4, 29, 32, 4, 55, 56, 59,
	60, 64, 38, 39, 40, 41, 42, 49,
	50, 52, 61, 53, 46, 47, 48, 43,
	44, 45, 62, 63, 65, 54, 4, 4,
	4, 4, 7, 0, 28, 11, 12, 13,
	14, 15, 22, 23, 25, 26, 19, 20,
	21, 16, 17, 18, 27, 10, 4, 9,
	24, 4, 30, 31, 4, 33, 34, 35,
	4, 4, 3, 37, 51, 4, 57, 58,
	0
};

static const char _use_syllable_machine_cond_actions[] = {
	1, 0, 2, 3, 0, 4, 0, 0,
	7, 8, 0, 7, 9, 10, 0, 10,
	3, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 3, 3, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 3, 11, 12,
	13, 14, 7, 0, 7, 0, 0, 0,
	0, 0, 0, 0, 0, 7, 0, 0,
	0, 0, 0, 0, 0, 7, 15, 0,
	0, 16, 0, 0, 17, 7, 0, 0,
	18, 19, 0, 3, 0, 20, 0, 0,
	0
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
	0, 0, 0
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
	0, 0, 0
};

static const char _use_syllable_machine_eof_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, 0
};

static const char _use_syllable_machine_eof_cond_key_offs[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _use_syllable_machine_eof_cond_key_lens[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _use_syllable_machine_eof_cond_keys[] = {
	0
};

static const char _use_syllable_machine_eof_trans[] = {
	1, 3, 3, 6, 0, 39, 41, 41,
	63, 63, 41, 41, 41, 41, 41, 41,
	41, 41, 41, 41, 41, 41, 41, 41,
	41, 41, 41, 63, 41, 66, 69, 66,
	41, 41, 73, 73, 74, 74, 74, 74,
	74, 74, 74, 74, 74, 74, 74, 74,
	74, 74, 74, 74, 74, 74, 74, 78,
	74, 74, 74, 78, 74, 74, 74, 74,
	73, 78, 0
};

static const char _use_syllable_machine_nfa_targs[] = {
	0, 0
};

static const char _use_syllable_machine_nfa_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _use_syllable_machine_nfa_push_actions[] = {
	0, 0
};

static const char _use_syllable_machine_nfa_pop_trans[] = {
	0, 0
};

static const int use_syllable_machine_start = 4;
static const int use_syllable_machine_first_final = 4;
static const int use_syllable_machine_error = -1;

static const int use_syllable_machine_en_main = 4;





#define found_syllable(syllable_type) \
HB_STMT_START { \
	if (0) fprintf (stderr, "syllable %d..%d %s\n", ts, te, #syllable_type); \
	for (unsigned int i = ts; i < te; i++) \
	info[i].syllable() = (syllable_serial << 4) | syllable_type; \
	syllable_serial++; \
	if (unlikely (syllable_serial == 16)) syllable_serial=1; \
} HB_STMT_END

static void
find_syllables (hb_buffer_t *buffer)
{
	unsigned int p, pe, eof, ts, te, act;
	int cs;
	hb_glyph_info_t *info = buffer->info;
	
	{
		cs = (int)use_syllable_machine_start;
		ts = 0;
		te = 0;
		act = 0;
	}
	
	
	p=0;
	pe = eof = buffer->len;
	
	unsigned int syllable_serial=1;
	
	{
		int _cpc;
		int _klen;const char * _cekeys;unsigned int _trans = 0;const unsigned char * _keys;const char * _inds;	 {
			if ( p == pe )
			goto _test_eof;
			_resume:  {
				switch ( _use_syllable_machine_from_state_actions[cs] ) {
					case 6:  {
						{
							#line 1 "NONE"
							{ts = p;}}
						
						break; }
				}
				
				_keys = ( _use_syllable_machine_trans_keys + ((cs<<1)));
				_inds = ( _use_syllable_machine_indicies + (_use_syllable_machine_index_offsets[cs]));
				
				if ( (info[p].use_category()) <= 44 )
				{
					int _ic = (int)_use_syllable_machine_char_class[(int)(info[p].use_category()) - 0];
					if ( _ic <= (int)(*( _keys+1)) && _ic >= (int)(*( _keys)) )
					_trans = (unsigned int)(*( _inds + (int)( _ic - (int)(*( _keys)) ) )); 
					else
					_trans = (unsigned int)_use_syllable_machine_index_defaults[cs];
				}
				else {
					_trans = (unsigned int)_use_syllable_machine_index_defaults[cs];
				}
				
				goto _match_cond;
			}
			_match_cond:  {
				cs = (int)_use_syllable_machine_cond_targs[_trans];
				
				if ( _use_syllable_machine_cond_actions[_trans] == 0 )
				goto _again;
				
				switch ( _use_syllable_machine_cond_actions[_trans] ) {
					case 7:  {
						{
							#line 1 "NONE"
							{te = p+1;}}
						
						break; }
					case 12:  {
						{
							#line 135 "hb-ot-shape-complex-use-machine.rl"
							{te = p+1;{
									#line 135 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (independent_cluster); }}}
						
						break; }
					case 14:  {
						{
							#line 137 "hb-ot-shape-complex-use-machine.rl"
							{te = p+1;{
									#line 137 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (standard_cluster); }}}
						
						break; }
					case 9:  {
						{
							#line 141 "hb-ot-shape-complex-use-machine.rl"
							{te = p+1;{
									#line 141 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (broken_cluster); }}}
						
						break; }
					case 8:  {
						{
							#line 142 "hb-ot-shape-complex-use-machine.rl"
							{te = p+1;{
									#line 142 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (non_cluster); }}}
						
						break; }
					case 11:  {
						{
							#line 135 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 135 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (independent_cluster); }}}
						
						break; }
					case 15:  {
						{
							#line 136 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 136 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (virama_terminated_cluster); }}}
						
						break; }
					case 13:  {
						{
							#line 137 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 137 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (standard_cluster); }}}
						
						break; }
					case 17:  {
						{
							#line 138 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 138 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (number_joiner_terminated_cluster); }}}
						
						break; }
					case 16:  {
						{
							#line 139 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 139 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (numeral_cluster); }}}
						
						break; }
					case 18:  {
						{
							#line 140 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 140 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (symbol_cluster); }}}
						
						break; }
					case 19:  {
						{
							#line 141 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 141 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (broken_cluster); }}}
						
						break; }
					case 20:  {
						{
							#line 142 "hb-ot-shape-complex-use-machine.rl"
							{te = p;p = p - 1;{
									#line 142 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (non_cluster); }}}
						
						break; }
					case 1:  {
						{
							#line 137 "hb-ot-shape-complex-use-machine.rl"
							{p = ((te))-1;
								{
									#line 137 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (standard_cluster); }}}
						
						break; }
					case 4:  {
						{
							#line 141 "hb-ot-shape-complex-use-machine.rl"
							{p = ((te))-1;
								{
									#line 141 "hb-ot-shape-complex-use-machine.rl"
									found_syllable (broken_cluster); }}}
						
						break; }
					case 2:  {
						{
							#line 1 "NONE"
							{switch( act ) {
									case 7:  {
										p = ((te))-1;
										{
											#line 141 "hb-ot-shape-complex-use-machine.rl"
											found_syllable (broken_cluster); } break; }
									case 8:  {
										p = ((te))-1;
										{
											#line 142 "hb-ot-shape-complex-use-machine.rl"
											found_syllable (non_cluster); } break; }
								}}
						}
						
						break; }
					case 3:  {
						{
							#line 1 "NONE"
							{te = p+1;}}
						{
							#line 141 "hb-ot-shape-complex-use-machine.rl"
							{act = 7;}}
						
						break; }
					case 10:  {
						{
							#line 1 "NONE"
							{te = p+1;}}
						{
							#line 142 "hb-ot-shape-complex-use-machine.rl"
							{act = 8;}}
						
						break; }
				}
				
				
			}
			_again:  {
				switch ( _use_syllable_machine_to_state_actions[cs] ) {
					case 5:  {
						{
							#line 1 "NONE"
							{ts = 0;}}
						
						break; }
				}
				
				p += 1;
				if ( p != pe )
				goto _resume;
			}
			_test_eof:  { {}
				if ( p == eof )
				{
					if ( _use_syllable_machine_eof_cond_spaces[cs] != -1 ) {
						_cekeys = ( _use_syllable_machine_eof_cond_keys + (_use_syllable_machine_eof_cond_key_offs[cs]));
						_klen = (int)_use_syllable_machine_eof_cond_key_lens[cs];
						_cpc = 0;
						{
							const char *_lower = _cekeys;
							const char *_upper = _cekeys + _klen - 1;
							const char *_mid;
							while ( 1 ) {
								if ( _upper < _lower )
								break;
								
								_mid = _lower + ((_upper-_lower) >> 1);
								if ( _cpc < (int)(*( _mid)) )
								_upper = _mid - 1;
								else if ( _cpc > (int)(*( _mid)) )
								_lower = _mid + 1;
								else {
									goto _ok;
								}
							}
							cs = -1;
							goto _out;
						}
						_ok: {}
					}
					if ( _use_syllable_machine_eof_trans[cs] > 0 ) {
						_trans = (unsigned int)_use_syllable_machine_eof_trans[cs] - 1;
						goto _match_cond;
					}
				}
				
			}
			_out:  { {}
			}
		}
	}
	
}

#undef found_syllable

#endif /* HB_OT_SHAPE_COMPLEX_USE_MACHINE_HH */
