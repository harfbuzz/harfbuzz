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
#include <cmath>

#define FONT_FILE      "fonts/Roboto-Regular.abc.ttf"
#define COLR_FONT_FILE "fonts/test_glyphs-glyf_colr_1.ttf"

struct hb_gpu_test_texel_t
{
  int16_t r, g, b, a;
};

static const hb_gpu_test_texel_t *
blob_as_texels (hb_blob_t *blob,
		unsigned  *texel_count)
{
  unsigned length = hb_blob_get_length (blob);
  g_assert_cmpuint (length % sizeof (hb_gpu_test_texel_t), ==, 0);

  *texel_count = length / sizeof (hb_gpu_test_texel_t);
  /* hb_blob_get_data returns malloc'd memory which is suitably
   * aligned for any standard type; the cast is safe. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
  return (const hb_gpu_test_texel_t *) hb_blob_get_data (blob, nullptr);
#pragma GCC diagnostic pop
}

static hb_bool_t
blob_has_texel (hb_blob_t                   *blob,
		const hb_gpu_test_texel_t  needle)
{
  unsigned texel_count;
  const hb_gpu_test_texel_t *texels = blob_as_texels (blob, &texel_count);

  for (unsigned i = 0; i < texel_count; i++)
    if (texels[i].r == needle.r &&
	texels[i].g == needle.g &&
	texels[i].b == needle.b &&
	texels[i].a == needle.a)
      return true;

  return false;
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

  /* Encode with no curves should return empty blob and zero extents. */
  hb_glyph_extents_t ext;
  hb_blob_t *blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);

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

  /* Encode should produce non-empty blob with non-zero extents. */
  hb_glyph_extents_t ext;
  hb_blob_t *blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);

  /* Blob size should be a multiple of 8 (RGBA16I texels). */
  g_assert_cmpuint (hb_blob_get_length (blob) % 8, ==, 0);

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
  hb_blob_t *blob1 = hb_gpu_draw_encode (draw, nullptr);
  g_assert_cmpuint (hb_blob_get_length (blob1), >, 0);
  hb_blob_destroy (blob1);

  hb_gpu_draw_reset (draw);

  /* After reset, encode should be empty. */
  hb_blob_t *blob2 = hb_gpu_draw_encode (draw, nullptr);
  g_assert_cmpuint (hb_blob_get_length (blob2), ==, 0);
  hb_blob_destroy (blob2);

  /* Draw again after reset. */
  hb_gpu_draw_glyph (draw, font, gid);
  hb_blob_t *blob3 = hb_gpu_draw_encode (draw, nullptr);
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
  g_assert_nonnull (hb_gpu_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL));
  g_assert_nonnull (hb_gpu_shader_source (HB_GPU_SHADER_STAGE_VERTEX, HB_GPU_SHADER_LANG_GLSL));
  g_assert_nonnull (hb_gpu_draw_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL));
  g_assert_nonnull (hb_gpu_draw_shader_source (HB_GPU_SHADER_STAGE_VERTEX, HB_GPU_SHADER_LANG_GLSL));
}

static void
test_encode_quantizes_extents_outward (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  hb_draw_funcs_t *funcs = hb_gpu_draw_get_funcs ();
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  hb_draw_move_to (funcs, draw, &st, 0.24f, 0.24f);
  hb_draw_line_to (funcs, draw, &st, 0.76f, 0.24f);
  hb_draw_line_to (funcs, draw, &st, 0.76f, 0.76f);
  hb_draw_close_path (funcs, draw, &st);

  hb_blob_t *blob = hb_gpu_draw_encode (draw, nullptr);
  g_assert_nonnull (blob);

  unsigned texel_count;
  const hb_gpu_test_texel_t *texels = blob_as_texels (blob, &texel_count);
  g_assert_cmpuint (texel_count, >, 0);
  g_assert_cmpint (texels[0].r, ==, 0);
  g_assert_cmpint (texels[0].g, ==, 0);
  g_assert_cmpint (texels[0].b, ==, 4);
  g_assert_cmpint (texels[0].a, ==, 4);

  hb_blob_destroy (blob);
  hb_gpu_draw_destroy (draw);
}

