/*
 * Copyright © 2022 Matthias Clasen
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

#include <hb-features.h>
#include <hb-ot.h>

#ifdef HB_HAS_FREETYPE
#include <hb-ft.h>

#if (FREETYPE_MAJOR*10000 + FREETYPE_MINOR*100 + FREETYPE_PATCH) >= 21300
#include FT_COLOR_H
#endif
#endif

static inline hb_bool_t
have_ft_colrv1 (void)
{
#if defined(HB_HAS_FREETYPE) && (FREETYPE_MAJOR*10000 + FREETYPE_MINOR*100 + FREETYPE_PATCH) >= 21300
  return TRUE;
#else
  return FALSE;
#endif
}

/* Unit tests for hb-paint.h */

/* ---- */

typedef struct {
  int level;
  GString *string;
} paint_data_t;

static float
clean_float (float v)
{
  /* Keep text fixtures stable across backends that may produce -0.f. */
  return v == 0.f ? 0.f : v;
}

static void print (paint_data_t *data, const char *format, ...) G_GNUC_PRINTF (2, 3);

static void
print (paint_data_t *data,
       const char *format,
       ...)
{
  va_list args;

  g_string_append_printf (data->string, "%*s", 2 * data->level, "");

  va_start (args, format);
  g_string_append_vprintf (data->string, format, args);
  va_end (args);

  g_string_append (data->string, "\n");
}

static void
push_transform (hb_paint_funcs_t *funcs HB_UNUSED,
                void *paint_data,
                float xx, float yx,
                float xy, float yy,
                float dx, float dy,
                void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  xx = clean_float (xx);
  yx = clean_float (yx);
  xy = clean_float (xy);
  yy = clean_float (yy);
  dx = clean_float (dx);
  dy = clean_float (dy);

  print (data, "start transform %.3g %.3g %.3g %.3g %.3g %.3g", xx, yx, xy, yy, dx, dy);
  data->level++;
}

static void
pop_transform (hb_paint_funcs_t *funcs HB_UNUSED,
               void *paint_data,
               void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  data->level--;
  print (data, "end transform");
}

static void
fill_glyph (hb_paint_funcs_t *funcs HB_UNUSED,
            void *paint_data,
            hb_codepoint_t glyph,
            hb_font_t *font HB_UNUSED,
            hb_bool_t use_foreground HB_UNUSED,
            hb_color_t color,
            void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  print (data, "fill glyph %u solid %d %d %d %d", glyph,
         hb_color_get_red (color),
         hb_color_get_green (color),
         hb_color_get_blue (color),
         hb_color_get_alpha (color));
}

static hb_bool_t
paint_color_glyph (hb_paint_funcs_t *funcs HB_UNUSED,
                   void *paint_data,
                   hb_codepoint_t glyph,
                   hb_font_t *font HB_UNUSED,
                   void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  print (data, "paint color glyph %u; acting as failed", glyph);

  return FALSE;
}

static void
push_clip_glyph (hb_paint_funcs_t *funcs HB_UNUSED,
                 void *paint_data,
                 hb_codepoint_t glyph,
                 hb_font_t *font HB_UNUSED,
                 void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  print (data, "start clip glyph %u", glyph);
  data->level++;
}

static void
push_clip_rectangle (hb_paint_funcs_t *funcs HB_UNUSED,
                     void *paint_data,
                     float xmin, float ymin, float xmax, float ymax,
                     void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  print (data, "start clip rectangle %.3g %.3g %.3g %.3g", xmin, ymin, xmax, ymax);
  data->level++;
}

static void
pop_clip (hb_paint_funcs_t *funcs HB_UNUSED,
          void *paint_data,
          void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  data->level--;
  print (data, "end clip");
}

static void
paint_color (hb_paint_funcs_t *funcs HB_UNUSED,
             void *paint_data,
             hb_bool_t use_foreground HB_UNUSED,
             hb_color_t color,
             void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  print (data, "solid %d %d %d %d",
         hb_color_get_red (color),
         hb_color_get_green (color),
         hb_color_get_blue (color),
         hb_color_get_alpha (color));
}

static hb_bool_t
paint_image (hb_paint_funcs_t *funcs HB_UNUSED,
             void *paint_data,
             hb_blob_t *blob HB_UNUSED,
             unsigned int width,
             unsigned int height,
             hb_tag_t format,
             float slant,
             hb_glyph_extents_t *extents,
             void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;
  char buf[5] = { 0, };

  hb_tag_to_string (format, buf);
  print (data, "image type %s size %u %u slant %.3g extents %d %d %d %d\n",
         buf, width, height, slant,
         extents->x_bearing, extents->y_bearing, extents->width, extents->height);

  return TRUE;
}

static void
print_color_stops (paint_data_t *data,
                   hb_paint_extend_t extend,
                   hb_color_stop_t *stops,
                   unsigned int len)
{
  print (data, "colors %d", extend);
  data->level += 1;
  for (unsigned int i = 0; i < len; i++)
    print (data, "%.3g %d %d %d %d",
           stops[i].offset,
           hb_color_get_red (stops[i].color),
           hb_color_get_green (stops[i].color),
           hb_color_get_blue (stops[i].color),
           hb_color_get_alpha (stops[i].color));
  data->level -= 1;
}

static void
print_color_line (paint_data_t *data,
                  hb_color_line_t *color_line)
{
  hb_color_stop_t *stops;
  unsigned int len;

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = malloc (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);

  print_color_stops (data, hb_color_line_get_extend (color_line), stops, len);
  free (stops);
}

