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
#include <stdlib.h>

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

/* Embedded bitmaps are not outline work, so their base64 output is bounded
 * by the document-size cap, not the outline budget: painting an oversized
 * image must keep the serialized document bounded instead of ballooning
 * with the image.  Exercised through SVG, whose image path base64-embeds
 * the raw blob directly; PDF assembles its whole document through the same
 * capped buffer (hb_vector_buf_t), but validates the PNG, so a bogus blob
 * cannot drive it here. */
static void
test_document_cap (void)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  unsigned len = 40u << 20;           /* 40 MB, well past the 16 MB cap */
  char *data = (char *) calloc (len, 1);
  assert (data);
  hb_blob_t *image = hb_blob_create (data, len, HB_MEMORY_MODE_READONLY, data, free);

  hb_glyph_extents_t ext = {0, 100, 100, -100};
  hb_vector_paint_set_glyph_extents (paint, &ext);
  hb_paint_image (funcs, paint, image, 100, 100,
		  HB_PAINT_IMAGE_FORMAT_PNG, 0.f, &ext);

  hb_blob_t *out = hb_vector_paint_render (paint);
  /* The cap either makes render refuse the over-budget document (null) or
   * bounds its size; either way it must not balloon to the base64 of the
   * whole 40 MB image (~53 MB), which is what an uncapped buffer produces. */
  assert (!out || hb_blob_get_length (out) < (32u << 20));
  hb_blob_destroy (out);
  hb_blob_destroy (image);

  /* Reusing the context after an over-cap glyph must recover: the sticky
   * overflow flag has to be cleared, or every later render fails forever. */
  hb_vector_paint_clear (paint);
  hb_vector_paint_set_glyph_extents (paint, &ext);
  unsigned small_len = 12;
  char *small = (char *) calloc (small_len, 1);
  assert (small);
  hb_blob_t *small_image = hb_blob_create (small, small_len, HB_MEMORY_MODE_READONLY, small, free);
  hb_paint_image (funcs, paint, small_image, 4, 3,
		  HB_PAINT_IMAGE_FORMAT_PNG, 0.f, &ext);
  hb_blob_t *out2 = hb_vector_paint_render (paint);
  assert (out2);
  hb_blob_destroy (out2);
  hb_blob_destroy (small_image);

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
  test_document_cap ();

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}
