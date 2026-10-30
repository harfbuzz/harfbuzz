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

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_hb_cairo_budget);
  hb_test_add (test_hb_cairo_color_glyph_cycle);
  hb_test_add (test_hb_cairo_face_lifecycle);

  return hb_test_run ();
}