static void
paint_linear_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0,
                       float x1, float y1,
                       float x2, float y2,
                       void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;
  float xx0, yy0, xx1, yy1;
  float xxx0, yyy0, xxx1, yyy1;
  float min, max;
  hb_color_stop_t *stops;
  unsigned int len;

  hb_paint_reduce_linear_anchors (x0, y0, x1, y1, x2, y2,
                                  &xx0, &yy0, &xx1, &yy1);
  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = malloc (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);
  hb_paint_normalize_color_line (stops, len, &min, &max);

  xxx0 = xx0 + min * (xx1 - xx0);
  yyy0 = yy0 + min * (yy1 - yy0);
  xxx1 = xx0 + max * (xx1 - xx0);
  yyy1 = yy0 + max * (yy1 - yy0);

  print (data, "linear gradient");
  data->level += 1;
  print (data, "p0 %.3g %.3g", xxx0, yyy0);
  print (data, "p1 %.3g %.3g", xxx1, yyy1);

  print_color_stops (data, hb_color_line_get_extend (color_line), stops, len);
  data->level -= 1;
  free (stops);
}

static void
paint_radial_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0, float r0,
                       float x1, float y1, float r1,
                       void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  print (data, "radial gradient");
  data->level += 1;
  print (data, "p0 %.3g %.3g radius %.3g", x0, y0, r0);
  print (data, "p1 %.3g %.3g radius %.3g", x1, y1, r1);

  print_color_line (data, color_line);
  data->level -= 1;
}

static void
paint_sweep_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
                      void *paint_data,
                      hb_color_line_t *color_line,
                      float cx, float cy,
                      float start_angle,
                      float end_angle,
                      void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;

  print (data, "sweep gradient");
  data->level++;
  print (data, "center %.3g %.3g", cx, cy);
  print (data, "angles %.3g %.3g", start_angle, end_angle);

  print_color_line (data, color_line);
  data->level -= 1;
}

static void
push_group (hb_paint_funcs_t *funcs HB_UNUSED,
            void *paint_data,
            void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;
  print (data, "push group");
  data->level++;
}

static void
pop_group (hb_paint_funcs_t *funcs HB_UNUSED,
           void *paint_data,
           hb_paint_composite_mode_t mode,
           void *user_data HB_UNUSED)
{
  paint_data_t *data = paint_data;
  data->level--;
  print (data, "pop group mode %d", mode);
}

static hb_paint_funcs_t *
get_test_paint_funcs (void)
{
  static hb_paint_funcs_t *funcs = NULL;

  if (!funcs)
  {
    funcs = hb_paint_funcs_create ();

    hb_paint_funcs_set_push_transform_func (funcs, push_transform, NULL, NULL);
    hb_paint_funcs_set_pop_transform_func (funcs, pop_transform, NULL, NULL);
    hb_paint_funcs_set_fill_glyph_func (funcs, fill_glyph, NULL, NULL);
    hb_paint_funcs_set_color_glyph_func (funcs, paint_color_glyph, NULL, NULL);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, NULL, NULL);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, push_clip_rectangle, NULL, NULL);
    hb_paint_funcs_set_pop_clip_func (funcs, pop_clip, NULL, NULL);
    hb_paint_funcs_set_push_group_func (funcs, push_group, NULL, NULL);
    hb_paint_funcs_set_pop_group_func (funcs, pop_group, NULL, NULL);
    hb_paint_funcs_set_color_func (funcs, paint_color, NULL, NULL);
    hb_paint_funcs_set_image_func (funcs, paint_image, NULL, NULL);
    hb_paint_funcs_set_linear_gradient_func (funcs, paint_linear_gradient, NULL, NULL);
    hb_paint_funcs_set_radial_gradient_func (funcs, paint_radial_gradient, NULL, NULL);
    hb_paint_funcs_set_sweep_gradient_func (funcs, paint_sweep_gradient, NULL, NULL);

    hb_paint_funcs_make_immutable (funcs);
  }

  return funcs;
}

typedef struct {
  const char *font_file;
  float slant;
  hb_codepoint_t glyph;
  unsigned int palette;
  const char *output;
} paint_test_t;

#define NOTO_HAND   "fonts/noto_handwriting-cff2_colr_1.otf"
#define TEST_GLYPHS "fonts/test_glyphs-glyf_colr_1.ttf"
#define TEST_GLYPHS_VF "fonts/test_glyphs-glyf_colr_1_variable.ttf"
#define BAD_COLRV1  "fonts/bad_colrv1.ttf"
#define ROCHER_ABC  "fonts/RocherColorGX.abc.ttf"

/* To verify the rendering visually, use
 *
 * hb-view --font-slant SLANT --font-palette PALETTE FONT --glyphs gidGID
 *
 * where GID is the glyph value of the test.
 */
static paint_test_t paint_tests[] = {
  /* COLRv1 */
  { NOTO_HAND,   0.,  10,   0, "hand-10" },
  { NOTO_HAND,   0.2f,10,   0, "hand-10.2" },

  { TEST_GLYPHS, 0,    6,   0, "test-6" },   // linear gradient
  { TEST_GLYPHS, 0,   10,   0, "test-10" },  // sweep gradient
  { TEST_GLYPHS, 0,   92,   0, "test-92" },  // radial gradient
  { TEST_GLYPHS, 0,  106,   0, "test-106" },
  { TEST_GLYPHS, 0,  116,   0, "test-116" }, // compositing
  { TEST_GLYPHS, 0,  123,   0, "test-123" },
  { TEST_GLYPHS, 0,  154,   0, "test-154" },
  { TEST_GLYPHS, 0,  165,   0, "test-165" }, // linear gradient
  { TEST_GLYPHS, 0,  175,   0, "test-175" }, // layers

  { TEST_GLYPHS_VF, 0,    6,   0, "testvf-6" },
  { TEST_GLYPHS_VF, 0,   10,   0, "testvf-10" },
  { TEST_GLYPHS_VF, 0,   92,   0, "testvf-92" },
  { TEST_GLYPHS_VF, 0,  106,   0, "testvf-106" },
  { TEST_GLYPHS_VF, 0,  116,   0, "testvf-116" },
  { TEST_GLYPHS_VF, 0,  123,   0, "testvf-123" },
  { TEST_GLYPHS_VF, 0,  154,   0, "testvf-154" },
  { TEST_GLYPHS_VF, 0,  165,   0, "testvf-165" },
  { TEST_GLYPHS_VF, 0,  175,   0, "testvf-175" },

  { BAD_COLRV1,  0,  154,   0, "bad-154" },  // recursion

  /* COLRv0 */
  { ROCHER_ABC, 0.3f, 1,   0, "rocher-1" },
  { ROCHER_ABC, 0.3f, 2,   2, "rocher-2" },
  { ROCHER_ABC, 0,    3, 200, "rocher-3" },
};

