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

#include <math.h>

/* --show-extents overlay helpers: a stroked rectangle around
 * each glyph's ink box, plus a filled dot at the glyph's pen
 * origin.  Both render in the foreground color.
 *
 * In DRAW mode, the geometry goes straight into the glyph's
 * draw context so it gets rendered/encoded alongside the
 * outline in the foreground color.
 *
 * In PAINT mode, the same geometry feeds an arbitrary-path
 * clip and the clipped region is filled with the paint's
 * current foreground; the draw-mode and paint-mode emitters
 * share all of the coord math. */

static inline void
util_emit_extents_overlay_into_draw (hb_draw_funcs_t *df, void *dd,
				     const hb_glyph_extents_t *ge,
				     float pen_x, float pen_y,
				     float stroke_width)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

  /* Stroked rectangle around the ink box.  Padded outward by
   * stroke_width / 2 so the stroke's inner edge sits at the
   * box, instead of straddling it (and the inner half
   * knocking out against glyph ink under non-zero winding).
   * copysignf on the padding keeps the shift outward
   * regardless of glyph-extent height-sign convention. */
  if (ge->width != 0 || ge->height != 0)
  {
    float half = 0.5f * stroke_width;
    float sx = copysignf (half, ge->width  ? (float) ge->width  : 1.f);
    float sy = copysignf (half, ge->height ? (float) ge->height : 1.f);
    hb_draw_rectangle (df, dd, &st,
		       pen_x + (float) ge->x_bearing  - sx,
		       pen_y + (float) ge->y_bearing  - sy,
		       (float) ge->width  + 2 * sx,
		       (float) ge->height + 2 * sy,
		       stroke_width);
  }

  /* Filled dot at the glyph origin (pen_x, pen_y).  Radius
   * scales with stroke_width so the dot stays visible at the
   * same render scale the stroke does. */
  hb_draw_circle (df, dd, &st,
		  pen_x, pen_y,
		  /*radius*/ 2.f * stroke_width,
		  /*stroke_width*/ NAN);
}

static inline void
util_emit_extents_overlay_into_paint (hb_paint_funcs_t *pf, void *pd,
				      const hb_glyph_extents_t *ge,
				      float pen_x, float pen_y,
				      float stroke_width)
{
  void *dd = nullptr;
  hb_draw_funcs_t *df = hb_paint_push_clip_path_start (pf, pd, &dd);
  if (!df) return;
  util_emit_extents_overlay_into_draw (df, dd, ge, pen_x, pen_y, stroke_width);
  hb_paint_push_clip_path_end (pf, pd);
  hb_paint_color (pf, pd, /*is_foreground*/ 1, HB_COLOR (0, 0, 0, 255));
  hb_paint_pop_clip (pf, pd);
}

#endif /* HELPER_EXTENTS_OVERLAY_HH */