static void
test_encode_preserves_touching_contours (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  hb_draw_funcs_t *funcs = hb_gpu_draw_get_funcs ();
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

  hb_draw_move_to (funcs, draw, &st, 0.f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 1.f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 0.f, 1.f);
  hb_draw_close_path (funcs, draw, &st);

  hb_draw_move_to (funcs, draw, &st, 0.f, 0.f);
  hb_draw_quadratic_to (funcs, draw, &st, -0.75f, 0.25f, -0.25f, -1.f);
  hb_draw_close_path (funcs, draw, &st);

  hb_blob_t *blob = hb_gpu_draw_encode (draw, nullptr);
  g_assert_nonnull (blob);

  g_assert_true (blob_has_texel (blob, {0, 0, -3, 1}));

  hb_blob_destroy (blob);
  hb_gpu_draw_destroy (draw);
}

static void
test_recycle_blob (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  hb_draw_funcs_t *funcs = hb_gpu_draw_get_funcs ();
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

  hb_draw_move_to (funcs, draw, &st, 0.f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 1.f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 0.f, 1.f);
  hb_draw_close_path (funcs, draw, &st);

  hb_blob_t *blob = hb_gpu_draw_encode (draw, nullptr);
  g_assert_nonnull (blob);
  unsigned first_length = hb_blob_get_length (blob);
  g_assert_cmpuint (first_length, >, 0);

  hb_gpu_draw_recycle_blob (draw, blob);

  hb_gpu_draw_reset (draw);

  hb_draw_move_to (funcs, draw, &st, 0.f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 2.f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 2.f, 2.f);
  hb_draw_line_to (funcs, draw, &st, 0.f, 2.f);
  hb_draw_close_path (funcs, draw, &st);
  hb_draw_move_to (funcs, draw, &st, 0.25f, 0.25f);
  hb_draw_quadratic_to (funcs, draw, &st, 1.f, 1.75f, 1.75f, 0.25f);
  hb_draw_quadratic_to (funcs, draw, &st, 1.f, -0.5f, 0.25f, 0.25f);
  hb_draw_close_path (funcs, draw, &st);

  hb_blob_t *blob2 = hb_gpu_draw_encode (draw, nullptr);
  g_assert_nonnull (blob2);
  g_assert_cmpuint (hb_blob_get_length (blob2), >, first_length);

  hb_blob_destroy (blob2);
  hb_gpu_draw_destroy (draw);
}

static void
test_extents_saturate_overflow (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);

  hb_draw_funcs_t *funcs = hb_gpu_draw_get_funcs ();
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

  hb_draw_move_to (funcs, draw, &st, -0x1p31f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 821.f, 0.f);
  hb_draw_line_to (funcs, draw, &st, 821.f, 1.f);
  hb_draw_close_path (funcs, draw, &st);

  hb_glyph_extents_t ext;
  hb_blob_t *blob = hb_gpu_draw_encode (draw, &ext);
  hb_blob_destroy (blob);

  g_assert_cmpint (ext.x_bearing, ==, G_MININT32);
  g_assert_cmpint (ext.y_bearing, ==, 1);
  g_assert_cmpint (ext.width, ==, G_MAXINT32);
  g_assert_cmpint (ext.height, ==, -1);

  hb_gpu_draw_destroy (draw);
}

/* ===== Paint encoder tests ===== */

static void
test_paint_create_destroy (void)
{
  hb_gpu_paint_t *p = hb_gpu_paint_create_or_fail ();
  g_assert_nonnull (p);

  hb_gpu_paint_t *ref = hb_gpu_paint_reference (p);
  g_assert_true (ref == p);
  hb_gpu_paint_destroy (ref);

  hb_gpu_paint_destroy (p);
}

static void
test_paint_get_funcs (void)
{
  hb_paint_funcs_t *funcs = hb_gpu_paint_get_funcs ();
  g_assert_nonnull (funcs);
  /* Singleton. */
  g_assert_true (funcs == hb_gpu_paint_get_funcs ());
}

static void
test_paint_shader_sources (void)
{
  /* Fragment helpers exist for all four supported languages. */
  g_assert_nonnull (hb_gpu_paint_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL));
  g_assert_nonnull (hb_gpu_paint_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_MSL));
  g_assert_nonnull (hb_gpu_paint_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_HLSL));
  g_assert_nonnull (hb_gpu_paint_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_WGSL));
}