static void
test_hb_paint (gconstpointer d,
               hb_bool_t     use_ft HB_UNUSED)
{
  const paint_test_t *test = d;
  hb_face_t *face;
  hb_font_t *font;
  hb_paint_funcs_t *funcs;
  paint_data_t data;
  char *file;
  char *buffer;
  gsize len;
  GError *error = NULL;

  face = hb_test_open_font_file (test->font_file);
  font = hb_font_create (face);

  hb_font_set_synthetic_slant (font, test->slant);

#ifdef HB_HAS_FREETYPE
  if (use_ft)
    hb_ft_font_set_funcs (font);
#endif

  funcs = get_test_paint_funcs ();

  data.string = g_string_new ("");
  data.level = 0;

  hb_font_paint_glyph (font, test->glyph, funcs, &data, 0, HB_COLOR (0, 0, 0, 255));

  /* Run
   *
   * GENERATE_DATA=1 G_TEST_SRCDIR=./test/api ./build/test/api/test-paint
   *
   * to regenerate the expected output for all tests.
   */
  file = g_test_build_filename (G_TEST_DIST, "results-paint", test->output, NULL);
  if (getenv ("GENERATE_DATA"))
    {
      if (!g_file_set_contents (file, data.string->str, data.string->len, NULL))
        g_error ("Failed to write %s.", file);

      return;
    }

  if (!g_file_get_contents (file, &buffer, &len, &error))
  {
    g_test_message ("File %s not found.", file);
    g_test_fail ();
    return;
  }

  char **lines = g_strsplit (data.string->str, "\n", 0);
  char **expected;
  if (strstr (buffer, "\r\n"))
    expected = g_strsplit (buffer, "\r\n", 0);
  else
    expected = g_strsplit (buffer, "\n", 0);

  if (g_strv_length (lines) != g_strv_length (expected))
  {
    g_test_message ("Unexpected number of lines in output (%d instead of %d):\n%s", g_strv_length (lines), g_strv_length (expected), data.string->str);
    g_test_fail ();
  }
  else
  {
    unsigned int length = g_strv_length (lines);
    for (unsigned int i = 0; i < length; i++)
    {
      if (strcmp (lines[i], expected[i]) != 0)
      {
        int pos;
        for (pos = 0; lines[i][pos]; pos++)
          if (lines[i][pos] != expected[i][pos])
            break;

        g_test_message ("Unexpected output at %d:%d (%c instead of %c):\n%s", i, pos, lines[i][pos], expected[i][pos], data.string->str);
        g_test_fail ();
      }
    }
  }

  g_strfreev (lines);
  g_strfreev (expected);

  g_free (buffer);
  g_free (file);

  g_string_free (data.string, TRUE);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_compare_to_ot (const char *file, hb_codepoint_t glyph)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_paint_funcs_t *funcs;
  GString *ot_str;
  paint_data_t data;

  face = hb_test_open_font_file (file);
  font = hb_font_create (face);

  funcs = get_test_paint_funcs ();

  data.string = g_string_new ("");
  data.level = 0;

  hb_font_paint_glyph (font, glyph, funcs, &data, 0, HB_COLOR (0, 0, 0, 255));

  g_assert_true (data.level == 0);

  ot_str = data.string;

#ifdef HB_HAS_FREETYPE
  hb_ft_font_set_funcs (font);
#endif

  data.string = g_string_new ("");
  data.level = 0;

  hb_font_paint_glyph (font, glyph, funcs, &data, 0, HB_COLOR (0, 0, 0, 255));

  g_assert_true (data.level == 0);

  g_assert_cmpstr (ot_str->str, ==, data.string->str);

  g_string_free (data.string, TRUE);
  hb_font_destroy (font);
  hb_face_destroy (face);

  g_string_free (ot_str, TRUE);
}

static void
test_hb_paint_ot (gconstpointer data)
{
  test_hb_paint (data, 0);
}

static void
test_hb_paint_ft (gconstpointer data)
{
  if (have_ft_colrv1 ())
    test_hb_paint (data, 1);
  else
    g_test_skip ("FreeType COLRv1 support not present");
}

static void
test_compare_to_ot_novf (gconstpointer d)
{
  test_compare_to_ot (TEST_GLYPHS, GPOINTER_TO_UINT (d));
}

static void
test_compare_to_ot_vf (gconstpointer d)
{
  test_compare_to_ot (TEST_GLYPHS_VF, GPOINTER_TO_UINT (d));
}

