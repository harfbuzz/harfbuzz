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


static const unsigned char _deserialize_text_trans_keys[] = {
	1u, 0u, 0u, 10u, 4u, 7u, 6u, 7u,
	4u, 7u, 6u, 7u, 6u, 7u, 4u, 7u,
	6u, 7u, 3u, 3u, 4u, 7u, 6u, 7u,
	3u, 7u, 0u, 12u, 0u, 12u, 1u, 0u,
	0u, 10u, 0u, 12u, 0u, 12u, 0u, 12u,
	0u, 12u, 0u, 12u, 0u, 12u, 0u, 12u,
	0u, 12u, 0u, 12u, 0u, 12u, 0u
};

static const char _deserialize_text_char_class[] = {
	0, 0, 0, 0, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 2, 3, 4, 5, 1, 6,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 1, 1, 1, 8, 1, 1, 9,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 1, 1, 11, 1, 5, 1,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 1, 12, 0
};

static const short _deserialize_text_index_offsets[] = {
	0, 0, 11, 15, 17, 21, 23, 25,
	29, 31, 32, 36, 38, 43, 56, 69,
	69, 80, 93, 106, 119, 132, 145, 158,
	171, 184, 197, 0
};

static const char _deserialize_text_indicies[] = {
	0, 1, 1, 1, 1, 1, 2, 3,
	1, 1, 4, 5, 1, 6, 7, 8,
	9, 10, 1, 11, 12, 13, 14, 15,
	16, 17, 1, 18, 19, 20, 21, 22,
	23, 1, 24, 25, 26, 27, 22, 1,
	1, 21, 21, 28, 1, 29, 1, 1,
	1, 1, 1, 30, 31, 1, 32, 33,
	34, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 35, 36, 0, 1, 1,
	1, 1, 1, 2, 3, 1, 1, 4,
	28, 1, 29, 1, 1, 1, 37, 37,
	30, 31, 1, 32, 33, 38, 1, 1,
	39, 1, 1, 1, 1, 1, 1, 1,
	40, 41, 42, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 43, 44, 42,
	1, 1, 1, 1, 1, 14, 14, 1,
	1, 1, 43, 44, 38, 1, 1, 39,
	1, 1, 9, 9, 1, 1, 1, 40,
	41, 45, 1, 46, 1, 1, 1, 1,
	1, 1, 47, 1, 48, 49, 50, 1,
	51, 1, 1, 1, 1, 1, 1, 1,
	1, 52, 53, 50, 1, 51, 1, 1,
	1, 27, 27, 1, 1, 1, 52, 53,
	45, 1, 46, 1, 1, 1, 54, 54,
	1, 47, 1, 48, 49, 28, 1, 29,
	1, 55, 55, 55, 55, 30, 31, 55,
	32, 33, 0
};

static const char _deserialize_text_index_defaults[] = {
	0, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 0
};

static const char _deserialize_text_trans_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	0
};

static const char _deserialize_text_cond_targs[] = {
	1, 0, 13, 17, 26, 3, 18, 21,
	18, 21, 5, 19, 20, 19, 20, 22,
	25, 8, 9, 12, 9, 12, 10, 11,
	23, 24, 23, 24, 14, 2, 6, 7,
	15, 16, 14, 15, 16, 17, 14, 4,
	15, 16, 14, 15, 16, 14, 2, 7,
	15, 16, 14, 2, 15, 16, 25, 26,
	0
};

static const char _deserialize_text_cond_actions[] = {
	0, 0, 1, 1, 1, 2, 2, 2,
	0, 0, 2, 2, 2, 0, 0, 2,
	2, 2, 2, 2, 0, 0, 3, 2,
	2, 2, 0, 0, 4, 5, 5, 5,
	4, 4, 0, 0, 0, 0, 6, 7,
	6, 6, 8, 8, 8, 9, 10, 10,
	9, 9, 11, 12, 11, 11, 0, 0,
	0
};

static const char _deserialize_text_eof_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, 0
};

