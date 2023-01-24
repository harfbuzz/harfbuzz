
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


#line 33 "hb-buffer-deserialize-text.hh"
static const unsigned char _deserialize_text_trans_keys[] = {
	0u, 0u, 9u, 91u, 85u, 85u, 43u, 43u, 48u, 102u, 9u, 124u, 9u, 124u, 9u, 85u, 
	48u, 57u, 9u, 124u, 9u, 124u, 9u, 124u, 9u, 124u, 48u, 57u, 9u, 124u, 9u, 124u, 
	9u, 124u, 45u, 57u, 48u, 57u, 9u, 124u, 45u, 57u, 48u, 57u, 9u, 124u, 9u, 124u, 
	9u, 124u, 48u, 57u, 9u, 124u, 45u, 57u, 48u, 57u, 44u, 44u, 45u, 57u, 48u, 57u, 
	9u, 124u, 9u, 124u, 44u, 57u, 9u, 124u, 43u, 124u, 9u, 124u, 0u, 0u, 9u, 85u, 
	9u, 124u, 0
};

static const char _deserialize_text_key_spans[] = {
	0, 83, 1, 1, 55, 116, 116, 77, 
	10, 116, 116, 116, 116, 10, 116, 116, 
	116, 13, 10, 116, 13, 10, 116, 116, 
	116, 10, 116, 13, 10, 1, 13, 10, 
	116, 116, 14, 116, 82, 116, 0, 77, 
	116
};