static void
scrutinize_linear_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
                            void *paint_data,
                            hb_color_line_t *color_line,
                            float x0 HB_UNUSED, float y0 HB_UNUSED,
                            float x1 HB_UNUSED, float y1 HB_UNUSED,
                            float x2 HB_UNUSED, float y2 HB_UNUSED,
                            void *user_data HB_UNUSED)
{
  hb_bool_t *result = paint_data;
  hb_color_stop_t *stops;
  unsigned int len;
  hb_color_stop_t *stops2;
  unsigned int len2;

  *result = FALSE;

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  if (len == 0)
    return;

  stops = malloc (len * sizeof (hb_color_stop_t));
  stops2 = malloc (len * sizeof (hb_color_stop_t));

  hb_color_line_get_color_stops (color_line, 0, &len, stops);
  hb_color_line_get_color_stops (color_line, 0, &len, stops2);

  // check that we can get stops twice
  if (memcmp (stops, stops2, len * sizeof (hb_color_stop_t)) != 0)
  {
    free (stops);
    free (stops2);
    return;
  }

  // check that we can get a single stop in the middle
  len2 = 1;
  hb_color_line_get_color_stops (color_line, len - 1, &len2, stops2);
  if (memcmp (&stops[len - 1], stops2, sizeof (hb_color_stop_t)) != 0)
  {
    free (stops);
    free (stops2);
    return;
  }

  free (stops);
  free (stops2);

  *result = TRUE;
}

static void
test_color_stops (hb_bool_t use_ft HB_UNUSED)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_paint_funcs_t *funcs;
  hb_bool_t result = FALSE;

  face = hb_test_open_font_file (NOTO_HAND);
  font = hb_font_create (face);

#ifdef HB_HAS_FREETYPE
  if (use_ft)
    hb_ft_font_set_funcs (font);
#endif

  funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_linear_gradient_func (funcs, scrutinize_linear_gradient, NULL, NULL);

  hb_font_paint_glyph (font, 10, funcs, &result, 0, HB_COLOR (0, 0, 0, 255));

  g_assert_true (result);

  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_color_stops_ot (void)
{
  test_color_stops (0);
}

static void
test_color_stops_ft (void)
{
  if (have_ft_colrv1 ())
    test_color_stops (1);
  else
    g_test_skip ("FreeType COLRv1 support not present");
}


typedef struct
{
  hb_font_t *font;
  hb_paint_funcs_t *empty_funcs;
  GString *colors;
  unsigned int gradients[3];
  unsigned int palette_calls;
  hb_bool_t reenter;
} reentrant_paint_t;

static void
reentrant_query_font (reentrant_paint_t *data)
{
  if (data->reenter)
  {
    hb_glyph_extents_t extents;
    hb_font_get_glyph_extents (data->font, 8, &extents);
  }
}

static hb_bool_t
reentrant_palette (hb_paint_funcs_t *funcs HB_UNUSED,
                   void *paint_data,
                   unsigned int color_index HB_UNUSED,
                   hb_color_t *color HB_UNUSED,
                   void *user_data HB_UNUSED)
{
  reentrant_paint_t *data = paint_data;
  reentrant_query_font (data);
  if (data->reenter)
    /* A nested paint selects a different FreeType palette. */
    hb_font_paint_glyph (data->font, 8, data->empty_funcs, NULL, 1,
                         HB_COLOR (0, 0, 0, 255));
  data->palette_calls++;
  return FALSE;
}

static void
reentrant_color (hb_paint_funcs_t *funcs HB_UNUSED,
                 void *paint_data,
                 hb_bool_t is_foreground,
                 hb_color_t color,
                 void *user_data HB_UNUSED)
{
  reentrant_paint_t *data = paint_data;
  reentrant_query_font (data);
  g_string_append_printf (data->colors, "c %u %u;", is_foreground, color);
}

static void
reentrant_gradient (reentrant_paint_t *data, hb_color_line_t *line, unsigned int kind)
{
  unsigned int total = hb_color_line_get_color_stops (line, 0, NULL, NULL);
  hb_color_stop_t *stops = g_new0 (hb_color_stop_t, total);
  hb_color_stop_t *again = g_new0 (hb_color_stop_t, total);
  unsigned int count = total;

  reentrant_query_font (data);
  hb_color_line_get_color_stops (line, 0, &count, stops);
  g_assert_cmpuint (count, ==, total);
  reentrant_query_font (data);
  count = total;
  hb_color_line_get_color_stops (line, 0, &count, again);
  g_assert_cmpuint (count, ==, total);

  for (unsigned int i = 0; i < total; i++)
  {
    g_assert_cmpfloat (stops[i].offset, ==, again[i].offset);
    g_assert_cmpuint (stops[i].color, ==, again[i].color);
    g_assert_cmpuint (stops[i].is_foreground, ==, again[i].is_foreground);
    count = 1;
    hb_color_line_get_color_stops (line, i, &count, again);
    g_assert_cmpuint (count, ==, 1);
    g_assert_cmpuint (stops[i].color, ==, again[0].color);
    g_string_append_printf (data->colors, "g%u %u %u;", kind,
                            stops[i].is_foreground, stops[i].color);
  }
  data->gradients[kind]++;
  g_free (again);
  g_free (stops);
}

static void
reentrant_linear (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data,
                  hb_color_line_t *line,
                  float x0 HB_UNUSED, float y0 HB_UNUSED,
                  float x1 HB_UNUSED, float y1 HB_UNUSED,
                  float x2 HB_UNUSED, float y2 HB_UNUSED,
                  void *user_data HB_UNUSED)
{
  reentrant_gradient (paint_data, line, 0);
}

static void
reentrant_radial (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data,
                  hb_color_line_t *line,
                  float x0 HB_UNUSED, float y0 HB_UNUSED, float r0 HB_UNUSED,
                  float x1 HB_UNUSED, float y1 HB_UNUSED, float r1 HB_UNUSED,
                  void *user_data HB_UNUSED)
{
  reentrant_gradient (paint_data, line, 1);
}

