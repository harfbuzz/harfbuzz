/*
 * Copyright © 2018  Google, Inc.
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
 * Google Author(s): Garret Rieger
 */

#include "hb-test.h"
#include "hb-subset-test.h"
#include "hb-ot.h"
#include <math.h>

/* MATH VariationIndex devices share GDEF's item variation store. */

static hb_face_t *
subset_math (hb_face_t *face, const hb_tag_t *tags, const float *values,
             unsigned count, hb_bool_t only_a)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *glyphs = hb_subset_input_glyph_set (input);
  hb_set_add_range (glyphs, 0, only_a ? 1 : 2);
  for (unsigned i = 0; i < count; i++)
    g_assert_true (hb_subset_input_pin_axis_location (input, face, tags[i], values[i]));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  hb_subset_input_destroy (input);
  return subset;
}

static hb_position_t
expected_value (int base, int dw, int dd, float weight, float width)
{
  return base + (int) floor (weight * dw + .5f) + (int) floor (width * dd + .5f);
}

static void
check_math (hb_face_t *face, float weight, float width, hb_bool_t only_a)
{
  hb_font_t *font = hb_font_create (face);
  hb_ot_font_set_funcs (font);
  hb_font_set_scale (font, 1000, 1000);
  hb_variation_t variations[] = {
    {HB_TAG ('w', 'g', 'h', 't'), weight},
    {HB_TAG ('w', 'd', 't', 'h'), width}
  };
  hb_font_set_variations (font, variations, 2);
#define CHECK_VALUE(actual, base, dw, dd) \
  g_assert_cmpint (actual, ==, expected_value (base, dw, dd, weight, width))
  CHECK_VALUE (hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_MATH_LEADING), 100, 10, 20);
  CHECK_VALUE (hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_AXIS_HEIGHT), 200, -20, 30);
  CHECK_VALUE (hb_ot_math_get_glyph_italics_correction (font, 1), 10, 30, -10);
  CHECK_VALUE (hb_ot_math_get_glyph_top_accent_attachment (font, 1), 300, 50, 5);
  if (!only_a)
  {
    CHECK_VALUE (hb_ot_math_get_glyph_italics_correction (font, 2), 20, 40, -20);
    CHECK_VALUE (hb_ot_math_get_glyph_top_accent_attachment (font, 2), 400, 60, -5);
  }
  hb_ot_math_kern_entry_t entries[2];
  unsigned count = 2;
  g_assert_cmpuint (hb_ot_math_get_glyph_kernings (font, 1, HB_OT_MATH_KERN_TOP_RIGHT,
                                                0, &count, entries), ==, 2);
  g_assert_cmpuint (count, ==, 2);
  CHECK_VALUE (entries[0].max_correction_height, 50, 70, 7);
  CHECK_VALUE (entries[0].kern_value, 70, 80, 8);
  CHECK_VALUE (entries[1].kern_value, 80, 90, 9);
  hb_position_t italic;
  count = 0;
  g_assert_cmpuint (hb_ot_math_get_glyph_assembly (font, 1, HB_DIRECTION_BTT,
                                                0, &count, NULL, &italic), ==, 1);
  CHECK_VALUE (italic, 40, 100, 10);
#undef CHECK_VALUE
  g_assert_cmpint (hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_ACCENT_BASE_HEIGHT), ==, 500);
  hb_font_set_ppem (font, 10, 10);
  g_assert_cmpint (hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FLATTENED_ACCENT_BASE_HEIGHT), ==, 700);
  hb_font_destroy (font);
}

static void
check_full (hb_face_t *face, float weight, float width, hb_bool_t only_a)
{
  check_math (face, weight, width, only_a);
  hb_blob_t *blob = hb_face_reference_table (face, HB_TAG ('f', 'v', 'a', 'r'));
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);
  blob = hb_face_reference_table (face, HB_TAG ('G', 'D', 'E', 'F'));
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);
}

static void
test_subset_math_variations (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/math-variations.ttf");
  const hb_tag_t tags[] = {HB_TAG ('w', 'g', 'h', 't'), HB_TAG ('w', 'd', 't', 'h')};
  const float locations[] = {0.f, .5f, 1.f};
  for (unsigned only_a = 0; only_a < 2; only_a++)
  {
    hb_face_t *subset = subset_math (face, NULL, NULL, 0, only_a);
    for (unsigned i = 0; i < 3; i++)
    {
      hb_font_t *font = hb_font_create (subset);
      hb_ot_font_set_funcs (font);
      hb_variation_t vars[] = {{tags[0], locations[i]}, {tags[1], 0.f}};
      hb_font_set_variations (font, vars, 2);
      g_assert_cmpint (hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_MATH_LEADING),
                       ==, 100 + (int) (10 * locations[i]));
      hb_font_destroy (font);
      hb_face_t *partial = subset_math (face, tags, &locations[i], 1, only_a);
      for (unsigned j = 0; j < 3; j++)
      {
        const float values[] = {locations[i], locations[j]};
        check_math (partial, values[0], values[1], only_a);
        check_math (face, values[0], values[1], false);
        hb_face_t *full = subset_math (face, tags, values, 2, only_a);
        check_full (full, values[0], values[1], only_a);
        hb_face_t *composed = subset_math (partial, tags + 1, values + 1, 1, only_a);
        check_full (composed, values[0], values[1], only_a);
        hb_subset_test_check (full, composed, HB_TAG ('M', 'A', 'T', 'H'));
        hb_face_destroy (composed);
        hb_face_t *width_only = subset_math (face, tags + 1, values + 1, 1, only_a);
        composed = subset_math (width_only, tags, values, 1, only_a);
        check_full (composed, values[0], values[1], only_a);
        hb_subset_test_check (full, composed, HB_TAG ('M', 'A', 'T', 'H'));
        hb_face_destroy (composed);
        hb_face_destroy (width_only);
        hb_face_destroy (full);
      }
      hb_face_destroy (partial);
    }
    hb_face_destroy (subset);
  }
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_subset_math_variations);
  return hb_test_run ();
}
