
#line 1 "hb-buffer-deserialize-text.rl"
/*
 * Copyright © 2013  Google, Inc.
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

#ifndef HB_BUFFER_DESERIALIZE_TEXT_HH
#define HB_BUFFER_DESERIALIZE_TEXT_HH

#include "hb.hh"


#line 36 "hb-buffer-deserialize-text.hh"
static const unsigned char _deserialize_text_trans_keys[] = {
	0u, 0u, 9u, 91u, 85u, 85u, 43u, 43u, 48u, 102u, 9u, 85u, 48u, 57u, 48u, 122u, 
	9u, 122u, 45u, 57u, 48u, 57u, 45u, 57u, 48u, 57u, 48u, 57u, 45u, 57u, 48u, 57u, 
	44u, 44u, 45u, 57u, 48u, 57u, 44u, 57u, 9u, 124u, 9u, 124u, 0u, 0u, 9u, 85u, 
	9u, 124u, 9u, 124u, 9u, 124u, 9u, 124u, 9u, 122u, 9u, 124u, 9u, 124u, 9u, 124u, 
	9u, 124u, 9u, 124u, 9u, 124u, 9u, 124u, 9u, 124u, 9u, 124u, 9u, 124u, 0
};

static const char _deserialize_text_key_spans[] = {
	0, 83, 1, 1, 55, 77, 10, 75, 
	114, 13, 10, 13, 10, 10, 13, 10, 
	1, 13, 10, 14, 116, 116, 0, 77, 
	116, 116, 116, 116, 114, 116, 116, 116, 
	116, 116, 116, 116, 116, 116, 116
};

static const short _deserialize_text_index_offsets[] = {
	0, 0, 84, 86, 88, 144, 222, 233, 
	309, 424, 438, 449, 463, 474, 485, 499, 
	510, 512, 526, 537, 552, 669, 786, 787, 
	865, 982, 1099, 1216, 1333, 1448, 1565, 1682, 
	1799, 1916, 2033, 2150, 2267, 2384, 2501
};

static const char _deserialize_text_indicies[] = {
	0, 0, 0, 0, 0, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	0, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 2, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 3, 1, 4, 1, 5, 
	1, 6, 6, 6, 6, 6, 6, 6, 
	6, 6, 6, 1, 1, 1, 1, 1, 
	1, 1, 6, 6, 6, 6, 6, 6, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 6, 6, 6, 6, 6, 6, 
	1, 7, 7, 7, 7, 7, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	7, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 4, 1, 8, 
	9, 9, 9, 9, 9, 9, 9, 9, 
	9, 1, 10, 11, 11, 11, 11, 11, 
	11, 11, 11, 11, 1, 1, 1, 1, 
	1, 1, 1, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 1, 1, 1, 
	1, 1, 1, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 1, 13, 13, 
	13, 13, 13, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 13, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 10, 11, 11, 
	11, 11, 11, 11, 11, 11, 11, 1, 
	1, 1, 1, 1, 1, 1, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	1, 1, 1, 1, 1, 1, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	1, 14, 1, 1, 15, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 1, 17, 
	18, 18, 18, 18, 18, 18, 18, 18, 
	18, 1, 19, 1, 1, 20, 21, 21, 
	21, 21, 21, 21, 21, 21, 21, 1, 
	22, 23, 23, 23, 23, 23, 23, 23, 
	23, 23, 1, 24, 25, 25, 25, 25, 
	25, 25, 25, 25, 25, 1, 26, 1, 
	1, 27, 28, 28, 28, 28, 28, 28, 
	28, 28, 28, 1, 29, 30, 30, 30, 
	30, 30, 30, 30, 30, 30, 1, 31, 
	1, 32, 1, 1, 33, 34, 34, 34, 
	34, 34, 34, 34, 34, 34, 1, 35, 
	36, 36, 36, 36, 36, 36, 36, 36, 
	36, 1, 31, 1, 1, 1, 30, 30, 
	30, 30, 30, 30, 30, 30, 30, 30, 
	1, 37, 37, 37, 37, 37, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	37, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	38, 38, 38, 38, 38, 38, 38, 38, 
	38, 38, 1, 1, 1, 39, 40, 1, 
	1, 38, 38, 38, 38, 38, 38, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 38, 38, 38, 38, 38, 38, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 41, 1, 42, 42, 
	42, 42, 42, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 42, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 43, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 44, 1, 1, 7, 7, 7, 7, 
	7, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 7, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	4, 1, 45, 45, 45, 45, 45, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 45, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 46, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 47, 1, 45, 
	45, 45, 45, 45, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 45, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	1, 1, 1, 1, 46, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 47, 1, 49, 49, 49, 49, 
	49, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 49, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 50, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	51, 1, 1, 52, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	53, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 54, 
	1, 55, 55, 55, 55, 55, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	55, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 43, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 56, 1, 13, 13, 
	13, 13, 13, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 13, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 10, 11, 11, 
	11, 11, 11, 11, 11, 11, 11, 1, 
	1, 1, 1, 1, 1, 1, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	1, 1, 1, 1, 1, 1, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	12, 12, 12, 12, 12, 12, 12, 12, 
	1, 49, 49, 49, 49, 49, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	49, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 50, 1, 1, 1, 1, 
	57, 57, 57, 57, 57, 57, 57, 57, 
	57, 57, 1, 1, 1, 51, 1, 1, 
	52, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 53, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 54, 1, 58, 58, 
	58, 58, 58, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 58, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 59, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 60, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 61, 1, 62, 62, 62, 62, 62, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 62, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 63, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 64, 1, 
	62, 62, 62, 62, 62, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 62, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 23, 
	23, 23, 23, 23, 23, 23, 23, 23, 
	23, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 63, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 64, 1, 58, 58, 58, 
	58, 58, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 58, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	59, 1, 1, 1, 18, 18, 18, 18, 
	18, 18, 18, 18, 18, 18, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 60, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	61, 1, 65, 65, 65, 65, 65, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 65, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 66, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 67, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 46, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 68, 1, 69, 
	69, 69, 69, 69, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 69, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 70, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 71, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 72, 1, 69, 69, 69, 69, 
	69, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 69, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 70, 1, 
	1, 1, 1, 36, 36, 36, 36, 36, 
	36, 36, 36, 36, 36, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	71, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 72, 
	1, 65, 65, 65, 65, 65, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	65, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 66, 1, 1, 1, 1, 
	73, 73, 73, 73, 73, 73, 73, 73, 
	73, 73, 1, 1, 1, 1, 1, 1, 
	67, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 46, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 68, 1, 49, 49, 
	49, 49, 49, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 49, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	50, 1, 74, 74, 1, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 1, 
	1, 1, 51, 1, 1, 52, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	1, 1, 53, 1, 74, 1, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	74, 74, 74, 74, 74, 74, 74, 74, 
	1, 54, 1, 0
};

static const char _deserialize_text_trans_targs[] = {
	1, 0, 2, 7, 3, 4, 20, 5, 
	24, 25, 26, 29, 38, 8, 10, 30, 
	33, 30, 33, 12, 31, 32, 31, 32, 
	34, 37, 15, 16, 19, 16, 19, 17, 
	18, 35, 36, 35, 36, 21, 20, 6, 
	22, 23, 21, 22, 23, 21, 22, 23, 
	25, 27, 9, 13, 14, 22, 28, 27, 
	28, 29, 27, 11, 22, 28, 27, 22, 
	28, 27, 9, 14, 28, 27, 9, 22, 
	28, 37, 38
};

static const char _deserialize_text_trans_actions[] = {
	0, 0, 0, 0, 1, 0, 2, 0, 
	3, 3, 4, 4, 4, 0, 3, 3, 
	3, 0, 0, 3, 3, 3, 0, 0, 
	3, 3, 3, 3, 3, 0, 0, 5, 
	3, 3, 3, 0, 0, 6, 7, 8, 
	6, 6, 0, 0, 0, 9, 9, 9, 
	0, 10, 11, 11, 11, 10, 10, 0, 
	0, 12, 13, 14, 13, 13, 15, 15, 
	15, 9, 16, 16, 9, 17, 18, 17, 
	17, 0, 12
};

static const char _deserialize_text_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 6, 0, 0, 0, 
	9, 9, 10, 0, 0, 10, 13, 15, 
	15, 13, 9, 17, 17, 9, 10
};

static const int deserialize_text_start = 1;
static const int deserialize_text_first_final = 20;
static const int deserialize_text_error = 0;

static const int deserialize_text_en_main = 1;


#line 132 "hb-buffer-deserialize-text.rl"


static hb_bool_t
_hb_buffer_deserialize_text (hb_buffer_t *buffer,
				    const char *buf,
				    unsigned int buf_len,
				    const char **end_ptr,
				    hb_font_t *font)
{
  const char *p = buf, *pe = buf + buf_len;

  /* Ensure we have positions. */
  (void) hb_buffer_get_glyph_positions (buffer, nullptr);

  while (p < pe && ISSPACE (*p))
    p++;

  const char *eof = pe, *tok = nullptr;
  int cs;
  hb_glyph_info_t info = {0};
  hb_glyph_position_t pos = {0};
  