static void
reentrant_sweep (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data,
                 hb_color_line_t *line,
                 float x0 HB_UNUSED, float y0 HB_UNUSED,
                 float start HB_UNUSED, float end HB_UNUSED,
                 void *user_data HB_UNUSED)
{
  reentrant_gradient (paint_data, line, 2);
}

static void
test_ft_reentrant_paint (void)
{
  if (!have_ft_colrv1 ())
  {
    g_test_skip ("FreeType COLRv1 support not present");
    return;
  }
#if GLIB_CHECK_VERSION(2,38,0)
  if (!g_test_subprocess ())
  {
    g_test_trap_subprocess (NULL, 5 * G_USEC_PER_SEC, 0);
    g_test_trap_assert_passed ();
    return;
  }
#endif

  static const hb_codepoint_t glyphs[] = {8, 12, 94, 116, 177};
  hb_face_t *face = hb_test_open_font_file (TEST_GLYPHS);
  hb_font_t *font = hb_font_create (face);
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  hb_paint_funcs_t *empty_funcs = hb_paint_funcs_create ();
  reentrant_paint_t data = {font, empty_funcs, g_string_new (""), {0, 0, 0}, 0, FALSE};

  hb_paint_funcs_set_color_func (funcs, reentrant_color, NULL, NULL);
  hb_paint_funcs_set_custom_palette_color_func (funcs, reentrant_palette, NULL, NULL);
  hb_paint_funcs_set_linear_gradient_func (funcs, reentrant_linear, NULL, NULL);
  hb_paint_funcs_set_radial_gradient_func (funcs, reentrant_radial, NULL, NULL);
  hb_paint_funcs_set_sweep_gradient_func (funcs, reentrant_sweep, NULL, NULL);

  /* Get reference colors before enabling font reentry and nested paints. */
  for (unsigned int i = 0; i < G_N_ELEMENTS (glyphs); i++)
    hb_font_paint_glyph (font, glyphs[i], funcs, &data, 0, HB_COLOR (0, 0, 0, 255));
  char *expected = g_strdup (data.colors->str);
  g_string_truncate (data.colors, 0);
  data.reenter = TRUE;
  data.palette_calls = 0;
#ifdef HB_HAS_FREETYPE
  hb_ft_font_set_funcs (font);
#endif

  for (unsigned int i = 0; i < G_N_ELEMENTS (glyphs); i++)
    hb_font_paint_glyph (font, glyphs[i], funcs, &data, 0, HB_COLOR (0, 0, 0, 255));

  g_assert_cmpstr (data.colors->str, ==, expected);
  g_assert_cmpuint (data.palette_calls, >, 0);
  for (unsigned int i = 0; i < 3; i++)
    g_assert_cmpuint (data.gradients[i], >, 0);

  g_free (expected);
  g_string_free (data.colors, TRUE);
  hb_paint_funcs_destroy (empty_funcs);
  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
  hb_face_destroy (face);
}


/* ── Arbitrary-path clip API (push_clip_path_start/_end) ─────────── */

static void
test_push_clip_path_nil_returns_null (void)
{
  /* Unset callbacks fall through to nil stubs; start must return
   * NULL + clear draw_data so callers know the backend can't do
   * arbitrary-path clipping. */
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  void *draw_data = (void *) 0xdeadbeef;
  hb_draw_funcs_t *df = hb_paint_push_clip_path_start (funcs, NULL, &draw_data);
  g_assert_null (df);
  g_assert_null (draw_data);

  /* end is a no-op on the nil stub. */
  hb_paint_push_clip_path_end (funcs, NULL);

  /* Also: NULL draw_data must be accepted; the entry point routes
   * it through an internal scratch. */
  df = hb_paint_push_clip_path_start (funcs, NULL, NULL);
  g_assert_null (df);

  hb_paint_funcs_destroy (funcs);
}

typedef struct
{
  int              move_to_calls;
  int              line_to_calls;
  int              close_path_calls;
  float            last_x, last_y;
  hb_bool_t        start_called;
  hb_bool_t        end_called;
  int              pop_clip_calls;
  hb_draw_funcs_t *start_draw_funcs_out;
} clip_path_rec_t;

static void
clip_path_sink_move_to (hb_draw_funcs_t *funcs HB_UNUSED,
                        void *draw_data,
                        hb_draw_state_t *st HB_UNUSED,
                        float x, float y,
                        void *user_data HB_UNUSED)
{
  clip_path_rec_t *r = (clip_path_rec_t *) draw_data;
  r->move_to_calls++;
  r->last_x = x; r->last_y = y;
}

static void
clip_path_sink_line_to (hb_draw_funcs_t *funcs HB_UNUSED,
                        void *draw_data,
                        hb_draw_state_t *st HB_UNUSED,
                        float x, float y,
                        void *user_data HB_UNUSED)
{
  clip_path_rec_t *r = (clip_path_rec_t *) draw_data;
  r->line_to_calls++;
  r->last_x = x; r->last_y = y;
}

static void
clip_path_sink_close_path (hb_draw_funcs_t *funcs HB_UNUSED,
                           void *draw_data,
                           hb_draw_state_t *st HB_UNUSED,
                           void *user_data HB_UNUSED)
{
  clip_path_rec_t *r = (clip_path_rec_t *) draw_data;
  r->close_path_calls++;
}

static hb_draw_funcs_t *
clip_path_start_cb (hb_paint_funcs_t *pfuncs HB_UNUSED,
                    void *paint_data,
                    void **draw_data,
                    void *user_data HB_UNUSED)
{
  clip_path_rec_t *r = (clip_path_rec_t *) paint_data;
  r->start_called = TRUE;
  *draw_data = r;
  return r->start_draw_funcs_out;
}

