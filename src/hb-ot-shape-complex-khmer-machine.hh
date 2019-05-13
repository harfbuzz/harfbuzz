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

#ifndef HB_OT_SHAPE_COMPLEX_KHMER_MACHINE_HH
#define HB_OT_SHAPE_COMPLEX_KHMER_MACHINE_HH

#include "hb.hh"


static const short _khmer_syllable_machine_key_offsets[] = {
	0, 5, 8, 12, 15, 18, 21, 25,
	28, 32, 35, 38, 42, 45, 48, 51,
	55, 58, 62, 65, 70, 84, 94, 103,
	109, 110, 115, 122, 130, 139, 142, 146,
	155, 161, 162, 167, 174, 182, 185, 195,
	0
};

static const unsigned char _khmer_syllable_machine_trans_keys[] = {
	20u, 21u, 26u, 5u, 6u, 21u, 5u, 6u,
	21u, 26u, 5u, 6u, 21u, 5u, 6u, 16u,
	1u, 2u, 21u, 5u, 6u, 21u, 26u, 5u,
	6u, 21u, 5u, 6u, 21u, 26u, 5u, 6u,
	21u, 5u, 6u, 21u, 5u, 6u, 21u, 26u,
	5u, 6u, 21u, 5u, 6u, 16u, 1u, 2u,
	21u, 5u, 6u, 21u, 26u, 5u, 6u, 21u,
	5u, 6u, 21u, 26u, 5u, 6u, 21u, 5u,
	6u, 20u, 21u, 26u, 5u, 6u, 14u, 16u,
	21u, 22u, 26u, 27u, 28u, 29u, 1u, 2u,
	5u, 6u, 11u, 12u, 14u, 20u, 21u, 22u,
	26u, 27u, 28u, 29u, 5u, 6u, 14u, 21u,
	22u, 26u, 27u, 28u, 29u, 5u, 6u, 14u,
	21u, 22u, 29u, 5u, 6u, 22u, 14u, 21u,
	22u, 5u, 6u, 14u, 21u, 22u, 26u, 29u,
	5u, 6u, 14u, 21u, 22u, 26u, 27u, 29u,
	5u, 6u, 14u, 21u, 22u, 26u, 27u, 28u,
	29u, 5u, 6u, 16u, 1u, 2u, 21u, 26u,
	5u, 6u, 14u, 21u, 22u, 26u, 27u, 28u,
	29u, 5u, 6u, 14u, 21u, 22u, 29u, 5u,
	6u, 22u, 14u, 21u, 22u, 5u, 6u, 14u,
	21u, 22u, 26u, 29u, 5u, 6u, 14u, 21u,
	22u, 26u, 27u, 29u, 5u, 6u, 16u, 1u,
	2u, 14u, 20u, 21u, 22u, 26u, 27u, 28u,
	29u, 5u, 6u, 14u, 21u, 22u, 26u, 27u,
	28u, 29u, 5u, 6u, 0u
};

static const char _khmer_syllable_machine_single_lengths[] = {
	3, 1, 2, 1, 1, 1, 2, 1,
	2, 1, 1, 2, 1, 1, 1, 2,
	1, 2, 1, 3, 8, 8, 7, 4,
	1, 3, 5, 6, 7, 1, 2, 7,
	4, 1, 3, 5, 6, 1, 8, 7,
	0
};

static const char _khmer_syllable_machine_range_lengths[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 3, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 1, 1, 1, 1, 1, 1,
	0
};

static const short _khmer_syllable_machine_index_offsets[] = {
	0, 5, 8, 12, 15, 18, 21, 25,
	28, 32, 35, 38, 42, 45, 48, 51,
	55, 58, 62, 65, 70, 82, 92, 101,
	107, 109, 114, 121, 129, 138, 141, 145,
	154, 160, 162, 167, 174, 182, 185, 195,
	0
};

