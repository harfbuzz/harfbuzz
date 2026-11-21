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
test_hb_cairo_transform_no_inverse (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif
  if (cairo_version () < CAIRO_VERSION_ENCODE (1, 18, 0))
  {
    g_test_skip ("This cairo has no color user fonts");
    return;
  }

  /* Each glyph paints a red bar on the left, then a green square
   * under a transform, then a blue bar on the right.  In all but the
   * last glyph the transform has no inverse: a scale by zero, say, or
   * forty scales one in another that come to zero.  What it paints
   * has no area, so the glyph is the two bars.  cairo refuses such a
   * matrix, and a context that has refused anything draws nothing
   * more: not the blue bar, and not what the application shows after
   * the glyph. */
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-transform-no-inverse.ttf");
  hb_font_t *font = hb_font_create (face);
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_font (font);

  for (hb_codepoint_t u = 'a'; u <= 'h'; u++)
  {
    hb_codepoint_t gid;
    g_assert_true (hb_font_get_nominal_glyph (font, u, &gid));

    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
    cairo_t *cr = cairo_create (surface);
    cairo_set_font_face (cr, cairo_face);
    cairo_set_font_size (cr, 64);

    cairo_glyph_t glyph = {gid, 0, 64};
    cairo_show_glyphs (cr, &glyph, 1);

    g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
    g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

    /* The middle row, as 0xAARRGGBB: in the red bar, in the middle of
     * the square, and in the blue bar. */
    cairo_surface_flush (surface);
    uint32_t row[64];
    memcpy (row,
	    cairo_image_surface_get_data (surface) +
	    32 * cairo_image_surface_get_stride (surface),
	    sizeof (row));
    g_assert_cmphex (row[8], ==, 0xFFFF0000u);
    g_assert_cmphex (row[32], ==, u == 'h' ? 0xFF00FF00u : 0u);
    g_assert_cmphex (row[56], ==, 0xFF0000FFu);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
  }

  cairo_font_face_destroy (cairo_face);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

/* Paints any glyph as an image of no width. */
static hb_bool_t
paint_image_of_no_width (hb_font_t *font HB_UNUSED,
			 void *font_data HB_UNUSED,
			 hb_codepoint_t glyph HB_UNUSED,
			 hb_paint_funcs_t *paint_funcs,
			 void *paint_data,
			 unsigned int palette_index HB_UNUSED,
			 hb_color_t foreground HB_UNUSED,
			 void *user_data HB_UNUSED)
{
  hb_glyph_extents_t extents = {0, 1000, 1000, -1000};
  hb_paint_image (paint_funcs, paint_data, hb_blob_get_empty (), 0, 8,
		  HB_PAINT_IMAGE_FORMAT_BGRA, 0.f, &extents);
  return TRUE;
}

static void
test_hb_cairo_image_not_paintable (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif
  if (cairo_version () < CAIRO_VERSION_ENCODE (1, 18, 0))
  {
    g_test_skip ("This cairo has no color user fonts");
    return;
  }

  /* Glyphs that are images cairo cannot paint: two whose PNG data
   * cannot be read, one shown so small that its extents come to
   * nothing, and one that font functions paint as an image of no
   * width.  cairo refuses the surface of the first two and a scale by
   * zero for the others, and a context that has refused anything
   * draws nothing more. */
  static const struct
  {
    const char *font;
    hb_codepoint_t unicode;
    double size;
  } cases[] =
  {
    {"../fuzzing/fonts/sbix-png-plte-chunklen-oob.ttf", 0x270D, 64},
    {"../fuzzing/fonts/cbdt-png-indexed-trns-smask-dimension-oob.ttf", 0xE903, 64},
    {"fonts/sbix.ttf", 'X', 0.005},
    {"fonts/sbix.ttf", 'X', 64},
  };

  for (unsigned i = 0; i < G_N_ELEMENTS (cases); i++)
  {
    hb_face_t *face = hb_test_open_font_file (cases[i].font);
    hb_font_t *font = hb_font_create (face);
    hb_codepoint_t gid;
    g_assert_true (hb_font_get_nominal_glyph (font, cases[i].unicode, &gid));

    cairo_font_face_t *cairo_face;
    if (i + 1 < G_N_ELEMENTS (cases))
    {
      /* As hb-view shows text: the font is scaled to 64 times its size. */
      cairo_face = hb_cairo_font_face_create_for_face (face);
      hb_cairo_font_face_set_scale_factor (cairo_face, 64);
    }
    else
    {
      hb_font_funcs_t *ffuncs = hb_font_funcs_create ();
      hb_font_funcs_set_paint_glyph_or_fail_func (ffuncs, paint_image_of_no_width, NULL, NULL);
      hb_font_t *subfont = hb_font_create_sub_font (font);
      hb_font_set_funcs (subfont, ffuncs, NULL, NULL);
      hb_font_funcs_destroy (ffuncs);
      cairo_face = hb_cairo_font_face_create_for_font (subfont);
      hb_font_destroy (subfont);
    }
    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
    cairo_t *cr = cairo_create (surface);
    cairo_set_font_face (cr, cairo_face);
    cairo_set_font_size (cr, cases[i].size);

    cairo_glyph_t glyph = {gid, 0, 64};
    cairo_show_glyphs (cr, &glyph, 1);

    g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
    g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

    /* What is drawn after the glyph is drawn. */
    cairo_set_source_rgb (cr, 1, 0, 0);
    cairo_rectangle (cr, 60, 0, 4, 4);
    cairo_fill (cr);
    cairo_surface_flush (surface);
    uint32_t pixel;
    memcpy (&pixel, cairo_image_surface_get_data (surface) + 62 * 4, sizeof (pixel));
    g_assert_cmphex (pixel, ==, 0xFFFF0000u);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    cairo_font_face_destroy (cairo_face);
    hb_font_destroy (font);
    hb_face_destroy (face);
  }
}

static void
test_hb_cairo_image_not_square (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif
  if (cairo_version () < CAIRO_VERSION_ENCODE (1, 18, 0))
  {
    g_test_skip ("This cairo has no color user fonts");
    return;
  }

  /* Two sbix glyphs whose images are not square, each red above its
   * middle and blue below: one of 2 by 4 pixels, half an em wide and
   * one tall, and one of 4 by 2, one em wide and half an em tall.
   * The height of a PNG image used to be taken for its width, so the
   * first showed its upper half over all of its height, and the second
   * was over within half of its own. */
  static const struct
  {
    hb_codepoint_t unicode;
    int x, y;
    uint32_t pixel;
  } pixels[] =
  {
    {'a', 16, 12, 0xFFFF0000u},
    {'a', 16, 52, 0xFF0000FFu},
    {'b', 32, 39, 0xFFFF0000u},
    {'b', 32, 60, 0xFF0000FFu},
  };

  hb_face_t *face = hb_test_open_font_file ("fonts/sbix-not-square.ttf");
  hb_font_t *font = hb_font_create (face);
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_font (font);

  for (unsigned i = 0; i < G_N_ELEMENTS (pixels); i++)
  {
    hb_codepoint_t gid;
    g_assert_true (hb_font_get_nominal_glyph (font, pixels[i].unicode, &gid));

    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
    cairo_t *cr = cairo_create (surface);
    cairo_set_font_face (cr, cairo_face);
    cairo_set_font_size (cr, 64);

    cairo_glyph_t glyph = {gid, 0, 64};
    cairo_show_glyphs (cr, &glyph, 1);
    g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);

    cairo_surface_flush (surface);
    uint32_t pixel;
    memcpy (&pixel,
	    cairo_image_surface_get_data (surface) +
	    pixels[i].y * cairo_image_surface_get_stride (surface) + pixels[i].x * 4,
	    sizeof (pixel));
    g_assert_cmphex (pixel, ==, pixels[i].pixel);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
  }

  cairo_font_face_destroy (cairo_face);
  hb_font_destroy (font);
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
    /* Monotone clusters exceeding text/character length or wrapping sign. */
    {HB_DIRECTION_LTR, {0, 4, 8, 12}, 4},
    {HB_DIRECTION_RTL, {12, 8, 4, 0}, 4},
    {HB_DIRECTION_LTR, {0, 1, 2, 0x80000000u}, 4},
    /* Not monotone. */
    {HB_DIRECTION_LTR, {0, 2, 0, 0}, 1},
    {HB_DIRECTION_LTR, {3, 1, 1, 0}, 1},
    {HB_DIRECTION_RTL, {0, 0, 2, 0}, 1},
    {HB_DIRECTION_RTL, {0, 1, 1, 3}, 1},
  };
  static const char * const strings[] = {
    "abcd",
    "\xce\xb1\xce\xb2\xce\xb3\xce\xb4",  /* 4 2-byte Greek characters */
    "\xf0\x9f\x98\x80\xf0\x9f\x98\x81\xf0\x9f\x98\x82\xf0\x9f\x98\x83",  /* 4 4-byte emoji */
  };

  for (unsigned s = 0; s < G_N_ELEMENTS (strings); s++)
  {
    const char *utf8 = strings[s];
    unsigned utf8_len = strlen (utf8);
    /* Exact-sized allocation so ASan catches any OOB read beyond utf8_len. */
    char *exact_utf8 = (char *) malloc (utf8_len);
    memcpy (exact_utf8, utf8, utf8_len);

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
				     exact_utf8, (int) utf8_len,
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
	g_assert_cmpint (total_bytes, ==, (int) utf8_len);
	g_assert_cmpint (total_glyphs, ==, 4);

	cairo_glyph_free (glyphs);
	cairo_text_cluster_free (clusters);
	hb_buffer_destroy (buffer);
      }
    free (exact_utf8);
  }
}