static void
clip_path_end_cb (hb_paint_funcs_t *pfuncs HB_UNUSED,
                  void *paint_data,
                  void *user_data HB_UNUSED)
{
  clip_path_rec_t *r = (clip_path_rec_t *) paint_data;
  r->end_called = TRUE;
}

static void
clip_path_pop_clip_cb (hb_paint_funcs_t *pfuncs HB_UNUSED,
                       void *paint_data,
                       void *user_data HB_UNUSED)
{
  clip_path_rec_t *r = (clip_path_rec_t *) paint_data;
  r->pop_clip_calls++;
}

/* Round-trip: registered start/end fire in order, the returned
 * draw-funcs + draw-data propagate to the caller, and every
 * hb_draw_*() call on those funcs reaches the accumulator. */
static void
test_push_clip_path_round_trip (void)
{
  hb_draw_funcs_t *sink = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func    (sink, clip_path_sink_move_to,    NULL, NULL);
  hb_draw_funcs_set_line_to_func    (sink, clip_path_sink_line_to,    NULL, NULL);
  hb_draw_funcs_set_close_path_func (sink, clip_path_sink_close_path, NULL, NULL);
  hb_draw_funcs_make_immutable (sink);

  clip_path_rec_t r = {0};
  r.start_draw_funcs_out = sink;

  hb_paint_funcs_t *pfuncs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_clip_path_start_func (pfuncs, clip_path_start_cb,    NULL, NULL);
  hb_paint_funcs_set_push_clip_path_end_func   (pfuncs, clip_path_end_cb,      NULL, NULL);
  hb_paint_funcs_set_pop_clip_func             (pfuncs, clip_path_pop_clip_cb, NULL, NULL);
  hb_paint_funcs_make_immutable (pfuncs);

  void *draw_data = NULL;
  hb_draw_funcs_t *df = hb_paint_push_clip_path_start (pfuncs, &r, &draw_data);

  g_assert_true (r.start_called);
  g_assert_nonnull (df);
  g_assert_true (df == sink);
  g_assert_true (draw_data == &r);

  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  hb_draw_move_to    (df, draw_data, &st, 1.f, 2.f);
  hb_draw_line_to    (df, draw_data, &st, 3.f, 4.f);
  hb_draw_line_to    (df, draw_data, &st, 5.f, 6.f);
  hb_draw_close_path (df, draw_data, &st);

  g_assert_cmpint (r.move_to_calls, ==, 1);
  /* hb_draw_close_path synthesizes a line_to back to the contour
   * start, so our two explicit line_tos show up as three. */
  g_assert_cmpint (r.line_to_calls, ==, 3);
  g_assert_cmpint (r.close_path_calls, ==, 1);
  /* The synthetic closing line_to took the pen back to the
   * move_to point (1, 2). */
  g_assert_cmpfloat (r.last_x, ==, 1.f);
  g_assert_cmpfloat (r.last_y, ==, 2.f);

  hb_paint_push_clip_path_end (pfuncs, &r);
  g_assert_true (r.end_called);

  hb_paint_pop_clip (pfuncs, &r);
  g_assert_cmpint (r.pop_clip_calls, ==, 1);

  hb_paint_funcs_destroy (pfuncs);
  hb_draw_funcs_destroy (sink);
}

/* Test the default implementation of fill_glyph does the
 * push_clip_glyph + color + pop_clip sequence. */
static void
test_fill_glyph_nil_decomposes (void)
{
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, NULL, NULL);
  hb_paint_funcs_set_color_func (funcs, paint_color, NULL, NULL);
  hb_paint_funcs_set_pop_clip_func (funcs, pop_clip, NULL, NULL);
  hb_paint_funcs_make_immutable (funcs);

  paint_data_t data;
  data.string = g_string_new ("");
  data.level = 0;

  hb_paint_fill_glyph (funcs, &data, 42, hb_font_get_empty (),
                       FALSE, HB_COLOR (1, 2, 3, 4));

  g_assert_true (data.level == 0);
  g_assert_cmpstr (data.string->str, ==,
                   "start clip glyph 42\n"
                   "  solid 3 2 1 4\n"
                   "end clip\n");

  g_string_free (data.string, TRUE);
  hb_paint_funcs_destroy (funcs);
}

static void *color_glyph_user_data_seen;

static hb_bool_t
color_glyph_record_user_data (hb_paint_funcs_t *funcs HB_UNUSED,
                              void *paint_data HB_UNUSED,
                              hb_codepoint_t glyph HB_UNUSED,
                              hb_font_t *font HB_UNUSED,
                              void *user_data)
{
  color_glyph_user_data_seen = user_data;
  return TRUE;
}

/* Each callback must be handed the user_data it was registered with,
 * not another callback's. */
static void
test_color_glyph_user_data (void)
{
  char color_glyph_ctx = 0;
  char push_clip_glyph_ctx = 0;

  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_color_glyph_func (funcs, color_glyph_record_user_data,
                                       &color_glyph_ctx, NULL);
  hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph,
                                           &push_clip_glyph_ctx, NULL);
  hb_paint_funcs_make_immutable (funcs);

  color_glyph_user_data_seen = NULL;
  hb_paint_color_glyph (funcs, NULL, 42, hb_font_get_empty ());

  g_assert_true (color_glyph_user_data_seen == &color_glyph_ctx);

  hb_paint_funcs_destroy (funcs);
}

typedef struct
{
  int64_t policy;
  int64_t live;
} paint_budget_data_t;

static hb_bool_t
paint_set_budget (hb_paint_funcs_t *funcs HB_UNUSED,
		  void *paint_data,
		  int64_t budget,
		  void *user_data HB_UNUSED)
{
  paint_budget_data_t *data = (paint_budget_data_t *) paint_data;
  data->policy = budget;
  data->live = budget == HB_BUDGET_DEFAULT ? 4321 : budget;
  return TRUE;
}

