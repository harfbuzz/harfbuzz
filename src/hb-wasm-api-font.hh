/*
 * Copyright © 2023  Behdad Esfahbod
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
 */

#ifndef HB_WASM_API_FONT_HH
#define HB_WASM_API_FONT_HH

#include "hb-wasm-api.hh"

namespace hb {
namespace wasm {


ptr_t(face_t)
font_get_face (HB_WASM_EXEC_ENV
	       ptr_t(font_t) fontref)
{
  HB_REF2OBJ (font);

  hb_face_t *face = hb_font_get_face (font);

  HB_OBJ2REF (face);
  return faceref;
}

void
font_get_scale (HB_WASM_EXEC_ENV
		ptr_t(font_t) fontref,
		ptr_t(int32_t) x_scaleptr,
		ptr_t(int32_t) y_scaleptr)
{
  HB_REF2OBJ (font);

  HB_OUT_PARAM(int32_t, x_scale);
  HB_OUT_PARAM(int32_t, y_scale);

  hb_font_get_scale (font, x_scale, y_scale);
}

hb_codepoint_t
font_get_glyph (HB_WASM_EXEC_ENV
		ptr_t(font_t) fontref,
		hb_codepoint_t unicode,
		hb_codepoint_t variation_selector)
{
  HB_REF2OBJ (font);
  hb_codepoint_t glyph;

  hb_font_get_glyph (font, unicode, variation_selector, &glyph);
  return glyph;
}

hb_position_t
font_get_glyph_h_advance (HB_WASM_EXEC_ENV
			  ptr_t(font_t) fontref,
			  hb_codepoint_t glyph)
{
  HB_REF2OBJ (font);
  return hb_font_get_glyph_h_advance (font, glyph);
}

hb_position_t
font_get_glyph_v_advance (HB_WASM_EXEC_ENV
			  ptr_t(font_t) fontref,
			  hb_codepoint_t glyph)
{
  HB_REF2OBJ (font);
  return hb_font_get_glyph_v_advance (font, glyph);
}

}}

#endif /* HB_WASM_API_FONT_HH */