static void
test_hb_cairo_sweep_gradient_alpha (void)
{
#ifdef HB_TEST_CAIRO_TSAN
  g_test_skip ("Cairo user-font rendering has a pre-existing lock-order inversion under TSan");
  return;
#endif
  if (cairo_version () < CAIRO_VERSION_ENCODE (1, 18, 0))
  {
    g_test_skip ("This cairo has no color user fonts");
    return;
  }

  /* Sweeps around the center of the em, from red to blue.
   *
   * The first goes once around, both colors at a half.  It is painted
   * in sixteen patches, and alpha used to be multiplied into the two
   * colors once more for each, so that it turned black on its way
   * around.
   *
   * The other two go over the first 45 degrees, from opaque red to
   * blue that is transparent, or at a quarter.  cairo interpolates the
   * colors at the corners of a patch apart from their alpha, which put
   * blue into the red as it faded out; or black, which the transparent
   * blue was taken for. */
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-sweep-alpha.ttf");
  hb_font_t *font = hb_font_create (face);
  cairo_font_face_t *cairo_face = hb_cairo_font_face_create_for_font (font);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 256, 256);
  cairo_t *cr = cairo_create (surface);

  cairo_set_font_face (cr, cairo_face);
  cairo_set_font_size (cr, 256);

  /* A glyph, which fills the picture; the alpha of its red and of its
   * blue; and pixels of it, with how far along the sweep is at each. */
  static const struct {
    char unicode;
    double red_alpha, blue_alpha;
    struct {
      int x, y;
      double t;
    } at[4];
  } tests[] = {
    /* Pixels in the 1st, 6th, 10th and 14th patch. */
    {'a', .5, .5, {{222, 111, .02751}, {79, 45, .33458}, {47, 179, .59058}, {178, 209, .83829}}},
    {'b', 1., 0., {{227, 114, .17170}, {220, 90, .49040}, {207, 67, .82825}}},
    {'c', 1., .25, {{227, 114, .17170}, {220, 90, .49040}, {207, 67, .82825}}},
  };
  for (unsigned i = 0; i < G_N_ELEMENTS (tests); i++)
  {
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);

    hb_codepoint_t gid;
    g_assert_true (hb_font_get_nominal_glyph (font, tests[i].unicode, &gid));
    cairo_glyph_t glyph = {gid, 0, 256};
    cairo_show_glyphs (cr, &glyph, 1);
    g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
    cairo_surface_flush (surface);

    for (unsigned j = 0; j < G_N_ELEMENTS (tests[i].at) && tests[i].at[j].x; j++)
    {
      uint32_t pixel;
      memcpy (&pixel,
	      cairo_image_surface_get_data (surface) +
	      tests[i].at[j].y * cairo_image_surface_get_stride (surface) +
	      tests[i].at[j].x * 4,
	      sizeof (pixel));

      /* The pixel has alpha multiplied into its color, which is how
       * the two colors are to be interpolated. */
      double red = 255 * tests[i].red_alpha * (1 - tests[i].at[j].t);
      double blue = 255 * tests[i].blue_alpha * tests[i].at[j].t;
      g_assert_cmpfloat_with_epsilon ((pixel >> 24) & 0xFF, red + blue, 5);
      g_assert_cmpfloat_with_epsilon ((pixel >> 16) & 0xFF, red, 5);
      g_assert_cmpfloat_with_epsilon ((pixel >> 8) & 0xFF, 0, 1);
      g_assert_cmpfloat_with_epsilon (pixel & 0xFF, blue, 5);
    }
  }

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  cairo_font_face_destroy (cairo_face);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_hb_cairo_transform_no_inverse);
  hb_test_add (test_hb_cairo_image_not_paintable);
  hb_test_add (test_hb_cairo_image_not_square);
  hb_test_add (test_hb_cairo_color_glyph_cycle);
  hb_test_add (test_hb_cairo_face_lifecycle);
  hb_test_add (test_hb_cairo_glyphs_from_buffer_clusters);
  hb_test_add (test_hb_cairo_sweep_gradient_alpha);

  return hb_test_run ();
}