static void
test_paint_empty_encode (void)
{
  /* Nothing painted -> empty blob (no ops to encode).  Reserved for
   * the "no ink" case; nullptr is only returned for unsupported
   * features. */
  hb_gpu_paint_t *p = hb_gpu_paint_create_or_fail ();
  g_assert_nonnull (p);
  hb_blob_t *blob = hb_gpu_paint_encode (p, nullptr);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);
  hb_gpu_paint_destroy (p);
}

static void
test_paint_encode_monochrome (void)
{
  /* hb_gpu_paint_glyph synthesizes a single foreground-colored
   * layer for non-color glyphs; the encoded blob should be
   * non-empty and 8-byte (RGBA16I texel) aligned. */
  hb_face_t *face = hb_test_open_font_file (FONT_FILE);
  g_assert_nonnull (face);
  hb_font_t *font = hb_font_create (face);

  hb_gpu_paint_t *p = hb_gpu_paint_create_or_fail ();
  g_assert_nonnull (p);

  hb_codepoint_t gid;
  g_assert_true (hb_font_get_nominal_glyph (font, 'a', &gid));
  g_assert_true (hb_gpu_paint_glyph (p, font, gid));

  hb_glyph_extents_t ext;
  hb_blob_t *blob = hb_gpu_paint_encode (p, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  g_assert_cmpuint (hb_blob_get_length (blob) % 8, ==, 0);
  g_assert_cmpint (ext.width, >, 0);
  g_assert_cmpint (ext.height, <, 0);

  hb_blob_destroy (blob);
  hb_gpu_paint_destroy (p);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_paint_encode_color (void)
{
  /* Real COLRv1 paint tree: scan for any color glyph that we
   * support end-to-end (the test font deliberately exercises
   * features we may flag as unsupported, so check several rather
   * than asserting on the first). */
  hb_face_t *face = hb_test_open_font_file (COLR_FONT_FILE);
  g_assert_nonnull (face);
  hb_font_t *font = hb_font_create (face);

  hb_gpu_paint_t *p = hb_gpu_paint_create_or_fail ();
  g_assert_nonnull (p);

  unsigned num = hb_face_get_glyph_count (face);
  for (unsigned i = 0; i < num; i++)
  {
    if (!hb_ot_color_glyph_has_paint (face, i))
      continue;
    /* paint_glyph's return is the supported flag; we don't gate
     * on it -- encode handles both cases (returns empty blob for
     * unsupported / empty, nullptr only on allocation failure). */
    hb_gpu_paint_glyph (p, font, i);
    hb_blob_t *blob = hb_gpu_paint_encode (p, nullptr);
    g_assert_nonnull (blob);
    g_assert_cmpuint (hb_blob_get_length (blob) % 8, ==, 0);
    hb_blob_destroy (blob);
  }
  /* Test exists to confirm we don't crash / corrupt while walking
   * a real COLRv1 paint tree, not that our encoder covers every
   * COLRv1 feature. */

  hb_gpu_paint_destroy (p);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_paint_auto_clear_on_encode (void)
{
  /* hb_gpu_paint_encode auto-clears so the encoder can be reused
   * straight away; user configuration (palette) is preserved. */
  hb_face_t *face = hb_test_open_font_file (FONT_FILE);
  hb_font_t *font = hb_font_create (face);

  hb_gpu_paint_t *p = hb_gpu_paint_create_or_fail ();
  hb_gpu_paint_set_palette (p, 3);
  g_assert_cmpuint (hb_gpu_paint_get_palette (p), ==, 3);

  hb_codepoint_t gid;
  hb_font_get_nominal_glyph (font, 'a', &gid);

  hb_gpu_paint_glyph (p, font, gid);
  hb_blob_t *blob1 = hb_gpu_paint_encode (p, nullptr);
  g_assert_nonnull (blob1);
  hb_blob_destroy (blob1);

  /* Auto-clear: re-encoding without painting again returns the
   * empty-blob singleton (nothing accumulated). */
  hb_blob_t *empty = hb_gpu_paint_encode (p, nullptr);
  g_assert_nonnull (empty);
  g_assert_cmpuint (hb_blob_get_length (empty), ==, 0);
  hb_blob_destroy (empty);

  /* Palette config is preserved. */
  g_assert_cmpuint (hb_gpu_paint_get_palette (p), ==, 3);

  /* Paint another glyph -- works because state is clean and config
   * is still in effect. */
  hb_font_get_nominal_glyph (font, 'b', &gid);
  hb_gpu_paint_glyph (p, font, gid);
  hb_blob_t *blob2 = hb_gpu_paint_encode (p, nullptr);
  g_assert_nonnull (blob2);
  hb_blob_destroy (blob2);

  hb_gpu_paint_destroy (p);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_paint_reset (void)
{
  /* paint_reset wipes user configuration too (vs. paint_clear which
   * keeps it). */
  hb_gpu_paint_t *p = hb_gpu_paint_create_or_fail ();
  hb_gpu_paint_set_palette (p, 5);
  g_assert_cmpuint (hb_gpu_paint_get_palette (p), ==, 5);
  hb_gpu_paint_reset (p);
  g_assert_cmpuint (hb_gpu_paint_get_palette (p), ==, 0);
  hb_gpu_paint_destroy (p);
}

static void
test_paint_recycle_blob (void)
{
  hb_face_t *face = hb_test_open_font_file (FONT_FILE);
  hb_font_t *font = hb_font_create (face);

  hb_gpu_paint_t *p = hb_gpu_paint_create_or_fail ();
  hb_codepoint_t gid;
  hb_font_get_nominal_glyph (font, 'a', &gid);

  hb_gpu_paint_glyph (p, font, gid);
  hb_blob_t *blob = hb_gpu_paint_encode (p, nullptr);
  g_assert_nonnull (blob);
  hb_gpu_paint_recycle_blob (p, blob);

  /* Encoder still works after recycling. */
  hb_gpu_paint_glyph (p, font, gid);
  hb_blob_t *blob2 = hb_gpu_paint_encode (p, nullptr);
  g_assert_nonnull (blob2);
  hb_blob_destroy (blob2);

  hb_gpu_paint_destroy (p);
  hb_font_destroy (font);
  hb_face_destroy (face);
}


static void
test_shapes (void)
{
  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (draw);
  /* Use a reasonable scale so the coordinates below stay in
   * the encoder's representable range. */
  hb_gpu_draw_set_scale (draw, 1000, 1000);

  /* Tapered line. */
  hb_gpu_draw_line (draw,
		    10.f, 10.f, 4.f,
		    50.f, 30.f, 2.f);
  hb_glyph_extents_t ext;
  hb_blob_t *blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  g_assert_cmpint (ext.width, >, 0);
  hb_blob_destroy (blob);

  /* Degenerate line (zero length) is silently a no-op. */
  hb_gpu_draw_line (draw,
		    10.f, 10.f, 3.f,
		    10.f, 10.f, 3.f);
  blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);

  /* Filled rect (stroke_width = NaN). */
  hb_gpu_draw_rect (draw, 0.f, 0.f, 20.f, 10.f,
		    (float) std::nan (""));
  blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  g_assert_cmpint (ext.width, ==, 20);
  hb_blob_destroy (blob);

  /* Stroked rect: outer edge extends stroke_width/2 beyond the
   * nominal rect on each side. */
  hb_gpu_draw_rect (draw, 0.f, 0.f, 20.f, 10.f, 2.f);
  blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  g_assert_cmpint (ext.width, ==, 22);
  hb_blob_destroy (blob);

  /* Filled circle. */
  hb_gpu_draw_circle (draw, 50.f, 50.f, 25.f,
		      (float) std::nan (""));
  blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  hb_blob_destroy (blob);

  /* Stroked circle. */
  hb_gpu_draw_circle (draw, 50.f, 50.f, 25.f, 2.f);
  blob = hb_gpu_draw_encode (draw, &ext);
  g_assert_nonnull (blob);
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  hb_blob_destroy (blob);

  hb_gpu_draw_destroy (draw);
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
  hb_test_add (test_encode_quantizes_extents_outward);
  hb_test_add (test_encode_preserves_touching_contours);
  hb_test_add (test_recycle_blob);
  hb_test_add (test_extents_saturate_overflow);
  hb_test_add (test_shapes);

  hb_test_add (test_paint_create_destroy);
  hb_test_add (test_paint_get_funcs);
  hb_test_add (test_paint_shader_sources);
  hb_test_add (test_paint_empty_encode);
  hb_test_add (test_paint_encode_monochrome);
  hb_test_add (test_paint_encode_color);
  hb_test_add (test_paint_auto_clear_on_encode);
  hb_test_add (test_paint_reset);
  hb_test_add (test_paint_recycle_blob);

  return hb_test_run ();
}
