/*
 * Copyright © 2026  Behdad Esfahbod
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
#include <hb-aat.h>

/* Regression test for GHSA-v5mv-8hhw-vff8: the "list getter" APIs document
 * their output array as nullable ("may be NULL"; pass NULL to just query the
 * count).  Passing a NULL output array together with a non-NULL count on a
 * populated table used to write through the NULL pointer and crash.  Each case
 * below calls the getter with a non-NULL count and a NULL output array on a
 * font whose table is populated, and checks it returns the total without
 * crashing. */

static void
test_nullable_output_cpal_colors (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cpal-v0.ttf");
  unsigned int count = 100;
  g_assert_cmpuint (hb_ot_color_palette_get_colors (face, 0, 0, &count, NULL), ==, 2);
  hb_face_destroy (face);
}

static void
test_nullable_output_colr_layers (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cpal-v1.ttf");
  unsigned int count = 100;
  g_assert_cmpuint (hb_ot_color_glyph_get_layers (face, 2, 0, &count, NULL), ==, 2);
  hb_face_destroy (face);
}

static void
test_nullable_output_meta_entries (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/meta.ttf");
  unsigned int count = 100;
  unsigned int total = hb_ot_meta_get_entry_tags (face, 0, &count, NULL);
  g_assert_cmpuint (total, >, 0);
  hb_face_destroy (face);
}

#ifndef HB_NO_AAT
static void
test_nullable_output_aat_feature_types (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/aat-feat.ttf");
  unsigned int count = 100;
  g_assert_cmpuint (hb_aat_layout_get_feature_types (face, 0, &count, NULL), ==, 11);
  hb_face_destroy (face);
}

static void
test_nullable_output_aat_selector_infos (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/aat-feat.ttf");
  unsigned int count = 100;
  g_assert_cmpuint (hb_aat_layout_feature_type_get_selector_infos (
		      face, HB_AAT_LAYOUT_FEATURE_TYPE_DESIGN_COMPLEXITY_TYPE,
		      0, &count, NULL, NULL), ==, 4);
  hb_face_destroy (face);
}
#endif

static void
test_nullable_output_cv_characters (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cv01.otf");
  unsigned int feature_index;
  if (hb_ot_layout_language_find_feature (face, HB_OT_TAG_GSUB, 0,
					  HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
					  HB_TAG ('c','v','0','1'), &feature_index))
  {
    unsigned int count = 100;
    g_assert_cmpuint (hb_ot_layout_feature_get_characters (face, HB_OT_TAG_GSUB,
							   feature_index, 0,
							   &count, NULL), ==, 2);
  }
  hb_face_destroy (face);
}

static void
test_nullable_output_gsub_alternates (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");
  unsigned int count = 100;
  g_assert_cmpuint (hb_ot_layout_lookup_get_glyph_alternates (face, 1, 1091, 0,
							      &count, NULL), ==, 7);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_nullable_output_cpal_colors);
  hb_test_add (test_nullable_output_colr_layers);
  hb_test_add (test_nullable_output_meta_entries);
#ifndef HB_NO_AAT
  hb_test_add (test_nullable_output_aat_feature_types);
  hb_test_add (test_nullable_output_aat_selector_infos);
#endif
  hb_test_add (test_nullable_output_cv_characters);
  hb_test_add (test_nullable_output_gsub_alternates);

  return hb_test_run ();
}
