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

/* Unit tests for hb-subset-glyf.h */

static void
test_subset_32_tables (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/oom-6ef8c96d3710262511bcc730dce9c00e722cb653");

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);
  hb_face_t *subset;

  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');

  subset = hb_subset_or_fail (face, input);
  g_assert_true (!subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_no_inf_loop (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/clusterfuzz-testcase-minimized-hb-subset-fuzzer-5521982557782016");

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);
  hb_face_t *subset;

  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');

  subset = hb_subset_or_fail (face, input);
  g_assert_true (!subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_crash (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/crash-4b60576767ee4d9fe1cc10959d89baf73d4e8249");

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);
  hb_face_t *subset;

  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');

  subset = hb_subset_or_fail (face, input);
  g_assert_true (!subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}

#ifndef HB_NO_VAR
static void
assert_lookup_variations_shape (hb_face_t *face,
				const int *coords,
				unsigned coord_count,
				const hb_codepoint_t *expected)
{
  hb_font_t *font = hb_font_create (face);
  hb_font_set_var_coords_normalized (font, coords, coord_count);

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, "ABCDEF", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, NULL, 0);

  unsigned length;
  const hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, &length);
  g_assert_cmpuint (length, ==, 6);
  for (unsigned i = 0; i < length; i++)
    g_assert_cmpuint (info[i].codepoint, ==, expected[i]);

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
}

static void
test_subset_lookup_variations (void)
{
  hb_face_t *face = hb_test_open_font_file (
      "../shape/data/in-house/fonts/4e9f0bc6a8f25b5fd3547bbc17423ce8cedb915f.ttf");
  const hb_codepoint_t expected_all[] = {2, 4, 6, 8, 9, 10};
  const hb_codepoint_t expected_test_08_dumy_08[] = {2, 3, 6, 8, 9, 10};
  const hb_codepoint_t expected_test_03_dumy_0[] = {2, 4, 5, 7, 8, 11};
  const hb_codepoint_t expected_test_03_dumy_08[] = {2, 3, 6, 7, 8, 11};
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();

  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'F');
  hb_set_t *features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
  hb_set_clear (features);
  hb_set_add (features, HB_TAG ('l', 'i', 'g', 'a'));

  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  int coords[] = {13107}; /* 0.8 */
  assert_lookup_variations_shape (subset, coords, 1, expected_all);

  hb_face_destroy (subset);
  hb_subset_input_destroy (input);

  input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'F');
  features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
  hb_set_clear (features);
  hb_set_add (features, HB_TAG ('l', 'i', 'g', 'a'));
  g_assert_true (hb_subset_input_pin_axis_location (
	input, face, HB_TAG ('T', 'E', 'S', 'T'), 0.8f));
  g_assert_true (hb_subset_input_pin_axis_location (
	input, face, HB_TAG ('D', 'U', 'M', 'Y'), 0.f));

  subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  assert_lookup_variations_shape (subset, NULL, 0, expected_all);

  hb_face_destroy (subset);
  hb_subset_input_destroy (input);

  input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'F');
  features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
  hb_set_clear (features);
  hb_set_add (features, HB_TAG ('l', 'i', 'g', 'a'));
  g_assert_true (hb_subset_input_pin_axis_location (
	input, face, HB_TAG ('T', 'E', 'S', 'T'), 0.8f));

  subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  coords[0] = 0;
  assert_lookup_variations_shape (subset, coords, 1, expected_all);
  coords[0] = 13107; /* DUMY=0.8 */
  assert_lookup_variations_shape (
	subset, coords, 1, expected_test_08_dumy_08);

  hb_face_destroy (subset);
  hb_subset_input_destroy (input);

  input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'F');
  features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
  hb_set_clear (features);
  hb_set_add (features, HB_TAG ('l', 'i', 'g', 'a'));
  hb_set_add (features, HB_TAG ('r', 'l', 'i', 'g'));
  g_assert_true (hb_subset_input_pin_axis_location (
	input, face, HB_TAG ('T', 'E', 'S', 'T'), 0.3f));

  subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  coords[0] = 0;
  assert_lookup_variations_shape (
	subset, coords, 1, expected_test_03_dumy_0);
  coords[0] = 13107; /* DUMY=0.8 */
  assert_lookup_variations_shape (
	subset, coords, 1, expected_test_03_dumy_08);

  hb_face_destroy (subset);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

