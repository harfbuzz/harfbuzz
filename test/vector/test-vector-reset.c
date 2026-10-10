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
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

static void
test_draw_reset (hb_vector_format_t format)
{
  hb_vector_draw_t *fresh = hb_vector_draw_create_or_fail (format);
  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (format);
  assert (fresh && draw);
  hb_color_t foreground = HB_COLOR (0, 0, 255, 128);
  hb_color_t background = HB_COLOR (0, 255, 0, 255);

  hb_vector_draw_set_foreground (draw, foreground);
  hb_vector_draw_set_background (draw, background);
  hb_vector_draw_clear (draw);
  assert (hb_vector_draw_get_foreground (draw) == foreground);
  assert (hb_vector_draw_get_background (draw) == background);

  hb_vector_draw_reset (draw);
  assert (hb_vector_draw_get_format (draw) == format);
  assert (hb_vector_draw_get_foreground (draw) == hb_vector_draw_get_foreground (fresh));
  assert (hb_vector_draw_get_background (draw) == hb_vector_draw_get_background (fresh));
  hb_vector_draw_destroy (draw);
  hb_vector_draw_destroy (fresh);
}

static void
test_paint_reset (hb_vector_format_t format)
{
  hb_vector_paint_t *fresh = hb_vector_paint_create_or_fail (format);
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (fresh && paint);
  hb_color_t foreground = HB_COLOR (0, 0, 255, 128);
  hb_color_t background = HB_COLOR (0, 255, 0, 255);
  hb_color_t custom = HB_COLOR (255, 0, 0, 255);
  hb_color_t color;
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  hb_vector_paint_set_foreground (paint, foreground);
  hb_vector_paint_set_background (paint, background);
  hb_vector_paint_set_palette (paint, 2);
  hb_vector_paint_set_svg_prefix (paint, "old-");
  hb_vector_paint_set_custom_palette_color (paint, 7, custom);
  hb_vector_paint_clear (paint);
  assert (hb_vector_paint_get_foreground (paint) == foreground);
  assert (hb_vector_paint_get_background (paint) == background);
  assert (hb_vector_paint_get_palette (paint) == 2);
  assert (!strcmp (hb_vector_paint_get_svg_prefix (paint), "old-"));
  if (format == HB_VECTOR_FORMAT_SVG)
  {
    assert (hb_paint_custom_palette_color (funcs, paint, 7, &color));
    assert (color == custom);
  }

  hb_vector_paint_reset (paint);
  assert (hb_vector_paint_get_format (paint) == format);
  assert (hb_vector_paint_get_foreground (paint) == hb_vector_paint_get_foreground (fresh));
  assert (hb_vector_paint_get_background (paint) == hb_vector_paint_get_background (fresh));
  assert (hb_vector_paint_get_palette (paint) == hb_vector_paint_get_palette (fresh));
  assert (!strcmp (hb_vector_paint_get_svg_prefix (paint), hb_vector_paint_get_svg_prefix (fresh)));
  if (format == HB_VECTOR_FORMAT_SVG)
    assert (!hb_paint_custom_palette_color (funcs, paint, 7, &color));

  hb_vector_paint_destroy (paint);
  hb_vector_paint_destroy (fresh);
}

static void
draw_triangle (hb_vector_draw_t *draw)
{
  hb_vector_extents_t extents = {0, 0, 10, 10};
  hb_vector_draw_set_extents (draw, &extents);
  hb_draw_funcs_t *funcs = hb_vector_draw_get_funcs (draw);
  hb_draw_state_t state = HB_DRAW_STATE_DEFAULT;
  hb_draw_move_to (funcs, draw, &state, 0, 0);
  hb_draw_line_to (funcs, draw, &state, 10, 0);
  hb_draw_line_to (funcs, draw, &state, 0, 10);
  hb_draw_close_path (funcs, draw, &state);
  hb_vector_draw_new_path (draw);
}

static void
assert_blob_equal (hb_blob_t *output, hb_blob_t *expected)
{
  assert (output && expected);
  unsigned int length, expected_length;
  const char *data = hb_blob_get_data (output, &length);
  const char *expected_data = hb_blob_get_data (expected, &expected_length);
  assert (length && length == expected_length);
  assert (!memcmp (data, expected_data, length));
  hb_blob_destroy (output);
  hb_blob_destroy (expected);
}

static void
test_draw_render_failure (hb_vector_format_t format)
{
  hb_vector_draw_t *fresh = hb_vector_draw_create_or_fail (format);
  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (format);
  assert (fresh && draw);
  hb_color_t foreground = HB_COLOR (0, 0, 255, 128);
  hb_vector_draw_set_foreground (fresh, foreground);
  hb_vector_draw_set_foreground (draw, foreground);

  /* Discard content, including PDF opacity resources, on failure. */
  draw_triangle (draw);
  hb_vector_draw_set_extents (draw, NULL);
  assert (!hb_vector_draw_render (draw));
  assert (hb_vector_draw_get_foreground (draw) == foreground);

  draw_triangle (draw);
  draw_triangle (fresh);
  assert_blob_equal (hb_vector_draw_render (draw), hb_vector_draw_render (fresh));
  assert (!hb_vector_draw_get_extents (draw, NULL));
  hb_vector_draw_destroy (draw);
  hb_vector_draw_destroy (fresh);
}

