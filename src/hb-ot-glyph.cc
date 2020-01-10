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

#ifndef HB_NO_OT_GLYPH

#include "hb-ot.h"
#include "hb-ot-glyf-table.hh"
#include "hb-ot-cff1-table.hh"
#include "hb-ot-cff2-table.hh"

hb_bool_t
hb_ot_glyph_decompose (hb_font_t *font, hb_codepoint_t glyph,
		       hb_ot_glyph_decompose_funcs_t *funcs,
		       void *user_data)
{
  if (unlikely (!funcs || glyph >= font->face->get_num_glyphs ())) return false;

  if (font->face->table.glyf->get_path (font, glyph, funcs, user_data)) return true;
#ifndef HB_NO_CFF
  if (font->face->table.cff1->get_path (font, glyph, funcs, user_data)) return true;
  if (font->face->table.cff2->get_path (font, glyph, funcs, user_data)) return true;
#endif

  return false;
}

#endif