static void
assert_condition_instance_matches (hb_face_t *face, hb_face_t *subset,
				   const int *coords,
				   const int *subset_coords,
				   unsigned subset_coord_count)
{
  hb_font_t *fonts[] = {hb_font_create (face), hb_font_create (subset)};
  hb_buffer_t *buffers[] = {hb_buffer_create (), hb_buffer_create ()};
  hb_font_set_var_coords_normalized (fonts[0], coords, 2);
  hb_font_set_var_coords_normalized (fonts[1], subset_coords, subset_coord_count);
  for (unsigned i = 0; i < 2; i++)
  {
    hb_buffer_add_utf8 (buffers[i], "ABCDEF", -1, 0, -1);
    hb_buffer_guess_segment_properties (buffers[i]);
    hb_shape (fonts[i], buffers[i], NULL, 0);
  }

  unsigned length, subset_length;
  const hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffers[0], &length);
  const hb_glyph_info_t *subset_info = hb_buffer_get_glyph_infos (buffers[1], &subset_length);
  const hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffers[0], NULL);
  const hb_glyph_position_t *subset_pos = hb_buffer_get_glyph_positions (buffers[1], NULL);
  g_assert_cmpuint (length, ==, 6);
  g_assert_cmpuint (subset_length, ==, length);
  for (unsigned i = 0; i < length; i++)
  {
    g_assert_cmpuint (subset_info[i].codepoint, ==, info[i].codepoint);
    g_assert_cmpint (subset_pos[i].x_advance, ==, pos[i].x_advance);
  }

  for (unsigned i = 0; i < 2; i++)
  {
    hb_buffer_destroy (buffers[i]);
    hb_font_destroy (fonts[i]);
  }
}

static void
test_subset_feature_variation_universal (void)
{
  const char *filenames[] = {
    "fonts/feature-variation-universal.ttf",
    "fonts/feature-variation-universal-prefix.ttf"
  };
  const int widths[] = {-16384, 0, 9830, 13107, 16384};
  for (unsigned f = 0; f < G_N_ELEMENTS (filenames); f++)
  {
    hb_face_t *face = hb_test_open_font_file (filenames[f]);
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
    hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'F');
    g_assert_true (hb_subset_input_pin_axis_location (
        input, face, HB_TAG ('T', 'E', 'S', 'T'), 0.3f));
    hb_face_t *subset = hb_subset_or_fail (face, input);
    g_assert_nonnull (subset);
    for (unsigned i = 0; i < G_N_ELEMENTS (widths); i++)
    {
      int coords[] = {4915, widths[i]};
      assert_condition_instance_matches (face, subset, coords, &widths[i], 1);
    }
    hb_face_destroy (subset);
    hb_subset_input_destroy (input);
    hb_face_destroy (face);
  }
}

static void
test_subset_lookup_variation_fractional_value (void)
{
  hb_face_t *face = hb_test_open_font_file (
      "../shape/data/in-house/fonts/4e9f0bc6a8f25b5fd3547bbc17423ce8cedb915f.ttf");
  const float locations[] = {0.5f, 0.6f, 0.7f};
  const int normalized[] = {8192, 9830, 11469};
  const int test_coords[] = {4915, 13107};

  for (unsigned i = 0; i < G_N_ELEMENTS (locations); i++)
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
    hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'F');
    g_assert_true (hb_subset_input_pin_axis_location (
        input, face, HB_TAG ('D', 'U', 'M', 'Y'), locations[i]));
    hb_face_t *subset = hb_subset_or_fail (face, input);
    g_assert_nonnull (subset);
    for (unsigned j = 0; j < G_N_ELEMENTS (test_coords); j++)
    {
      int coords[] = {test_coords[j], normalized[i]};
      assert_condition_instance_matches (face, subset, coords, &test_coords[j], 1);
    }
    hb_face_destroy (subset);
    hb_subset_input_destroy (input);
  }
  hb_face_destroy (face);
}

