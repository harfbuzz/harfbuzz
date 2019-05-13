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


static const short _deserialize_text_key_offsets[] = {
	0, 0, 10, 14, 17, 21, 24, 27,
	31, 34, 35, 39, 42, 45, 53, 58,
	58, 68, 78, 84, 89, 96, 104, 111,
	117, 125, 134, 0
};

static const unsigned char _deserialize_text_trans_keys[] = {
	32u, 48u, 9u, 13u, 49u, 57u, 65u, 90u,
	97u, 122u, 45u, 48u, 49u, 57u, 48u, 49u,
	57u, 45u, 48u, 49u, 57u, 48u, 49u, 57u,
	48u, 49u, 57u, 45u, 48u, 49u, 57u, 48u,
	49u, 57u, 44u, 45u, 48u, 49u, 57u, 48u,
	49u, 57u, 44u, 48u, 57u, 32u, 43u, 61u,
	64u, 93u, 124u, 9u, 13u, 32u, 93u, 124u,
	9u, 13u, 32u, 48u, 9u, 13u, 49u, 57u,
	65u, 90u, 97u, 122u, 32u, 43u, 61u, 64u,
	93u, 124u, 9u, 13u, 48u, 57u, 32u, 44u,
	93u, 124u, 9u, 13u, 32u, 93u, 124u, 9u,
	13u, 32u, 93u, 124u, 9u, 13u, 48u, 57u,
	32u, 44u, 93u, 124u, 9u, 13u, 48u, 57u,
	32u, 43u, 64u, 93u, 124u, 9u, 13u, 32u,
	43u, 93u, 124u, 9u, 13u, 32u, 43u, 93u,
	124u, 9u, 13u, 48u, 57u, 32u, 43u, 64u,
	93u, 124u, 9u, 13u, 48u, 57u, 32u, 43u,
	61u, 64u, 93u, 95u, 124u, 9u, 13u, 45u,
	46u, 48u, 57u, 65u, 90u, 97u, 122u, 0u
};

static const char _deserialize_text_single_lengths[] = {
	0, 2, 2, 1, 2, 1, 1, 2,
	1, 1, 2, 1, 1, 6, 3, 0,
	2, 6, 4, 3, 3, 4, 5, 4,
	4, 5, 7, 0
};

static const char _deserialize_text_range_lengths[] = {
	0, 4, 1, 1, 1, 1, 1, 1,
	1, 0, 1, 1, 1, 1, 1, 0,
	4, 2, 1, 1, 2, 2, 1, 1,
	2, 2, 5, 0
};

static const char _deserialize_text_index_offsets[] = {
	0, 0, 7, 11, 14, 18, 21, 24,
	28, 31, 33, 37, 40, 43, 51, 56,
	57, 64, 73, 79, 84, 90, 97, 104,
	110, 117, 125, 0
};

static const char _deserialize_text_trans_cond_spaces[] = {
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
	-1, -1, 0
};

static const short _deserialize_text_trans_offsets[] = {
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
	136, 137, 0
};

static const char _deserialize_text_trans_lengths[] = {
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
	1, 1, 0
};

static const char _deserialize_text_cond_keys[] = {
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
	0, 0, 0
};

static const char _deserialize_text_cond_targs[] = {
	1, 13, 1, 17, 26, 26, 0, 3,
	18, 21, 0, 18, 21, 0, 5, 19,
	20, 0, 19, 20, 0, 22, 25, 0,
	8, 9, 12, 0, 9, 12, 0, 10,
	0, 11, 23, 24, 0, 23, 24, 0,
	10, 12, 0, 14, 2, 6, 7, 15,
	16, 14, 0, 14, 15, 16, 14, 0,
	0, 1, 13, 1, 17, 26, 26, 0,
	14, 2, 6, 7, 15, 16, 14, 17,
	0, 14, 4, 15, 16, 14, 0, 14,
	15, 16, 14, 0, 14, 15, 16, 14,
	20, 0, 14, 4, 15, 16, 14, 21,
	0, 14, 2, 7, 15, 16, 14, 0,
	14, 2, 15, 16, 14, 0, 14, 2,
	15, 16, 14, 24, 0, 14, 2, 7,
	15, 16, 14, 25, 0, 14, 2, 6,
	7, 15, 26, 16, 14, 26, 26, 26,
	26, 0, 0
};

static const char _deserialize_text_cond_actions[] = {
	0, 1, 0, 1, 1, 1, 0, 2,
	2, 2, 0, 0, 0, 0, 2, 2,
	2, 0, 0, 0, 0, 2, 2, 0,
	2, 2, 2, 0, 0, 0, 0, 3,
	0, 2, 2, 2, 0, 0, 0, 0,
	3, 0, 0, 4, 5, 5, 5, 4,
	4, 4, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 1, 1, 1, 0,
	4, 5, 5, 5, 4, 4, 4, 0,
	0, 6, 7, 6, 6, 6, 0, 8,
	8, 8, 8, 0, 8, 8, 8, 8,
	0, 0, 6, 7, 6, 6, 6, 0,
	0, 9, 10, 10, 9, 9, 9, 0,
	11, 12, 11, 11, 11, 0, 11, 12,
	11, 11, 11, 0, 0, 9, 10, 10,
	9, 9, 9, 0, 0, 4, 5, 5,
	5, 4, 0, 4, 4, 0, 0, 0,
	0, 0, 0
};

static const char _deserialize_text_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 4, 0, 0,
	0, 4, 6, 8, 8, 6, 9, 11,
	11, 9, 4, 0
};

static const char _deserialize_text_eof_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, 0
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
		int _klen;const char * _cekeys;unsigned int _trans = 0;const unsigned char * _keys;	 {
			if ( p == pe )
			goto _test_eof;
			if ( cs == 0 )
			goto _out;
			_resume:  {
				_keys = ( _deserialize_text_trans_keys + (_deserialize_text_key_offsets[cs]));
				_trans = (unsigned int)_deserialize_text_index_offsets[cs];
				
				_klen = (int)_deserialize_text_single_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + _klen - 1;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + ((_upper-_lower) >> 1);
						if ( ( (*( p))) < (*( _mid)) )
						_upper = _mid - 1;
						else if ( ( (*( p))) > (*( _mid)) )
						_lower = _mid + 1;
						else {
							_trans += (unsigned int)(_mid - _keys);
							goto _match;
						}
					}
					_keys += _klen;
					_trans += (unsigned int)_klen;
				}
				
				_klen = (int)_deserialize_text_range_lengths[cs];
				if ( _klen > 0 ) {
					const unsigned char *_lower = _keys;
					const unsigned char *_upper = _keys + (_klen<<1) - 2;
					const unsigned char *_mid;
					while ( 1 ) {
						if ( _upper < _lower )
						break;
						
						_mid = _lower + (((_upper-_lower) >> 1) & ~1);
						if ( ( (*( p))) < (*( _mid)) )
						_upper = _mid - 2;
						else if ( ( (*( p))) > (*( _mid + 1)) )
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
