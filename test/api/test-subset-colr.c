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
#include <math.h>

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
instance_colr_at (hb_face_t *face, unsigned pins, float weight)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, 30);
  if (pins & 1) g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','g','h','t'), weight));
  if (pins & 2) g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','d','t','h'), 0));
  return hb_subset_test_create_subset (face, input);
}

static void
check_clip_rounding (hb_face_t *face, float weight, bool variable)
{
  const int deltas[][4] = {{-1, -1, 1, 1}, {1, 1, -1, -1},
                           {1, -1, 1, -1}, {-1, 1, -1, 1}};
  hb_font_t *font = hb_font_create (face);
  hb_ot_font_set_funcs (font);
  if (variable)
  {
    int coords[] = {(int) (weight * 16384), 0};
    hb_font_set_var_coords_normalized (font, coords, 2);
  }
  for (unsigned i = 0; i < 4; i++)
  {
    int xMin = (int) floorf (50 + weight * deltas[i][0]);
    int yMin = (int) floorf (50 + weight * deltas[i][1]);
    int xMax = (int) ceilf (500 + weight * deltas[i][2]);
    int yMax = (int) ceilf (500 + weight * deltas[i][3]);
    hb_glyph_extents_t extents;
    g_assert_true (hb_font_get_glyph_extents (font, i + 1, &extents));
    g_assert_cmpint (extents.x_bearing, ==, xMin);
    g_assert_cmpint (extents.y_bearing, ==, yMax);
    g_assert_cmpint (extents.width, ==, xMax - xMin);
    g_assert_cmpint (extents.height, ==, yMin - yMax);
  }
  hb_font_destroy (font);
}

static void
test_subset_colr_clip_rounding (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-clip-rounding.ttf");
  for (unsigned step = 0; step <= 4; step++)
  {
    float weight = step / 4.f;
    check_clip_rounding (face, weight, true);
    for (unsigned first = 0; first <= 2; first++)
    {
      hb_face_t *partial = first ? instance_colr_at (face, first, weight) : hb_face_reference (face);
      hb_face_t *full = instance_colr_at (partial, first ? 3 - first : 3, weight);
      check_clip_rounding (full, weight, false);
      hb_face_destroy (full);
      hb_face_destroy (partial);
    }
  }
  hb_face_destroy (face);
}