static void
test_subset_feature_variation_conditions (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/feature-variation-conditions.ttf");
  const float locations[] = {-0.8f, -0.6f, 0.f, 0.3f, 0.6f, 0.8f};
  const int normalized[] = {-13107, -9830, 0, 4915, 9830, 13107};
  const hb_tag_t tags[] = {HB_TAG ('T','E','S','T'), HB_TAG ('D','U','M','Y')};
  const int original_coords[][2] = {{0, 0}, {4915, 0}, {4915, 13107},
				   {13107, -13107}, {4915, -9830}};
  const hb_codepoint_t expected[][6] = {
    {2, 3, 5, 7, 9, 11}, {2, 4, 5, 7, 9, 11}, {2, 3, 6, 7, 9, 11},
    {2, 3, 5, 7, 9, 12}, {2, 3, 5, 8, 9, 11}
  };
  for (unsigned i = 0; i < G_N_ELEMENTS (original_coords); i++)
    assert_lookup_variations_shape (face, original_coords[i], 2, expected[i]);

  for (unsigned pinned = 0; pinned < 3; pinned++)
    for (unsigned i = 0; i < G_N_ELEMENTS (locations); i++)
      for (unsigned j = 0; j < (pinned == 2 ? G_N_ELEMENTS (locations) : 1); j++)
      {
        hb_subset_input_t *input = hb_subset_input_create_or_fail ();
        hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
        hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'F');
        if (pinned == 2)
        {
          g_assert_true (hb_subset_input_pin_axis_location (input, face, tags[0], locations[i]));
          g_assert_true (hb_subset_input_pin_axis_location (input, face, tags[1], locations[j]));
        }
        else
          g_assert_true (hb_subset_input_pin_axis_location (input, face, tags[pinned], locations[i]));

        hb_face_t *subset = hb_subset_or_fail (face, input);
        g_assert_nonnull (subset);
        if (pinned == 2)
        {
          int coords[] = {normalized[i], normalized[j]};
          assert_condition_instance_matches (face, subset, coords, NULL, 0);
        }
        else
          for (unsigned k = 0; k < G_N_ELEMENTS (locations); k++)
          {
            int coords[] = {0, 0};
            coords[pinned] = normalized[i];
            coords[1 - pinned] = normalized[k];
            assert_condition_instance_matches (face, subset, coords, &normalized[k], 1);
          }
        hb_face_destroy (subset);
        hb_subset_input_destroy (input);
      }
  hb_face_destroy (face);
}
#endif

static void
test_subset_set_flags (void)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();

  g_assert_true (hb_subset_input_get_flags (input) == HB_SUBSET_FLAGS_DEFAULT);

  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NAME_LEGACY |
                             HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
                             HB_SUBSET_FLAGS_GLYPH_NAMES);

  g_assert_true (hb_subset_input_get_flags (input) ==
            (hb_subset_flags_t) (
            HB_SUBSET_FLAGS_NAME_LEGACY |
            HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
            HB_SUBSET_FLAGS_GLYPH_NAMES));

  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NAME_LEGACY |
                             HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
                             HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES);

  g_assert_true (hb_subset_input_get_flags (input) ==
            (hb_subset_flags_t) (
            HB_SUBSET_FLAGS_NAME_LEGACY |
            HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
            HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES));


  hb_subset_input_destroy (input);
}


static void
test_subset_sets (void)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t* set = hb_set_create ();

  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_GLYPH_INDEX), 83);
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_UNICODE), 85);

  hb_set_clear (hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG));
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG), 87);

  hb_set_add (set, 83);
  g_assert_true (hb_set_is_equal (hb_subset_input_glyph_set (input), set));
  hb_set_clear (set);

  hb_set_add (set, 85);
  g_assert_true (hb_set_is_equal (hb_subset_input_unicode_set (input), set));
  hb_set_clear (set);

  hb_set_add (set, 87);
  g_assert_true (hb_set_is_equal (hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG), set));
  hb_set_clear (set);

  hb_set_destroy (set);
  hb_subset_input_destroy (input);
}

static void
test_subset_plan (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 99);
  hb_subset_input_t* input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);

  hb_subset_plan_t* plan = hb_subset_plan_create_or_fail (face_abc, input);
  g_assert_true (plan);

  const hb_map_t* mapping = hb_subset_plan_old_to_new_glyph_mapping (plan);
  g_assert_true (hb_map_get (mapping, 1) == 1);
  g_assert_true (hb_map_get (mapping, 3) == 2);

  mapping = hb_subset_plan_new_to_old_glyph_mapping (plan);
  g_assert_true (hb_map_get (mapping, 1) == 1);
  g_assert_true (hb_map_get (mapping, 2) == 3);

  mapping = hb_subset_plan_unicode_to_old_glyph_mapping (plan);
  g_assert_true (hb_map_get (mapping, 0x63) == 3);

  hb_face_t* face_abc_subset = hb_subset_plan_execute_or_fail (plan);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  hb_subset_input_destroy (input);
  hb_subset_plan_destroy (plan);
  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static hb_blob_t*
