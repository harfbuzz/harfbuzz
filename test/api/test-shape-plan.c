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
  hb_buffer_add_utf8 (buffer, u8"()", -1, 0, -1);

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
  hb_buffer_add_utf8 (buffer, u8" ", -1, 0, -1);

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
  hb_buffer_add_utf8 (buffer, u8" ", -1, 0, -1);

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
  hb_buffer_add_utf8 (buffer, u8"   ", -1, 0, -1);

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
  hb_buffer_add_utf8 (buffer, u8"   ", -1, 0, -1);

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
  hb_buffer_add_utf8 (buffer, u8"   ", -1, 0, -1);

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
  hb_buffer_add_utf8 (buffer, u8"   ", -1, 0, -1);

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

  return hb_test_run();
}
