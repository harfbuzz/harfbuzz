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

#include <hb-ot.h>

/* Unit tests for hb-aat.h */

static void
test_aat_get_feature_settings (void)
{
  hb_aat_feature_setting_t default_setting;
  hb_aat_feature_type_selector_t records[3];
  unsigned int count = 3;

  hb_face_t *face = hb_test_open_font_file ("fonts/aat-feat.ttf");

  g_assert_cmpuint (4, ==, hb_aat_layout_get_feature_settings (face, 18, &default_setting,
							0, &count, records));
  g_assert_cmpuint (3, ==, count);
  g_assert_cmpuint (0, ==, default_setting);

  g_assert_cmpuint (0, ==, records[0].setting);
  g_assert_cmpuint (294, ==, records[0].name_id);

  g_assert_cmpuint (1, ==, records[1].setting);
  g_assert_cmpuint (295, ==, records[1].name_id);

  g_assert_cmpuint (2, ==, records[2].setting);
  g_assert_cmpuint (296, ==, records[2].name_id);

  count = 3;
  g_assert_cmpuint (4, ==, hb_aat_layout_get_feature_settings (face, 18, &default_setting,
							3, &count, records));
  g_assert_cmpuint (1, ==, count);
  g_assert_cmpuint (0, ==, default_setting);

  g_assert_cmpuint (3, ==, records[0].setting);
  g_assert_cmpuint (297, ==, records[0].name_id);


  count = 1;
  g_assert_cmpuint (1, ==, hb_aat_layout_get_feature_settings (face, 14, &default_setting,
							0, &count, records));
  g_assert_cmpuint (1, ==, count);
  g_assert_cmpuint (HB_AAT_FEATURE_NO_DEFAULT_SETTING, ==, default_setting);

  g_assert_cmpuint (8, ==, records[0].setting);
  g_assert_cmpuint (308, ==, records[0].name_id);


  count = 100;
  g_assert_cmpuint (0, ==, hb_aat_layout_get_feature_settings (face, 32, NULL,
							0, &count, records));
  g_assert_cmpuint (0, ==, count);

  hb_face_destroy (face);

  hb_face_t *sbix = hb_test_open_font_file ("fonts/chromacheck-sbix.ttf");
  g_assert_cmpuint (0, ==, hb_aat_layout_get_feature_settings (sbix, 100, NULL,
							0, &count, records));
  hb_face_destroy (sbix);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_aat_get_feature_settings);

  return hb_test_run();
}
