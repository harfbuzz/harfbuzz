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
 */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include <hb.h>
#include <hb-vector.h>

static void
test_format (hb_vector_format_t format, hb_font_t *font)
{
  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (format);
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (draw);
  assert (paint);

  hb_draw_funcs_t *draw_funcs = hb_vector_draw_get_funcs (draw);
  hb_paint_funcs_t *paint_funcs = hb_vector_paint_get_funcs (paint);

  assert (hb_draw_get_budget (draw_funcs, draw) == HB_BUDGET_DEFAULT);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) > 0);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) < HB_BUDGET_UNLIMITED);
  assert (hb_paint_get_budget (paint_funcs, paint) == HB_BUDGET_DEFAULT);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) > 0);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) < HB_BUDGET_UNLIMITED);

  assert (hb_draw_set_budget (draw_funcs, draw, 1));
  hb_vector_draw_glyph (draw, font, 0, HB_VECTOR_EXTENTS_MODE_NONE);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) < 0);
  hb_vector_draw_clear (draw);
  assert (hb_draw_get_budget (draw_funcs, draw) == 1);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) == 1);
  hb_vector_draw_reset (draw);
  assert (hb_draw_get_budget (draw_funcs, draw) == HB_BUDGET_DEFAULT);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) > 0);

  assert (hb_paint_set_budget (paint_funcs, paint, 1));
  hb_vector_paint_glyph (paint, font, 0, HB_VECTOR_EXTENTS_MODE_NONE);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) < 0);
  hb_vector_paint_clear (paint);
  assert (hb_paint_get_budget (paint_funcs, paint) == 1);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) == 1);
  hb_vector_paint_reset (paint);
  assert (hb_paint_get_budget (paint_funcs, paint) == HB_BUDGET_DEFAULT);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) > 0);

  assert (hb_draw_set_budget (draw_funcs, draw, HB_BUDGET_UNLIMITED));
  assert (hb_draw_get_budget (draw_funcs, draw) == HB_BUDGET_UNLIMITED);
  assert (hb_draw_get_budget_remaining (draw_funcs, draw) == HB_BUDGET_UNLIMITED);
  assert (hb_paint_set_budget (paint_funcs, paint, HB_BUDGET_UNLIMITED));
  assert (hb_paint_get_budget (paint_funcs, paint) == HB_BUDGET_UNLIMITED);
  assert (hb_paint_get_budget_remaining (paint_funcs, paint) == HB_BUDGET_UNLIMITED);

  hb_vector_draw_destroy (draw);
  hb_vector_paint_destroy (paint);
}

int
main (int argc, char **argv)
{
  assert (argc == 2);
  hb_blob_t *blob = hb_blob_create_from_file_or_fail (argv[1]);
  assert (blob);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  test_format (HB_VECTOR_FORMAT_SVG, font);
  test_format (HB_VECTOR_FORMAT_PDF, font);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}
