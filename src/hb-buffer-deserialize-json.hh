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


static const short _deserialize_json_key_offsets[] = {
	0, 0, 4, 8, 12, 14, 15, 19,
	26, 29, 34, 39, 46, 47, 51, 58,
	61, 66, 73, 74, 75, 79, 85, 90,
	97, 99, 100, 104, 111, 114, 119, 126,
	127, 131, 138, 141, 146, 153, 154, 158,
	165, 169, 179, 184, 191, 196, 200, 0
};

static const unsigned char _deserialize_json_trans_keys[] = {
	32u, 123u, 9u, 13u, 32u, 34u, 9u, 13u,
	97u, 99u, 100u, 103u, 120u, 121u, 34u, 32u,
	58u, 9u, 13u, 32u, 45u, 48u, 9u, 13u,
	49u, 57u, 48u, 49u, 57u, 32u, 44u, 125u,
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 32u,
	44u, 125u, 9u, 13u, 48u, 57u, 34u, 32u,
	58u, 9u, 13u, 32u, 45u, 48u, 9u, 13u,
	49u, 57u, 48u, 49u, 57u, 32u, 44u, 125u,
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 48u,
	57u, 108u, 34u, 32u, 58u, 9u, 13u, 32u,
	48u, 9u, 13u, 49u, 57u, 32u, 44u, 125u,
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 48u,
	57u, 120u, 121u, 34u, 32u, 58u, 9u, 13u,
	32u, 45u, 48u, 9u, 13u, 49u, 57u, 48u,
	49u, 57u, 32u, 44u, 125u, 9u, 13u, 32u,
	44u, 125u, 9u, 13u, 48u, 57u, 34u, 32u,
	58u, 9u, 13u, 32u, 45u, 48u, 9u, 13u,
	49u, 57u, 48u, 49u, 57u, 32u, 44u, 125u,
	9u, 13u, 32u, 44u, 125u, 9u, 13u, 48u,
	57u, 34u, 32u, 58u, 9u, 13u, 32u, 34u,
	48u, 9u, 13u, 49u, 57u, 65u, 90u, 97u,
	122u, 34u, 95u, 45u, 46u, 48u, 57u, 65u,
	90u, 97u, 122u, 32u, 44u, 125u, 9u, 13u,
	32u, 44u, 125u, 9u, 13u, 48u, 57u, 32u,
	44u, 93u, 9u, 13u, 32u, 123u, 9u, 13u,
	0u
};

static const char _deserialize_json_single_lengths[] = {
	0, 2, 2, 4, 2, 1, 2, 3,
	1, 3, 3, 3, 1, 2, 3, 1,
	3, 3, 1, 1, 2, 2, 3, 3,
	2, 1, 2, 3, 1, 3, 3, 1,
	2, 3, 1, 3, 3, 1, 2, 3,
	0, 2, 3, 3, 3, 2, 0, 0
};

static const char _deserialize_json_range_lengths[] = {
	0, 1, 1, 0, 0, 0, 1, 2,
	1, 1, 1, 2, 0, 1, 2, 1,
	1, 2, 0, 0, 1, 2, 1, 2,
	0, 0, 1, 2, 1, 1, 2, 0,
	1, 2, 1, 1, 2, 0, 1, 2,
	2, 4, 1, 2, 1, 1, 0, 0
};

static const short _deserialize_json_index_offsets[] = {
	0, 0, 4, 8, 13, 16, 18, 22,
	28, 31, 36, 41, 47, 49, 53, 59,
	62, 67, 73, 75, 77, 81, 86, 91,
	97, 100, 102, 106, 112, 115, 120, 126,
	128, 132, 138, 141, 146, 152, 154, 158,
	164, 167, 174, 179, 185, 190, 194, 0
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

static const short _deserialize_json_trans_offsets[] = {
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
	192, 193, 194, 0
};

static const char _deserialize_json_trans_lengths[] = {
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

static const char _deserialize_json_cond_keys[] = {
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

static const char _deserialize_json_cond_targs[] = {
	1, 2, 1, 0, 2, 3, 2, 0,
	4, 18, 24, 37, 0, 5, 12, 0,
	6, 0, 6, 7, 6, 0, 7, 8,
	9, 7, 11, 0, 9, 11, 0, 10,
	2, 44, 10, 0, 10, 2, 44, 10,
	0, 10, 2, 44, 10, 11, 0, 13,
	0, 13, 14, 13, 0, 14, 15, 16,
	14, 17, 0, 16, 17, 0, 10, 2,
	44, 10, 0, 10, 2, 44, 10, 17,
	0, 19, 0, 20, 0, 20, 21, 20,
	0, 21, 22, 21, 23, 0, 10, 2,
	44, 10, 0, 10, 2, 44, 10, 23,
	0, 25, 31, 0, 26, 0, 26, 27,
	26, 0, 27, 28, 29, 27, 30, 0,
	29, 30, 0, 10, 2, 44, 10, 0,
	10, 2, 44, 10, 30, 0, 32, 0,
	32, 33, 32, 0, 33, 34, 35, 33,
	36, 0, 35, 36, 0, 10, 2, 44,
	10, 0, 10, 2, 44, 10, 36, 0,
	38, 0, 38, 39, 38, 0, 39, 40,
	42, 39, 43, 0, 41, 41, 0, 10,
	41, 41, 41, 41, 41, 0, 10, 2,
	44, 10, 0, 10, 2, 44, 10, 43,
	0, 44, 45, 46, 44, 0, 1, 2,
	1, 0, 0, 0
};

static const char _deserialize_json_cond_actions[] = {
	0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2,
	2, 0, 2, 0, 0, 0, 0, 3,
	3, 4, 3, 0, 0, 0, 5, 0,
	0, 3, 3, 4, 3, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 2,
	0, 2, 0, 0, 0, 0, 6, 6,
	7, 6, 0, 6, 6, 7, 6, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 0, 2, 0, 8, 8,
	9, 8, 0, 8, 8, 9, 8, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 2, 2, 0, 2, 0,
	0, 0, 0, 10, 10, 11, 10, 0,
	10, 10, 11, 10, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 0,
	2, 0, 0, 0, 0, 12, 12, 13,
	12, 0, 12, 12, 13, 12, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 2, 0, 2, 2, 0, 14,
	0, 0, 0, 0, 0, 0, 15, 15,
	16, 15, 0, 15, 15, 16, 15, 0,
	0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0
};

static const char _deserialize_json_eof_cond_spaces[] = {
	-1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, 0
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
		int _klen;unsigned int _trans = 0;const unsigned char * _keys;	 {
			if ( p == pe )
			goto _test_eof;
			if ( cs == 0 )
			goto _out;
			_resume:  {
				_keys = ( _deserialize_json_trans_keys + (_deserialize_json_key_offsets[cs]));
				_trans = (unsigned int)_deserialize_json_index_offsets[cs];
				
				_klen = (int)_deserialize_json_single_lengths[cs];
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
				
				_klen = (int)_deserialize_json_range_lengths[cs];
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
