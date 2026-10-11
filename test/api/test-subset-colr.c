/*
 * Copyright © 2020  Google, Inc.
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
 * Google Author(s): Calder Kitagawa
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for COLR subsetting */

static void
test_subset_colr_noop (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/TwemojiMozilla.subset.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, '2');
  hb_set_add (codepoints, 0x3297);
  hb_set_add (codepoints, 0x3299);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face, face_subset, HB_TAG ('C','O','L','R'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_colr_keep_one_colr_glyph (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/TwemojiMozilla.subset.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/TwemojiMozilla.subset.default.3297.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x3297);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','O','L','R'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

static void
test_subset_colr_keep_mixed_glyph (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/TwemojiMozilla.subset.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/TwemojiMozilla.subset.default.32,3299.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, '2');
  hb_set_add (codepoints, 0x3299);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','O','L','R'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

static void
test_subset_colr_keep_no_colr_glyph (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/TwemojiMozilla.subset.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/TwemojiMozilla.subset.default.32.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, '2');
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','O','L','R'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

static unsigned
read_uint_be (const unsigned char *p, unsigned count)
{
  unsigned value = 0;
  for (unsigned i = 0; i < count; i++)
    value = (value << 8) | p[i];
  return value;
}

static void
test_subset_colr_wide_scales (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-wide-scale.ttf");
  for (unsigned composed = 0; composed < 2; composed++)
  {
    hb_face_t *source = hb_face_reference (face);
    if (composed)
    {
      hb_subset_input_t *input = hb_subset_input_create_or_fail ();
      hb_set_add_range (hb_subset_input_glyph_set (input), 0, 16);
      g_assert_true (hb_subset_input_pin_axis_location (input, source, HB_TAG ('w','d','t','h'), 0));
      hb_face_t *partial = hb_subset_test_create_subset (source, input);
      hb_face_destroy (source);
      source = partial;
    }
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add_range (hb_subset_input_glyph_set (input), 0, 16);
    g_assert_true (hb_subset_input_pin_axis_location (input, source, HB_TAG ('w','g','h','t'), 1));
    if (!composed)
      g_assert_true (hb_subset_input_pin_axis_location (input, source, HB_TAG ('w','d','t','h'), 0));
    hb_face_t *subset = hb_subset_test_create_subset (source, input);
    hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('C','O','L','R'));
    unsigned length;
    const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
    g_assert_cmpuint (length, >=, 34);
    unsigned list = read_uint_be (data + 14, 4);
    g_assert_cmpuint (list + 4 + 16 * 6, <=, length);
    g_assert_cmpuint (read_uint_be (data + list, 4), ==, 16);
    for (unsigned i = 0; i < 16; i++)
    {
      unsigned paint = list + read_uint_be (data + list + 4 + i * 6 + 2, 4);
      g_assert_cmpuint (paint + 7, <=, length);
      g_assert_cmpuint (data[paint], ==, 12); /* PaintTransform */
      float sx = i % 8 < 4 ? 2.5f : -2.5f;
      float sy = i % 4 < 2 ? -sx : sx;
      float cx = i % 2 ? (i < 8 ? 40 : 30003) : 0;
      float cy = i % 2 ? (i < 8 ? -34 : -30005) : 0;
      float xx = 1, yy = 1, dx = 0, dy = 0;
      unsigned depth = 0;
      while (data[paint] == 12)
      {
        g_assert_cmpuint (paint + 7, <=, length);
        unsigned transform = paint + read_uint_be (data + paint + 4, 3);
        g_assert_cmpuint (transform + 24, <=, length);
        float values[6];
        for (unsigned j = 0; j < 6; j++)
          values[j] = (int32_t) read_uint_be (data + transform + j * 4, 4) / 65536.f;
        g_assert_cmpfloat (values[1], ==, 0);
        g_assert_cmpfloat (values[2], ==, 0);
        dx += xx * values[4];
        dy += yy * values[5];
        xx *= values[0];
        yy *= values[3];
        paint += read_uint_be (data + paint + 1, 3);
        g_assert_cmpuint (paint, <, length);
        g_assert_cmpuint (++depth, <=, 5);
      }
      g_assert_cmpuint (data[paint], ==, 2); /* PaintSolid */
      g_assert_cmpfloat (xx, ==, sx);
      g_assert_cmpfloat (yy, ==, sy);
      g_assert_cmpfloat (dx, ==, cx * (1 - sx));
      g_assert_cmpfloat (dy, ==, cy * (1 - sy));
    }
    hb_blob_destroy (blob);
    hb_face_destroy (subset);
    hb_face_destroy (source);
  }
  hb_face_destroy (face);
}