static int64_t
paint_get_budget (hb_paint_funcs_t *funcs HB_UNUSED,
		  void *paint_data,
		  void *user_data HB_UNUSED)
{
  return ((paint_budget_data_t *) paint_data)->policy;
}

static int64_t *
paint_get_budget_remaining (hb_paint_funcs_t *funcs HB_UNUSED,
			    void *paint_data,
			    void *user_data HB_UNUSED)
{
  return &((paint_budget_data_t *) paint_data)->live;
}

static void
test_hb_paint_budget (void)
{
  paint_budget_data_t data = {HB_BUDGET_DEFAULT, 4321};
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();

  g_assert_false (hb_paint_set_budget (funcs, &data, 100));
  g_assert_cmpint (hb_paint_get_budget (funcs, &data), ==, HB_BUDGET_DEFAULT);
  g_assert_cmpint (hb_paint_get_budget_remaining (funcs, &data), ==, HB_BUDGET_UNLIMITED);

  hb_paint_funcs_set_set_budget_func (funcs, paint_set_budget, NULL, NULL);
  hb_paint_funcs_set_get_budget_func (funcs, paint_get_budget, NULL, NULL);
  hb_paint_funcs_set_get_budget_remaining_func (funcs, paint_get_budget_remaining, NULL, NULL);

  g_assert_true (hb_paint_set_budget (funcs, &data, -7));
  g_assert_cmpint (hb_paint_get_budget (funcs, &data), ==, 0);
  g_assert_cmpint (hb_paint_get_budget_remaining (funcs, &data), ==, 0);

  g_assert_true (hb_paint_set_budget (funcs, &data, HB_BUDGET_DEFAULT));
  g_assert_cmpint (hb_paint_get_budget (funcs, &data), ==, HB_BUDGET_DEFAULT);
  g_assert_cmpint (hb_paint_get_budget_remaining (funcs, &data), ==, 4321);

  g_assert_true (hb_paint_set_budget (funcs, &data, 9876));
  g_assert_cmpint (hb_paint_get_budget (funcs, &data), ==, 9876);
  g_assert_cmpint (hb_paint_get_budget_remaining (funcs, &data), ==, 9876);

  g_assert_true (hb_paint_set_budget (funcs, &data, HB_BUDGET_UNLIMITED));
  g_assert_cmpint (hb_paint_get_budget (funcs, &data), ==, HB_BUDGET_UNLIMITED);
  g_assert_cmpint (hb_paint_get_budget_remaining (funcs, &data), ==, HB_BUDGET_UNLIMITED);

  hb_paint_funcs_destroy (funcs);
}

static unsigned color_stops_fetched;

static void
fetch_color_stops (hb_paint_funcs_t *funcs HB_UNUSED,
		   void *paint_data HB_UNUSED,
		   hb_color_line_t *color_line,
		   float x0 HB_UNUSED, float y0 HB_UNUSED,
		   float x1 HB_UNUSED, float y1 HB_UNUSED,
		   float x2 HB_UNUSED, float y2 HB_UNUSED,
		   void *user_data HB_UNUSED)
{
  hb_color_stop_t stops[100];
  unsigned count = G_N_ELEMENTS (stops);
  g_assert_cmpuint (hb_color_line_get_color_stops (color_line, 0, &count, stops), ==, 100);
  g_assert_cmpuint (count, ==, 100);
  color_stops_fetched += count;
}

static void
test_hb_paint_color_stops_budget (void)
{
  /* A COLRv1 glyph whose four layers all are the same linear gradient
   * with 100 color stops.  Fetching the stops must consume budget. */
  char colr_data[34 + 16 + 20 + 16 + 3 + 100 * 6 + 21] = {0};
  colr_data[1] = 1; /* version */
  colr_data[17] = 34; /* baseGlyphList */
  colr_data[21] = 50; /* layerList */
  colr_data[24] = 0x02; colr_data[25] = (char) 0xB1; /* clipList */
  colr_data[34 + 3] = 1; /* BaseGlyphList: glyph 0 */
  colr_data[34 + 9] = 10;
  colr_data[34 + 10] = 1; /* PaintColrLayers */
  colr_data[34 + 11] = 4;
  colr_data[50 + 3] = 4; /* LayerList */
  for (unsigned i = 0; i < 4; i++)
    colr_data[50 + 4 + i * 4 + 3] = 20;
  colr_data[50 + 20] = 4; /* PaintLinearGradient */
  colr_data[50 + 23] = 16;
  colr_data[50 + 36 + 2] = 100; /* ColorLine */
  colr_data[689] = 1; /* ClipList: glyph 0 */
  colr_data[689 + 4] = 1;
  colr_data[689 + 11] = 12;
  colr_data[689 + 12] = 1; /* ClipBox */

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "COLR", colr_data);
  hb_face_set_glyph_count (face, 1);
  hb_font_t *font = hb_font_create (face);

  paint_budget_data_t data = {HB_BUDGET_DEFAULT, 0};
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_linear_gradient_func (funcs, fetch_color_stops, NULL, NULL);
  hb_paint_funcs_set_set_budget_func (funcs, paint_set_budget, NULL, NULL);
  hb_paint_funcs_set_get_budget_remaining_func (funcs, paint_get_budget_remaining, NULL, NULL);

  color_stops_fetched = 0;
  g_assert_true (hb_paint_set_budget (funcs, &data, 100000));
  hb_font_paint_glyph (font, 0, funcs, &data, 0, HB_COLOR (0, 0, 0, 255));
  g_assert_cmpuint (color_stops_fetched, ==, 400);
  g_assert_cmpint (data.live, <=, 100000 - 400);
  g_assert_cmpint (data.live, >=, 0);

  color_stops_fetched = 0;
  g_assert_true (hb_paint_set_budget (funcs, &data, 100));
  hb_font_paint_glyph (font, 0, funcs, &data, 0, HB_COLOR (0, 0, 0, 255));
  g_assert_cmpuint (color_stops_fetched, ==, 100);
  g_assert_cmpint (data.live, <, 0);

  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

