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


static const unsigned char _khmer_syllable_machine_trans_keys[] = {
	2u, 8u, 2u, 6u, 2u, 8u, 2u, 6u,
	0u, 0u, 2u, 6u, 2u, 8u, 2u, 6u,
	2u, 8u, 2u, 6u, 2u, 6u, 2u, 8u,
	2u, 6u, 0u, 0u, 2u, 6u, 2u, 8u,
	2u, 6u, 2u, 8u, 2u, 6u, 2u, 8u,
	0u, 11u, 2u, 11u, 2u, 11u, 2u, 11u,
	7u, 7u, 2u, 7u, 2u, 11u, 2u, 11u,
	2u, 11u, 0u, 0u, 2u, 8u, 2u, 11u,
	2u, 11u, 7u, 7u, 2u, 7u, 2u, 11u,
	2u, 11u, 0u, 0u, 2u, 11u, 2u, 11u,
	0u
};

static const char _khmer_syllable_machine_char_class[] = {
	0, 0, 1, 1, 2, 2, 1, 1,
	1, 1, 3, 3, 1, 4, 1, 0,
	1, 1, 1, 5, 6, 7, 1, 1,
	1, 8, 9, 10, 11, 0
};

static const short _khmer_syllable_machine_index_offsets[] = {
	0, 7, 12, 19, 24, 25, 30, 37,
	42, 49, 54, 59, 66, 71, 72, 77,
	84, 89, 96, 101, 108, 120, 130, 140,
	150, 151, 157, 167, 177, 187, 188, 195,
	205, 215, 216, 222, 232, 242, 243, 253,
	0
};

static const char _khmer_syllable_machine_indicies[] = {
	1, 0, 0, 2, 3, 0, 4, 1,
	0, 0, 0, 3, 1, 0, 0, 0,
	3, 0, 4, 5, 0, 0, 0, 4,
	6, 7, 0, 0, 0, 8, 9, 0,
	0, 0, 10, 0, 4, 9, 0, 0,
	0, 10, 11, 0, 0, 0, 12, 0,
	4, 11, 0, 0, 0, 12, 14, 13,
	13, 13, 15, 14, 16, 16, 16, 15,
	16, 17, 18, 16, 16, 16, 17, 19,
	20, 16, 16, 16, 21, 22, 16, 16,
	16, 23, 16, 17, 22, 16, 16, 16,
	23, 24, 16, 16, 16, 25, 16, 17,
	24, 16, 16, 16, 25, 14, 16, 16,
	26, 15, 16, 17, 28, 27, 29, 2,
	30, 27, 15, 19, 17, 23, 25, 21,
	32, 31, 33, 2, 3, 6, 4, 10,
	12, 8, 34, 31, 35, 31, 3, 6,
	4, 10, 12, 8, 5, 31, 35, 31,
	4, 6, 31, 31, 31, 8, 6, 7,
	31, 35, 31, 8, 6, 36, 31, 35,
	31, 10, 6, 4, 31, 31, 8, 37,
	31, 35, 31, 12, 6, 4, 10, 31,
	8, 34, 31, 33, 31, 3, 6, 4,
	10, 12, 8, 28, 14, 38, 38, 38,
	15, 38, 17, 40, 39, 41, 39, 15,
	19, 17, 23, 25, 21, 18, 39, 41,
	39, 17, 19, 39, 39, 39, 21, 19,
	20, 39, 41, 39, 21, 19, 42, 39,
	41, 39, 23, 19, 17, 39, 39, 21,
	43, 39, 41, 39, 25, 19, 17, 23,
	39, 21, 44, 45, 39, 30, 26, 15,
	19, 17, 23, 25, 21, 40, 39, 30,
	39, 15, 19, 17, 23, 25, 21, 0
};

static const char _khmer_syllable_machine_index_defaults[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 13, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 27, 31, 31, 31,
	31, 31, 31, 31, 31, 31, 38, 39,
	39, 39, 39, 39, 39, 39, 39, 39,
	0
};

static const char _khmer_syllable_machine_trans_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, 0
};

static const char _khmer_syllable_machine_cond_targs[] = {
	20, 1, 28, 22, 23, 3, 24, 5,
	25, 7, 26, 9, 27, 20, 10, 31,
	20, 32, 12, 33, 14, 34, 16, 35,
	18, 36, 39, 20, 21, 30, 37, 20,
	0, 29, 2, 4, 6, 8, 20, 20,
	11, 13, 15, 17, 38, 19, 0
};

static const char _khmer_syllable_machine_cond_actions[] = {
	1, 0, 2, 2, 2, 0, 0, 0,
	2, 0, 2, 0, 2, 3, 0, 4,
	5, 2, 0, 0, 0, 2, 0, 2,
	0, 2, 4, 8, 2, 9, 0, 10,
	0, 0, 0, 0, 0, 0, 11, 12,
	0, 0, 0, 0, 4, 0, 0
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
	-1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, 0
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

static const char _khmer_syllable_machine_eof_trans[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 14, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 0, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 39, 40,
	40, 40, 40, 40, 40, 40, 40, 40,
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
		int _klen;const char * _cekeys;unsigned int _trans = 0;const unsigned char * _keys;const char * _inds;	 {
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
				
				_keys = ( _khmer_syllable_machine_trans_keys + ((cs<<1)));
				_inds = ( _khmer_syllable_machine_indicies + (_khmer_syllable_machine_index_offsets[cs]));
				
				if ( (info[p].khmer_category()) <= 29 && (info[p].khmer_category()) >= 1 )
				{
					int _ic = (int)_khmer_syllable_machine_char_class[(int)(info[p].khmer_category()) - 1];
					if ( _ic <= (int)(*( _keys+1)) && _ic >= (int)(*( _keys)) )
					_trans = (unsigned int)(*( _inds + (int)( _ic - (int)(*( _keys)) ) )); 
					else
					_trans = (unsigned int)_khmer_syllable_machine_index_defaults[cs];
				}
				else {
					_trans = (unsigned int)_khmer_syllable_machine_index_defaults[cs];
				}
				
				goto _match_cond;
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
