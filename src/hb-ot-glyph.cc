/*
 * Copyright © 2019  Ebrahim Byagowi
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

unsigned int
hb_ot_glyph_get_outline_path (hb_font_t                *font,
			      hb_codepoint_t            glyph,
			      unsigned int              start_offset,
			      unsigned int             *points_count /* IN/OUT.  May be NULL. */,
			      hb_ot_glyph_path_point_t *points       /* OUT.     May be NULL. */)
{
  unsigned int points_to_write = likely (points && points_count) ? *points_count : 0;
  if (likely (points_count)) *points_count = 0;
  unsigned int all_points_count = 0;
  font->face->table.glyf->get_path (font, glyph,
				    [&] (char cmd, float x, float y)
				    {
				      all_points_count++;
				      if (start_offset) { start_offset--; return; }
				      if (points_to_write)
				      {
					points[*points_count] = {cmd, font->em_scalef_x (x), font->em_scalef_y (y)};
					*points_count += 1;
					points_to_write--;
				      }
				    });
  return all_points_count;
}

#endif