static void
test_subset_colr_negative_radii (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-negative-radii.ttf");
  hb_face_t *full = instance_colr_at (face, 3, 1);
  hb_blob_t *blob = hb_face_reference_table (full, HB_TAG ('C','O','L','R'));
  unsigned length;
  const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, >=, 34);
  unsigned list = read_uint_be (data + 14, 4);
  g_assert_cmpuint (list + 4 + 30 * 6, <=, length);
  g_assert_cmpuint (read_uint_be (data + list, 4), ==, 30);
  for (unsigned i = 0; i < 30; i++)
  {
    unsigned paint = list + read_uint_be (data + list + 4 + i * 6 + 2, 4);
    g_assert_cmpuint (paint + 5, <=, length);
    unsigned variant = i % 5, extend = i / 5 % 3;
    if (variant == 4)
    {
      g_assert_cmpuint (data[paint], ==, 2);
      g_assert_cmpuint (read_uint_be (data + paint + 3, 2), ==, 0);
      continue;
    }
    g_assert_cmpuint (data[paint], ==, 6);
    g_assert_cmpuint (paint + 16, <=, length);
    const int expected[][6] = {
      {410, 215, 500, 810, 415, 1100},
      {-90, -35, 650, 310, 165, 50},
      {-790, -385, 100, -390, -185, 0},
      {810, 415, 0, 1210, 615, 100}
    };
    for (unsigned j = 0; j < 6; j++)
    {
      int value = read_uint_be (data + paint + 4 + j * 2, 2);
      if (j % 3 != 2) value = (int16_t) value;
      int wanted = expected[variant][j];
      if (variant == 1 && extend) wanted = (int[]) {-390, -185, 1100, 10, 15, 500}[j];
      g_assert_cmpint (value, ==, wanted);
    }
    unsigned line = paint + read_uint_be (data + paint + 1, 3);
    g_assert_cmpuint (line + 3, <=, length);
    g_assert_cmpuint (data[line], ==, extend);
    unsigned count = read_uint_be (data + line + 1, 2);
    unsigned wanted_count = !extend && variant >= 2 ? 1 : 3;
    g_assert_cmpuint (count, ==, wanted_count);
    g_assert_cmpuint (line + 3 + count * 6, <=, length);
    for (unsigned j = 0; j < count; j++)
    {
      double offset = (int16_t) read_uint_be (data + line + 3 + j * 6, 2) / 16384.;
      double alpha = (int16_t) read_uint_be (data + line + 7 + j * 6, 2) / 16384.;
      double wanted_offset = 0, wanted_alpha = (double[]) {0.25, 0.5, 1}[j];
      if (!extend && variant == 0) wanted_offset = -1 + j * 0.5;
      if (!extend && variant == 1) wanted_offset = 0.25 + j * 0.5;
      if (!extend && variant == 2) wanted_alpha = 0.25;
      if (!extend && variant == 3) wanted_alpha = 1;
      if (extend) wanted_offset = (extend == 2 && variant < 2 ? -1 : -2) + j * 0.5;
      g_assert_cmpfloat (offset, ==, wanted_offset);
      g_assert_cmpfloat (alpha, ==, wanted_alpha);
    }
  }
  hb_blob_destroy (blob);
  for (unsigned first = 1; first <= 2; first++)
  {
    hb_face_t *partial = instance_colr_at (face, first, 1);
    hb_face_t *composed = instance_colr_at (partial, 3 - first, 1);
    hb_subset_test_check (full, composed, HB_TAG ('C','O','L','R'));
    hb_face_destroy (composed);
    hb_face_destroy (partial);
  }
  hb_face_destroy (full);
  hb_face_destroy (face);
}

static void
test_subset_colr_negative_radii_precision (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-negative-radii-precision.ttf");
  hb_face_t *subset = instance_colr_at (face, 3, 1);
  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('C','O','L','R'));
  unsigned length;
  const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, >=, 34);
  unsigned list = read_uint_be (data + 14, 4);
  unsigned paint = list + read_uint_be (data + list + 6, 4);
  g_assert_cmpuint (paint + 16, <=, length);
  g_assert_cmpuint (data[paint], ==, 6);
  g_assert_cmpuint (read_uint_be (data + paint + 8, 2), ==, 650);
  g_assert_cmpuint (read_uint_be (data + paint + 14, 2), ==, 50);
  unsigned line = paint + read_uint_be (data + paint + 1, 3);
  g_assert_cmpuint (line + 21, <=, length);
  g_assert_cmpuint (read_uint_be (data + line + 1, 2), ==, 3);
  const int offsets[] = {4096, 4097, 20480};
  for (unsigned i = 0; i < 3; i++)
    g_assert_cmpint ((int16_t) read_uint_be (data + line + 3 + i * 6, 2), ==, offsets[i]);
  hb_blob_destroy (blob);
  for (unsigned first = 1; first <= 2; first++)
  {
    hb_face_t *partial = instance_colr_at (face, first, 1);
    hb_face_t *composed = instance_colr_at (partial, 3 - first, 1);
    hb_subset_test_check (subset, composed, HB_TAG ('C','O','L','R'));
    hb_face_destroy (composed);
    hb_face_destroy (partial);
  }
  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
