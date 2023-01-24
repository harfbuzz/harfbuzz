
#line 1 "hb-buffer-deserialize-text-glyphs.rl"
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

#ifndef HB_BUFFER_DESERIALIZE_TEXT_GLYPHS_HH
#define HB_BUFFER_DESERIALIZE_TEXT_GLYPHS_HH

#include "hb.hh"


#line 33 "hb-buffer-deserialize-text-glyphs.hh"
static const unsigned char _deserialize_text_glyphs_trans_keys[] = {
	0u, 0u, 9u, 124u, 9u, 124u, 48u, 57u, 9u, 124u, 9u, 124u, 45u, 57u, 48u, 57u, 
	9u, 124u, 45u, 57u, 48u, 57u, 9u, 124u, 9u, 124u, 9u, 124u, 48u, 57u, 9u, 124u, 
	45u, 57u, 48u, 57u, 44u, 44u, 45u, 57u, 48u, 57u, 9u, 124u, 9u, 124u, 44u, 57u, 
	9u, 124u, 43u, 124u, 9u, 124u, 0u, 0u, 9u, 124u, 0
};

static const char _deserialize_text_glyphs_key_spans[] = {
	0, 116, 116, 10, 116, 116, 13, 10, 
	116, 13, 10, 116, 116, 116, 10, 116, 
	13, 10, 1, 13, 10, 116, 116, 14, 
	116, 82, 116, 0, 116
};

static const short _deserialize_text_glyphs_index_offsets[] = {
	0, 0, 117, 234, 245, 362, 479, 493, 
	504, 621, 635, 646, 763, 880, 997, 1008, 
	1125, 1139, 1150, 1152, 1166, 1177, 1294, 1411, 
	1426, 1543, 1626, 1743, 1744
};

static const char _deserialize_text_glyphs_indicies[] = {
	1, 1, 1, 1, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 0, 2, 0, 0, 0, 0, 
	0, 0, 0, 3, 4, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 5, 0, 0, 
	6, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 7, 8, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 9, 0, 11, 11, 
	11, 11, 11, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 11, 10, 10, 
	12, 10, 10, 10, 10, 10, 10, 10, 
	13, 4, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 14, 10, 10, 15, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 16, 17, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 10, 10, 10, 10, 10, 10, 10, 
	10, 18, 10, 19, 20, 20, 20, 20, 
	20, 20, 20, 20, 20, 4, 21, 21, 
	21, 21, 21, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 21, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 22, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 23, 4, 24, 24, 24, 24, 24, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 24, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 25, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 26, 4, 
	27, 4, 4, 28, 29, 29, 29, 29, 
	29, 29, 29, 29, 29, 4, 30, 31, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	4, 32, 32, 32, 32, 32, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	32, 4, 4, 33, 4, 4, 4, 4, 
	4, 4, 4, 4, 34, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 35, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 36, 4, 37, 4, 
	4, 38, 39, 39, 39, 39, 39, 39, 
	39, 39, 39, 4, 40, 41, 41, 41, 
	41, 41, 41, 41, 41, 41, 4, 42, 
	42, 42, 42, 42, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 42, 4, 
	4, 43, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 44, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 45, 4, 42, 42, 42, 42, 
	42, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 42, 4, 4, 43, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 41, 41, 41, 41, 41, 
	41, 41, 41, 41, 41, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	44, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 45, 
	4, 32, 32, 32, 32, 32, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	32, 4, 4, 33, 4, 4, 4, 4, 
	4, 4, 4, 4, 34, 4, 4, 4, 
	31, 31, 31, 31, 31, 31, 31, 31, 
	31, 31, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 35, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 36, 4, 46, 47, 
	47, 47, 47, 47, 47, 47, 47, 47, 
	4, 48, 48, 48, 48, 48, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	48, 4, 4, 49, 4, 4, 4, 4, 
	4, 4, 4, 50, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	51, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 52, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 53, 4, 54, 4, 
	4, 55, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 4, 57, 58, 58, 58, 
	58, 58, 58, 58, 58, 58, 4, 59, 
	4, 60, 4, 4, 61, 62, 62, 62, 
	62, 62, 62, 62, 62, 62, 4, 63, 
	64, 64, 64, 64, 64, 64, 64, 64, 
	64, 4, 65, 65, 65, 65, 65, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 65, 4, 4, 66, 4, 4, 4, 
	4, 4, 4, 4, 67, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 68, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 69, 4, 65, 
	65, 65, 65, 65, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 65, 4, 
	4, 66, 4, 4, 4, 4, 4, 4, 
	4, 67, 4, 4, 4, 4, 64, 64, 
	64, 64, 64, 64, 64, 64, 64, 64, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 68, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 69, 4, 59, 4, 4, 4, 
	58, 58, 58, 58, 58, 58, 58, 58, 
	58, 58, 4, 48, 48, 48, 48, 48, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 48, 4, 4, 49, 4, 4, 
	4, 4, 4, 4, 4, 50, 4, 4, 
	4, 4, 70, 70, 70, 70, 70, 70, 
	70, 70, 70, 70, 4, 4, 4, 4, 
	4, 4, 51, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 52, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 53, 4, 
	10, 10, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 10, 4, 4, 10, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 10, 10, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 10, 4, 21, 21, 21, 21, 21, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 21, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 71, 71, 71, 71, 71, 71, 
	71, 71, 71, 71, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 22, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 23, 4, 
	4, 1, 1, 1, 1, 1, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 0, 0, 2, 0, 0, 0, 0, 
	0, 0, 0, 3, 4, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 5, 0, 0, 
	6, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 7, 8, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 9, 0, 0
};

