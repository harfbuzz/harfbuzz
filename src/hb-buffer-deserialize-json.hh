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

#ifndef HB_BUFFER_DESERIALIZE_JSON_HH
#define HB_BUFFER_DESERIALIZE_JSON_HH

#include "hb.hh"


static const unsigned char _deserialize_json_trans_keys[] = {
	1u, 0u, 0u, 18u, 0u, 2u, 11u, 14u,
	16u, 17u, 2u, 2u, 0u, 8u, 0u, 7u,
	6u, 7u, 0u, 19u, 0u, 19u, 0u, 19u,
	2u, 2u, 0u, 8u, 0u, 7u, 6u, 7u,
	0u, 19u, 0u, 19u, 15u, 15u, 2u, 2u,
	0u, 8u, 0u, 7u, 0u, 19u, 0u, 19u,
	16u, 17u, 2u, 2u, 0u, 8u, 0u, 7u,
	6u, 7u, 0u, 19u, 0u, 19u, 2u, 2u,
	0u, 8u, 0u, 7u, 6u, 7u, 0u, 19u,
	0u, 19u, 2u, 2u, 0u, 8u, 0u, 7u,
	9u, 17u, 2u, 17u, 0u, 19u, 0u, 19u,
	0u, 10u, 0u, 18u, 1u, 0u, 0u
};

static const char _deserialize_json_char_class[] = {
	0, 0, 0, 0, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 2, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 3, 4, 5, 1, 6,
	7, 7, 7, 7, 7, 7, 7, 7,
	7, 8, 1, 1, 1, 1, 1, 1,
	9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9,
	9, 9, 1, 1, 10, 1, 5, 1,
	11, 9, 12, 13, 9, 9, 14, 9,
	9, 9, 9, 15, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 16,
	17, 9, 18, 1, 19, 0
};

static const short _deserialize_json_index_offsets[] = {
	0, 0, 19, 22, 26, 28, 29, 38,
	46, 48, 68, 88, 108, 109, 118, 126,
	128, 148, 168, 169, 170, 179, 187, 207,
	227, 229, 230, 239, 247, 249, 269, 289,
	290, 299, 307, 309, 329, 349, 350, 359,
	367, 376, 392, 412, 432, 443, 462, 0
};

static const char _deserialize_json_indicies[] = {
	0, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 2, 3, 1, 4, 5, 6,
	7, 8, 9, 10, 11, 11, 1, 1,
	1, 1, 1, 1, 1, 12, 12, 1,
	1, 1, 13, 1, 14, 15, 16, 17,
	18, 1, 1, 19, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 20, 21, 1, 1, 3,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 22,
	18, 1, 1, 19, 1, 1, 17, 17,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 20, 23, 23, 1, 1,
	1, 1, 1, 1, 1, 24, 24, 1,
	1, 1, 25, 1, 26, 27, 28, 29,
	30, 1, 1, 31, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 32, 30, 1, 1, 31,
	1, 1, 29, 29, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 32,
	33, 34, 34, 1, 1, 1, 1, 1,
	1, 1, 35, 35, 1, 1, 1, 1,
	1, 36, 37, 38, 1, 1, 39, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 40, 38,
	1, 1, 39, 1, 1, 41, 41, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 40, 42, 43, 44, 44, 1,
	1, 1, 1, 1, 1, 1, 45, 45,
	1, 1, 1, 46, 1, 47, 48, 49,
	50, 51, 1, 1, 52, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 53, 51, 1, 1,
	52, 1, 1, 50, 50, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	53, 54, 54, 1, 1, 1, 1, 1,
	1, 1, 55, 55, 1, 1, 1, 56,
	1, 57, 58, 59, 60, 61, 1, 1,
	62, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	63, 61, 1, 1, 62, 1, 1, 60,
	60, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 63, 64, 64, 1,
	1, 1, 1, 1, 1, 1, 65, 65,
	1, 66, 1, 1, 1, 67, 68, 69,
	1, 69, 69, 69, 69, 69, 69, 69,
	70, 1, 71, 71, 71, 71, 1, 71,
	1, 71, 71, 71, 71, 71, 71, 71,
	72, 1, 1, 73, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 74, 72, 1, 1, 73,
	1, 1, 75, 75, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 74,
	76, 1, 1, 77, 1, 1, 1, 1,
	1, 1, 78, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 2, 0
};

static const char _deserialize_json_index_defaults[] = {
	0, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 0
};

static const char _deserialize_json_trans_cond_spaces[] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, 0
};

static const char _deserialize_json_cond_targs[] = {
	1, 0, 2, 2, 3, 4, 18, 24,
	37, 5, 12, 6, 7, 8, 9, 11,
	9, 11, 10, 2, 44, 10, 44, 13,
	14, 15, 16, 17, 16, 17, 10, 2,
	44, 19, 20, 21, 22, 23, 10, 2,
	44, 23, 25, 31, 26, 27, 28, 29,
	30, 29, 30, 10, 2, 44, 32, 33,
	34, 35, 36, 35, 36, 10, 2, 44,
	38, 39, 40, 42, 43, 41, 10, 41,
	10, 2, 44, 43, 44, 45, 46, 0
};

static const char _deserialize_json_cond_actions[] = {
	0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2,
	0, 0, 3, 3, 4, 0, 5, 0,
	0, 2, 2, 2, 0, 0, 6, 6,
	7, 0, 0, 0, 2, 2, 8, 8,
	9, 0, 0, 0, 0, 0, 2, 2,
	2, 0, 0, 10, 10, 11, 0, 0,
	2, 2, 2, 0, 0, 12, 12, 13,
	0, 0, 0, 2, 2, 2, 14, 0,
	15, 15, 16, 0, 0, 0, 0, 0
};