check_affine_values (hb_face_t *face)
{
  hb_blob_t *blob = hb_face_reference_table (face, HB_TAG ('C','O','L','R'));
  unsigned length;
  const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, >=, 34);
  unsigned list = read_uint_be (data + 14, 4);
  g_assert_cmpuint (list + 4 + 28 * 6, <=, length);
  g_assert_cmpuint (read_uint_be (data + list, 4), ==, 28);
  for (unsigned i = 0; i < 28; i++)
  {
    unsigned paint = list + read_uint_be (data + list + 6 + i * 6, 4);
    g_assert_cmpuint (paint + 7, <=, length);
    g_assert_cmpuint (data[paint], ==, 12);
    unsigned affine = paint + read_uint_be (data + paint + 4, 3);
    g_assert_cmpuint (affine + 24, <=, length);
    int32_t expected[6] = {65536, 0, 0, 65536, 0, 0};
    int sign = i % 4 < 2 ? 1 : -1, delta = i % 2 ? -1 : 1;
    if (i < 24)
      expected[i / 4] = sign * ((i / 4 < 4 ? 1 : 1000) * 65536 + 1) + delta;
    else
      expected[4] = sign * (1000 * 65536 + (i % 2 ? 2 : 1));
    for (unsigned j = 0; j < 6; j++)
      g_assert_cmpint ((int32_t) read_uint_be (data + affine + j * 4, 4), ==, expected[j]);
  }
  hb_blob_destroy (blob);
}

static void
test_subset_colr_affine_values (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-affine-values.ttf");
  for (unsigned first = 0; first <= 2; first++)
  {
    hb_face_t *partial = first ? instance_colr_at (face, first, 1) : hb_face_reference (face);
    hb_face_t *full = instance_colr_at (partial, first ? 3 - first : 3, 1);
    check_affine_values (full);
    hb_face_destroy (full);
    hb_face_destroy (partial);
  }
  hb_face_destroy (face);
}

static void
test_subset_colr_sentinel_map_without_variations (void)
{
  const char *fonts[] = {
    "fonts/colr-null-store-sentinel-map.ttf",
    "fonts/colr-empty-store-sentinel-map.ttf"
  };
  for (unsigned f = 0; f < G_N_ELEMENTS (fonts); f++)
  {
    hb_face_t *face = hb_test_open_font_file (fonts[f]);
    for (unsigned axes = 0; axes <= 3; axes++)
    {
      hb_face_t *subset = instance_colr_at (face, axes, 1);
      hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('C','O','L','R'));
      unsigned length;
      const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
      g_assert_cmpuint (length, >=, 34);
      unsigned list = read_uint_be (data + 14, 4);
      g_assert_cmpuint (list + 10, <=, length);
      g_assert_cmpuint (read_uint_be (data + list, 4), ==, 1);
      unsigned paint = list + read_uint_be (data + list + 6, 4);
      g_assert_cmpuint (paint + 16, <=, length);
      g_assert_cmpuint (data[paint], ==, axes == 3 ? 6 : 7);
      g_assert_cmpuint (read_uint_be (data + paint + 8, 2), ==, 10);
      g_assert_cmpuint (read_uint_be (data + paint + 10, 2), ==, 100);
      g_assert_cmpuint (read_uint_be (data + paint + 14, 2), ==, 20);
      unsigned line = paint + read_uint_be (data + paint + 1, 3);
      unsigned stride = axes == 3 ? 6 : 10;
      g_assert_cmpuint (line + 3 + 2 * stride, <=, length);
      g_assert_cmpuint (data[line], ==, 0);
      g_assert_cmpuint (read_uint_be (data + line + 1, 2), ==, 2);
      for (unsigned stop = 0; stop < 2; stop++)
      {
        unsigned at = line + 3 + stop * stride;
        g_assert_cmpuint (read_uint_be (data + at, 2), ==, stop * 16384);
        g_assert_cmpuint (read_uint_be (data + at + 2, 2), ==, stop);
        g_assert_cmpuint (read_uint_be (data + at + 4, 2), ==, 16384);
      }
      hb_blob_destroy (blob);
      hb_face_destroy (subset);
    }
    hb_face_destroy (face);
  }
}

