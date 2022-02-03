/*
 * Copyright © 2019-2020  Ebrahim Byagowi
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

#include "hb.hh"

#ifndef HB_NO_DRAW

#include "hb-ot-glyf-table.hh"
#include "hb-ot-cff1-table.hh"
#include "hb-ot-cff2-table.hh"

/**
 * hb_font_draw_glyph:
 * @font: a font object
 * @glyph: a glyph id
 * @funcs: draw callbacks object
 * @draw_data: parameter you like be passed to the callbacks when are called
 *
 * Draw a glyph.
 *
 * Returns: Whether the font had the glyph and the operation completed successfully.
 * Since: REPLACEME
 **/
void
hb_font_draw_glyph (hb_font_t *font,
		    hb_codepoint_t glyph,
		    hb_draw_funcs_t *funcs,
		    void *draw_data)
{
  if (unlikely (funcs == &Null (hb_draw_funcs_t) ||
		glyph >= font->face->get_num_glyphs ()))
    return;

  draw_helper_t draw_helper (funcs, draw_data);
  if (font->face->table.glyf->get_path (font, glyph, draw_helper)) return;
#ifndef HB_NO_CFF
  if (font->face->table.cff1->get_path (font, glyph, draw_helper)) return;
  if (font->face->table.cff2->get_path (font, glyph, draw_helper)) return;
#endif
}

#endif
