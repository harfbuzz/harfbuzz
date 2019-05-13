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


static const short _myanmar_syllable_machine_key_offsets[] = {
	0, 24, 41, 47, 50, 55, 62, 67,
	71, 81, 88, 97, 105, 108, 123, 134,
	144, 153, 161, 172, 185, 197, 211, 227,
	233, 236, 241, 248, 253, 257, 267, 274,
	283, 291, 294, 311, 326, 337, 347, 356,
	364, 375, 388, 400, 414, 430, 447, 463,
	485, 490, 0
};

static const unsigned char _myanmar_syllable_machine_trans_keys[] = {
	3u, 4u, 8u, 10u, 11u, 16u, 18u, 19u,
	21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u,
	29u, 30u, 31u, 32u, 1u, 2u, 5u, 6u,
	3u, 4u, 8u, 10u, 18u, 21u, 22u, 23u,
	24u, 25u, 26u, 27u, 28u, 29u, 30u, 5u,
	6u, 8u, 18u, 25u, 29u, 5u, 6u, 8u,
	5u, 6u, 8u, 25u, 29u, 5u, 6u, 3u,
	8u, 10u, 18u, 25u, 5u, 6u, 8u, 18u,
	25u, 5u, 6u, 8u, 25u, 5u, 6u, 3u,
	8u, 10u, 18u, 21u, 25u, 26u, 29u, 5u,
	6u, 3u, 8u, 10u, 25u, 29u, 5u, 6u,
	3u, 8u, 10u, 18u, 25u, 26u, 29u, 5u,
	6u, 3u, 8u, 10u, 25u, 26u, 29u, 5u,
	6u, 16u, 1u, 2u, 3u, 8u, 10u, 18u,
	21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u,
	29u, 5u, 6u, 3u, 8u, 10u, 18u, 25u,
	26u, 27u, 28u, 29u, 5u, 6u, 3u, 8u,
	10u, 25u, 26u, 27u, 28u, 29u, 5u, 6u,
	3u, 8u, 10u, 25u, 26u, 27u, 29u, 5u,
	6u, 3u, 8u, 10u, 25u, 27u, 29u, 5u,
	6u, 3u, 8u, 10u, 25u, 26u, 27u, 28u,
	29u, 30u, 5u, 6u, 3u, 8u, 10u, 18u,
	21u, 23u, 25u, 26u, 27u, 28u, 29u, 5u,
	6u, 3u, 8u, 10u, 18u, 21u, 25u, 26u,
	27u, 28u, 29u, 5u, 6u, 3u, 8u, 10u,
	18u, 21u, 22u, 23u, 25u, 26u, 27u, 28u,
	29u, 5u, 6u, 3u, 4u, 8u, 10u, 18u,
	21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u,
	29u, 5u, 6u, 8u, 18u, 25u, 29u, 5u,
	6u, 8u, 5u, 6u, 8u, 25u, 29u, 5u,
	6u, 3u, 8u, 10u, 18u, 25u, 5u, 6u,
	8u, 18u, 25u, 5u, 6u, 8u, 25u, 5u,
	6u, 3u, 8u, 10u, 18u, 21u, 25u, 26u,
	29u, 5u, 6u, 3u, 8u, 10u, 25u, 29u,
	5u, 6u, 3u, 8u, 10u, 18u, 25u, 26u,
	29u, 5u, 6u, 3u, 8u, 10u, 25u, 26u,
	29u, 5u, 6u, 16u, 1u, 2u, 3u, 4u,
	8u, 10u, 18u, 21u, 22u, 23u, 24u, 25u,
	26u, 27u, 28u, 29u, 30u, 5u, 6u, 3u,
	8u, 10u, 18u, 21u, 22u, 23u, 24u, 25u,
	26u, 27u, 28u, 29u, 5u, 6u, 3u, 8u,
	10u, 18u, 25u, 26u, 27u, 28u, 29u, 5u,
	6u, 3u, 8u, 10u, 25u, 26u, 27u, 28u,
	29u, 5u, 6u, 3u, 8u, 10u, 25u, 26u,
	27u, 29u, 5u, 6u, 3u, 8u, 10u, 25u,
	27u, 29u, 5u, 6u, 3u, 8u, 10u, 25u,
	26u, 27u, 28u, 29u, 30u, 5u, 6u, 3u,
	8u, 10u, 18u, 21u, 23u, 25u, 26u, 27u,
	28u, 29u, 5u, 6u, 3u, 8u, 10u, 18u,
	21u, 25u, 26u, 27u, 28u, 29u, 5u, 6u,
	3u, 8u, 10u, 18u, 21u, 22u, 23u, 25u,
	26u, 27u, 28u, 29u, 5u, 6u, 3u, 4u,
	8u, 10u, 18u, 21u, 22u, 23u, 24u, 25u,
	26u, 27u, 28u, 29u, 5u, 6u, 3u, 4u,
	8u, 10u, 18u, 21u, 22u, 23u, 24u, 25u,
	26u, 27u, 28u, 29u, 30u, 5u, 6u, 3u,
	4u, 8u, 10u, 18u, 21u, 22u, 23u, 24u,
	25u, 26u, 27u, 28u, 29u, 5u, 6u, 3u,
	4u, 8u, 10u, 11u, 16u, 18u, 21u, 22u,
	23u, 24u, 25u, 26u, 27u, 28u, 29u, 30u,
	32u, 1u, 2u, 5u, 6u, 11u, 16u, 32u,
	1u, 2u, 8u, 0u
};

