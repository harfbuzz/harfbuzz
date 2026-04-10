/*
 * Copyright © 2026  Behdad Esfahbod
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
 */

#include "hb.hh"
#include "hb-geometry.hh"


int
main (int argc HB_UNUSED, char **argv HB_UNUSED)
{
  {
    hb_extents_t<> extents (-0x1p31f, 0.f, 427.f, 1.f);
    hb_glyph_extents_t glyph_extents = extents.to_glyph_extents ();

    hb_always_assert (glyph_extents.x_bearing == hb_int_min (hb_position_t));
    hb_always_assert (glyph_extents.y_bearing == 1);
    hb_always_assert (glyph_extents.width == hb_int_max (hb_position_t));
    hb_always_assert (glyph_extents.height == -1);
  }

  {
    hb_glyph_extents_t glyph_extents = {
      hb_int_max (hb_position_t),
      7,
      1,
      -3
    };
    hb_extents_t<double> extents (glyph_extents);

    hb_always_assert (extents.xmin == (double) hb_int_max (hb_position_t));
    hb_always_assert (extents.xmax == (double) hb_int_max (hb_position_t) + 1);
    hb_always_assert (extents.ymin == 4);
    hb_always_assert (extents.ymax == 7);
  }

  return 0;
}
