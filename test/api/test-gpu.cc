/*
 * Copyright (C) 2026  Behdad Esfahbod
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

#include "hb-test.h"
#include "hb-gpu.h"
#include <hb-ot.h>

#define FONT_FILE "fonts/Roboto-Regular.abc.ttf"

static void
draw_rect (hb_gpu_draw_t *draw,
	   float x0, float y0, float x1, float y1)
{
  hb_draw_funcs_t *df = hb_gpu_draw_get_funcs ();
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

  hb_draw_move_to (df, draw, &st, x0, y0);
  hb_draw_line_to (df, draw, &st, x1, y0);
  hb_draw_line_to (df, draw, &st, x1, y1);
  hb_draw_line_to (df, draw, &st, x0, y1);
  hb_draw_close_path (df, draw, &st);
}

static uint16_t
blob_header_u16 (hb_blob_t *blob, unsigned short_index)
{
  unsigned length = 0;
  const char *data = hb_blob_get_data (blob, &length);

  g_assert_nonnull (data);
  g_assert_cmpuint (length, >=, 8 * sizeof (int16_t));

  const int16_t *words = (const int16_t *) (const void *) data;
  return (uint16_t) words[short_index];
}

static void
test_band_complexity_masks (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  draw_rect (draw, 0.f, 0.f, 40.f, 40.f);
  draw_rect (draw, 10.f, 10.f, 50.f, 50.f);

  hb_blob_t *blob = hb_gpu_draw_encode (draw);
  g_assert_nonnull (blob);

  g_assert_cmpuint (blob_header_u16 (blob, 6), !=, 0);
  g_assert_cmpuint (blob_header_u16 (blob, 7), !=, 0);

  hb_blob_destroy (blob);
  hb_gpu_draw_destroy (draw);
}

static void
test_band_complexity_masks_disjoint (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  draw_rect (draw, 0.f, 0.f, 20.f, 20.f);
  draw_rect (draw, 30.f, 0.f, 50.f, 20.f);

  hb_blob_t *blob = hb_gpu_draw_encode (draw);
  g_assert_nonnull (blob);

  g_assert_cmpuint (blob_header_u16 (blob, 6), ==, 0);
  g_assert_cmpuint (blob_header_u16 (blob, 7), ==, 0);

  hb_blob_destroy (blob);
  hb_gpu_draw_destroy (draw);
}

static void
test_create_destroy (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  hb_gpu_draw_t *ref = hb_gpu_draw_reference (draw);
  g_assert_true (ref == draw);
  hb_gpu_draw_destroy (ref);

  hb_gpu_draw_destroy (draw);
}

static void
test_empty_encode (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  /* Encode with no curves should return empty blob. */
  hb_blob_t *blob = hb_gpu_draw_encode (draw);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);

  /* Extents should be zero. */
  hb_glyph_extents_t ext;
  hb_gpu_draw_get_extents (draw, &ext);
  g_assert_cmpint (ext.x_bearing, ==, 0);
  g_assert_cmpint (ext.y_bearing, ==, 0);
  g_assert_cmpint (ext.width, ==, 0);
  g_assert_cmpint (ext.height, ==, 0);

  hb_gpu_draw_destroy (draw);
}

static void
test_encode_glyph (void)
{
  hb_face_t *face = hb_test_open_font_file (FONT_FILE);
  g_assert_nonnull (face);
  hb_font_t *font = hb_font_create (face);

  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  /* Draw glyph for 'a' (glyph index from cmap). */
  hb_codepoint_t gid;
  hb_bool_t found = hb_font_get_nominal_glyph (font, 'a', &gid);
  g_assert_true (found);

  hb_gpu_draw_glyph (draw, font, gid);

  /* Encode should produce non-empty blob. */
  hb_blob_t *blob = hb_gpu_draw_encode (draw);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);

  /* Blob size should be a multiple of 8 (RGBA16I texels). */
  g_assert_cmpuint (hb_blob_get_length (blob) % 8, ==, 0);

  /* Extents should be non-zero. */
  hb_glyph_extents_t ext;
  hb_gpu_draw_get_extents (draw, &ext);
  g_assert_cmpint (ext.width, >, 0);
  g_assert_cmpint (ext.height, <, 0); /* height is negative (y-down) */

  hb_blob_destroy (blob);
  hb_gpu_draw_destroy (draw);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_reset (void)
{
  hb_face_t *face = hb_test_open_font_file (FONT_FILE);
  hb_font_t *font = hb_font_create (face);

  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  hb_codepoint_t gid;
  hb_font_get_nominal_glyph (font, 'a', &gid);

  /* Draw, encode, reset, encode again — should work. */
  hb_gpu_draw_glyph (draw, font, gid);
  hb_blob_t *blob1 = hb_gpu_draw_encode (draw);
  g_assert_cmpuint (hb_blob_get_length (blob1), >, 0);
  hb_blob_destroy (blob1);

  hb_gpu_draw_reset (draw);

  /* After reset, encode should be empty. */
  hb_blob_t *blob2 = hb_gpu_draw_encode (draw);
  g_assert_cmpuint (hb_blob_get_length (blob2), ==, 0);
  hb_blob_destroy (blob2);

  /* Draw again after reset. */
  hb_gpu_draw_glyph (draw, font, gid);
  hb_blob_t *blob3 = hb_gpu_draw_encode (draw);
  g_assert_cmpuint (hb_blob_get_length (blob3), >, 0);
  hb_blob_destroy (blob3);

  hb_gpu_draw_destroy (draw);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_draw_funcs (void)
{
  hb_draw_funcs_t *funcs = hb_gpu_draw_get_funcs ();
  g_assert_nonnull (funcs);

  /* Should be the same singleton each time. */
  g_assert_true (funcs == hb_gpu_draw_get_funcs ());
}

static void
test_shader_sources (void)
{
  g_assert_nonnull (hb_gpu_shader_fragment_source (HB_GPU_SHADER_LANG_GLSL));
  g_assert_nonnull (hb_gpu_shader_vertex_source (HB_GPU_SHADER_LANG_GLSL));
}

static void
test_recycle_blob (void)
{
  hb_face_t *face = hb_test_open_font_file (FONT_FILE);
  hb_font_t *font = hb_font_create (face);

  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();

  hb_codepoint_t gid;
  hb_font_get_nominal_glyph (font, 'b', &gid);
  hb_gpu_draw_glyph (draw, font, gid);

  hb_blob_t *blob = hb_gpu_draw_encode (draw);
  g_assert_nonnull (blob);

  /* Recycle should not crash. */
  hb_gpu_draw_recycle_blob (draw, blob);

  hb_gpu_draw_destroy (draw);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_create_destroy);
  hb_test_add (test_empty_encode);
  hb_test_add (test_encode_glyph);
  hb_test_add (test_reset);
  hb_test_add (test_draw_funcs);
  hb_test_add (test_shader_sources);
  hb_test_add (test_recycle_blob);
  hb_test_add (test_band_complexity_masks);
  hb_test_add (test_band_complexity_masks_disjoint);

  return hb_test_run ();
}
