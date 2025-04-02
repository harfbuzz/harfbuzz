
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


#line 36 "hb-buffer-deserialize-text-glyphs.hh"
static const unsigned char _deserialize_text_glyphs_trans_keys[] = {
	0u, 0u, 35u, 124u, 48u, 57u, 93u, 124u, 45u, 57u, 48u, 57u, 35u, 124u, 45u, 57u, 
	48u, 57u, 35u, 124u, 35u, 124u, 35u, 124u, 48u, 57u, 35u, 124u, 45u, 57u, 48u, 57u, 
	44u, 44u, 45u, 57u, 48u, 57u, 35u, 124u, 35u, 124u, 44u, 57u, 35u, 124u, 43u, 124u, 
	48u, 124u, 35u, 124u, 35u, 124u, 35u, 124u, 0
};

static const char _deserialize_text_glyphs_key_spans[] = {
	0, 90, 10, 32, 13, 10, 90, 13, 
	10, 90, 90, 90, 10, 90, 13, 10, 
	1, 13, 10, 90, 90, 14, 90, 82, 
	77, 90, 90, 90
};

static const short _deserialize_text_glyphs_index_offsets[] = {
	0, 0, 91, 102, 135, 149, 160, 251, 
	265, 276, 367, 458, 549, 560, 651, 665, 
	676, 678, 692, 703, 794, 885, 900, 991, 
	1074, 1152, 1243, 1334
};

static const char _deserialize_text_glyphs_indicies[] = {
	1, 0, 0, 0, 0, 0, 0, 
	0, 2, 3, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 4, 0, 0, 5, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 6, 7, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 7, 0, 8, 9, 9, 9, 
	9, 9, 9, 9, 9, 9, 3, 10, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 10, 3, 
	11, 3, 3, 12, 13, 13, 13, 13, 
	13, 13, 13, 13, 13, 3, 14, 15, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	3, 16, 3, 3, 3, 3, 3, 3, 
	3, 3, 17, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 18, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 18, 3, 19, 3, 3, 20, 
	21, 21, 21, 21, 21, 21, 21, 21, 
	21, 3, 22, 23, 23, 23, 23, 23, 
	23, 23, 23, 23, 3, 24, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 25, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 25, 3, 
	24, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 23, 23, 23, 
	23, 23, 23, 23, 23, 23, 23, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 25, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 25, 3, 16, 3, 3, 3, 3, 
	3, 3, 3, 3, 17, 3, 3, 3, 
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 18, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 18, 3, 26, 27, 
	27, 27, 27, 27, 27, 27, 27, 27, 
	3, 28, 3, 3, 3, 3, 3, 3, 
	3, 29, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 30, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 31, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 31, 3, 32, 3, 3, 33, 
	34, 34, 34, 34, 34, 34, 34, 34, 
	34, 3, 35, 36, 36, 36, 36, 36, 
	36, 36, 36, 36, 3, 37, 3, 38, 
	3, 3, 39, 40, 40, 40, 40, 40, 
	40, 40, 40, 40, 3, 41, 42, 42, 
	42, 42, 42, 42, 42, 42, 42, 3, 
	43, 3, 3, 3, 3, 3, 3, 3, 
	44, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 45, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 45, 3, 43, 3, 3, 3, 3, 
	3, 3, 3, 44, 3, 3, 3, 3, 
	42, 42, 42, 42, 42, 42, 42, 42, 
	42, 42, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 45, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 45, 3, 37, 3, 
	3, 3, 36, 36, 36, 36, 36, 36, 
	36, 36, 36, 36, 3, 28, 3, 3, 
	3, 3, 3, 3, 3, 29, 3, 3, 
	3, 3, 46, 46, 46, 46, 46, 46, 
	46, 46, 46, 46, 3, 3, 3, 3, 
	3, 3, 30, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 31, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 31, 3, 
	0, 0, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 0, 3, 3, 0, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 0, 0, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 0, 3, 47, 47, 47, 47, 47, 
	47, 47, 47, 47, 47, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	10, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 10, 
	3, 49, 48, 48, 48, 48, 48, 48, 
	48, 50, 3, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 51, 48, 48, 52, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 53, 54, 55, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 55, 48, 57, 56, 56, 56, 
	56, 56, 56, 56, 58, 3, 56, 56, 
	56, 56, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 56, 56, 56, 59, 56, 
	56, 60, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 56, 56, 61, 62, 56, 
	56, 56, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 56, 56, 56, 56, 56, 
	56, 56, 56, 56, 56, 62, 56, 63, 
	48, 48, 48, 48, 48, 48, 48, 64, 
	3, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 65, 48, 48, 66, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	54, 67, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	48, 48, 48, 48, 48, 48, 48, 48, 
	67, 48, 0
};

