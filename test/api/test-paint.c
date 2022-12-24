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
#endif

/* Unit tests for hb-paint.h */

/* ---- */

typedef struct {
  int level;
  GString *string;
} paint_data_t;

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
push_transform (hb_paint_funcs_t *funcs,
                void *paint_data,
                float xx, float yx,
                float xy, float yy,
                float dx, float dy,
                void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "start transform %f %f %f %f %f %f", xx, yx, xy, yy, dx, dy);
  data->level++;
}

static void
pop_transform (hb_paint_funcs_t *funcs,
               void *paint_data,
               void *user_data)
{
  paint_data_t *data = user_data;

  data->level--;
  print (data, "end transform");
}

static void
push_clip_glyph (hb_paint_funcs_t *funcs,
                 void *paint_data,
                 hb_codepoint_t glyph,
                 hb_font_t *font,
                 void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "start clip glyph %u", glyph);
  data->level++;
}

static void
push_clip_rectangle (hb_paint_funcs_t *funcs,
                     void *paint_data,
                     float xmin, float ymin, float xmax, float ymax,
                     void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "start clip rectangle %f %f %f %f", xmin, ymin, xmax, ymax);
  data->level++;
}

static void
pop_clip (hb_paint_funcs_t *funcs,
          void *paint_data,
          void *user_data)
{
  paint_data_t *data = user_data;

  data->level--;
  print (data, "end clip");
}

static void
paint_color (hb_paint_funcs_t *funcs,
             void *paint_data,
             hb_bool_t use_foreground,
             hb_color_t color,
             void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "solid %d %d %d %d",
         hb_color_get_red (color),
         hb_color_get_green (color),
         hb_color_get_blue (color),
         hb_color_get_alpha (color));
}

static void
paint_image (hb_paint_funcs_t *funcs,
             void *paint_data,
             hb_blob_t *blob,
             unsigned int width,
             unsigned int height,
             hb_tag_t format,
             float slant,
             hb_glyph_extents_t *extents,
             void *user_data)
{
  paint_data_t *data = user_data;
  char buf[5] = { 0, };

  hb_tag_to_string (format, buf);
  print (data, "image type %s size %u %u slant %f extents %d %d %d %d\n",
         buf, width, height, slant,
         extents->x_bearing, extents->y_bearing, extents->width, extents->height);
}

static void
print_color_line (paint_data_t *data,
                  hb_color_line_t *color_line)
{
  hb_color_stop_t *stops;
  unsigned int len;

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = alloca (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);

  print (data, "colors");
  data->level += 1;
  for (unsigned int i = 0; i < len; i++)
    print (data, "%f %d %d %d %d",
           stops[i].offset,
           hb_color_get_red (stops[i].color),
           hb_color_get_green (stops[i].color),
           hb_color_get_blue (stops[i].color),
           hb_color_get_alpha (stops[i].color));
  data->level -= 1;
}

static void
paint_linear_gradient (hb_paint_funcs_t *funcs,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0,
                       float x1, float y1,
                       float x2, float y2,
                       void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "linear gradient");
  data->level += 1;
  print (data, "p0 %f %f", x0, y0);
  print (data, "p1 %f %f", x1, y1);
  print (data, "p2 %f %f", x2, y2);

  print_color_line (data, color_line);
  data->level -= 1;
}

static void
paint_radial_gradient (hb_paint_funcs_t *funcs,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0, float r0,
                       float x1, float y1, float r1,
                       void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "radial gradient");
  data->level += 1;
  print (data, "p0 %f %f radius %f", x0, y0, r0);
  print (data, "p1 %f %f radius %f", x1, y1, r1);

  print_color_line (data, color_line);
  data->level -= 1;
}

static void
paint_sweep_gradient (hb_paint_funcs_t *funcs,
                      void *paint_data,
                      hb_color_line_t *color_line,
                      float cx, float cy,
                      float start_angle,
                      float end_angle,
                      void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "sweep gradient");
  data->level++;
  print (data, "center %f %f", cx, cy);
  print (data, "angles %f %f", start_angle, end_angle);

  print_color_line (data, color_line);
  data->level -= 1;
}