static void
paint_rectangle (hb_vector_paint_t *paint, hb_color_t color)
{
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);
  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 10, 10);
  hb_paint_color (funcs, paint, 0, color);
  hb_paint_pop_clip (funcs, paint);
}

static void
test_paint_render_failure (hb_vector_format_t format)
{
  hb_vector_paint_t *fresh = hb_vector_paint_create_or_fail (format);
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (fresh && paint);
  hb_vector_extents_t extents = {0, 0, 10, 10};
  hb_vector_paint_set_svg_prefix (fresh, "kept-");
  hb_vector_paint_set_svg_prefix (paint, "kept-");

  /* An empty PDF render fails even when extents are available. */
  hb_vector_paint_set_extents (paint, &extents);
  hb_blob_t *empty = hb_vector_paint_render (paint);
  if (format == HB_VECTOR_FORMAT_PDF)
    assert (!empty);
  else
    assert (empty);
  hb_blob_destroy (empty);
  assert (!hb_vector_paint_get_extents (paint, NULL));

  /* Missing extents must discard the old content and resources too. */
  paint_rectangle (paint, HB_COLOR (255, 0, 0, 128));
  assert (!hb_vector_paint_render (paint));
  assert (!strcmp (hb_vector_paint_get_svg_prefix (paint), "kept-"));

  hb_vector_paint_set_extents (paint, &extents);
  hb_vector_paint_set_extents (fresh, &extents);
  paint_rectangle (paint, HB_COLOR (0, 0, 255, 128));
  paint_rectangle (fresh, HB_COLOR (0, 0, 255, 128));
  assert_blob_equal (hb_vector_paint_render (paint), hb_vector_paint_render (fresh));
  assert (!hb_vector_paint_get_extents (paint, NULL));
  hb_vector_paint_destroy (paint);
  hb_vector_paint_destroy (fresh);
}

static void
test_pdf_clear (void)
{
  hb_vector_draw_t *fresh = hb_vector_draw_create_or_fail (HB_VECTOR_FORMAT_PDF);
  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (HB_VECTOR_FORMAT_PDF);
  assert (fresh && draw);
  hb_color_t foreground = HB_COLOR (0, 0, 0, 128);
  hb_vector_draw_set_foreground (fresh, foreground);
  hb_vector_draw_set_foreground (draw, foreground);
  draw_triangle (fresh);
  hb_blob_t *expected = hb_vector_draw_render (fresh);
  assert (expected);
  unsigned int expected_length;
  const char *expected_data = hb_blob_get_data (expected, &expected_length);
  assert (expected_length);

  /* Discard a path whose opacity resource has already been emitted. */
  draw_triangle (draw);
  hb_vector_draw_clear (draw);
  for (unsigned int i = 0; i < 3; i++)
  {
    draw_triangle (draw);
    hb_blob_t *output = hb_vector_draw_render (draw);
    assert (output);
    unsigned int length;
    const char *data = hb_blob_get_data (output, &length);
    assert (length == expected_length);
    assert (!memcmp (data, expected_data, length));
    hb_blob_destroy (output);
    /* Subsequent iterations rely on render's automatic clear. */
  }

  /* Reset must discard PDF resources too, while restoring the colors. */
  draw_triangle (draw);
  hb_vector_draw_reset (draw);
  hb_vector_draw_set_foreground (draw, foreground);
  draw_triangle (draw);
  hb_blob_t *output = hb_vector_draw_render (draw);
  assert (output);
  unsigned int length;
  const char *data = hb_blob_get_data (output, &length);
  assert (length == expected_length);
  assert (!memcmp (data, expected_data, length));
  hb_blob_destroy (output);
  hb_blob_destroy (expected);
  hb_vector_draw_destroy (draw);
  hb_vector_draw_destroy (fresh);
}

int
main (void)
{
  test_draw_reset (HB_VECTOR_FORMAT_SVG);
  test_draw_reset (HB_VECTOR_FORMAT_PDF);
  test_paint_reset (HB_VECTOR_FORMAT_SVG);
  test_paint_reset (HB_VECTOR_FORMAT_PDF);
  test_pdf_clear ();
  test_draw_render_failure (HB_VECTOR_FORMAT_SVG);
  test_draw_render_failure (HB_VECTOR_FORMAT_PDF);
  test_paint_render_failure (HB_VECTOR_FORMAT_SVG);
  test_paint_render_failure (HB_VECTOR_FORMAT_PDF);
  return 0;
}