static const char _deserialize_json_eof_cond_spaces[] = {
	-1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, 0
};

static const char _deserialize_json_eof_cond_key_offs[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _deserialize_json_eof_cond_key_lens[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _deserialize_json_eof_cond_keys[] = {
	0
};

static const char _deserialize_json_nfa_targs[] = {
	0, 0
};

static const char _deserialize_json_nfa_offsets[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

static const char _deserialize_json_nfa_push_actions[] = {
	0, 0
};

static const char _deserialize_json_nfa_pop_trans[] = {
	0, 0
};

static const int deserialize_json_start = 1;
static const int deserialize_json_first_final = 44;
static const int deserialize_json_error = 0;

static const int deserialize_json_en_main = 1;



static hb_bool_t
_hb_buffer_deserialize_glyphs_json (hb_buffer_t *buffer,
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
	if (p < pe && *p == (buffer->len ? ',' : '['))
	{
		*end_ptr = ++p;
	}
	
	const char *tok = nullptr;
	int cs;
	hb_glyph_info_t info = {0};
	hb_glyph_position_t pos = {0};
	
	{
		cs = (int)deserialize_json_start;
	}
	
	{
		unsigned int _trans = 0;const unsigned char * _keys;const char * _inds;	 {
			if ( p == pe )
			goto _test_eof;
			if ( cs == 0 )
			goto _out;
			_resume:  {
				_keys = ( _deserialize_json_trans_keys + ((cs<<1)));
				_inds = ( _deserialize_json_indicies + (_deserialize_json_index_offsets[cs]));
				
				if ( ( (*( p))) <= 125 && ( (*( p))) >= 9 )
				{
					int _ic = (int)_deserialize_json_char_class[(int)( (*( p))) - 9];
					if ( _ic <= (int)(*( _keys+1)) && _ic >= (int)(*( _keys)) )
					_trans = (unsigned int)(*( _inds + (int)( _ic - (int)(*( _keys)) ) )); 
					else
					_trans = (unsigned int)_deserialize_json_index_defaults[cs];
				}
				else {
					_trans = (unsigned int)_deserialize_json_index_defaults[cs];
				}
				
				goto _match_cond;
			}
			_match_cond:  {
				cs = (int)_deserialize_json_cond_targs[_trans];
				
				if ( _deserialize_json_cond_actions[_trans] == 0 )
				goto _again;
				
				switch ( _deserialize_json_cond_actions[_trans] ) {
					case 1:  {
						{
							#line 38 "hb-buffer-deserialize-json.rl"
							
							memset (&info, 0, sizeof (info));
							memset (&pos , 0, sizeof (pos ));
						}
						
						break; }
					case 5:  {
						{
							#line 43 "hb-buffer-deserialize-json.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 2:  {
						{
							#line 51 "hb-buffer-deserialize-json.rl"
							
							tok = p;
						}
						
						break; }
					case 14:  {
						{
							#line 55 "hb-buffer-deserialize-json.rl"
							
							if (!hb_font_glyph_from_string (font,
							tok, p - tok,
							&info.codepoint))
							return false;
						}
						
						break; }
					case 15:  {
						{
							#line 62 "hb-buffer-deserialize-json.rl"
							if (!parse_uint (tok, p, &info.codepoint)) return false; }
						
						break; }
					case 8:  {
						{
							#line 63 "hb-buffer-deserialize-json.rl"
							if (!parse_uint (tok, p, &info.cluster )) return false; }
						
						break; }
					case 10:  {
						{
							#line 64 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.x_offset )) return false; }
						
						break; }
					case 12:  {
						{
							#line 65 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.y_offset )) return false; }
						
						break; }
					case 3:  {
						{
							#line 66 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.x_advance)) return false; }
						
						break; }
					case 6:  {
						{
							#line 67 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.y_advance)) return false; }
						
						break; }
					case 16:  {
						{
							#line 62 "hb-buffer-deserialize-json.rl"
							if (!parse_uint (tok, p, &info.codepoint)) return false; }
						{
							#line 43 "hb-buffer-deserialize-json.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 9:  {
						{
							#line 63 "hb-buffer-deserialize-json.rl"
							if (!parse_uint (tok, p, &info.cluster )) return false; }
						{
							#line 43 "hb-buffer-deserialize-json.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 11:  {
						{
							#line 64 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.x_offset )) return false; }
						{
							#line 43 "hb-buffer-deserialize-json.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 13:  {
						{
							#line 65 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.y_offset )) return false; }
						{
							#line 43 "hb-buffer-deserialize-json.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 4:  {
						{
							#line 66 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.x_advance)) return false; }
						{
							#line 43 "hb-buffer-deserialize-json.rl"
							
							buffer->add_info (info);
							if (unlikely (!buffer->successful))
							return false;
							buffer->pos[buffer->len - 1] = pos;
							*end_ptr = p;
						}
						
						break; }
					case 7:  {
						{
							#line 67 "hb-buffer-deserialize-json.rl"
							if (!parse_int  (tok, p, &pos.y_advance)) return false; }
						{
							#line 43 "hb-buffer-deserialize-json.rl"
							
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
			}
			_out:  { {}
			}
		}
	}
	
	
	*end_ptr = p;
	
	return p == pe && *(p-1) != ']';
}

#endif /* HB_BUFFER_DESERIALIZE_JSON_HH */