static hb_face_t *
instance_colr (hb_face_t *face, float weight, float width, hb_bool_t pin_weight, hb_bool_t pin_width)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, 3);
  if (pin_weight)
    g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','g','h','t'), weight));
  if (pin_width)
    g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','d','t','h'), width));
  return hb_subset_test_create_subset (face, input);
}

static void
test_subset_colr_partial_overflow (void)
{
  const char *files[] = {"fonts/colr-partial-overflow.ttf", "fonts/colr-partial-overflow-mapped.ttf"};
  for (unsigned file = 0; file < G_N_ELEMENTS (files); file++)
  {
    hb_face_t *face = hb_test_open_font_file (files[file]);
    hb_face_t *partial = instance_colr (face, 1, 0, true, false);
    /* Retained deltas bring alpha, scale, and stop offsets back into range. */
    const float widths[] = {0.5f, 0.75f, 1.f};
    for (unsigned i = 0; i < G_N_ELEMENTS (widths); i++)
    {
      hb_face_t *direct = instance_colr (face, 1, widths[i], true, true);
      hb_face_t *composed = instance_colr (partial, 0, widths[i], false, true);
      hb_subset_test_check (direct, composed, HB_TAG ('C','O','L','R'));
      hb_face_destroy (composed);
      hb_face_destroy (direct);
    }
    hb_face_destroy (partial);

    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add_range (hb_subset_input_glyph_set (input), 0, 3);
    g_assert_true (hb_subset_input_set_axis_range (input, face, HB_TAG ('w','g','h','t'), 0, 1, 0.5f));
    partial = hb_subset_test_create_subset (face, input);
    for (unsigned i = 0; i < G_N_ELEMENTS (widths); i++)
    {
      float weight = i * 0.5f;
      hb_face_t *direct = instance_colr (face, weight, widths[i], true, true);
      hb_face_t *composed = instance_colr (partial, weight, widths[i], true, true);
      hb_subset_test_check (direct, composed, HB_TAG ('C','O','L','R'));
      hb_face_destroy (composed);
      hb_face_destroy (direct);
    }
    hb_face_destroy (partial);
    hb_face_destroy (face);
  }
}

typedef struct {
  float xx, dx, radius;
} geometry_t;

static void
record_transform (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data,
                  float xx, float yx HB_UNUSED, float xy HB_UNUSED, float yy HB_UNUSED,
                  float dx, float dy HB_UNUSED, void *user_data HB_UNUSED)
{
  geometry_t *geometry = paint_data;
  geometry->xx = xx;
  geometry->dx = dx;
}

static void
record_radial (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data,
               hb_color_line_t *line HB_UNUSED,
               float x0 HB_UNUSED, float y0 HB_UNUSED, float r0,
               float x1 HB_UNUSED, float y1 HB_UNUSED, float r1 HB_UNUSED,
               void *user_data HB_UNUSED)
{
  ((geometry_t *) paint_data)->radius = r0;
}

static void
test_subset_colr_geometry_overflow (void)
{
  const char *files[] = {"fonts/colr-geometry-overflow.ttf", "fonts/colr-geometry-overflow-mapped.ttf"};
  for (unsigned file = 0; file < G_N_ELEMENTS (files); file++)
  {
    hb_face_t *face = hb_test_open_font_file (files[file]);
    hb_face_t *partial = instance_colr (face, 1, 0, true, false);
    hb_font_t *font = hb_font_create (partial);
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
    hb_paint_funcs_set_push_transform_func (funcs, record_transform, NULL, NULL);
    hb_paint_funcs_set_radial_gradient_func (funcs, record_radial, NULL, NULL);
    geometry_t geometry = {0};
    hb_font_paint_glyph (font, 1, funcs, &geometry, 0, HB_COLOR (0, 0, 0, 255));
    g_assert_cmpfloat (geometry.dx, ==, 40000.f);
    hb_font_paint_glyph (font, 2, funcs, &geometry, 0, HB_COLOR (0, 0, 0, 255));
    g_assert_cmpfloat (geometry.radius, ==, -100.f);
    hb_font_paint_glyph (font, 3, funcs, &geometry, 0, HB_COLOR (0, 0, 0, 255));
    g_assert_cmpfloat (geometry.xx, ==, 40000.f);
    hb_paint_funcs_destroy (funcs);
    hb_font_destroy (font);
    const float widths[] = {0.5f, 0.75f, 1.f};
    for (unsigned i = 0; i < G_N_ELEMENTS (widths); i++)
    {
      hb_face_t *direct = instance_colr (face, 1, widths[i], true, true);
      hb_face_t *composed = instance_colr (partial, 0, widths[i], false, true);
      hb_subset_test_check (direct, composed, HB_TAG ('C','O','L','R'));
      hb_face_destroy (composed);
      hb_face_destroy (direct);
    }
    hb_face_destroy (partial);
    hb_face_destroy (face);
  }
}