static const short _deserialize_text_index_offsets[] = {
	0, 0, 84, 86, 88, 144, 261, 378, 
	456, 467, 584, 701, 818, 935, 946, 1063, 
	1180, 1297, 1311, 1322, 1439, 1453, 1464, 1581, 
	1698, 1815, 1826, 1943, 1957, 1968, 1970, 1984, 
	1995, 2112, 2229, 2244, 2361, 2444, 2561, 2562, 
	2640
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
	8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 1, 1, 1, 9, 10, 1, 
	1, 8, 8, 8, 8, 8, 8, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 8, 8, 8, 8, 8, 8, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 11, 1, 12, 12, 
	12, 12, 12, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 12, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 13, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 14, 1, 15, 15, 15, 15, 15, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 15, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 4, 
	1, 16, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 1, 18, 18, 18, 18, 
	18, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 18, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 19, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 20, 
	1, 18, 18, 18, 18, 18, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	18, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 21, 1, 1, 1, 1, 19, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 20, 1, 23, 23, 
	23, 23, 23, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 23, 22, 22, 
	24, 22, 22, 22, 22, 22, 22, 22, 
	25, 1, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 26, 22, 22, 27, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 28, 29, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 30, 22, 32, 32, 32, 32, 32, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 32, 31, 31, 33, 31, 31, 
	31, 31, 31, 31, 31, 34, 1, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 35, 
	31, 31, 36, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 37, 38, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 31, 31, 31, 31, 39, 31, 
	40, 41, 41, 41, 41, 41, 41, 41, 
	41, 41, 1, 42, 42, 42, 42, 42, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 42, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 43, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 44, 1, 
	45, 45, 45, 45, 45, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 45, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 13, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 46, 1, 47, 47, 47, 
	47, 47, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 47, 22, 22, 24, 
	22, 22, 22, 22, 22, 22, 22, 25, 
	1, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 26, 22, 22, 27, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	28, 29, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	30, 22, 48, 1, 1, 49, 50, 50, 
	50, 50, 50, 50, 50, 50, 50, 1, 
	51, 52, 52, 52, 52, 52, 52, 52, 
	52, 52, 1, 53, 53, 53, 53, 53, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 53, 1, 1, 54, 1, 1, 
	1, 1, 1, 1, 1, 1, 55, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 56, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 57, 1, 
	58, 1, 1, 59, 60, 60, 60, 60, 
	60, 60, 60, 60, 60, 1, 61, 62, 
	62, 62, 62, 62, 62, 62, 62, 62, 
	1, 63, 63, 63, 63, 63, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	63, 1, 1, 64, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 65, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 66, 1, 63, 63, 
	63, 63, 63, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 63, 1, 1, 
	64, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 62, 62, 62, 
	62, 62, 62, 62, 62, 62, 62, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 65, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 66, 1, 53, 53, 53, 53, 53, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 53, 1, 1, 54, 1, 1, 
	1, 1, 1, 1, 1, 1, 55, 1, 
	1, 1, 52, 52, 52, 52, 52, 52, 
	52, 52, 52, 52, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 56, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 57, 1, 
	67, 68, 68, 68, 68, 68, 68, 68, 
	68, 68, 1, 69, 69, 69, 69, 69, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 69, 1, 1, 70, 1, 1, 
	1, 1, 1, 1, 1, 71, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 72, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 19, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 73, 1, 
	74, 1, 1, 75, 76, 76, 76, 76, 
	76, 76, 76, 76, 76, 1, 77, 78, 
	78, 78, 78, 78, 78, 78, 78, 78, 
	1, 79, 1, 80, 1, 1, 81, 82, 
	82, 82, 82, 82, 82, 82, 82, 82, 
	1, 83, 84, 84, 84, 84, 84, 84, 
	84, 84, 84, 1, 85, 85, 85, 85, 
	85, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 85, 1, 1, 86, 1, 
	1, 1, 1, 1, 1, 1, 87, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	88, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 89, 
	1, 85, 85, 85, 85, 85, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	85, 1, 1, 86, 1, 1, 1, 1, 
	1, 1, 1, 87, 1, 1, 1, 1, 
	84, 84, 84, 84, 84, 84, 84, 84, 
	84, 84, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 88, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 89, 1, 79, 1, 
	1, 1, 78, 78, 78, 78, 78, 78, 
	78, 78, 78, 78, 1, 69, 69, 69, 
	69, 69, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 69, 1, 1, 70, 
	1, 1, 1, 1, 1, 1, 1, 71, 
	1, 1, 1, 1, 90, 90, 90, 90, 
	90, 90, 90, 90, 90, 90, 1, 1, 
	1, 1, 1, 1, 72, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 19, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	73, 1, 31, 31, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 31, 1, 1, 31, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 31, 31, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 31, 1, 42, 42, 42, 
	42, 42, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 42, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 91, 91, 91, 91, 
	91, 91, 91, 91, 91, 91, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 43, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	44, 1, 1, 15, 15, 15, 15, 15, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 15, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 4, 
	1, 47, 47, 47, 47, 47, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	47, 22, 22, 24, 22, 22, 22, 22, 
	22, 22, 22, 25, 1, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 26, 22, 22, 
	27, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 28, 29, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 22, 22, 22, 22, 
	22, 22, 22, 22, 30, 22, 0
};

static const char _deserialize_text_trans_targs[] = {
	1, 0, 2, 11, 3, 4, 5, 6, 
	5, 8, 38, 39, 6, 38, 39, 7, 
	9, 10, 6, 38, 39, 10, 12, 12, 
	13, 17, 25, 27, 36, 38, 40, 12, 
	12, 13, 17, 25, 27, 36, 38, 40, 
	14, 37, 15, 38, 40, 15, 40, 16, 
	18, 19, 24, 19, 24, 15, 13, 20, 
	38, 40, 21, 22, 23, 22, 23, 15, 
	13, 38, 40, 26, 35, 15, 13, 17, 
	27, 40, 28, 29, 34, 29, 34, 30, 
	31, 32, 33, 32, 33, 15, 13, 17, 
	38, 40, 35, 37
};

static const char _deserialize_text_trans_actions[] = {
	0, 0, 0, 0, 1, 0, 2, 3, 
	4, 5, 3, 3, 0, 0, 0, 0, 
	2, 2, 6, 6, 6, 4, 7, 8, 
	9, 9, 9, 9, 10, 11, 11, 12, 
	13, 14, 14, 14, 14, 0, 15, 15, 
	16, 16, 17, 17, 17, 0, 0, 8, 
	18, 16, 16, 12, 12, 19, 20, 20, 
	19, 19, 18, 16, 16, 12, 12, 21, 
	22, 21, 21, 16, 16, 6, 23, 23, 
	23, 6, 18, 18, 18, 0, 0, 24, 
	18, 16, 16, 12, 12, 25, 26, 26, 
	25, 25, 12, 12
};