static const char _myanmar_syllable_machine_single_lengths[] = {
	20, 15, 4, 1, 3, 5, 3, 2,
	8, 5, 7, 6, 1, 13, 9, 8,
	7, 6, 9, 11, 10, 12, 14, 4,
	1, 3, 5, 3, 2, 8, 5, 7,
	6, 1, 15, 13, 9, 8, 7, 6,
	9, 11, 10, 12, 14, 15, 14, 18,
	3, 1, 0
};

static const char _myanmar_syllable_machine_range_lengths[] = {
	2, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2,
	1, 0, 0
};

static const short _myanmar_syllable_machine_index_offsets[] = {
	0, 23, 40, 46, 49, 54, 61, 66,
	70, 80, 87, 96, 104, 107, 122, 133,
	143, 152, 160, 171, 184, 196, 210, 226,
	232, 235, 240, 247, 252, 256, 266, 273,
	282, 290, 293, 310, 325, 336, 346, 355,
	363, 374, 387, 399, 413, 429, 446, 462,
	483, 488, 0
};

static const char _myanmar_syllable_machine_trans_cond_spaces[] = {
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
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, 0
};

static const short _myanmar_syllable_machine_trans_offsets[] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71,
	72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87,
	88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135,
	136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151,
	152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167,
	168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183,
	184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199,
	200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215,
	216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231,
	232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247,
	248, 249, 250, 251, 252, 253, 254, 255,
	256, 257, 258, 259, 260, 261, 262, 263,
	264, 265, 266, 267, 268, 269, 270, 271,
	272, 273, 274, 275, 276, 277, 278, 279,
	280, 281, 282, 283, 284, 285, 286, 287,
	288, 289, 290, 291, 292, 293, 294, 295,
	296, 297, 298, 299, 300, 301, 302, 303,
	304, 305, 306, 307, 308, 309, 310, 311,
	312, 313, 314, 315, 316, 317, 318, 319,
	320, 321, 322, 323, 324, 325, 326, 327,
	328, 329, 330, 331, 332, 333, 334, 335,
	336, 337, 338, 339, 340, 341, 342, 343,
	344, 345, 346, 347, 348, 349, 350, 351,
	352, 353, 354, 355, 356, 357, 358, 359,
	360, 361, 362, 363, 364, 365, 366, 367,
	368, 369, 370, 371, 372, 373, 374, 375,
	376, 377, 378, 379, 380, 381, 382, 383,
	384, 385, 386, 387, 388, 389, 390, 391,
	392, 393, 394, 395, 396, 397, 398, 399,
	400, 401, 402, 403, 404, 405, 406, 407,
	408, 409, 410, 411, 412, 413, 414, 415,
	416, 417, 418, 419, 420, 421, 422, 423,
	424, 425, 426, 427, 428, 429, 430, 431,
	432, 433, 434, 435, 436, 437, 438, 439,
	440, 441, 442, 443, 444, 445, 446, 447,
	448, 449, 450, 451, 452, 453, 454, 455,
	456, 457, 458, 459, 460, 461, 462, 463,
	464, 465, 466, 467, 468, 469, 470, 471,
	472, 473, 474, 475, 476, 477, 478, 479,
	480, 481, 482, 483, 484, 485, 486, 487,
	488, 489, 490, 491, 492, 493, 494, 495,
	496, 497, 498, 499, 500, 501, 502, 503,
	504, 505, 506, 507, 508, 509, 510, 511,
	512, 513, 514, 515, 516, 517, 518, 519,
	520, 521, 522, 523, 524, 525, 526, 527,
	528, 529, 530, 531, 532, 533, 534, 535,
	536, 537, 538, 0
};