static void
check_colr_var_overflow (hb_face_t *face, bool variable)
{
  const unsigned formats[] = {5, 7, 9, 13, 15, 19, 23, 27, 31};
  const unsigned offsets[] = {4, 4, 4, 0, 4, 8, 6, 6, 8};
  const unsigned counts[] = {6, 6, 2, 6, 2, 2, 2, 2, 2};
  const int32_t affine_values[] = {INT32_MAX, INT32_MIN, INT32_MAX, INT32_MIN, 98604, -98607};
  hb_blob_t *blob = hb_face_reference_table (face, HB_TAG ('C','O','L','R'));
  unsigned length;
  const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, >=, 34);
  unsigned list = read_uint_be (data + 14, 4);
  unsigned num_paints = G_N_ELEMENTS (formats) * G_N_ELEMENTS (affine_values);
  g_assert_cmpuint (list + 4 + (num_paints + 18) * 6, <=, length);
  g_assert_cmpuint (read_uint_be (data + list, 4), ==, num_paints + 18);
  for (unsigned i = 0; i < num_paints; i++)
  {
    unsigned kind = i % G_N_ELEMENTS (formats), scenario = i / G_N_ELEMENTS (formats);
    unsigned paint = list + read_uint_be (data + list + 6 + i * 6, 4);
    g_assert_cmpuint (paint + 7, <=, length);
    g_assert_cmpuint (data[paint], ==, formats[kind] - !variable);
    unsigned values = paint + offsets[kind], width = 2;
    if (formats[kind] == 13)
    {
      values = paint + read_uint_be (data + paint + 4, 3);
      width = 4;
    }
    g_assert_cmpuint (values + counts[kind] * width, <=, length);
    for (unsigned j = 0; j < counts[kind]; j++)
    {
      unsigned raw = read_uint_be (data + values + j * width, width);
      int32_t actual = width == 4 ? (int32_t) raw : (int16_t) raw;
      int32_t expected = width == 4 ? affine_values[scenario] : scenario % 2 ? -32768 : 32767;
      if (formats[kind] == 7 && j % 3 == 2)
      {
        actual = raw;
        expected = 65535;
      }
      g_assert_cmpint (actual, ==, expected);
    }
  }
  unsigned clips = read_uint_be (data + 22, 4);
  g_assert_cmpuint (clips, >, 0);
  g_assert_cmpuint (clips + 5 + G_N_ELEMENTS (affine_values) * 7, <=, length);
  g_assert_cmpuint (read_uint_be (data + clips + 1, 4), ==, G_N_ELEMENTS (affine_values));
  for (unsigned i = 0; i < G_N_ELEMENTS (affine_values); i++)
  {
    unsigned record = clips + 5 + i * 7;
    g_assert_cmpuint (read_uint_be (data + record, 2), ==, i * G_N_ELEMENTS (formats) + 1);
    g_assert_cmpuint (read_uint_be (data + record + 2, 2), ==, i * G_N_ELEMENTS (formats) + 1);
    unsigned box = clips + read_uint_be (data + record + 4, 3);
    g_assert_cmpuint (box + 9, <=, length);
    g_assert_cmpuint (data[box], ==, variable ? 2 : 1);
    for (unsigned j = 0; j < 4; j++)
      g_assert_cmpint ((int16_t) read_uint_be (data + box + 1 + j * 2, 2), ==, i % 2 ? -32768 : 32767);
  }
  hb_blob_destroy (blob);
}

