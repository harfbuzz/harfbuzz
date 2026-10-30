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

#include "hb-test.h"

#include <hb-cairo.h>

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define HB_TEST_CAIRO_TSAN 1
#endif
#endif
#if defined(__SANITIZE_THREAD__)
#define HB_TEST_CAIRO_TSAN 1
#endif

static void
test_hb_cairo_budget (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  /* Cairo's user-font rendering takes two internal mutexes in both orders.
   * The same workload reports this lock-order inversion against unmodified
   * HarfBuzz; keep race detection enabled rather than suppressing Cairo
   * deadlock reports process-wide. */
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif

  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/clusterfuzz-testcase-minimized-hb-vector-fuzzer-4886959609413632");
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_face (face);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
  cairo_t *cr = cairo_create (surface);

  cairo_set_font_face (cr, cairo_face);
  cairo_set_font_size (cr, 30);

  cairo_glyph_t glyph = {0, 0, 0};
  cairo_show_glyphs (cr, &glyph, 1);

  g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
  g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_font_face_destroy (cairo_face);
  hb_face_destroy (face);
}

static void
test_hb_cairo_color_glyph_cycle (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif

  /* Two COLRv1 glyphs that paint each other with PaintColrGlyph.  The
   * color_glyph callback used to show the nested glyph through cairo,
   * re-entering hb_font_paint_glyph() without the cycle and depth
   * guards of the paint walk in progress. */
  hb_face_t *face = hb_test_open_font_file ("fonts/test_glyphs-glyf_colr_1.ttf");
  hb_font_t *font = hb_font_create (face);
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_font (font);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
  cairo_t *cr = cairo_create (surface);

  cairo_set_font_face (cr, cairo_face);
  cairo_set_font_size (cr, 30);

  /* paintcolrglyph_cycle_first, paintcolrglyph_cycle_second */
  for (hb_codepoint_t u = 0xF1100; u <= 0xF1101; u++)
  {
    hb_codepoint_t gid;
    g_assert_true (hb_font_get_nominal_glyph (font, u, &gid));

    cairo_glyph_t glyph = {gid, 0, 30};
    cairo_show_glyphs (cr, &glyph, 1);
  }

  g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
  g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_font_face_destroy (cairo_face);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
face_destroyed (void *user_data)
{
  *(hb_bool_t *) user_data = 1;
}

static void
test_hb_cairo_face_lifecycle (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif

  static hb_user_data_key_t key;
  hb_bool_t destroyed = 0;

  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_set_user_data (face, &key, &destroyed, face_destroyed, 1);
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_face (face);
  hb_face_destroy (face);

  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
  cairo_t *cr = cairo_create (surface);

  cairo_set_font_face (cr, cairo_face);
  cairo_set_font_size (cr, 30);

  cairo_glyph_t glyph = {1, 0, 0};
  cairo_show_glyphs (cr, &glyph, 1);

  g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_font_face_destroy (cairo_face);

  /* Cairo keeps unreferenced scaled fonts cached; flush them so the
   * hb_font_t created for the scaled font, and with it the face, go away. */
  cairo_debug_reset_static_data ();
  g_assert_true (destroyed);
}

static void
test_hb_cairo_glyphs_from_buffer_clusters (void)
{
  /* Non-monotone clusters come out of hb_shape() for some inputs
   * (HB_BUFFER_CLUSTER_LEVEL_CHARACTERS, shaping that fails midway,
   * shaper bugs).  Cairo can only represent monotone clusters; make
   * sure such buffers map to one cluster instead of aborting. */
  static const struct {
    hb_direction_t direction;
    unsigned clusters[4];
    unsigned num_clusters;
  } tests[] = {
    {HB_DIRECTION_LTR, {0, 1, 1, 3}, 3},
    {HB_DIRECTION_RTL, {3, 1, 1, 0}, 3},
    /* Not monotone. */
    {HB_DIRECTION_LTR, {0, 2, 0, 0}, 1},
    {HB_DIRECTION_LTR, {3, 1, 1, 0}, 1},
    {HB_DIRECTION_RTL, {0, 0, 2, 0}, 1},
    {HB_DIRECTION_RTL, {0, 1, 1, 3}, 1},
  };
  const char *utf8 = "abcd";

  for (unsigned t = 0; t < G_N_ELEMENTS (tests); t++)
    for (unsigned utf8_clusters = 0; utf8_clusters < 2; utf8_clusters++)
    {
      hb_buffer_t *buffer = hb_buffer_create ();
      hb_buffer_set_direction (buffer, tests[t].direction);
      for (unsigned i = 0; i < 4; i++)
	hb_buffer_add (buffer, i, tests[t].clusters[i]);

      cairo_glyph_t *glyphs = NULL;
      unsigned num_glyphs = 0;
      cairo_text_cluster_t *clusters = NULL;
      unsigned num_clusters = 0;
      cairo_text_cluster_flags_t cluster_flags;
      hb_cairo_glyphs_from_buffer (buffer, utf8_clusters,
				   1., 1., 0., 0.,
				   utf8, -1,
				   &glyphs, &num_glyphs,
				   &clusters, &num_clusters,
				   &cluster_flags);

      g_assert_cmpuint (num_glyphs, ==, 4);
      g_assert_cmpuint (num_clusters, ==, tests[t].num_clusters);

      int total_bytes = 0, total_glyphs = 0;
      for (unsigned i = 0; i < num_clusters; i++)
      {
	g_assert_cmpint (clusters[i].num_bytes, >=, 0);
	g_assert_cmpint (clusters[i].num_glyphs, >, 0);
	total_bytes += clusters[i].num_bytes;
	total_glyphs += clusters[i].num_glyphs;
      }
      g_assert_cmpint (total_bytes, ==, 4);
      g_assert_cmpint (total_glyphs, ==, 4);

      cairo_glyph_free (glyphs);
      cairo_text_cluster_free (clusters);
      hb_buffer_destroy (buffer);
    }
}

/* One clip glyph of 300 points with 15830 solid paints under it.  Cairo
 * rasterizes the clip path anew for every paint, so the paints must be
 * charged to the glyph budget or this takes over a minute.  The test has
 * no discriminating assertion; it guards against reintroduction through
 * the meson test timeout, finishing in about a second once the paints
 * are charged. */
static void
test_hb_cairo_paint_budget (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif

  hb_face_t *face = hb_test_open_font_file ("fonts/colr-clip-paint-budget.ttf");
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_face (face);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 256, 256);
  cairo_t *cr = cairo_create (surface);

  cairo_set_font_face (cr, cairo_face);
  cairo_set_font_size (cr, 256);

  cairo_glyph_t glyph = {1, 0, 256};
  cairo_show_glyphs (cr, &glyph, 1);
  cairo_surface_flush (surface);

  g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
  g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

  /* The budget truncates the repeated paints but keeps the first ones. */
  const unsigned char *data = cairo_image_surface_get_data (surface);
  int stride = cairo_image_surface_get_stride (surface);
  gboolean painted = FALSE;
  for (int y = 0; y < 256 && !painted; y++)
    for (int x = 0; x < 256; x++)
      if (data[y * stride + x * 4 + 3])
      {
	painted = TRUE;
	break;
      }
  g_assert_true (painted);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_font_face_destroy (cairo_face);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_hb_cairo_budget);
  hb_test_add (test_hb_cairo_color_glyph_cycle);
  hb_test_add (test_hb_cairo_face_lifecycle);
  hb_test_add (test_hb_cairo_glyphs_from_buffer_clusters);
  hb_test_add (test_hb_cairo_paint_budget);

  return hb_test_run ();
}
