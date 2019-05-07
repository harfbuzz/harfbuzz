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


static const unsigned char _myanmar_syllable_machine_trans_keys[] = {
	0u, 21u, 1u, 20u, 3u, 19u, 3u, 5u,
	3u, 19u, 1u, 15u, 3u, 15u, 3u, 15u,
	1u, 19u, 1u, 19u, 1u, 19u, 1u, 19u,
	0u, 8u, 1u, 19u, 1u, 19u, 1u, 19u,
	1u, 19u, 1u, 19u, 1u, 20u, 1u, 19u,
	1u, 19u, 1u, 19u, 1u, 19u, 3u, 19u,
	3u, 5u, 3u, 19u, 1u, 15u, 3u, 15u,
	3u, 15u, 1u, 19u, 1u, 19u, 1u, 19u,
	1u, 19u, 0u, 8u, 1u, 20u, 1u, 19u,
	1u, 19u, 1u, 19u, 1u, 19u, 1u, 19u,
	1u, 20u, 1u, 19u, 1u, 19u, 1u, 19u,
	1u, 19u, 1u, 20u, 1u, 19u, 0u, 20u,
	0u, 8u, 5u, 5u, 0u
};

static const char _myanmar_syllable_machine_char_class[] = {
	0, 0, 1, 2, 3, 3, 4, 5,
	4, 6, 7, 4, 4, 4, 4, 8,
	4, 9, 10, 4, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 7,
	0
};

static const short _myanmar_syllable_machine_index_offsets[] = {
	0, 22, 42, 59, 62, 79, 94, 107,
	120, 139, 158, 177, 196, 205, 224, 243,
	262, 281, 300, 320, 339, 358, 377, 396,
	413, 416, 433, 448, 461, 474, 493, 512,
	531, 550, 559, 579, 598, 617, 636, 655,
	674, 694, 713, 732, 751, 770, 790, 809,
	830, 839, 0
};

static const char _myanmar_syllable_machine_indicies[] = {
	1, 2, 3, 4, 0, 5, 6, 1,
	7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 22, 23,
	24, 21, 25, 26, 21, 21, 27, 21,
	28, 29, 30, 31, 32, 33, 34, 35,
	36, 37, 24, 21, 25, 21, 21, 21,
	38, 21, 21, 21, 21, 21, 32, 21,
	21, 21, 36, 24, 21, 25, 24, 21,
	25, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 32, 21, 21, 21, 36, 39,
	21, 24, 21, 25, 32, 21, 21, 40,
	21, 21, 21, 21, 21, 32, 24, 21,
	25, 21, 21, 21, 40, 21, 21, 21,
	21, 21, 32, 24, 21, 25, 21, 21,
	21, 21, 21, 21, 21, 21, 21, 32,
	22, 21, 24, 21, 25, 26, 21, 21,
	41, 21, 41, 21, 21, 21, 32, 42,
	21, 21, 36, 22, 21, 24, 21, 25,
	26, 21, 21, 21, 21, 21, 21, 21,
	21, 32, 21, 21, 21, 36, 22, 21,
	24, 21, 25, 26, 21, 21, 41, 21,
	21, 21, 21, 21, 32, 42, 21, 21,
	36, 22, 21, 24, 21, 25, 26, 21,
	21, 21, 21, 21, 21, 21, 21, 32,
	42, 21, 21, 36, 1, 21, 21, 21,
	21, 21, 21, 21, 1, 22, 21, 24,
	21, 25, 26, 21, 21, 27, 21, 28,
	29, 30, 31, 32, 33, 34, 35, 36,
	22, 21, 24, 21, 25, 26, 21, 21,
	43, 21, 21, 21, 21, 21, 32, 33,
	34, 35, 36, 22, 21, 24, 21, 25,
	26, 21, 21, 21, 21, 21, 21, 21,
	21, 32, 33, 34, 35, 36, 22, 21,
	24, 21, 25, 26, 21, 21, 21, 21,
	21, 21, 21, 21, 32, 33, 34, 21,
	36, 22, 21, 24, 21, 25, 26, 21,
	21, 21, 21, 21, 21, 21, 21, 32,
	21, 34, 21, 36, 22, 21, 24, 21,
	25, 26, 21, 21, 21, 21, 21, 21,
	21, 21, 32, 33, 34, 35, 36, 43,
	22, 21, 24, 21, 25, 26, 21, 21,
	43, 21, 28, 21, 30, 21, 32, 33,
	34, 35, 36, 22, 21, 24, 21, 25,
	26, 21, 21, 43, 21, 28, 21, 21,
	21, 32, 33, 34, 35, 36, 22, 21,
	24, 21, 25, 26, 21, 21, 43, 21,
	28, 29, 30, 21, 32, 33, 34, 35,
	36, 22, 23, 24, 21, 25, 26, 21,
	21, 27, 21, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 45, 44, 5, 44,
	44, 44, 46, 44, 44, 44, 44, 44,
	14, 44, 44, 44, 18, 45, 44, 5,
	45, 44, 5, 44, 44, 44, 44, 44,
	44, 44, 44, 44, 14, 44, 44, 44,
	18, 47, 44, 45, 44, 5, 14, 44,
	44, 48, 44, 44, 44, 44, 44, 14,
	45, 44, 5, 44, 44, 44, 48, 44,
	44, 44, 44, 44, 14, 45, 44, 5,
	44, 44, 44, 44, 44, 44, 44, 44,
	44, 14, 2, 44, 45, 44, 5, 6,
	44, 44, 49, 44, 49, 44, 44, 44,
	14, 50, 44, 44, 18, 2, 44, 45,
	44, 5, 6, 44, 44, 44, 44, 44,
	44, 44, 44, 14, 44, 44, 44, 18,
	2, 44, 45, 44, 5, 6, 44, 44,
	49, 44, 44, 44, 44, 44, 14, 50,
	44, 44, 18, 2, 44, 45, 44, 5,
	6, 44, 44, 44, 44, 44, 44, 44,
	44, 14, 50, 44, 44, 18, 51, 44,
	44, 44, 44, 44, 44, 44, 51, 2,
	3, 45, 44, 5, 6, 44, 44, 8,
	44, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 2, 44, 45, 44, 5,
	6, 44, 44, 8, 44, 10, 11, 12,
	13, 14, 15, 16, 17, 18, 2, 44,
	45, 44, 5, 6, 44, 44, 52, 44,
	44, 44, 44, 44, 14, 15, 16, 17,
	18, 2, 44, 45, 44, 5, 6, 44,
	44, 44, 44, 44, 44, 44, 44, 14,
	15, 16, 17, 18, 2, 44, 45, 44,
	5, 6, 44, 44, 44, 44, 44, 44,
	44, 44, 14, 15, 16, 44, 18, 2,
	44, 45, 44, 5, 6, 44, 44, 44,
	44, 44, 44, 44, 44, 14, 44, 16,
	44, 18, 2, 44, 45, 44, 5, 6,
	44, 44, 44, 44, 44, 44, 44, 44,
	14, 15, 16, 17, 18, 52, 2, 44,
	45, 44, 5, 6, 44, 44, 52, 44,
	10, 44, 12, 44, 14, 15, 16, 17,
	18, 2, 44, 45, 44, 5, 6, 44,
	44, 52, 44, 10, 44, 44, 44, 14,
	15, 16, 17, 18, 2, 44, 45, 44,
	5, 6, 44, 44, 52, 44, 10, 11,
	12, 44, 14, 15, 16, 17, 18, 2,
	3, 45, 44, 5, 6, 44, 44, 8,
	44, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 22, 23, 24, 21, 25, 26,
	21, 21, 53, 21, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 22, 54,
	24, 21, 25, 26, 21, 21, 27, 21,
	28, 29, 30, 31, 32, 33, 34, 35,
	36, 1, 2, 3, 45, 44, 5, 6,
	1, 1, 8, 44, 10, 11, 12, 13,
	14, 15, 16, 17, 18, 19, 1, 55,
	55, 55, 55, 55, 55, 1, 1, 56,
	0
};