static void
check_colr_fixed_overflow (hb_face_t *face, bool variable)
{
  const int32_t scales[] = {INT32_MAX, INT32_MIN, INT32_MAX, INT32_MIN, 458752, -327692};
  hb_blob_t *blob = hb_face_reference_table (face, HB_TAG ('C','O','L','R'));
  unsigned length;
  const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, >=, 34);
  unsigned list = read_uint_be (data + 14, 4);
  g_assert_cmpuint (list + 4 + 72 * 6, <=, length);
  for (unsigned i = 0; i < 18; i++)
  {
    unsigned paint = list + read_uint_be (data + list + 6 + (54 + i) * 6, 4);
    int expected = i / 3 % 2 ? -32768 : 32767;
    int alpha = variable ? expected : expected > 0 ? 16384 : 0;
    g_assert_cmpuint (paint + 5, <=, length);
    if (i % 3 == 0)
    {
      g_assert_cmpuint (data[paint], ==, variable ? 3 : 2);
      g_assert_cmpint ((int16_t) read_uint_be (data + paint + 3, 2), ==, alpha);
    }
    else if (i % 3 == 1)
    {
      g_assert_cmpuint (paint + 12, <=, length);
      g_assert_cmpuint (data[paint], ==, variable ? 9 : 8);
      for (unsigned j = 0; j < 2; j++)
        g_assert_cmpint ((int16_t) read_uint_be (data + paint + 8 + j * 2, 2), ==, expected);
      unsigned line = paint + read_uint_be (data + paint + 1, 3);
      unsigned stride = variable ? 10 : 6;
      g_assert_cmpuint (line + 3 + 2 * stride, <=, length);
      g_assert_cmpuint (read_uint_be (data + line + 1, 2), ==, 2);
      for (unsigned j = 0; j < 2; j++)
      {
        g_assert_cmpint ((int16_t) read_uint_be (data + line + 3 + j * stride, 2), ==, expected);
        g_assert_cmpint ((int16_t) read_uint_be (data + line + 7 + j * stride, 2), ==, alpha);
      }
    }
    else if (variable)
    {
      g_assert_cmpuint (paint + 8, <=, length);
      g_assert_cmpuint (data[paint], ==, 17);
      for (unsigned j = 0; j < 2; j++)
        g_assert_cmpint ((int16_t) read_uint_be (data + paint + 4 + j * 2, 2), ==, expected);
    }
    else
    {
      g_assert_cmpuint (paint + 7, <=, length);
      g_assert_cmpuint (data[paint], ==, 12);
      unsigned affine = paint + read_uint_be (data + paint + 4, 3);
      g_assert_cmpuint (affine + 24, <=, length);
      for (unsigned j = 0; j < 6; j++)
        g_assert_cmpint ((int32_t) read_uint_be (data + affine + j * 4, 4), ==, j == 0 || j == 3 ? scales[i / 3] : 0);
    }
  }
  hb_blob_destroy (blob);
}

static void
test_subset_colr_var_overflow (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-var-overflow.ttf");
  for (unsigned first = 0; first <= 2; first++)
  {
    hb_face_t *source = hb_face_reference (face);
    for (unsigned step = 0; step < (first ? 2u : 1u); step++)
    {
      unsigned pins = first ? (step ? 3 - first : first) : 3;
      hb_subset_input_t *input = hb_subset_input_create_or_fail ();
      hb_set_add_range (hb_subset_input_glyph_set (input), 0, hb_face_get_glyph_count (source) - 1);
      if (pins & 1) g_assert_true (hb_subset_input_pin_axis_location (input, source, HB_TAG ('w','g','h','t'), 1));
      if (pins & 2) g_assert_true (hb_subset_input_pin_axis_location (input, source, HB_TAG ('w','d','t','h'), 0));
      hb_face_t *subset = hb_subset_test_create_subset (source, input);
      if (!step && first == 1)
      {
        check_colr_var_overflow (subset, true);
        check_colr_fixed_overflow (subset, true);
      }
      hb_face_destroy (source);
      source = subset;
    }
    check_colr_var_overflow (source, false);
    if (first != 1) check_colr_fixed_overflow (source, false);
    hb_face_destroy (source);
  }
  hb_face_destroy (face);
}

/* A PaintColrLayers whose layer range lies past the end of the LayerList
 * must not make the subsetter retain (and allocate mappings for) layers
 * that do not exist. */
