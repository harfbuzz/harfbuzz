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
_ref_table (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data)
{
  return hb_face_reference_table ((hb_face_t*) user_data, tag);
}

static void
test_subset_create_for_tables_face (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.ttf");
  hb_face_t *face_create_for_tables = hb_face_create_for_tables (
      _ref_table,
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

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_32_tables);
  hb_test_add (test_subset_no_inf_loop);
  hb_test_add (test_subset_crash);
  hb_test_add (test_subset_set_flags);
  hb_test_add (test_subset_sets);
  hb_test_add (test_subset_plan);
  hb_test_add (test_subset_create_for_tables_face);

  #ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_subset_cff2_get_charstring_data);
  hb_test_add (test_subset_cff2_get_all_charstrings_data);
  hb_test_add (test_subset_cff2_get_charstring_data_no_cff);
  hb_test_add (test_subset_cff2_get_charstring_data_invalid_cff2);
  hb_test_add (test_subset_cff2_get_charstring_data_lifetime);
  #endif

  return hb_test_run();
}