static const char _deserialize_text_eof_cond_key_offs[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _deserialize_text_eof_cond_key_lens[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _deserialize_text_eof_cond_keys[] = {
	0
};

static const char _deserialize_text_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 4, 0, 0,
	0, 4, 6, 8, 8, 6, 9, 11,
	11, 9, 4, 0
};

static const char _deserialize_text_nfa_targs[] = {
	0, 0
};

static const char _deserialize_text_nfa_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

static const char _deserialize_text_nfa_push_actions[] = {
	0, 0
};

static const char _deserialize_text_nfa_pop_trans[] = {
	0, 0
};

static const int deserialize_text_start = 1;
static const int deserialize_text_first_final = 13;
static const int deserialize_text_error = 0;

static const int deserialize_text_en_main = 1;



static hb_bool_t
_hb_buffer_deserialize_glyphs_text (hb_buffer_t *buffer,
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
	if (p < pe && *p == (buffer->len ? '|' : '['))
	{
		*end_ptr = ++p;
	}
	
	const char *eof = pe, *tok = nullptr;
	int cs;
	hb_glyph_info_t info = {0};
	hb_glyph_position_t pos = {0};
	
	{
		cs = (int)deserialize_text_start;
	}
	
	{
		int _cpc;
		int _klen;const char * _cekeys;unsigned int _trans = 0;const unsigned char * _keys;const char * _inds;	 {
			if ( p == pe )
			goto _test_eof;
			if ( cs == 0 )
			goto _out;
			_resume:  {
				_keys = ( _deserialize_text_trans_keys + ((cs<<1)));
				_inds = ( _deserialize_text_indicies + (_deserialize_text_index_offsets[cs]));
				
				if ( ( (*( p))) <= 124 && ( (*( p))) >= 9 )
				{
					int _ic = (int)_deserialize_text_char_class[(int)( (*( p))) - 9];
					if ( _ic <= (int)(*( _keys+1)) && _ic >= (int)(*( _keys)) )
					_trans = (unsigned int)(*( _inds + (int)( _ic - (int)(*( _keys)) ) )); 
					else
					_trans = (unsigned int)_deserialize_text_index_defaults[cs];
				}
				else {
					_trans = (unsigned int)_deserialize_text_index_defaults[cs];
				}
				
				goto _match_cond;
			}
			_match_cond:  {
				cs = (int)_deserialize_text_cond_targs[_trans];
				
				if ( _deserialize_text_cond_actions[_trans] == 0 )
				goto _again;
				
				switch ( _deserialize_text_cond_actions[_trans] ) {
					case 2:  {
						{
							#line 51 "hb-buffer-deserialize-text.rl"
							
							tok = p;
						}
						
						break; }
					case 5:  {
						{
							#line 55 "hb-buffer-deserialize-text.rl"
							
							if (!hb_font_glyph_from_string (font,
							tok, p - tok,
							&info.codepoint))
							return false;
						}
						
						break; }
					case 10:  {
						{
							#line 62 "hb-buffer-deserialize-text.rl"
							if (!parse_uint (tok, p, &info.cluster )) return false; }
						
						break; }
					case 3:  {
						{
							#line 63 "hb-buffer-deserialize-text.rl"
							if (!parse_int  (tok, p, &pos.x_offset )) return false; }
						
						break; }
					case 12:  {
						{
							#line 64 "hb-buffer-deserialize-text.rl"
							if (!parse_int  (tok, p, &pos.y_offset )) return false; }
						
						break; }
					case 7:  {
						{
							#line 65 "hb-buffer-deserialize-text.rl"
							if (!parse_int  (tok, p, &pos.x_advance)) return false; }
						
						break; }
					case 1:  {
						{
							#line 38 "hb-buffer-deserialize-text.rl"
							
							memset (&info, 0, sizeof (info));
							memset (&pos , 0, sizeof (pos ));
						}
						{
							#line 51 "hb-buffer-deserialize-text.rl"
							
							tok = p;
						}
						
						break; }
					case 4:  {
						{
							#line 55 "hb-buffer-deserialize-text.rl"
							
							if (!hb_font_glyph_from_string (font,
							tok, p - tok,
							&info.codepoint))
							return false;
						}
						{
							#line 43 "hb-buffer-deserialize-text.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 9:  {
						{
							#line 62 "hb-buffer-deserialize-text.rl"
							if (!parse_uint (tok, p, &info.cluster )) return false; }
						{
							#line 43 "hb-buffer-deserialize-text.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 11:  {
						{
							#line 64 "hb-buffer-deserialize-text.rl"
							if (!parse_int  (tok, p, &pos.y_offset )) return false; }
						{
							#line 43 "hb-buffer-deserialize-text.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 6:  {
						{
							#line 65 "hb-buffer-deserialize-text.rl"
							if (!parse_int  (tok, p, &pos.x_advance)) return false; }
						{
							#line 43 "hb-buffer-deserialize-text.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 8:  {
						{
							#line 66 "hb-buffer-deserialize-text.rl"
							if (!parse_int  (tok, p, &pos.y_advance)) return false; }
						{
							#line 43 "hb-buffer-deserialize-text.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
				}
				
				
			}
			_again:  {
				if ( cs == 0 )
				goto _out;
				p += 1;
				if ( p != pe )
				goto _resume;
			}
			_test_eof:  { {}
				if ( p == eof )
				{
					if ( _deserialize_text_eof_cond_spaces[cs] != -1 ) {
						_cekeys = ( _deserialize_text_eof_cond_keys + (_deserialize_text_eof_cond_key_offs[cs]));
						_klen = (int)_deserialize_text_eof_cond_key_lens[cs];
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
							cs = 0;
							goto _out;
						}
						_ok: {}
					}
					switch ( _deserialize_text_eof_actions[cs] ) {
						case 4:  {
							{
								#line 55 "hb-buffer-deserialize-text.rl"
								
								if (!hb_font_glyph_from_string (font,
								tok, p - tok,
								&info.codepoint))
								return false;
							}
							{
								#line 43 "hb-buffer-deserialize-text.rl"
								
								buffer->add_info (info);
								if (unlikely (!buffer->successful))
								return false;
								buffer->pos[buffer->len - 1] = pos;
								*end_ptr = p;
							}
							
							break; }
						case 9:  {
							{
								#line 62 "hb-buffer-deserialize-text.rl"
								if (!parse_uint (tok, p, &info.cluster )) return false; }
							{
								#line 43 "hb-buffer-deserialize-text.rl"
								
								buffer->add_info (info);
								if (unlikely (!buffer->successful))
								return false;
								buffer->pos[buffer->len - 1] = pos;
								*end_ptr = p;
							}
							
							break; }
						case 11:  {
							{
								#line 64 "hb-buffer-deserialize-text.rl"
								if (!parse_int  (tok, p, &pos.y_offset )) return false; }
							{
								#line 43 "hb-buffer-deserialize-text.rl"
								
								buffer->add_info (info);
								if (unlikely (!buffer->successful))
								return false;
								buffer->pos[buffer->len - 1] = pos;
								*end_ptr = p;
							}
							
							break; }
						case 6:  {
							{
								#line 65 "hb-buffer-deserialize-text.rl"
								if (!parse_int  (tok, p, &pos.x_advance)) return false; }
							{
								#line 43 "hb-buffer-deserialize-text.rl"
								
								buffer->add_info (info);
								if (unlikely (!buffer->successful))
								return false;
								buffer->pos[buffer->len - 1] = pos;
								*end_ptr = p;
							}
							
							break; }
						case 8:  {
							{
								#line 66 "hb-buffer-deserialize-text.rl"
								if (!parse_int  (tok, p, &pos.y_advance)) return false; }
							{
								#line 43 "hb-buffer-deserialize-text.rl"
								
								buffer->add_info (info);
								if (unlikely (!buffer->successful))
								return false;
								buffer->pos[buffer->len - 1] = pos;
								*end_ptr = p;
							}
							
							break; }
					}
				}
				
			}
			_out:  { {}
			}
		}
	}
	
	
	*end_ptr = p;
	
	return p == pe && *(p-1) != ']';
}

#endif /* HB_BUFFER_DESERIALIZE_TEXT_HH */