static void
test_subset_colr_layers_out_of_range (void)
{
  const uint8_t maxp_data[6] = {
    0x00, 0x00, 0x50, 0x00,
    0x00, 0x02 /* numGlyphs */
  };
  const uint8_t colr_data[73] = {
    /* header: version 1, no COLRv0 records, BaseGlyphList @34, LayerList @50 */
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x32,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* BaseGlyphList: glyph 1 -> PaintColrLayers (numLayers 2, firstLayerIndex 0) */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A,
    0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
    /* LayerList: PaintSolid, then PaintColrLayers (numLayers 255, firstLayerIndex 0xFFFFFF00) */
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x11,
    0x02, 0x00, 0x00, 0x40, 0x00,
    0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
  };

  hb_blob_t *colr = hb_blob_create ((const char *) colr_data, sizeof (colr_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_blob_t *maxp = hb_blob_create ((const char *) maxp_data, sizeof (maxp_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_t *builder = hb_face_builder_create ();
  hb_face_builder_add_table (builder, HB_TAG ('C','O','L','R'), colr);
  hb_face_builder_add_table (builder, HB_TAG ('m','a','x','p'), maxp);
  hb_blob_t *face_blob = hb_face_reference_blob (builder);
  hb_face_t *face = hb_face_create (face_blob, 0);

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add (hb_subset_input_glyph_set (input), 1);
  hb_face_t *subset = hb_subset_test_create_subset (face, input);

  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('C','O','L','R'));
  unsigned length;
  const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, >=, 34);
  unsigned list = read_uint_be (data + 18, 4);
  g_assert_cmpuint (list + 12, <=, length);
  /* Only the two layers that exist are kept. */
  g_assert_cmpuint (read_uint_be (data + list, 4), ==, 2);
  unsigned paint = list + read_uint_be (data + list + 8, 4);
  g_assert_cmpuint (paint + 6, <=, length);
  g_assert_cmpuint (data[paint], ==, 1);
  g_assert_cmpuint (data[paint + 1], ==, 255);
  /* The out-of-range range was not retained, so it has no mapping. */
  g_assert_cmpuint (read_uint_be (data + paint + 2, 4), ==, 0xFFFFFFFFu);
  hb_blob_destroy (blob);

  hb_face_destroy (subset);
  hb_face_destroy (face);
  hb_blob_destroy (face_blob);
  hb_face_destroy (builder);
  hb_blob_destroy (maxp);
  hb_blob_destroy (colr);
}

static void
test_subset_colr_shared_layers (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/colr-shared-layers.ttf");
  /* Every base glyph claims all 256 layers.  Retaining 255 of them expands
   * to 65280 layer records; one more does not fit numLayers. */
  for (unsigned count = 255; count <= 256; count++)
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add_range (hb_subset_input_unicode_set (input), 0xE000, 0xE000 + count - 1);
    hb_face_t *subset = hb_subset_test_create_subset (face, input);

    hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('C','O','L','R'));
    unsigned length;
    const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
    if (count == 256)
    {
      g_assert_cmpuint (length, ==, 0);
      hb_blob_destroy (blob);
      hb_face_destroy (subset);
      continue;
    }
    g_assert_cmpuint (length, ==, 14 + count * 6 + count * 256 * 4);
    g_assert_cmpuint (read_uint_be (data + 2, 2), ==, count);
    g_assert_cmpuint (read_uint_be (data + 12, 2), ==, count * 256);
    unsigned bases = read_uint_be (data + 4, 4), layers = read_uint_be (data + 8, 4);
    for (unsigned i = 0; i < count; i++)
    {
      g_assert_cmpuint (read_uint_be (data + bases + i * 6, 2), ==, i + 1);
      g_assert_cmpuint (read_uint_be (data + bases + i * 6 + 2, 2), ==, i * 256);
      g_assert_cmpuint (read_uint_be (data + bases + i * 6 + 4, 2), ==, 256);
      for (unsigned j = 0; j < 256; j++)
      {
        unsigned layer = layers + (i * 256 + j) * 4;
        g_assert_cmpuint (read_uint_be (data + layer, 2), ==, count + 1 + j);
        g_assert_cmpuint (read_uint_be (data + layer + 2, 2), ==, j % 2);
      }
    }
    hb_blob_destroy (blob);
    hb_face_destroy (subset);
  }
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
  hb_test_add (test_subset_colr_clip_rounding);
  hb_test_add (test_subset_colr_negative_radii);
  hb_test_add (test_subset_colr_negative_radii_precision);
  hb_test_add (test_subset_colr_affine_values);
  hb_test_add (test_subset_colr_sentinel_map_without_variations);
  hb_test_add (test_subset_colr_var_overflow);
  hb_test_add (test_subset_colr_layers_out_of_range);
  hb_test_add (test_subset_colr_shared_layers);

  return hb_test_run();
}
