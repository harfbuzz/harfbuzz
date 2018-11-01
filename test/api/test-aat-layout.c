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
 */

#include "hb-test.h"

#include <hb.h>
#include <hb-ot.h>
#include <hb-aat.h>

/* Unit tests for hb-aat.h */

static hb_face_t *face;
static hb_face_t *sbix;

static void
test_aat_get_features (void)
{
  hb_aat_layout_feature_record_t records[3];
  unsigned int record_count = 3;
  g_assert_cmpuint (11, ==, hb_aat_layout_get_features (face, 0, &record_count, records));
  g_assert_cmpuint (1, ==, records[0].feature);
  g_assert_cmpuint (258, ==, records[0].name_id);
  g_assert_cmpuint (3, ==, records[1].feature);
  g_assert_cmpuint (261, ==, records[1].name_id);
  g_assert_cmpuint (6, ==, records[2].feature);
  g_assert_cmpuint (265, ==, records[2].name_id);
}

static void
test_aat_get_feature_settings (void)
{
  hb_aat_layout_feature_setting_t default_setting;
  hb_aat_layout_feature_type_selector_t records[3];
  unsigned int count = 3;

  g_assert_cmpuint (4, ==, hb_aat_layout_get_feature_settings (face, (hb_aat_layout_feature_type_t) 18,
							       &default_setting, 0, &count, records));
  g_assert_cmpuint (3, ==, count);
  g_assert_cmpuint (0, ==, default_setting);

  g_assert_cmpuint (0, ==, records[0].setting);
  g_assert_cmpuint (294, ==, records[0].name_id);

  g_assert_cmpuint (1, ==, records[1].setting);
  g_assert_cmpuint (295, ==, records[1].name_id);

  g_assert_cmpuint (2, ==, records[2].setting);
  g_assert_cmpuint (296, ==, records[2].name_id);

  count = 3;
  g_assert_cmpuint (4, ==, hb_aat_layout_get_feature_settings (face, (hb_aat_layout_feature_type_t) 18,
							       &default_setting, 3, &count, records));
  g_assert_cmpuint (1, ==, count);
  g_assert_cmpuint (0, ==, default_setting);

  g_assert_cmpuint (3, ==, records[0].setting);
  g_assert_cmpuint (297, ==, records[0].name_id);


  count = 1;
  g_assert_cmpuint (1, ==, hb_aat_layout_get_feature_settings (face, HB_AAT_LAYOUT_FEATURE_TYPE_TYPOGRAPHIC_EXTRAS,
							       &default_setting, 0, &count, records));
  g_assert_cmpuint (1, ==, count);
  g_assert_cmpuint (HB_AAT_LAYOUT_FEATURE_TYPE_UNDEFINED, ==, default_setting);

  g_assert_cmpuint (8, ==, records[0].setting);
  g_assert_cmpuint (308, ==, records[0].name_id);


  count = 100;
  g_assert_cmpuint (0, ==, hb_aat_layout_get_feature_settings (face, HB_AAT_LAYOUT_FEATURE_TYPE_UNDEFINED,
							       NULL, 0, &count, records));
  g_assert_cmpuint (0, ==, count);

  g_assert_cmpuint (0, ==, hb_aat_layout_get_feature_settings (sbix, HB_AAT_LAYOUT_FEATURE_TYPE_UNDEFINED, NULL,
							       0, &count, records));
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_aat_get_features);
  hb_test_add (test_aat_get_feature_settings);

  face = hb_test_open_font_file ("fonts/aat-feat.ttf");
  sbix = hb_test_open_font_file ("fonts/chromacheck-sbix.ttf");
  unsigned int status = hb_test_run ();
  hb_face_destroy (sbix);
  hb_face_destroy (face);
  return status;
}