typedef struct {
  unsigned tiles;
  float covered;
  hb_color_t c0, c1;
} sweep_tiles_data_t;

static void
check_sweep_tile (float a0, hb_color_t c0,
		  float a1, hb_color_t c1,
		  void *user_data)
{
  sweep_tiles_data_t *data = (sweep_tiles_data_t *) user_data;

  g_assert_cmpfloat (a0, >=, 0.f);
  g_assert_cmpfloat (a0, <=, a1);
  g_assert_cmpfloat (a1, <=, (float) (2 * G_PI));
  data->tiles++;
  data->covered += a1 - a0;
  data->c0 = c0;
  data->c1 = c1;
}

static void
test_hb_paint_sweep_gradient_tiles_clipped (void)
{
  /* Variation deltas can put the angles far outside the circle.  The
   * tiles must still cover 0..2π, and nothing else. */
  static const struct {
    hb_paint_extend_t extend;
    unsigned n_stops;
    float start_angle, end_angle;
  } tests[] = {
    { HB_PAINT_EXTEND_PAD,     1,  1e6f,  1e6f },
    { HB_PAINT_EXTEND_PAD,     1, -1e6f, -1e6f },
    { HB_PAINT_EXTEND_PAD,     1,  1e6f,  2e6f },
    { HB_PAINT_EXTEND_PAD,     2,  1e6f,  2e6f },
    { HB_PAINT_EXTEND_PAD,     2, -2e6f, -1e6f },
    { HB_PAINT_EXTEND_PAD,     2, -1e6f,  1e6f },
    { HB_PAINT_EXTEND_PAD,     2,  1e6f, -1e6f },
    { HB_PAINT_EXTEND_PAD,     2,    1.f,  1e6f },
    { HB_PAINT_EXTEND_REPEAT,  2, -1e6f,  1e6f },
    { HB_PAINT_EXTEND_REPEAT,  2,  1e6f,  3e6f },
    { HB_PAINT_EXTEND_REPEAT,  2,  1e20f, 3e20f },
    { HB_PAINT_EXTEND_REFLECT, 2, -1e6f,  1e6f },
    { HB_PAINT_EXTEND_REFLECT, 2, -3e6f, -1e6f },
  };

  for (unsigned i = 0; i < G_N_ELEMENTS (tests); i++)
  {
    hb_color_stop_t stops[2] = {
      { 0.f, FALSE, HB_COLOR (0, 0, 0, 255) },
      { 1.f, FALSE, HB_COLOR (255, 255, 255, 255) },
    };
    sweep_tiles_data_t data = {0, 0.f, 0, 0};

    hb_paint_sweep_gradient_tiles (stops, tests[i].n_stops, tests[i].extend,
				   tests[i].start_angle, tests[i].end_angle,
				   check_sweep_tile, &data);

    g_assert_cmpuint (data.tiles, >=, 1);
    g_assert_cmpuint (data.tiles, <=, 3);
    g_assert_cmpfloat (data.covered, >, (float) (2 * G_PI) - 1e-3f);
    g_assert_cmpfloat (data.covered, <, (float) (2 * G_PI) + 1e-3f);
    if (tests[i].n_stops == 1)
    {
      g_assert_cmphex (data.c0, ==, stops[0].color);
      g_assert_cmphex (data.c1, ==, stops[0].color);
    }
  }
}


int
main (int argc, char **argv)
{
  int status = 0;

  hb_test_init (&argc, &argv);

  /* Register font-free API tests first so the missing-font bail
   * below doesn't prevent them from running. */
  hb_test_add (test_push_clip_path_nil_returns_null);
  hb_test_add (test_push_clip_path_round_trip);
  hb_test_add (test_fill_glyph_nil_decomposes);
  hb_test_add (test_color_glyph_user_data);
  hb_test_add (test_hb_paint_budget);
  hb_test_add (test_hb_paint_color_stops_budget);
  hb_test_add (test_hb_paint_sweep_gradient_tiles_clipped);

  for (unsigned int i = 0; i < G_N_ELEMENTS (paint_tests); i++)
  {
    hb_test_add_data_flavor (&paint_tests[i], paint_tests[i].output, test_hb_paint_ot);
    hb_test_add_data_flavor (&paint_tests[i], paint_tests[i].output, test_hb_paint_ft);
  }

  hb_face_t *face = hb_test_open_font_file (TEST_GLYPHS);
  unsigned glyph_count = hb_face_get_glyph_count (face);
  const char **font_funcs = hb_font_list_funcs ();
  for (const char **font_func = font_funcs; *font_func; font_func++)
  {
    if (strcmp (*font_func, "ot") == 0)
      continue;
    if (!have_ft_colrv1 () && strcmp (*font_func, "ft") == 0)
      continue;
    for (unsigned int i = 1; i < glyph_count; i++)
    {
      char buf[32];
      snprintf (buf, 32, "test-%s-%u", *font_func, i);
      hb_test_add_data_flavor (GUINT_TO_POINTER (i), buf, test_compare_to_ot_novf);
      hb_test_add_data_flavor (GUINT_TO_POINTER (i), buf, test_compare_to_ot_vf);
    }
  }
  hb_face_destroy (face);

  hb_test_add (test_color_stops_ot);
  hb_test_add (test_color_stops_ft);
  hb_test_add (test_ft_reentrant_paint);

  status = hb_test_run();

  return status;
}
