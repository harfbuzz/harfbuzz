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
 * Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-raster.hh"

#include <math.h>


/**
 * SECTION:hb-raster
 * @title: hb-raster
 * @short_description: Glyph rasterization
 * @include: hb-raster.h
 *
 * Functions for rasterizing glyph outlines into pixel buffers.
 *
 * The #hb_raster_draw_t object accumulates glyph outlines via
 * #hb_draw_funcs_t callbacks obtained from hb_raster_draw_get_funcs(),
 * then produces an #hb_raster_image_t with hb_raster_draw_render().
 **/

/**
 * hb_raster_extents_from_glyph_extents:
 * @glyph_extents: glyph extents from hb_font_get_glyph_extents()
 * @extents: (out): output raster extents
 *
 * Converts #hb_glyph_extents_t to #hb_raster_extents_t.
 *
 * This helper computes a tight raster box using floor/ceil bounds with
 * no extra padding, and sets @extents->stride to zero.
 *
 * Typical usage for paint:
 * ```
 * hb_glyph_extents_t gext;
 * hb_raster_extents_t ext;
 * if (hb_font_get_glyph_extents (font, gid, &gext) &&
 *     hb_raster_extents_from_glyph_extents (&gext, &ext))
 * {
 *   hb_raster_paint_set_extents (paint, &ext);
 *   hb_font_paint_glyph (font, gid, hb_raster_paint_get_funcs (), paint, 0, fg);
 * }
 * ```
 *
 * Return value: `true` if extents are non-empty and written to @extents;
 * `false` otherwise.
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_raster_extents_from_glyph_extents (const hb_glyph_extents_t *glyph_extents,
				      hb_raster_extents_t      *extents)
{
  if (unlikely (!glyph_extents || !extents))
    return false;

  int x0 = hb_min (glyph_extents->x_bearing,
		   glyph_extents->x_bearing + glyph_extents->width);
  int y0 = hb_min (glyph_extents->y_bearing,
		   glyph_extents->y_bearing + glyph_extents->height);
  int x1 = hb_max (glyph_extents->x_bearing,
		   glyph_extents->x_bearing + glyph_extents->width);
  int y1 = hb_max (glyph_extents->y_bearing,
		   glyph_extents->y_bearing + glyph_extents->height);

  /* Keep floor/ceil semantics explicit in case extents source changes. */
  x0 = (int) floorf ((float) x0);
  y0 = (int) floorf ((float) y0);
  x1 = (int) ceilf  ((float) x1);
  y1 = (int) ceilf  ((float) y1);

  if (x1 <= x0 || y1 <= y0)
  {
    *extents = {0, 0, 0, 0, 0};
    return false;
  }

  *extents = {
    x0, y0,
    (unsigned) (x1 - x0),
    (unsigned) (y1 - y0),
    0
  };
  return true;
}
