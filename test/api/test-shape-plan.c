/*
 * Copyright © 2025  Khaled Hosny
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
 */

#include "hb-ot.h"
#include "hb-test.h"

/* Unit tests for hb-shape-plan.h */

static void
test_ot_shape_plan_get_feature_tags_rtl (void)
{
  /* Test getting features enabled for horizontal RTL text */
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_ARABIC;
  props.direction = HB_DIRECTION_RTL;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, "()", -1, 0, -1);

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, NULL, 0, NULL);
  hb_bool_t ret = hb_shape_plan_execute (shape_plan, font, buffer, NULL, 0);
  g_assert_true (ret);

  hb_tag_t features[16];
  unsigned int feature_count = sizeof (features) / sizeof (features[0]);
  unsigned int count = hb_ot_shape_plan_get_feature_tags (shape_plan, 0, &feature_count, features);
  g_assert_cmpuint (count, ==, 16);
  g_assert_cmpuint (feature_count, ==, count);

  g_assert_cmpint (features[0], ==, HB_TAG ('a', 'b', 'v', 'm'));
  g_assert_cmpint (features[1], ==, HB_TAG ('c', 'c', 'm', 'p'));
  g_assert_cmpint (features[2], ==, HB_TAG ('d', 'i', 's', 't'));
  g_assert_cmpint (features[3], ==, HB_TAG ('d', 'n', 'o', 'm'));
  g_assert_cmpint (features[4], ==, HB_TAG ('f', 'i', 'n', 'a'));
  g_assert_cmpint (features[5], ==, HB_TAG ('f', 'r', 'a', 'c'));
  g_assert_cmpint (features[6], ==, HB_TAG ('i', 'n', 'i', 't'));
  g_assert_cmpint (features[7], ==, HB_TAG ('i', 's', 'o', 'l'));
  g_assert_cmpint (features[8], ==, HB_TAG ('k', 'e', 'r', 'n'));
  g_assert_cmpint (features[9], ==, HB_TAG ('l', 'i', 'g', 'a'));
  g_assert_cmpint (features[10], ==, HB_TAG ('m', 'a', 'r', 'k'));
  g_assert_cmpint (features[11], ==, HB_TAG ('m', 'e', 'd', 'i'));
  g_assert_cmpint (features[12], ==, HB_TAG ('m', 'k', 'm', 'k'));
  g_assert_cmpint (features[13], ==, HB_TAG ('n', 'u', 'm', 'r'));
  g_assert_cmpint (features[14], ==, HB_TAG ('r', 'l', 'i', 'g'));
  g_assert_cmpint (features[15], ==, HB_TAG ('r', 't', 'l', 'm'));

  hb_shape_plan_destroy (shape_plan);
  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
}