static const char _khmer_syllable_machine_trans_cond_spaces[] = {
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

static const short _khmer_syllable_machine_trans_offsets[] = {
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
	240, 241, 242, 0
};

static const char _khmer_syllable_machine_trans_lengths[] = {
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

static const char _khmer_syllable_machine_cond_keys[] = {
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

static const char _khmer_syllable_machine_cond_targs[] = {
	28, 22, 23, 1, 20, 22, 1, 20,
	22, 23, 1, 20, 23, 3, 20, 24,
	24, 20, 25, 5, 20, 26, 23, 7,
	20, 26, 7, 20, 27, 23, 9, 20,
	27, 9, 20, 31, 10, 20, 31, 32,
	10, 20, 32, 12, 20, 33, 33, 20,
	34, 14, 20, 35, 32, 16, 20, 35,
	16, 20, 36, 32, 18, 20, 36, 18,
	20, 39, 31, 32, 10, 20, 37, 21,
	31, 33, 32, 35, 36, 34, 21, 30,
	28, 20, 29, 28, 22, 24, 23, 26,
	27, 25, 0, 20, 4, 22, 24, 23,
	26, 27, 25, 2, 20, 4, 23, 24,
	25, 3, 20, 24, 20, 4, 25, 24,
	5, 20, 4, 26, 24, 23, 25, 6,
	20, 4, 27, 24, 23, 26, 25, 8,
	20, 29, 22, 24, 23, 26, 27, 25,
	2, 20, 21, 21, 20, 31, 32, 10,
	20, 13, 31, 33, 32, 35, 36, 34,
	11, 20, 13, 32, 33, 34, 12, 20,
	33, 20, 13, 34, 33, 14, 20, 13,
	35, 33, 32, 34, 15, 20, 13, 36,
	33, 32, 35, 34, 17, 20, 38, 38,
	20, 37, 39, 31, 33, 32, 35, 36,
	34, 19, 20, 37, 31, 33, 32, 35,
	36, 34, 11, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 0
};

static const char _khmer_syllable_machine_cond_actions[] = {
	2, 2, 2, 0, 1, 2, 0, 1,
	2, 2, 0, 1, 2, 0, 1, 0,
	0, 1, 2, 0, 1, 2, 2, 0,
	1, 2, 0, 1, 2, 2, 0, 1,
	2, 0, 1, 4, 0, 3, 4, 2,
	0, 5, 2, 0, 5, 0, 0, 5,
	2, 0, 5, 2, 2, 0, 5, 2,
	0, 5, 2, 2, 0, 5, 2, 0,
	5, 4, 4, 2, 0, 5, 0, 2,
	4, 0, 2, 2, 2, 2, 2, 9,
	2, 8, 0, 2, 2, 0, 2, 2,
	2, 2, 0, 10, 0, 2, 0, 2,
	2, 2, 2, 0, 10, 0, 2, 0,
	2, 0, 10, 0, 10, 0, 2, 0,
	0, 10, 0, 2, 0, 2, 2, 0,
	10, 0, 2, 0, 2, 2, 2, 0,
	10, 0, 2, 0, 2, 2, 2, 2,
	0, 10, 2, 2, 10, 4, 2, 0,
	11, 0, 4, 0, 2, 2, 2, 2,
	0, 12, 0, 2, 0, 2, 0, 12,
	0, 12, 0, 2, 0, 0, 12, 0,
	2, 0, 2, 2, 0, 12, 0, 2,
	0, 2, 2, 2, 0, 12, 4, 4,
	12, 0, 4, 4, 0, 2, 2, 2,
	2, 0, 12, 0, 4, 0, 2, 2,
	2, 2, 0, 12, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 3, 5,
	5, 5, 5, 5, 5, 5, 5, 5,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 11, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 0
};

static const char _khmer_syllable_machine_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 6, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static const char _khmer_syllable_machine_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 7, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static const char _khmer_syllable_machine_eof_cond_spaces[] = {
	-1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, 0
};

static const char _khmer_syllable_machine_eof_cond_key_offs[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static const char _khmer_syllable_machine_eof_cond_key_lens[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static const char _khmer_syllable_machine_eof_cond_keys[] = {
	0
};

static const short _khmer_syllable_machine_eof_trans[] = {
	205, 206, 207, 208, 209, 210, 211, 212,
	213, 214, 215, 216, 217, 218, 219, 220,
	221, 222, 223, 224, 0, 225, 226, 227,
	228, 229, 230, 231, 232, 233, 234, 235,
	236, 237, 238, 239, 240, 241, 242, 243,
	0
};

static const char _khmer_syllable_machine_nfa_targs[] = {
	0, 0
};

static const char _khmer_syllable_machine_nfa_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static const char _khmer_syllable_machine_nfa_push_actions[] = {
	0, 0
};

static const char _khmer_syllable_machine_nfa_pop_trans[] = {
	0, 0
};

static const int khmer_syllable_machine_start = 20;
static const int khmer_syllable_machine_first_final = 20;
static const int khmer_syllable_machine_error = -1;

static const int khmer_syllable_machine_en_main = 20;





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
		cs = (int)khmer_syllable_machine_start;
		ts = 0;
		te = 0;
		act = 0;
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
				switch ( _khmer_syllable_machine_from_state_actions[cs] ) {
					case 7:  {
						{
							#line 1 "NONE"
							{ts = p;}}
						
						break; }
				}
				
				_keys = ( _khmer_syllable_machine_trans_keys + (_khmer_syllable_machine_key_offsets[cs]));
				_trans = (unsigned int)_khmer_syllable_machine_index_offsets[cs];
				
				_klen = (int)_khmer_syllable_machine_single_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + _klen - 1;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + ((_upper-_lower) >> 1);
						if ( (info[p].khmer_category()) < (*( _mid)) )
						_upper = _mid - 1;
						else if ( (info[p].khmer_category()) > (*( _mid)) )
						_lower = _mid + 1;
						else {
							_trans += (unsigned int)(_mid - _keys);
							goto _match;
						}
					}
					_keys += _klen;
					_trans += (unsigned int)_klen;
				}
				
				_klen = (int)_khmer_syllable_machine_range_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + (_klen<<1) - 2;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + (((_upper-_lower) >> 1) & ~1);
						if ( (info[p].khmer_category()) < (*( _mid)) )
						_upper = _mid - 2;
						else if ( (info[p].khmer_category()) > (*( _mid + 1)) )
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
				cs = (int)_khmer_syllable_machine_cond_targs[_trans];
				
				if ( _khmer_syllable_machine_cond_actions[_trans] == 0 )
				goto _again;
				
				switch ( _khmer_syllable_machine_cond_actions[_trans] ) {
					case 2:  {
						{
							#line 1 "NONE"
							{te = p+1;}}
						
						break; }
					case 8:  {
						{
							#line 76 "hb-ot-shape-complex-khmer-machine.rl"
							{te = p+1;{
									#line 76 "hb-ot-shape-complex-khmer-machine.rl"
									found_syllable (non_khmer_cluster); }}}
						
						break; }
					case 10:  {
						{
							#line 74 "hb-ot-shape-complex-khmer-machine.rl"
							{te = p;p = p - 1;{
									#line 74 "hb-ot-shape-complex-khmer-machine.rl"
									found_syllable (consonant_syllable); }}}
						
						break; }
					case 12:  {
						{
							#line 75 "hb-ot-shape-complex-khmer-machine.rl"
							{te = p;p = p - 1;{
									#line 75 "hb-ot-shape-complex-khmer-machine.rl"
									found_syllable (broken_cluster); }}}
						
						break; }
					case 11:  {
						{
							#line 76 "hb-ot-shape-complex-khmer-machine.rl"
							{te = p;p = p - 1;{
									#line 76 "hb-ot-shape-complex-khmer-machine.rl"
									found_syllable (non_khmer_cluster); }}}
						
						break; }
					case 1:  {
						{
							#line 74 "hb-ot-shape-complex-khmer-machine.rl"
							{p = ((te))-1;
								{
									#line 74 "hb-ot-shape-complex-khmer-machine.rl"
									found_syllable (consonant_syllable); }}}
						
						break; }
					case 5:  {
						{
							#line 75 "hb-ot-shape-complex-khmer-machine.rl"
							{p = ((te))-1;
								{
									#line 75 "hb-ot-shape-complex-khmer-machine.rl"
									found_syllable (broken_cluster); }}}
						
						break; }
					case 3:  {
						{
							#line 1 "NONE"
							{switch( act ) {
									case 2:  {
										p = ((te))-1;
										{
											#line 75 "hb-ot-shape-complex-khmer-machine.rl"
											found_syllable (broken_cluster); } break; }
									case 3:  {
										p = ((te))-1;
										{
											#line 76 "hb-ot-shape-complex-khmer-machine.rl"
											found_syllable (non_khmer_cluster); } break; }
								}}
						}
						
						break; }
					case 4:  {
						{
							#line 1 "NONE"
							{te = p+1;}}
						{
							#line 75 "hb-ot-shape-complex-khmer-machine.rl"
							{act = 2;}}
						
						break; }
					case 9:  {
						{
							#line 1 "NONE"
							{te = p+1;}}
						{
							#line 76 "hb-ot-shape-complex-khmer-machine.rl"
							{act = 3;}}
						
						break; }
				}
				
				
			}
			_again:  {
				switch ( _khmer_syllable_machine_to_state_actions[cs] ) {
					case 6:  {
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
					if ( _khmer_syllable_machine_eof_cond_spaces[cs] != -1 ) {
						_cekeys = ( _khmer_syllable_machine_eof_cond_keys + (_khmer_syllable_machine_eof_cond_key_offs[cs]));
						_klen = (int)_khmer_syllable_machine_eof_cond_key_lens[cs];
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
					if ( _khmer_syllable_machine_eof_trans[cs] > 0 ) {
						_trans = (unsigned int)_khmer_syllable_machine_eof_trans[cs] - 1;
						goto _match_cond;
					}
				}
				
			}
			_out:  { {}
			}
		}
	}
	
}

#endif /* HB_OT_SHAPE_COMPLEX_KHMER_MACHINE_HH */