static const char _myanmar_syllable_machine_trans_lengths[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 0
};

static const char _myanmar_syllable_machine_cond_keys[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _myanmar_syllable_machine_cond_targs[] = {
	23, 33, 24, 30, 1, 45, 35, 48,
	36, 41, 42, 43, 26, 38, 39, 40,
	29, 44, 49, 1, 1, 0, 0, 2,
	12, 3, 9, 13, 14, 19, 20, 21,
	5, 16, 17, 18, 8, 22, 0, 0,
	3, 4, 5, 8, 0, 0, 3, 0,
	0, 3, 5, 8, 0, 0, 6, 3,
	5, 7, 5, 0, 0, 3, 7, 5,
	0, 0, 3, 5, 0, 0, 2, 3,
	9, 10, 10, 5, 11, 8, 0, 0,
	2, 3, 9, 5, 8, 0, 0, 2,
	3, 9, 10, 5, 11, 8, 0, 0,
	2, 3, 9, 5, 11, 8, 0, 0,
	1, 1, 0, 2, 3, 9, 13, 14,
	19, 20, 21, 5, 16, 17, 18, 8,
	0, 0, 2, 3, 9, 15, 5, 16,
	17, 18, 8, 0, 0, 2, 3, 9,
	5, 16, 17, 18, 8, 0, 0, 2,
	3, 9, 5, 16, 17, 8, 0, 0,
	2, 3, 9, 5, 17, 8, 0, 0,
	2, 3, 9, 5, 16, 17, 18, 8,
	15, 0, 0, 2, 3, 9, 15, 14,
	20, 5, 16, 17, 18, 8, 0, 0,
	2, 3, 9, 15, 14, 5, 16, 17,
	18, 8, 0, 0, 2, 3, 9, 15,
	14, 19, 20, 5, 16, 17, 18, 8,
	0, 0, 2, 12, 3, 9, 13, 14,
	19, 20, 21, 5, 16, 17, 18, 8,
	0, 0, 24, 25, 26, 29, 0, 0,
	24, 0, 0, 24, 26, 29, 0, 0,
	27, 24, 26, 28, 26, 0, 0, 24,
	28, 26, 0, 0, 24, 26, 0, 0,
	23, 24, 30, 31, 31, 26, 32, 29,
	0, 0, 23, 24, 30, 26, 29, 0,
	0, 23, 24, 30, 31, 26, 32, 29,
	0, 0, 23, 24, 30, 26, 32, 29,
	0, 0, 34, 34, 0, 23, 33, 24,
	30, 35, 36, 41, 42, 43, 26, 38,
	39, 40, 29, 44, 0, 0, 23, 24,
	30, 35, 36, 41, 42, 43, 26, 38,
	39, 40, 29, 0, 0, 23, 24, 30,
	37, 26, 38, 39, 40, 29, 0, 0,
	23, 24, 30, 26, 38, 39, 40, 29,
	0, 0, 23, 24, 30, 26, 38, 39,
	29, 0, 0, 23, 24, 30, 26, 39,
	29, 0, 0, 23, 24, 30, 26, 38,
	39, 40, 29, 37, 0, 0, 23, 24,
	30, 37, 36, 42, 26, 38, 39, 40,
	29, 0, 0, 23, 24, 30, 37, 36,
	26, 38, 39, 40, 29, 0, 0, 23,
	24, 30, 37, 36, 41, 42, 26, 38,
	39, 40, 29, 0, 0, 23, 33, 24,
	30, 35, 36, 41, 42, 43, 26, 38,
	39, 40, 29, 0, 0, 2, 12, 3,
	9, 46, 14, 19, 20, 21, 5, 16,
	17, 18, 8, 22, 0, 0, 2, 47,
	3, 9, 13, 14, 19, 20, 21, 5,
	16, 17, 18, 8, 0, 0, 23, 33,
	24, 30, 1, 1, 35, 36, 41, 42,
	43, 26, 38, 39, 40, 29, 44, 1,
	1, 0, 0, 1, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _myanmar_syllable_machine_cond_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 4, 3, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 6, 5,
	0, 0, 0, 0, 6, 5, 0, 6,
	5, 0, 0, 0, 6, 5, 0, 0,
	0, 0, 0, 6, 5, 0, 0, 0,
	6, 5, 0, 0, 6, 5, 0, 0,
	0, 0, 0, 0, 0, 0, 6, 5,
	0, 0, 0, 0, 0, 6, 5, 0,
	0, 0, 0, 0, 0, 0, 6, 5,
	0, 0, 0, 0, 0, 0, 6, 5,
	0, 0, 5, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	6, 5, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 5, 0, 0, 0,
	0, 0, 0, 0, 0, 6, 5, 0,
	0, 0, 0, 0, 0, 0, 6, 5,
	0, 0, 0, 0, 0, 0, 6, 5,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 6, 5, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 6, 5,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 6, 5, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	6, 5, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	6, 5, 0, 0, 0, 0, 8, 7,
	0, 8, 7, 0, 0, 0, 8, 7,
	0, 0, 0, 0, 0, 8, 7, 0,
	0, 0, 8, 7, 0, 0, 8, 7,
	0, 0, 0, 0, 0, 0, 0, 0,
	8, 7, 0, 0, 0, 0, 0, 8,
	7, 0, 0, 0, 0, 0, 0, 0,
	8, 7, 0, 0, 0, 0, 0, 0,
	8, 7, 0, 0, 7, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 8, 7, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 8, 7, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 8, 7,
	0, 0, 0, 0, 0, 0, 0, 0,
	8, 7, 0, 0, 0, 0, 0, 0,
	0, 8, 7, 0, 0, 0, 0, 0,
	0, 8, 7, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 8, 7, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 8, 7, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 8, 7, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 8, 7, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 8, 7, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 6, 5, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 6, 5, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 8, 7, 0, 0, 0, 0, 9,
	10, 9, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 5, 5,
	7, 9, 9, 0
};

static const char _myanmar_syllable_machine_to_state_actions[] = {
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _myanmar_syllable_machine_from_state_actions[] = {
	2, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _myanmar_syllable_machine_eof_cond_spaces[] = {
	-1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, 0
};

static const char _myanmar_syllable_machine_eof_cond_key_offs[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _myanmar_syllable_machine_eof_cond_key_lens[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _myanmar_syllable_machine_eof_cond_keys[] = {
	0
};

static const short _myanmar_syllable_machine_eof_trans[] = {
	0, 491, 492, 493, 494, 495, 496, 497,
	498, 499, 500, 501, 502, 503, 504, 505,
	506, 507, 508, 509, 510, 511, 512, 513,
	514, 515, 516, 517, 518, 519, 520, 521,
	522, 523, 524, 525, 526, 527, 528, 529,
	530, 531, 532, 533, 534, 535, 536, 537,
	538, 539, 0
};

static const char _myanmar_syllable_machine_nfa_targs[] = {
	0, 0
};

static const char _myanmar_syllable_machine_nfa_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0
};

static const char _myanmar_syllable_machine_nfa_push_actions[] = {
	0, 0
};

static const char _myanmar_syllable_machine_nfa_pop_trans[] = {
	0, 0
};

static const int myanmar_syllable_machine_start = 0;
static const int myanmar_syllable_machine_first_final = 0;
static const int myanmar_syllable_machine_error = -1;

static const int myanmar_syllable_machine_en_main = 0;





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
	unsigned int p, pe, eof, ts, te, act HB_UNUSED;
	int cs;
	hb_glyph_info_t *info = buffer->info;
	
	{
		cs = (int)myanmar_syllable_machine_start;
		ts = 0;
		te = 0;
	}
	
	
	p=0;
	pe = eof = buffer->len;
	
	unsigned int syllable_serial=1;
	
	{
		int _cpc;
		int _klen;const char * _cekeys;unsigned int _trans = 0;const unsigned char * _keys;	 {
			if ( p == pe )
			goto _test_eof;
			_resume:  {
				switch ( _myanmar_syllable_machine_from_state_actions[cs] ) {
					case 2:  {
						{
							#line 1 "NONE"
							{ts = p;}}
						
						break; }
				}
				
				_keys = ( _myanmar_syllable_machine_trans_keys + (_myanmar_syllable_machine_key_offsets[cs]));
				_trans = (unsigned int)_myanmar_syllable_machine_index_offsets[cs];
				
				_klen = (int)_myanmar_syllable_machine_single_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + _klen - 1;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + ((_upper-_lower) >> 1);
						if ( (info[p].myanmar_category()) < (*( _mid)) )
						_upper = _mid - 1;
						else if ( (info[p].myanmar_category()) > (*( _mid)) )
						_lower = _mid + 1;
						else {
							_trans += (unsigned int)(_mid - _keys);
							goto _match;
						}
					}
					_keys += _klen;
					_trans += (unsigned int)_klen;
				}
				
				_klen = (int)_myanmar_syllable_machine_range_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + (_klen<<1) - 2;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + (((_upper-_lower) >> 1) & ~1);
						if ( (info[p].myanmar_category()) < (*( _mid)) )
						_upper = _mid - 2;
						else if ( (info[p].myanmar_category()) > (*( _mid + 1)) )
						_lower = _mid + 2;
						else {
							_trans += (unsigned int)((_mid - _keys)>>1);
							goto _match;
						}
					}
					_trans += (unsigned int)_klen;
				}
				
				_match:  {
					goto _match_cond;
				}
			}
			_match_cond:  {
				cs = (int)_myanmar_syllable_machine_cond_targs[_trans];
				
				if ( _myanmar_syllable_machine_cond_actions[_trans] == 0 )
				goto _again;
				
				switch ( _myanmar_syllable_machine_cond_actions[_trans] ) {
					case 6:  {
						{
							#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p+1;{
									#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (consonant_syllable); }}}
						
						break; }
					case 4:  {
						{
							#line 87 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p+1;{
									#line 87 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (non_myanmar_cluster); }}}
						
						break; }
					case 10:  {
						{
							#line 88 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p+1;{
									#line 88 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (punctuation_cluster); }}}
						
						break; }
					case 8:  {
						{
							#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p+1;{
									#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (broken_cluster); }}}
						
						break; }
					case 3:  {
						{
							#line 90 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p+1;{
									#line 90 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (non_myanmar_cluster); }}}
						
						break; }
					case 5:  {
						{
							#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p;p = p - 1;{
									#line 86 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (consonant_syllable); }}}
						
						break; }
					case 7:  {
						{
							#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p;p = p - 1;{
									#line 89 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (broken_cluster); }}}
						
						break; }
					case 9:  {
						{
							#line 90 "hb-ot-shape-complex-myanmar-machine.rl"
							{te = p;p = p - 1;{
									#line 90 "hb-ot-shape-complex-myanmar-machine.rl"
									found_syllable (non_myanmar_cluster); }}}
						
						break; }
				}
				
				
			}
			_again:  {
				switch ( _myanmar_syllable_machine_to_state_actions[cs] ) {
					case 1:  {
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
					if ( _myanmar_syllable_machine_eof_cond_spaces[cs] != -1 ) {
						_cekeys = ( _myanmar_syllable_machine_eof_cond_keys + (_myanmar_syllable_machine_eof_cond_key_offs[cs]));
						_klen = (int)_myanmar_syllable_machine_eof_cond_key_lens[cs];
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
					if ( _myanmar_syllable_machine_eof_trans[cs] > 0 ) {
						_trans = (unsigned int)_myanmar_syllable_machine_eof_trans[cs] - 1;
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

#endif /* HB_OT_SHAPE_COMPLEX_MYANMAR_MACHINE_HH */