_copy_table (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data)
{
  hb_blob_t *table = hb_face_reference_table ((hb_face_t*) user_data, tag);
  hb_blob_t *copy = hb_blob_copy_writable_or_fail (table);
  hb_blob_destroy (table);
  return copy;
}

static void
test_subset_create_for_tables_face (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.ttf");
  hb_face_t *face_create_for_tables = hb_face_create_for_tables (
      _copy_table,
      face_abc,
      NULL);

  hb_set_t *codepoints = hb_set_create();
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 99);

  hb_subset_input_t* input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);

  hb_face_t* face_abc_subset = hb_subset_or_fail (face_create_for_tables, input);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','a','s','p'));

  hb_subset_input_destroy (input);
  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_create_for_tables);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

#ifndef HB_NO_BEYOND_64K
static void
test_subset_create_for_tables_face_MAXP (void)
{
  static const char MAXP_data[] = "\x00\x00\x50\x00\x00\x00\x03";
  hb_face_t *source = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (source, "MAXP", MAXP_data);
  hb_face_t *callback_face = hb_face_create_for_tables (_copy_table, source, NULL);

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add (glyphs, 0);
  hb_set_add (glyphs, 2);
  hb_face_t *subset = hb_subset_test_create_subset (
      callback_face, hb_subset_test_create_input_from_glyphs (glyphs));
  hb_set_destroy (glyphs);

  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('M','A','X','P'));
  unsigned length;
  const unsigned char *data =
      (const unsigned char *) hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, 7);
  g_assert_cmpuint ((data[4] << 16) | (data[5] << 8) | data[6], ==, 2);
  hb_blob_destroy (blob);

  hb_face_destroy (subset);
  hb_face_destroy (callback_face);
  hb_face_destroy (source);
}
#endif

#ifdef HB_EXPERIMENTAL_API
const uint8_t CFF2[226] = {
  // From https://learn.microsoft.com/en-us/typography/opentype/spec/cff2
  0x02, 0x00, 0x05, 0x00, 0x07, 0xCF, 0x0C, 0x24, 0xC3, 0x11, 0x9B, 0x18, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x26, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x01,
  0x00, 0x02, 0xC0, 0x00, 0xE0, 0x00, 0x00, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xE0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x03, 0x05,
  0x20, 0x0A, 0x20, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x05, 0xF7, 0x06, 0xDA, 0x12, 0x77,
  0x9F, 0xF8, 0x6C, 0x9D, 0xAE, 0x9A, 0xF4, 0x9A, 0x95, 0x9F, 0xB3, 0x9F, 0x8B, 0x8B, 0x8B, 0x8B,
  0x85, 0x9A, 0x8B, 0x8B, 0x97, 0x73, 0x8B, 0x8B, 0x8C, 0x80, 0x8B, 0x8B, 0x8B, 0x8D, 0x8B, 0x8B,
  0x8C, 0x8A, 0x8B, 0x8B, 0x97, 0x17, 0x06, 0xFB, 0x8E, 0x95, 0x86, 0x9D, 0x8B, 0x8B, 0x8D, 0x17,
  0x07, 0x77, 0x9F, 0xF8, 0x6D, 0x9D, 0xAD, 0x9A, 0xF3, 0x9A, 0x95, 0x9F, 0xB3, 0x9F, 0x08, 0xFB,
  0x8D, 0x95, 0x09, 0x1E, 0xA0, 0x37, 0x5F, 0x0C, 0x09, 0x8B, 0x0C, 0x0B, 0xC2, 0x6E, 0x9E, 0x8C,
  0x17, 0x0A, 0xDB, 0x57, 0xF7, 0x02, 0x8C, 0x17, 0x0B, 0xB3, 0x9A, 0x77, 0x9F, 0x82, 0x8A, 0x8D,
  0x17, 0x0C, 0x0C, 0xDB, 0x95, 0x57, 0xF7, 0x02, 0x85, 0x8B, 0x8D, 0x17, 0x0C, 0x0D, 0xF7, 0x06,
  0x13, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x1B, 0xBD, 0xBD, 0xEF, 0x8C, 0x10, 0x8B, 0x15, 0xF8,
  0x88, 0x27, 0xFB, 0x5C, 0x8C, 0x10, 0x06, 0xF8, 0x88, 0x07, 0xFC, 0x88, 0xEF, 0xF7, 0x5C, 0x8C,
  0x10, 0x06
};

