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

  hb_raster_paint_destroy (paint);
  return 0;
}
