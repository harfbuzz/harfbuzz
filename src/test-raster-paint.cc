/*
 * Copyright © 2026  Behdad Esfahbod
 *
 * This is part of HarfBuzz, a text shaping library.
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

#include "hb-raster-paint.hh"

int
main ()
{
  hb_raster_paint_t *paint = hb_raster_paint_create_or_fail ();
  hb_always_assert (paint);
  hb_paint_funcs_t *funcs = hb_raster_paint_get_funcs (paint);
  hb_raster_extents_t root = {0, 0, 64, 64, 0};
  hb_raster_extents_t small_extents = {0, 0, 4, 4, 0};
  hb_raster_extents_t large = {0, 0, 8192, 8192, 0};

  hb_raster_paint_set_extents (paint, &root);
  hb_paint_push_transform (funcs, paint, 1, 0, 0, 1, 0, 0);
  hb_paint_pop_transform (funcs, paint);
  hb_always_assert (paint->surface_stack.length == 1);
  int64_t initial_budget = paint->pixel_remaining;

  hb_raster_paint_set_extents (paint, &small_extents);
  for (unsigned i = 0; i < 2; i++)
  {
    /* Both a newly allocated group and a cached group clear the root area. */
    int64_t before = paint->pixel_remaining;
    hb_paint_push_group (funcs, paint);
    hb_always_assert (paint->surface_stack.length == 2);
    hb_always_assert (paint->pixel_remaining == before - (int64_t) root.width * root.height);
    hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
  }

  /* Recharging must use the active root, without allocating the new size. */
  hb_raster_paint_set_extents (paint, &large);
  hb_always_assert (hb_paint_set_budget (funcs, paint, HB_BUDGET_DEFAULT));
  hb_always_assert (paint->pixel_remaining == initial_budget);

  /* An outline can surround the small visible surface while extending far
   * past the automatic dimension cap.  Rasterize its visible part: a mask
   * starting at -10000 and truncated to 4096 pixels misses the surface. */
  hb_raster_paint_clear (paint);
  hb_raster_paint_set_extents (paint, &root);
  void *draw_data = nullptr;
  hb_draw_funcs_t *draw_funcs = hb_paint_push_clip_path_start (funcs, paint, &draw_data);
  hb_always_assert (draw_funcs);
  hb_draw_state_t state = HB_DRAW_STATE_DEFAULT;
  hb_draw_move_to (draw_funcs, draw_data, &state, -10000.f, -10000.f);
  hb_draw_line_to (draw_funcs, draw_data, &state, 10000.f, -10000.f);
  hb_draw_line_to (draw_funcs, draw_data, &state, 10000.f, 10000.f);
  hb_draw_line_to (draw_funcs, draw_data, &state, -10000.f, 10000.f);
  hb_draw_close_path (draw_funcs, draw_data, &state);
  hb_paint_push_clip_path_end (funcs, paint);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  hb_raster_image_t *image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  hb_raster_extents_t extents;
  hb_raster_image_get_extents (image, &extents);
  hb_always_assert (extents.width == root.width && extents.height == root.height);
  const uint8_t *pixels = hb_raster_image_get_buffer (image);
  for (unsigned y = 0; y < root.height; y++)
    for (unsigned x = 0; x < root.width * 4; x++)
      hb_always_assert (pixels[y * extents.stride + x] == 255);
  hb_always_assert (paint->pixel_remaining >= 0);
  hb_raster_image_destroy (image);

  hb_raster_paint_destroy (paint);
  return 0;
}
