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

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_colr_noop);
  hb_test_add (test_subset_colr_keep_one_colr_glyph);
  hb_test_add (test_subset_colr_keep_mixed_glyph);
  hb_test_add (test_subset_colr_keep_no_colr_glyph);
  hb_test_add (test_subset_colr_wide_scales);

  return hb_test_run();
}