static void
test_subset_colr_long_delta_sums (void)
{
  const char *files[] = {"fonts/colr-long-sums-positive.ttf", "fonts/colr-long-sums-positive-mapped.ttf",
                         "fonts/colr-long-sums-negative.ttf", "fonts/colr-long-sums-negative-mapped.ttf"};
  for (unsigned file = 0; file < G_N_ELEMENTS (files); file++)
  {
    hb_face_t *face = hb_test_open_font_file (files[file]);
    for (unsigned weight = 0; weight < 2; weight++)
    {
      hb_face_t *partial = instance_colr (face, weight, 0, true, false);
      float width = (weight ? 16383.f : 1.f) / 16384;
      hb_face_t *direct = instance_colr (face, weight, width, true, true);
      hb_face_t *composed = instance_colr (partial, 0, width, false, true);
      hb_subset_test_check (direct, composed, HB_TAG ('C','O','L','R'));
      hb_face_destroy (composed);
      hb_face_destroy (direct);
      hb_face_destroy (partial);
    }
    hb_face_destroy (face);
  }
}

static void
test_subset_colr_constant_default (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-constant-bias.ttf");
  for (unsigned i = 0; i < 2; i++)
  {
    hb_face_t *subset = instance_colr (face, i, 0, true, true);
    hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('C','O','L','R'));
    unsigned length;
    const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
    g_assert_cmpuint (length, >=, 34);
    unsigned list = read_uint_be (data + 14, 4);
    g_assert_cmpuint (list + 10, <=, length);
    unsigned paint = list + read_uint_be (data + list + 6, 4);
    g_assert_cmpuint (paint + 5, <=, length);
    g_assert_cmpuint (data[paint], ==, 2); /* PaintSolid */
    g_assert_cmpuint (read_uint_be (data + paint + 3, 2), ==, 16384);
    hb_blob_destroy (blob);
    hb_face_destroy (subset);
  }
  hb_face_t *partial = instance_colr (face, 0, 0, true, false);
  hb_face_t *direct = instance_colr (face, 0, 0, true, true);
  hb_face_t *composed = instance_colr (partial, 0, 0, false, true);
  hb_subset_test_check (direct, composed, HB_TAG ('C','O','L','R'));
  hb_face_destroy (composed);
  hb_face_destroy (direct);
  hb_face_destroy (partial);
  hb_face_destroy (face);
}

static void
record_alpha (hb_paint_funcs_t *funcs HB_UNUSED,
              void *paint_data,
              hb_bool_t foreground HB_UNUSED,
              hb_color_t color,
              void *user_data HB_UNUSED)
{
  *(unsigned *) paint_data = hb_color_get_alpha (color);
}

static void
test_subset_colr_default_paint (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-constant-bias.ttf");
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_color_func (funcs, record_alpha, NULL, NULL);
  for (unsigned composed = 0; composed < 2; composed++)
  {
    hb_face_t *source = composed ? instance_colr (face, 0, 0, true, false) : hb_face_reference (face);
    hb_font_t *font = hb_font_create (source);
    for (unsigned explicit_coords = 0; explicit_coords < 2; explicit_coords++)
    {
      if (explicit_coords)
      {
        const int coords[] = {0, 0};
        hb_font_set_var_coords_normalized (font, coords, composed ? 1 : 2);
      }
      unsigned alpha = 999;
      hb_font_paint_glyph (font, 1, funcs, &alpha, 0, HB_COLOR (0, 0, 0, 255));
      g_assert_cmpuint (alpha, ==, 255);
    }
    hb_font_destroy (font);
    hb_face_destroy (source);
  }
  hb_paint_funcs_destroy (funcs);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_colr_noop);
  hb_test_add (test_subset_colr_keep_one_colr_glyph);
  hb_test_add (test_subset_colr_keep_mixed_glyph);
  hb_test_add (test_subset_colr_keep_no_colr_glyph);
  hb_test_add (test_subset_colr_wide_scales);
  hb_test_add (test_subset_colr_partial_overflow);
  hb_test_add (test_subset_colr_constant_default);
  hb_test_add (test_subset_colr_default_paint);
  hb_test_add (test_subset_colr_geometry_overflow);
  hb_test_add (test_subset_colr_long_delta_sums);

  return hb_test_run();
}