const uint8_t CFF2_ONLY_CHARSTRINGS[12] = {
  0x00, 0x00, 0x00, 0x02, 0x01, 0x01, 0x03, 0x05,
  0x20, 0x0A, 0x20, 0x0A
};

static void
test_subset_cff2_get_charstring_data (void)
{
  const uint8_t maxp_data[6] = {
    0x00, 0x00, 0x50, 0x00,
    0x00, 0x02 // numGlyphs
  };

  hb_blob_t* cff2 = hb_blob_create ((const char*) CFF2, 226, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_blob_t* maxp = hb_blob_create ((const char*) maxp_data, 6, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_face_t* builder = hb_face_builder_create ();
  hb_face_builder_add_table (builder, HB_TAG('C', 'F', 'F', '2'), cff2);
  hb_face_builder_add_table (builder, HB_TAG('m', 'a', 'x', 'p'), maxp);
  hb_blob_t* face_blob = hb_face_reference_blob (builder);
  hb_face_t* face = hb_face_create (face_blob, 0);

  hb_blob_t* cs0 = hb_subset_cff2_get_charstring_data (face, 0);
  unsigned int length;
  const uint8_t* data = (const uint8_t*) hb_blob_get_data (cs0, &length);
  g_assert_true (length == 2);
  g_assert_true (data[0] == 0x20);
  g_assert_true (data[1] == 0x0A);

  hb_blob_t* cs1 = hb_subset_cff2_get_charstring_data (face, 1);
  data = (const uint8_t*) hb_blob_get_data (cs1, &length);
  g_assert_true (length == 2);
  g_assert_true (data[0] == 0x20);
  g_assert_true (data[1] == 0x0A);

  hb_blob_t* cs2 = hb_subset_cff2_get_charstring_data (face, 2);
  data = (const uint8_t*) hb_blob_get_data (cs2, &length);
  g_assert_true (length == 0);

  hb_blob_destroy (cff2);
  hb_blob_destroy (maxp);
  hb_face_destroy (builder);
  hb_blob_destroy (face_blob);
  hb_face_destroy (face);
  hb_blob_destroy (cs0);
  hb_blob_destroy (cs1);
  hb_blob_destroy (cs2);
}

static void
test_subset_cff2_get_all_charstrings_data (void)
{
  const uint8_t maxp_data[6] = {
    0x00, 0x00, 0x50, 0x00,
    0x00, 0x02 // numGlyphs
  };

  hb_blob_t* cff2 = hb_blob_create ((const char*) CFF2, 226, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_blob_t* maxp = hb_blob_create ((const char*) maxp_data, 6, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_face_t* builder = hb_face_builder_create ();
  hb_face_builder_add_table (builder, HB_TAG('C', 'F', 'F', '2'), cff2);
  hb_face_builder_add_table (builder, HB_TAG('m', 'a', 'x', 'p'), maxp);
  hb_blob_t* face_blob = hb_face_reference_blob (builder);
  hb_face_t* face = hb_face_create (face_blob, 0);

  hb_blob_t* cs = hb_subset_cff2_get_charstrings_index (face);
  hb_blob_destroy (cff2);
  hb_blob_destroy (maxp);
  hb_face_destroy (builder);
  hb_blob_destroy (face_blob);
  hb_face_destroy (face);

  unsigned int length;
  const uint8_t* data = (const uint8_t*) hb_blob_get_data (cs, &length);
  g_assert_cmpint (length, ==, 12);
  for (int i = 0; i < 12; i++) {
    g_assert_cmpint(data[i], ==, CFF2_ONLY_CHARSTRINGS[i]);
  }

  hb_blob_destroy (cs);
}

static void
test_subset_cff2_get_charstring_data_no_cff (void)
{
  hb_face_t* builder = hb_face_builder_create ();
  hb_blob_t* face_blob = hb_face_reference_blob (builder);
  hb_face_t* face = hb_face_create (face_blob, 0);

  hb_blob_t* cs0 = hb_subset_cff2_get_charstring_data (face, 0);
  g_assert_true (hb_blob_get_length (cs0) == 0);

  hb_face_destroy (builder);
  hb_blob_destroy (face_blob);
  hb_face_destroy (face);
  hb_blob_destroy (cs0);
}

static void
test_subset_cff2_get_charstring_data_invalid_cff2 (void)
{
  // cff2 will parse as invalid since charstrings count doesn't not match num glyphs
  // (because there is no maxp).
  hb_blob_t* cff2 = hb_blob_create ((const char*) CFF2, 226, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_face_t* builder = hb_face_builder_create ();
  hb_face_builder_add_table (builder, HB_TAG('C', 'F', 'F', '2'), cff2);
  hb_blob_t* face_blob = hb_face_reference_blob (builder);
  hb_face_t* face = hb_face_create (face_blob, 0);

  hb_blob_t* cs0 = hb_subset_cff2_get_charstring_data (face, 0);
  g_assert_true (hb_blob_get_length (cs0) == 0);

  hb_blob_destroy (cff2);
  hb_face_destroy (builder);
  hb_blob_destroy (face_blob);
  hb_face_destroy (face);
  hb_blob_destroy (cs0);
}

static void
test_subset_cff2_get_charstring_data_lifetime (void)
{
  const uint8_t maxp_data[6] = {
    0x00, 0x00, 0x50, 0x00,
    0x00, 0x02 // numGlyphs
  };

  hb_blob_t* cff2 = hb_blob_create ((const char*) CFF2, 226, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_blob_t* maxp = hb_blob_create ((const char*) maxp_data, 6, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_face_t* builder = hb_face_builder_create ();
  hb_face_builder_add_table (builder, HB_TAG('C', 'F', 'F', '2'), cff2);
  hb_face_builder_add_table (builder, HB_TAG('m', 'a', 'x', 'p'), maxp);
  hb_blob_t* face_blob = hb_face_reference_blob (builder);
  hb_face_t* face = hb_face_create (face_blob, 0);

  hb_blob_t* cs0 = hb_subset_cff2_get_charstring_data (face, 0);

  // Destroy the blob that cs0 is referencing via subblob to ensure lifetimes are being
  // handled correctly.
  hb_blob_destroy (cff2);
  hb_blob_destroy (maxp);
  hb_face_destroy (builder);
  hb_blob_destroy (face_blob);
  hb_face_destroy (face);

  unsigned int length;
  const uint8_t* data = (const uint8_t*) hb_blob_get_data (cs0, &length);
  g_assert_true (length == 2);
  g_assert_true (data[0] == 0x20);
  g_assert_true (data[1] == 0x0A);

  hb_blob_destroy (cs0);
}

#endif

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_input_to_string (void)
{
  /* Test NULL input */
  g_assert_null (hb_subset_input_to_string_or_fail (NULL));

  /* Test default input -> empty string */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    g_assert_cmpint (len, ==, 1);
    g_assert_cmpstr (data, ==, "");
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test unicodes ranges */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_t *unicodes = hb_subset_input_unicode_set (input);
    hb_set_add (unicodes, 0x41);
    hb_set_add (unicodes, 0x42);
    hb_set_add (unicodes, 0x43);
    hb_set_add (unicodes, 0x45);
    hb_set_add (unicodes, 0x61);
    hb_set_add (unicodes, 0x62);
    hb_set_add (unicodes, 0x63);

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    g_assert_cmpstr (data, ==, "--unicodes=41-43,45,61-63");
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test gids ranges and flags */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_t *gids = hb_subset_input_glyph_set (input);
    hb_set_add (gids, 1);
    hb_set_add (gids, 2);
    hb_set_add (gids, 3);
    hb_set_add (gids, 5);
    hb_set_add (gids, 10);
    hb_set_add (gids, 11);
    hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING | HB_SUBSET_FLAGS_RETAIN_GIDS);

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    g_assert_cmpstr (data, ==, "--no-hinting --retain-gids --gids=1-3,5,10-11");
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test keep everything */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_subset_input_keep_everything (input);

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    const char *expected = "--name-legacy --passthrough-tables --notdef-outline --glyph-names --no-prune-unicode-ranges --unicodes=* --gids=* --name-IDs=* --name-languages=* --layout-features=* --drop-tables-=*";
    g_assert_cmpstr (data, ==, expected);
    g_assert_cmpint (len - 1, ==, strlen (expected));
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test inverted sets with exclusions */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_t *unicodes = hb_subset_input_set (input, HB_SUBSET_SETS_UNICODE);
    hb_set_invert (unicodes);
    hb_set_del (unicodes, 0x20);
    hb_set_del (unicodes, 0x41);
    hb_set_del (unicodes, 0x42);
    hb_set_del (unicodes, 0x43);

    hb_set_t *gids = hb_subset_input_set (input, HB_SUBSET_SETS_GLYPH_INDEX);
    hb_set_invert (gids);
    hb_set_del (gids, 5);
    hb_set_del (gids, 6);
    hb_set_del (gids, 7);

    hb_set_t *features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_clear (features);
    hb_set_invert (features);
    hb_set_del (features, HB_TAG ('k', 'e', 'r', 'n'));

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    const char *expected = "--unicodes=* --unicodes-=20,41-43 --gids=* --gids-=5-7 --layout-features=* --layout-features-=kern";
    g_assert_cmpstr (data, ==, expected);
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test empty non-default sets */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_clear (hb_subset_input_set (input, HB_SUBSET_SETS_NAME_ID));
    hb_set_clear (hb_subset_input_set (input, HB_SUBSET_SETS_NAME_LANG_ID));

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    const char *expected = "--name-IDs-=* --name-languages-=*";
    g_assert_cmpstr (data, ==, expected);
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test non-default non-inverted tag sets */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();

    hb_set_t *features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_clear (features);
    hb_set_add (features, HB_TAG ('l', 'i', 'g', 'a'));
    hb_set_add (features, HB_TAG ('k', 'e', 'r', 'n'));

    hb_set_t *scripts = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_SCRIPT_TAG);
    hb_set_clear (scripts);
    hb_set_add (scripts, HB_TAG ('l', 'a', 't', 'n'));

    hb_set_t *drop_tables = hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG);
    hb_set_clear (drop_tables);
    hb_set_add (drop_tables, HB_TAG ('G', 'S', 'U', 'B'));

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    const char *expected = "--layout-features=kern,liga --layout-scripts=latn --drop-tables=GSUB";
    g_assert_cmpstr (data, ==, expected);
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test tag sanitization with unsafe ASCII characters */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();

    hb_set_t *features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_clear (features);
    /* Tag with space, newline, and single-quote: 'a \n'' */
    hb_set_add (features, HB_TAG ('a', ' ', '\n', '\''));

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    const char *expected = "--layout-features=a___";
    g_assert_cmpstr (data, ==, expected);
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }

  /* Test trimming trailing whitespace from tags */
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();

    hb_set_t *drop_tables = hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG);
    hb_set_clear (drop_tables);
    hb_set_add (drop_tables, HB_TAG ('c', 'v', 't', ' '));

    hb_set_t *features = hb_subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG);
    hb_set_clear (features);
    hb_set_add (features, HB_TAG (' ', ' ', ' ', ' '));
    hb_set_add (features, HB_TAG ('f', 'o', 'o', ' '));

    hb_blob_t *blob = hb_subset_input_to_string_or_fail (input);
    g_assert_nonnull (blob);
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    const char *expected = "--layout-features=,foo --drop-tables=cvt";
    g_assert_cmpstr (data, ==, expected);
    hb_blob_destroy (blob);
    hb_subset_input_destroy (input);
  }
}
#endif

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_32_tables);
  hb_test_add (test_subset_no_inf_loop);
  hb_test_add (test_subset_crash);
#ifndef HB_NO_VAR
  hb_test_add (test_subset_lookup_variations);
  hb_test_add (test_subset_lookup_variation_fractional_value);
  hb_test_add (test_subset_feature_variation_conditions);
  hb_test_add (test_subset_feature_variation_universal);
#endif
  hb_test_add (test_subset_set_flags);
  hb_test_add (test_subset_sets);
  hb_test_add (test_subset_plan);
  hb_test_add (test_subset_create_for_tables_face);
#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_subset_create_for_tables_face_MAXP);
#endif

  #ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_subset_input_to_string);
  hb_test_add (test_subset_cff2_get_charstring_data);
  hb_test_add (test_subset_cff2_get_all_charstrings_data);
  hb_test_add (test_subset_cff2_get_charstring_data_no_cff);
  hb_test_add (test_subset_cff2_get_charstring_data_invalid_cff2);
  hb_test_add (test_subset_cff2_get_charstring_data_lifetime);
  #endif

  return hb_test_run();
}