static const char _deserialize_text_glyphs_trans_targs[] = {
	2, 1, 3, 6, 0, 14, 16, 25, 
	27, 28, 2, 2, 3, 6, 14, 16, 
	25, 27, 28, 4, 26, 5, 27, 28, 
	5, 27, 28, 7, 8, 13, 8, 13, 
	5, 3, 9, 27, 28, 10, 11, 12, 
	11, 12, 5, 3, 27, 28, 15, 24, 
	5, 3, 6, 16, 27, 28, 17, 18, 
	23, 18, 23, 19, 20, 21, 22, 21, 
	22, 5, 3, 6, 27, 28, 24, 26
};

static const char _deserialize_text_glyphs_trans_actions[] = {
	1, 2, 3, 3, 0, 3, 3, 1, 
	2, 2, 0, 4, 5, 5, 5, 5, 
	0, 4, 4, 6, 6, 7, 7, 7, 
	0, 0, 0, 6, 6, 6, 0, 0, 
	8, 9, 9, 8, 8, 6, 6, 6, 
	0, 0, 10, 11, 10, 10, 6, 6, 
	12, 13, 13, 13, 12, 12, 6, 6, 
	6, 0, 0, 14, 6, 6, 6, 0, 
	0, 15, 16, 16, 15, 15, 0, 0
};

static const int deserialize_text_glyphs_start = 1;
static const int deserialize_text_glyphs_first_final = 27;
static const int deserialize_text_glyphs_error = 0;

static const int deserialize_text_glyphs_en_main = 1;


#line 98 "hb-buffer-deserialize-text-glyphs.rl"


static hb_bool_t
_hb_buffer_deserialize_text_glyphs (hb_buffer_t *buffer,
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
    *end_ptr = ++p;

  const char *tok = nullptr;
  int cs;
  hb_glyph_info_t info = {0};
  hb_glyph_position_t pos = {0};
  
#line 343 "hb-buffer-deserialize-text-glyphs.hh"
	{
	cs = deserialize_text_glyphs_start;
	}

#line 346 "hb-buffer-deserialize-text-glyphs.hh"
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
	_keys = _deserialize_text_glyphs_trans_keys + (cs<<1);
	_inds = _deserialize_text_glyphs_indicies + _deserialize_text_glyphs_index_offsets[cs];

	_slen = _deserialize_text_glyphs_key_spans[cs];
	_trans = _inds[ _slen > 0 && _keys[0] <=(*p) &&
		(*p) <= _keys[1] ?
		(*p) - _keys[0] : _slen ];

	cs = _deserialize_text_glyphs_trans_targs[_trans];

	if ( _deserialize_text_glyphs_trans_actions[_trans] == 0 )
		goto _again;

	switch ( _deserialize_text_glyphs_trans_actions[_trans] ) {
	case 6:
#line 51 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
	break;
	case 5:
#line 55 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 13:
#line 63 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
	break;
	case 14:
#line 64 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.x_offset )) return false; }
	break;
	case 16:
#line 65 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
	break;
	case 9:
#line 66 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
	break;
	case 11:
#line 67 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
	break;
	case 1:
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
	break;
	case 4:
#line 55 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 12:
#line 63 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 15:
#line 65 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 8:
#line 66 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 10:
#line 67 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 7:
#line 68 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_uint (tok, p, &info.mask    )) return false; }
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
	case 3:
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
#line 55 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 2:
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 51 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
#line 55 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info (info);
	if (unlikely (!buffer->successful))
	  return false;
	buffer->pos[buffer->len - 1] = pos;
	*end_ptr = p;
}
	break;
#line 513 "hb-buffer-deserialize-text-glyphs.hh"
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 124 "hb-buffer-deserialize-text-glyphs.rl"


  *end_ptr = p;

  return p == pe && *(p-1) != ']';
}

#endif /* HB_BUFFER_DESERIALIZE_TEXT_GLYPHS_HH */