static void
push_group (hb_paint_funcs_t *funcs,
            void *paint_data,
            void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "push group");
  data->level++;
}

static void
pop_group (hb_paint_funcs_t *funcs,
           void *paint_data,
           hb_paint_composite_mode_t mode,
           void *user_data)
{
  paint_data_t *data = user_data;
  data->level--;
  print (data, "pop group mode %d", mode);
}

typedef struct {
  const char *font_file;
  int scale;
  float slant;
  hb_codepoint_t glyph;
  unsigned int palette;
  const char *output;
} paint_test_t;

#define NOTO_HAND   "fonts/noto_handwriting-cff2_colr_1.otf"
#define TEST_GLYPHS "fonts/test_glyphs-glyf_colr_1.ttf"
#define ROCHER_ABC  "fonts/RocherColorGX.abc.ttf"

/* To verify the rendering visually, use
 *
 * hb-view --font-size SCALE --font-slant SLANT --font-palette PALETTE FONT --glyphs [gidGID=0+1000]
 *
 * where GID is the glyph value of the test.
 */
static paint_test_t paint_tests[] = {
  /* COLRv1 */
  { NOTO_HAND,   20, 0.,  10,   0, "hand-20-0-10" },
  { NOTO_HAND,   20, 0.2, 10,   0, "hand-20-0.2-10" },
  { TEST_GLYPHS, 20, 0,    6,   0, "test-20-0-6" },   // linear gradient
  { TEST_GLYPHS, 20, 0,   10,   0, "test-20-0-10" },  // sweep gradient
  { TEST_GLYPHS, 20, 0,   92,   0, "test-20-0-92" },  // radial gradient
  { TEST_GLYPHS, 20, 0,  106,   0, "test-20-0-106" },
  { TEST_GLYPHS, 20, 0,  116,   0, "test-20-0-116" }, // compositing
  { TEST_GLYPHS, 20, 0,  123,   0, "test-20-0-123" },
  { TEST_GLYPHS, 20, 0,  165,   0, "test-20-0-165" }, // linear gradient
  { TEST_GLYPHS, 20, 0,  175,   0, "test-20-0-175" }, // layers
  /* COLRv0 */
  { ROCHER_ABC, 120, 0.3,  1,   0, "rocher-120-0.3-1" },
  { ROCHER_ABC, 120, 0.3,  2,   2, "rocher-120-0.3-2" },
  { ROCHER_ABC, 120, 0,    3, 200, "rocher-120-0-3" },
};

#ifdef HB_HAS_FREETYPE
static FT_Library library;
#endif

static void
test_hb_paint (gconstpointer d,
               hb_bool_t     use_ft)
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

#ifdef HB_HAS_FREETYPE
  if (use_ft)
  {
    FT_Face ft_face;
    char *path;

    path = g_test_build_filename (G_TEST_DIST, test->font_file, NULL);
    if (FT_New_Face (library, path, 0, &ft_face) != 0)
    {
      g_test_message ("Failed to create FT_Face for %s", path);
      g_test_fail ();
      g_free (path);
      return;
    }
    face = hb_ft_face_create_referenced (ft_face);
    FT_Done_Face (ft_face);
    g_free (path);
  }
  else