static const int deserialize_text_start = 1;
static const int deserialize_text_first_final = 38;
static const int deserialize_text_error = 0;

static const int deserialize_text_en_main = 1;


#line 117 "hb-buffer-deserialize-text.rl"


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

  const char *tok = nullptr;
  int cs;
  hb_glyph_info_t info = {0};
  hb_glyph_position_t pos = {0};
  
#line 465 "hb-buffer-deserialize-text.hh"
	{
	cs = deserialize_text_start;
	}

#line 468 "hb-buffer-deserialize-text.hh"
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
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
	break;
	case 18:
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
	break;
	case 12:
#line 55 "hb-buffer-deserialize-text.rl"
	{ if (unlikely (!buffer->ensure_glyphs ())) return false; }
	break;
	case 4:
#line 56 "hb-buffer-deserialize-text.rl"
	{ if (unlikely (!buffer->ensure_unicode ())) return false; }
	break;
	case 14:
#line 58 "hb-buffer-deserialize-text.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 5:
#line 66 "hb-buffer-deserialize-text.rl"
	{if (!parse_hex (tok, p, &info.codepoint )) return false; }
	break;
	case 23:
#line 68 "hb-buffer-deserialize-text.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
	break;
	case 24:
#line 69 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_offset )) return false; }
	break;
	case 26:
#line 70 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
	break;
	case 20:
#line 71 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
	break;
	case 22:
#line 72 "hb-buffer-deserialize-text.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
	break;
	case 10:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
	break;
	case 16:
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 55 "hb-buffer-deserialize-text.rl"
	{ if (unlikely (!buffer->ensure_glyphs ())) return false; }
	break;
	case 2:
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 56 "hb-buffer-deserialize-text.rl"
	{ if (unlikely (!buffer->ensure_unicode ())) return false; }
	break;
	case 15:
#line 58 "hb-buffer-deserialize-text.rl"
	{
	/* TODO Unescape delimiters. */
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
	case 3:
#line 66 "hb-buffer-deserialize-text.rl"
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
	case 6:
#line 68 "hb-buffer-deserialize-text.rl"
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
	case 25:
#line 70 "hb-buffer-deserialize-text.rl"
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
	case 19:
#line 71 "hb-buffer-deserialize-text.rl"
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
	case 21:
#line 72 "hb-buffer-deserialize-text.rl"
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
	case 17:
#line 73 "hb-buffer-deserialize-text.rl"
	{ if (!parse_uint (tok, p, &info.mask    )) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 7:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 55 "hb-buffer-deserialize-text.rl"
	{ if (unlikely (!buffer->ensure_glyphs ())) return false; }
	break;
	case 9:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 58 "hb-buffer-deserialize-text.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 13:
#line 58 "hb-buffer-deserialize-text.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
#line 55 "hb-buffer-deserialize-text.rl"
	{ if (unlikely (!buffer->ensure_glyphs ())) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 11:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 58 "hb-buffer-deserialize-text.rl"
	{
	/* TODO Unescape delimiters. */
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
	case 8:
#line 38 "hb-buffer-deserialize-text.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text.rl"
	{
	tok = p;
}
#line 58 "hb-buffer-deserialize-text.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
#line 55 "hb-buffer-deserialize-text.rl"
	{ if (unlikely (!buffer->ensure_glyphs ())) return false; }
#line 43 "hb-buffer-deserialize-text.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
#line 723 "hb-buffer-deserialize-text.hh"
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 141 "hb-buffer-deserialize-text.rl"


  *end_ptr = p;

  return p == pe && *(p-1) != ']';
}

#endif /* HB_BUFFER_DESERIALIZE_TEXT_HH */