static const char _deserialize_text_glyphs_trans_targs[] = {
	1, 2, 4, 0, 12, 14, 23, 26, 
	3, 24, 26, 5, 6, 11, 6, 11, 
	2, 7, 26, 8, 9, 10, 9, 10, 
	2, 26, 13, 22, 2, 4, 14, 26, 
	15, 16, 21, 16, 21, 17, 18, 19, 
	20, 19, 20, 2, 4, 26, 22, 24, 
	1, 2, 4, 12, 14, 27, 23, 26, 
	1, 2, 4, 12, 14, 23, 26, 2, 
	4, 12, 14, 26
};

static const char _deserialize_text_glyphs_trans_actions[] = {
	0, 1, 1, 0, 1, 1, 0, 1, 
	2, 2, 3, 2, 2, 2, 0, 0, 
	4, 4, 4, 2, 2, 2, 0, 0, 
	5, 5, 2, 2, 6, 6, 6, 6, 
	2, 2, 2, 0, 0, 7, 2, 2, 
	2, 0, 0, 8, 8, 8, 0, 0, 
	9, 10, 10, 10, 10, 9, 9, 10, 
	12, 13, 13, 13, 13, 12, 13, 14, 
	14, 14, 14, 14
};

static const char _deserialize_text_glyphs_eof_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 11, 0
};

static const int deserialize_text_glyphs_start = 25;
static const int deserialize_text_glyphs_first_final = 25;
static const int deserialize_text_glyphs_error = 0;

static const int deserialize_text_glyphs_en_main = 25;


#line 98 "hb-buffer-deserialize-text-glyphs.rl"


static hb_bool_t
_hb_buffer_deserialize_text_glyphs (hb_buffer_t *buffer,
				    const char *buf,
				    unsigned int buf_len,
				    const char **end_ptr,
				    hb_font_t *font)
{
  const char *p = buf, *pe = buf + buf_len, *eof = pe;

  /* Ensure we have positions. */
  (void) hb_buffer_get_glyph_positions (buffer, nullptr);

  const char *tok = nullptr;
  int cs;
  hb_glyph_info_t info = {0};
  hb_glyph_position_t pos = {0};
  
#line 298 "hb-buffer-deserialize-text-glyphs.hh"
	{
	cs = deserialize_text_glyphs_start;
	}

#line 303 "hb-buffer-deserialize-text-glyphs.hh"
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
	case 2:
#line 50 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
	break;
	case 1:
#line 54 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 6:
#line 62 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_uint (tok, p, &info.cluster )) return false; }
	break;
	case 7:
#line 63 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.x_offset )) return false; }
	break;
	case 8:
#line 64 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.y_offset )) return false; }
	break;
	case 4:
#line 65 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.x_advance)) return false; }
	break;
	case 5:
#line 66 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_int  (tok, p, &pos.y_advance)) return false; }
	break;
	case 3:
#line 67 "hb-buffer-deserialize-text-glyphs.rl"
	{ if (!parse_uint (tok, p, &info.mask    )) return false; }
	break;
	case 9:
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 50 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
	break;
	case 10:
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 50 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
#line 54 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
	case 12:
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info_and_pos (info, pos);
	if (unlikely (!buffer->successful))
	  return false;
	*end_ptr = p;
}
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 50 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
	break;
	case 14:
#line 54 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 50 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
	break;
	case 13:
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info_and_pos (info, pos);
	if (unlikely (!buffer->successful))
	  return false;
	*end_ptr = p;
}
#line 38 "hb-buffer-deserialize-text-glyphs.rl"
	{
	hb_memset (&info, 0, sizeof (info));
	hb_memset (&pos , 0, sizeof (pos ));
}
#line 50 "hb-buffer-deserialize-text-glyphs.rl"
	{
	tok = p;
}
#line 54 "hb-buffer-deserialize-text-glyphs.rl"
	{
	/* TODO Unescape delimiters. */
	if (!hb_font_glyph_from_string (font,
					tok, p - tok,
					&info.codepoint))
	  return false;
}
	break;
#line 461 "hb-buffer-deserialize-text-glyphs.hh"
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	switch ( _deserialize_text_glyphs_eof_actions[cs] ) {
	case 11:
#line 43 "hb-buffer-deserialize-text-glyphs.rl"
	{
	buffer->add_info_and_pos (info, pos);
	if (unlikely (!buffer->successful))
	  return false;
	*end_ptr = p;
}
	break;
#line 482 "hb-buffer-deserialize-text-glyphs.hh"
	}
	}

	_out: {}
	}

#line 119 "hb-buffer-deserialize-text-glyphs.rl"


  *end_ptr = p;

  return p == pe;
}

#endif /* HB_BUFFER_DESERIALIZE_TEXT_GLYPHS_HH */