#line 456 "hb-buffer-deserialize-text.hh"
	{
	cs = deserialize_text_start;
	}

#line 461 "hb-buffer-deserialize-text.hh"
	{
	int _slen;
	int _trans;
	const unsigned char *_keys;
	const char *_inds;
	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _deserialize_text_trans_keys + (cs<<1);
	_inds = _deserialize_text_indicies + _deserialize_text_index_offsets[cs];

	_slen = _deserialize_text_key_spans[cs];
	_trans = _inds[ _slen > 0 && _keys[0] <=(*p) &&
		(*p) <= _keys[1] ?
		(*p) - _keys[0] : _slen ];

	cs = _deserialize_text_trans_targs[_trans];

	if ( _deserialize_text_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _deserialize_text_trans_actions[_trans] ) {
	case 1:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	memset (&info, 0, sizeof (info));
	memset (&pos , 0, sizeof (pos ));
}
	break;
	case 3:
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
	break;
	case 12:
#line 55 "hb-buffer-deserialize-text.rl"
	{
  if (unlikely (buffer->content_type != HB_BUFFER_CONTENT_TYPE_GLYPHS))
  {
    if (buffer->content_type != HB_BUFFER_CONTENT_TYPE_INVALID) {
    	buffer->clear();
      return false;
    }
    assert (buffer->len == 0);
    buffer->content_type = HB_BUFFER_CONTENT_TYPE_GLYPHS;
  }
}
	break;
	case 7:
#line 67 "hb-buffer-deserialize-text.rl"
	{
  if (unlikely (buffer->content_type != HB_BUFFER_CONTENT_TYPE_UNICODE))
  {
    if (buffer->content_type != HB_BUFFER_CONTENT_TYPE_INVALID) {
    	buffer->clear();
      return false;
    }
    assert (buffer->len == 0);
    buffer->content_type = HB_BUFFER_CONTENT_TYPE_UNICODE;
  }
}
	break;
	case 11:
#line 79 "hb-buffer-deserialize-text.rl"
	{
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 8:
#line 86 "hb-buffer-deserialize-text.rl"
	{if (!parse_hex (tok, p, &info.codepoint )) return false; }
	break;
	case 16:
#line 88 "hb-buffer-deserialize-text.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
	break;
	case 5:
#line 89 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_offset )) return false; }
	break;
	case 18:
#line 90 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
	break;
	case 14:
#line 91 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
	break;
	case 2:
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 67 "hb-buffer-deserialize-text.rl"
	{
  if (unlikely (buffer->content_type != HB_BUFFER_CONTENT_TYPE_UNICODE))
  {
    if (buffer->content_type != HB_BUFFER_CONTENT_TYPE_INVALID) {
    	buffer->clear();
      return false;
    }
    assert (buffer->len == 0);
    buffer->content_type = HB_BUFFER_CONTENT_TYPE_UNICODE;
  }
}
	break;
	case 10:
#line 79 "hb-buffer-deserialize-text.rl"
	{
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 6:
#line 86 "hb-buffer-deserialize-text.rl"
	{if (!parse_hex (tok, p, &info.codepoint )) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 9:
#line 88 "hb-buffer-deserialize-text.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 17:
#line 90 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 13:
#line 91 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 15:
#line 92 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 4:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	memset (&info, 0, sizeof (info));
	memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 55 "hb-buffer-deserialize-text.rl"
	{
  if (unlikely (buffer->content_type != HB_BUFFER_CONTENT_TYPE_GLYPHS))
  {
    if (buffer->content_type != HB_BUFFER_CONTENT_TYPE_INVALID) {
    	buffer->clear();
      return false;
    }
    assert (buffer->len == 0);
    buffer->content_type = HB_BUFFER_CONTENT_TYPE_GLYPHS;
  }
}
	break;
#line 674 "hb-buffer-deserialize-text.hh"
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	switch ( _deserialize_text_eof_actions[cs] ) {
	case 10:
#line 79 "hb-buffer-deserialize-text.rl"
	{
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 6:
#line 86 "hb-buffer-deserialize-text.rl"
	{if (!parse_hex (tok, p, &info.codepoint )) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 9:
#line 88 "hb-buffer-deserialize-text.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 17:
#line 90 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 13:
#line 91 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 15:
#line 92 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
#line 763 "hb-buffer-deserialize-text.hh"
	}
	}

	_out: {}
	}

#line 156 "hb-buffer-deserialize-text.rl"


  *end_ptr = p;

  return p == pe && *(p-1) != ']';
}

#endif /* HB_BUFFER_DESERIALIZE_TEXT_HH */