static const char _myanmar_syllable_machine_index_defaults[] = {
	0, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 21, 21, 21, 21, 21, 44,
	44, 44, 44, 44, 44, 44, 44, 44,
	44, 44, 44, 44, 44, 44, 44, 44,
	44, 44, 44, 44, 44, 21, 21, 44,
	55, 55, 0
};

static const char _myanmar_syllable_machine_trans_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, 0
};

static const char _myanmar_syllable_machine_cond_targs[] = {
	0, 1, 23, 33, 0, 24, 30, 45,
	35, 48, 36, 41, 42, 43, 26, 38,
	39, 40, 29, 44, 49, 0, 2, 12,
	0, 3, 9, 13, 14, 19, 20, 21,
	5, 16, 17, 18, 8, 22, 4, 6,
	7, 10, 11, 15, 0, 0, 25, 27,
	28, 31, 32, 34, 37, 46, 47, 0,
	0, 0
};

static const char _myanmar_syllable_machine_cond_actions[] = {
	3, 0, 0, 0, 4, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 5, 0, 0,
	6, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 7, 8, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 9,
	10, 0
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
	-1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, 0
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

static const char _myanmar_syllable_machine_eof_trans[] = {
	0, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 45,
	45, 45, 45, 45, 45, 45, 45, 45,
	45, 45, 45, 45, 45, 45, 45, 45,
	45, 45, 45, 45, 45, 22, 22, 45,
	56, 56, 0
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
		int _klen;const char * _cekeys;unsigned int _trans = 0;const unsigned char * _keys;const char * _inds;	 {
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
				
				_keys = ( _myanmar_syllable_machine_trans_keys + ((cs<<1)));
				_inds = ( _myanmar_syllable_machine_indicies + (_myanmar_syllable_machine_index_offsets[cs]));
				
				if ( (info[p].myanmar_category()) <= 32 && (info[p].myanmar_category()) >= 1 )
				{
					int _ic = (int)_myanmar_syllable_machine_char_class[(int)(info[p].myanmar_category()) - 1];
					if ( _ic <= (int)(*( _keys+1)) && _ic >= (int)(*( _keys)) )
					_trans = (unsigned int)(*( _inds + (int)( _ic - (int)(*( _keys)) ) )); 
					else
					_trans = (unsigned int)_myanmar_syllable_machine_index_defaults[cs];
				}
				else {
					_trans = (unsigned int)_myanmar_syllable_machine_index_defaults[cs];
				}
				
				goto _match_cond;
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