static void
test_ot_shape_plan_get_feature_tags_ltr (void)
{
  /* Test getting features enabled for horizontal LTR text */
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, " ", -1, 0, -1);

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, NULL, 0, NULL);
  hb_bool_t ret = hb_shape_plan_execute (shape_plan, font, buffer, NULL, 0);
  g_assert_true (ret);

  hb_tag_t features[10];
  unsigned int feature_count = sizeof (features) / sizeof (features[0]);
  unsigned int count = hb_ot_shape_plan_get_feature_tags (shape_plan, 0, &feature_count, features);
  g_assert_cmpuint (count, ==, 10);
  g_assert_cmpuint (feature_count, ==, count);

  g_assert_cmpint (features[0], ==, HB_TAG ('a', 'b', 'v', 'm'));
  g_assert_cmpint (features[1], ==, HB_TAG ('c', 'c', 'm', 'p'));
  g_assert_cmpint (features[2], ==, HB_TAG ('d', 'i', 's', 't'));
  g_assert_cmpint (features[3], ==, HB_TAG ('d', 'n', 'o', 'm'));
  g_assert_cmpint (features[4], ==, HB_TAG ('f', 'r', 'a', 'c'));
  g_assert_cmpint (features[5], ==, HB_TAG ('k', 'e', 'r', 'n'));
  g_assert_cmpint (features[6], ==, HB_TAG ('l', 'i', 'g', 'a'));
  g_assert_cmpint (features[7], ==, HB_TAG ('m', 'a', 'r', 'k'));
  g_assert_cmpint (features[8], ==, HB_TAG ('m', 'k', 'm', 'k'));
  g_assert_cmpint (features[9], ==, HB_TAG ('n', 'u', 'm', 'r'));

  hb_buffer_destroy (buffer);
  hb_shape_plan_destroy (shape_plan);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_shape_plan_get_feature_tags_ttb (void)
{
  /* Test getting features enabled for vertical text */
  hb_face_t *face = hb_test_open_font_file ("fonts/Mplus1p-Regular.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_HAN;
  props.direction = HB_DIRECTION_TTB;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, " ", -1, 0, -1);

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, NULL, 0, NULL);
  hb_bool_t ret = hb_shape_plan_execute (shape_plan, font, buffer, NULL, 0);
  g_assert_true (ret);

  hb_tag_t features[4];
  unsigned int feature_count = sizeof (features) / sizeof (features[0]);
  unsigned int count = hb_ot_shape_plan_get_feature_tags (shape_plan, 0, &feature_count, features);
  g_assert_cmpuint (count, ==, 4);
  g_assert_cmpuint (feature_count, ==, count);

  g_assert_cmpint (features[0], ==, HB_TAG ('c', 'c', 'm', 'p'));
  g_assert_cmpint (features[1], ==, HB_TAG ('m', 'a', 'r', 'k'));
  g_assert_cmpint (features[2], ==, HB_TAG ('m', 'k', 'm', 'k'));
  g_assert_cmpint (features[3], ==, HB_TAG ('v', 'e', 'r', 't'));

  hb_buffer_destroy (buffer);
  hb_shape_plan_destroy (shape_plan);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_shape_plan_get_feature_tags_userfeatures_enable (void)
{
  /* test getting features enabled with user features enabling a non-default feature */
  hb_face_t *face = hb_test_open_font_file ("fonts/Mada-VF.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, "   ", -1, 0, -1);

  hb_feature_t user_features[1] = {{HB_TAG ('s', 's', '0', '1'), 1, 0, 1}};

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, user_features, 1, NULL);
  hb_bool_t ret = hb_shape_plan_execute (shape_plan, font, buffer, user_features, 1);
  g_assert_true (ret);

  hb_tag_t features[7];
  unsigned int feature_count = sizeof (features) / sizeof (features[0]);
  unsigned int count = hb_ot_shape_plan_get_feature_tags (shape_plan, 0, &feature_count, features);
  g_assert_cmpuint (count, ==, 7);
  g_assert_cmpuint (feature_count, ==, count);

  g_assert_cmpint (features[0], ==, HB_TAG ('c', 'c', 'm', 'p'));
  g_assert_cmpint (features[1], ==, HB_TAG ('k', 'e', 'r', 'n'));
  g_assert_cmpint (features[2], ==, HB_TAG ('m', 'a', 'r', 'k'));
  g_assert_cmpint (features[3], ==, HB_TAG ('m', 'k', 'm', 'k'));
  g_assert_cmpint (features[4], ==, HB_TAG ('r', 'c', 'l', 't'));
  g_assert_cmpint (features[5], ==, HB_TAG ('r', 'l', 'i', 'g'));
  g_assert_cmpint (features[6], ==, HB_TAG ('s', 's', '0', '1'));

  hb_buffer_destroy (buffer);
  hb_shape_plan_destroy (shape_plan);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_shape_plan_get_feature_tags_userfeatures_disable (void)
{
  /* test getting features enabled with user features disabling a default feature */
  hb_face_t *face = hb_test_open_font_file ("fonts/Mada-VF.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, "   ", -1, 0, -1);

  hb_feature_t user_features[1] = {{HB_TAG ('k', 'e', 'r', 'n'), 0, 0, HB_FEATURE_GLOBAL_END}};

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, user_features, 1, NULL);
  hb_bool_t ret = hb_shape_plan_execute (shape_plan, font, buffer, user_features, 1);
  g_assert_true (ret);

  hb_tag_t features[5];
  unsigned int feature_count = sizeof (features) / sizeof (features[0]);
  unsigned int count = hb_ot_shape_plan_get_feature_tags (shape_plan, 0, &feature_count, features);
  g_assert_cmpuint (count, ==, 5);
  g_assert_cmpuint (feature_count, ==, count);

  g_assert_cmpint (features[0], ==, HB_TAG ('c', 'c', 'm', 'p'));
  g_assert_cmpint (features[1], ==, HB_TAG ('m', 'a', 'r', 'k'));
  g_assert_cmpint (features[2], ==, HB_TAG ('m', 'k', 'm', 'k'));
  g_assert_cmpint (features[3], ==, HB_TAG ('r', 'c', 'l', 't'));
  g_assert_cmpint (features[4], ==, HB_TAG ('r', 'l', 'i', 'g'));

  hb_buffer_destroy (buffer);
  hb_shape_plan_destroy (shape_plan);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_shape_plan_get_feature_tags_userfeatures_disablepartial (void)
{
  /* test getting features enabled with user features disabling a non-default
     feature partially; the tag of the disabled feature will be returned since
     it is still enabled partially.*/
  hb_face_t *face = hb_test_open_font_file ("fonts/Mada-VF.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, "   ", -1, 0, -1);

  hb_feature_t user_features[1] = {{HB_TAG ('k', 'e', 'r', 'n'), 0, 0, 1}};

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, user_features, 1, NULL);
  hb_bool_t ret = hb_shape_plan_execute (shape_plan, font, buffer, user_features, 1);
  g_assert_true (ret);

  hb_tag_t features[6];
  unsigned int feature_count = sizeof (features) / sizeof (features[0]);
  unsigned int count = hb_ot_shape_plan_get_feature_tags (shape_plan, 0, &feature_count, features);
  g_assert_cmpuint (count, ==, 6);
  g_assert_cmpuint (feature_count, ==, count);

  g_assert_cmpint (features[0], ==, HB_TAG ('c', 'c', 'm', 'p'));
  g_assert_cmpint (features[1], ==, HB_TAG ('k', 'e', 'r', 'n'));
  g_assert_cmpint (features[2], ==, HB_TAG ('m', 'a', 'r', 'k'));
  g_assert_cmpint (features[3], ==, HB_TAG ('m', 'k', 'm', 'k'));
  g_assert_cmpint (features[4], ==, HB_TAG ('r', 'c', 'l', 't'));
  g_assert_cmpint (features[5], ==, HB_TAG ('r', 'l', 'i', 'g'));

  hb_buffer_destroy (buffer);
  hb_shape_plan_destroy (shape_plan);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_shape_plan_get_feature_tags_userfeatures_disablenondeafult (void)
{
  /* test getting features enabled with user features disabling a non-default
     feature; the tag of the disabled feature will be returned returned.*/
  hb_face_t *face = hb_test_open_font_file ("fonts/Mada-VF.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, "   ", -1, 0, -1);

  hb_feature_t user_features[1] = {{HB_TAG ('s', 's', '0', '1'), 0, 0, HB_FEATURE_GLOBAL_END}};

  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, user_features, 1, NULL);
  hb_bool_t ret = hb_shape_plan_execute (shape_plan, font, buffer, user_features, 1);
  g_assert_true (ret);

  hb_tag_t features[6];
  unsigned int feature_count = sizeof (features) / sizeof (features[0]);
  unsigned int count = hb_ot_shape_plan_get_feature_tags (shape_plan, 0, &feature_count, features);
  g_assert_cmpuint (count, ==, 6);
  g_assert_cmpuint (feature_count, ==, count);

  g_assert_cmpint (features[0], ==, HB_TAG ('c', 'c', 'm', 'p'));
  g_assert_cmpint (features[1], ==, HB_TAG ('k', 'e', 'r', 'n'));
  g_assert_cmpint (features[2], ==, HB_TAG ('m', 'a', 'r', 'k'));
  g_assert_cmpint (features[3], ==, HB_TAG ('m', 'k', 'm', 'k'));
  g_assert_cmpint (features[4], ==, HB_TAG ('r', 'c', 'l', 't'));
  g_assert_cmpint (features[5], ==, HB_TAG ('r', 'l', 'i', 'g'));

  hb_buffer_destroy (buffer);
  hb_shape_plan_destroy (shape_plan);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_shape_plan_execute_buffer_limits (void)
{
  /* A GSUB whose 'ccmp' feature multiplies the glyph of 'a' by 400, twice.
   * 160000 glyphs is more than a single character is allowed to grow to
   * (HB_BUFFER_MAX_LEN_MIN), so shaping stops after the first lookup. */
  enum { FACTOR = 400, LOOKUP_LEN = 12 + FACTOR };
  uint16_t words[26 + 2 * LOOKUP_LEN] = {
    1, 0, 10, 30, 46, /* GSUB header */
    1, 0x4446, 0x4C54, 8, 4, 0, 0, 0xFFFF, 1, 0, /* ScriptList: DFLT */
    1, 0x6363, 0x6D70, 8, 0, 2, 0, 1, /* FeatureList: ccmp */
    2, 6, 6 + 2 * LOOKUP_LEN, /* LookupList */
  };
  for (unsigned i = 0; i < 2; i++)
  {
    const uint16_t lookup[12] = {
      2, 0, 1, 8, /* Lookup: MultipleSubst */
      1, 8, 1, 14, /* MultipleSubstFormat1 */
      1, 1, 1 + i, /* Coverage */
      FACTOR, /* Sequence */
    };
    uint16_t *p = words + 26 + i * LOOKUP_LEN;
    memcpy (p, lookup, sizeof (lookup));
    for (unsigned j = 0; j < FACTOR; j++)
      p[12 + j] = 2 + i;
  }
  char gsub[sizeof (words)];
  for (unsigned i = 0; i < G_N_ELEMENTS (words); i++)
  {
    gsub[2 * i] = words[i] >> 8;
    gsub[2 * i + 1] = words[i] & 0xFF;
  }
  const char cmap[] = {
    0, 0, 0, 1, 0, 3, 0, 1, 0, 0, 0, 12,
    0, 6, 0, 12, 0, 0, 0, 'a', 0, 1, 0, 1, /* format 6: 'a' -> glyph 1 */
  };
  const char maxp[] = {0, 0, 0x50, 0, 0, 4};

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "GSUB", gsub);
  HB_FACE_ADD_TABLE (face, "cmap", cmap);
  HB_FACE_ADD_TABLE (face, "maxp", maxp);
  hb_font_t *font = hb_font_create (face);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;

  hb_buffer_t *expected = hb_buffer_create ();
  hb_buffer_set_segment_properties (expected, &props);
  hb_buffer_add_utf8 (expected, "a", -1, 0, -1);
  hb_shape (font, expected, NULL, 0);
  g_assert_cmpuint (hb_buffer_get_length (expected), ==, FACTOR);

  /* Executing a plan directly is subject to the same limits as hb_shape(). */
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_segment_properties (buffer, &props);
  hb_buffer_add_utf8 (buffer, "a", -1, 0, -1);
  hb_shape_plan_t *shape_plan = hb_shape_plan_create (face, &props, NULL, 0, NULL);
  g_assert_true (hb_shape_plan_execute (shape_plan, font, buffer, NULL, 0));
  g_assert_cmpuint (hb_buffer_get_length (buffer), ==, hb_buffer_get_length (expected));

  hb_shape_plan_destroy (shape_plan);
  hb_buffer_destroy (buffer);
  hb_buffer_destroy (expected);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

#ifndef HB_NO_VAR
static void
test_ot_shape_plan_default_condition_deltas (void)
{
  hb_face_t *source = hb_test_open_font_file (
      "../shape/data/in-house/fonts/4e9f0bc6a8f25b5fd3547bbc17423ce8cedb915f.ttf");
  hb_face_t *face = hb_face_builder_create ();
  hb_tag_t tags[32];
  unsigned count = G_N_ELEMENTS (tags);
  unsigned total = hb_face_get_table_tags (source, 0, &count, tags);
  g_assert_cmpuint (total, <=, G_N_ELEMENTS (tags));
  for (unsigned i = 0; i < count; i++)
  {
    if (tags[i] == HB_OT_TAG_GDEF) continue;
    hb_blob_t *blob = hb_face_reference_table (source, tags[i]);
    g_assert_true (hb_face_builder_add_table (face, tags[i], blob));
    hb_blob_destroy (blob);
  }
  /* The fixture's GDEF store, with both region axes neutral. Its delta of
   * two applies everywhere except the default instance (OFF 7.1). */
  const char gdef[] = {
    0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18,
    0, 1, 0, 0, 0, 12, 0, 1, 0, 0, 0, 28, /* store */
    0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* region */
    0, 1, 0, 0, 0, 1, 0, 0, 2, /* delta data */
  };
  hb_blob_t *blob = hb_blob_create (gdef, sizeof (gdef),
				  HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_OT_TAG_GDEF, blob));
  hb_blob_destroy (blob);

  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;
  hb_shape_plan_t *default_plan = hb_shape_plan_create_cached2 (
      face, &props, NULL, 0, NULL, 0, NULL);
  int coords[] = {0, 0};
  hb_shape_plan_t *zero_plan = hb_shape_plan_create_cached2 (
      face, &props, NULL, 0, coords, G_N_ELEMENTS (coords), NULL);
  g_assert_true (default_plan == zero_plan);
  coords[1] = 8192;
  hb_shape_plan_t *varied_plan = hb_shape_plan_create_cached2 (
      face, &props, NULL, 0, coords, G_N_ELEMENTS (coords), NULL);
  g_assert_true (default_plan != varied_plan);

  hb_font_t *font = hb_font_create (face);
  for (unsigned i = 0; i < 3; i++)
  {
    coords[1] = i == 1 ? 8192 : 0;
    hb_font_set_var_coords_normalized (font, coords, G_N_ELEMENTS (coords));
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_buffer_set_segment_properties (buffer, &props);
    hb_buffer_add_utf8 (buffer, "C", -1, 0, -1);
    hb_shape (font, buffer, NULL, 0);
    unsigned length;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, &length);
    g_assert_cmpuint (length, ==, 1);
    hb_codepoint_t expected;
    g_assert_true (hb_font_get_glyph_from_name (
	font, i == 1 ? "C.alt" : "C", -1, &expected));
    g_assert_cmpuint (info[0].codepoint, ==, expected);
    hb_buffer_destroy (buffer);
  }

  hb_font_destroy (font);
  hb_shape_plan_destroy (varied_plan);
  hb_shape_plan_destroy (zero_plan);
  hb_shape_plan_destroy (default_plan);
  hb_face_destroy (face);
  hb_face_destroy (source);
}

static void
test_ot_shape_plan_lookup_variations_cache (void)
{
  hb_face_t *face = hb_test_open_font_file (
      "../shape/data/in-house/fonts/4e9f0bc6a8f25b5fd3547bbc17423ce8cedb915f.ttf");
  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  int coords[] = {0};

  props.script = HB_SCRIPT_LATIN;
  props.direction = HB_DIRECTION_LTR;

  hb_shape_plan_t *plan_0 = hb_shape_plan_create_cached2 (face, &props,
							  NULL, 0, coords, 1, NULL);
  coords[0] = 4915; /* 0.3 */
  hb_shape_plan_t *plan_03 = hb_shape_plan_create_cached2 (face, &props,
							   NULL, 0, coords, 1, NULL);
  coords[0] = 6554; /* 0.4; same lookup set as 0.3. */
  hb_shape_plan_t *plan_04 = hb_shape_plan_create_cached2 (face, &props,
							   NULL, 0, coords, 1, NULL);
  coords[0] = 9830; /* 0.6 */
  hb_shape_plan_t *plan_06 = hb_shape_plan_create_cached2 (face, &props,
							   NULL, 0, coords, 1, NULL);
  coords[0] = 13107; /* 0.8 */
  hb_shape_plan_t *plan_08 = hb_shape_plan_create_cached2 (face, &props,
							   NULL, 0, coords, 1, NULL);
  coords[0] = 0;
  hb_shape_plan_t *plan_0_again = hb_shape_plan_create_cached2 (face, &props,
								NULL, 0, coords, 1, NULL);

  g_assert_true (plan_0 == plan_0_again);
  g_assert_true (plan_03 == plan_04);
  g_assert_true (plan_0 != plan_03);
  g_assert_true (plan_03 != plan_06);
  g_assert_true (plan_06 != plan_08);

  hb_shape_plan_destroy (plan_0_again);
  hb_shape_plan_destroy (plan_08);
  hb_shape_plan_destroy (plan_06);
  hb_shape_plan_destroy (plan_04);
  hb_shape_plan_destroy (plan_03);
  hb_shape_plan_destroy (plan_0);
  hb_face_destroy (face);
}
#endif

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_ot_shape_plan_get_feature_tags_rtl);
  hb_test_add (test_ot_shape_plan_get_feature_tags_ltr);
  hb_test_add (test_ot_shape_plan_get_feature_tags_ttb);
  hb_test_add (test_ot_shape_plan_get_feature_tags_userfeatures_enable);
  hb_test_add (test_ot_shape_plan_get_feature_tags_userfeatures_disable);
  hb_test_add (test_ot_shape_plan_get_feature_tags_userfeatures_disablepartial);
  hb_test_add (test_ot_shape_plan_get_feature_tags_userfeatures_disablenondeafult);
  hb_test_add (test_shape_plan_execute_buffer_limits);
#ifndef HB_NO_VAR
  hb_test_add (test_ot_shape_plan_default_condition_deltas);
  hb_test_add (test_ot_shape_plan_lookup_variations_cache);
#endif

  return hb_test_run();
}