#endif
  {
    face = hb_test_open_font_file (test->font_file);
  }

  font = hb_font_create (face);
  hb_font_set_scale (font, test->scale, test->scale);
  hb_font_set_synthetic_slant (font, test->slant);

  funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_transform_func (funcs, push_transform, &data, NULL);
  hb_paint_funcs_set_pop_transform_func (funcs, pop_transform, &data, NULL);
  hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, &data, NULL);
  hb_paint_funcs_set_push_clip_rectangle_func (funcs, push_clip_rectangle, &data, NULL);
  hb_paint_funcs_set_pop_clip_func (funcs, pop_clip, &data, NULL);
  hb_paint_funcs_set_push_group_func (funcs, push_group, &data, NULL);
  hb_paint_funcs_set_pop_group_func (funcs, pop_group, &data, NULL);
  hb_paint_funcs_set_color_func (funcs, paint_color, &data, NULL);
  hb_paint_funcs_set_image_func (funcs, paint_image, &data, NULL);
  hb_paint_funcs_set_linear_gradient_func (funcs, paint_linear_gradient, &data, NULL);
  hb_paint_funcs_set_radial_gradient_func (funcs, paint_radial_gradient, &data, NULL);
  hb_paint_funcs_set_sweep_gradient_func (funcs, paint_sweep_gradient, &data, NULL);

  data.string = g_string_new ("");
  data.level = 0;

  hb_font_paint_glyph (font, test->glyph, funcs, &data, 0, HB_COLOR (0, 0, 0, 255));

  /* Run
   *
   * GENERATE_DATA=1 G_TEST_SRCDIR=./test/api ./build/test/api/test-ot-color -p TESTCASE > test/api/results/OUTPUT
   *
   * to produce the expected results file.
   */
  if (getenv ("GENERATE_DATA"))
    {
      g_print ("%s", data.string->str);
      exit (0);
    }

  file = g_test_build_filename (G_TEST_DIST, "results", test->output, NULL);
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
    g_test_message ("Unexpected number of lines in output (%d instead of %d)", g_strv_length (lines), g_strv_length (expected));
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

        g_test_message ("Unxpected output at %d:%d (%#x instead of %#x):\n%s", i, pos, (unsigned int)lines[i][pos], (unsigned int)expected[i][pos], data.string->str);
        g_test_fail ();
      }
    }
  }

  g_strfreev (lines);
  g_strfreev (expected);

  g_free (buffer);
  g_free (file);

  g_string_free (data.string, TRUE);

  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_hb_paint_ot (gconstpointer data)
{
  test_hb_paint (data, 0);
}

static void
test_hb_paint_ft (gconstpointer data)
{
#ifdef HB_HAS_FREETYPE
  test_hb_paint (data, 1);
#else
  g_test_skip_printf ("freetype support not present");
#endif
}

static void
scrutinize_linear_gradient (hb_paint_funcs_t *funcs,
                            void *paint_data,
                            hb_color_line_t *color_line,
                            float x0, float y0,
                            float x1, float y1,
                            float x2, float y2,
                            void *user_data)
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
test_color_stops (hb_bool_t use_ft)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_paint_funcs_t *funcs;
  hb_bool_t result = FALSE;

#ifdef HB_HAS_FREETYPE
  if (use_ft)
  {
    FT_Face ft_face;
    char *path;

    path = g_test_build_filename (G_TEST_DIST, NOTO_HAND, NULL);
    if (FT_New_Face (library, path, 0, &ft_face) != 0)
    {
      g_test_message ("Failed to create FT_Face for %s", path);
      g_test_fail ();
      g_free (path);
      return;
    }
    face = hb_ft_face_create_referenced (ft_face);
    FT_Done_Face (ft_face);
    g_free (path);
  }
  else
#endif
    face = hb_test_open_font_file (NOTO_HAND);

  font = hb_font_create (face);

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
  test_color_stops (1);
}

int
main (int argc, char **argv)
{
  int status = 0;

#ifdef HB_HAS_FREETYPE
  FT_Init_FreeType (&library);
#endif

  hb_test_init (&argc, &argv);
  for (unsigned int i = 0; i < G_N_ELEMENTS (paint_tests); i++)
  {
    hb_test_add_data_flavor (&paint_tests[i], paint_tests[i].output, test_hb_paint_ot);
    hb_test_add_data_flavor (&paint_tests[i], paint_tests[i].output, test_hb_paint_ft);
  }
  hb_test_add (test_color_stops_ot);
  hb_test_add (test_color_stops_ft);

  status = hb_test_run();

  return status;
}
