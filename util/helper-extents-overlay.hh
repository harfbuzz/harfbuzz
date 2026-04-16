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

#ifndef HELPER_EXTENTS_OVERLAY_HH
#define HELPER_EXTENTS_OVERLAY_HH

#include <hb.h>

/* --show-extents overlay: emit a stroked rectangle into a paint
 * context via the arbitrary-path clip API plus hb_paint_color.
 * The paint backend's clip-path sink receives hb_draw_rect(),
 * which expands to an outer + inner contour whose coverage
 * ring is the stroke.  Then paint_color fills that clipped
 * region with the paint's current foreground so the overlay
 * takes on whatever color a draw-mode glyph would render in.
 *
 * Draw-only pipelines skip this helper — they just call
 * hb_draw_rect() on the glyph draw context so the rectangle
 * lands in the same output as the glyph outline. */
static inline void
util_emit_extents_rect_paint (hb_paint_funcs_t *pf, void *pd,
			      float x, float y, float w, float h,
			      float stroke_width)
{
  void *dd = nullptr;
  hb_draw_funcs_t *df = hb_paint_push_clip_path_start (pf, pd, &dd);
  if (!df) return;
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  hb_draw_rect (df, dd, &st, x, y, w, h, stroke_width);
  hb_paint_push_clip_path_end (pf, pd);
  hb_paint_color (pf, pd, /*is_foreground*/ 1, HB_COLOR (0, 0, 0, 255));
  hb_paint_pop_clip (pf, pd);
}

#endif /* HELPER_EXTENTS_OVERLAY_HH */
