/*
 * Copyright © 2018  Ebrahim Byagowi
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

/* Unit tests for hb-ot-layout.h baseline */

static void
test_ot_layout_base (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/base.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_position_t position;
  g_assert_true (hb_ot_layout_get_baseline (font, HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_BOTTOM_OR_LEFT, HB_DIRECTION_TTB,
					    HB_TAG ('h','a','n','i'),
					    HB_TAG ('E','N','G',' '),
					    &position));
  g_assert_cmpint (46, ==, position);

  g_assert_true (hb_ot_layout_get_baseline2 (font, HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_BOTTOM_OR_LEFT, HB_DIRECTION_TTB,
					     HB_SCRIPT_HAN,
					     hb_language_from_string ("en", -1),
					     &position));
  g_assert_cmpint (46, ==, position);

  g_assert_true (!hb_ot_layout_get_baseline (font, HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_TOP_OR_RIGHT, HB_DIRECTION_TTB,
					     HB_TAG ('h','a','n','i'),
					     HB_TAG ('E','N','G',' '),
					     &position));

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_layout_base_with_fallback (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/base.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_position_t position;
  hb_ot_layout_get_baseline_with_fallback (font, HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_BOTTOM_OR_LEFT, HB_DIRECTION_TTB,
					   HB_TAG ('h','a','n','i'),
					   HB_TAG ('E','N','G',' '),
					   &position);
  g_assert_cmpint (46, ==, position);

  hb_ot_layout_get_baseline_with_fallback2 (font, HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_BOTTOM_OR_LEFT, HB_DIRECTION_TTB,
					    HB_SCRIPT_HAN,
					    hb_language_from_string ("en", -1),
					    &position);
  g_assert_cmpint (46, ==, position);

  hb_ot_layout_get_baseline_with_fallback (font, HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_TOP_OR_RIGHT, HB_DIRECTION_TTB,
					   HB_TAG ('h','a','n','i'),
					   HB_TAG ('E','N','G',' '),
					   &position);
  g_assert_cmpint (1000, ==, position);

  hb_ot_layout_get_baseline_with_fallback (font, HB_OT_LAYOUT_BASELINE_TAG_MATH, HB_DIRECTION_LTR,
					   HB_TAG ('h','a','n','i'),
					   HB_TAG ('E','N','G',' '),
					   &position);
  g_assert_cmpint (271, ==, position);

  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/base2.ttf");
  font = hb_font_create (face);

  hb_ot_layout_get_baseline_with_fallback (font, HB_OT_LAYOUT_BASELINE_TAG_HANGING, HB_DIRECTION_LTR,
					   HB_SCRIPT_BENGALI,
					   HB_TAG ('E','N','G',' '),
					   &position);
  g_assert_cmpint (622, ==, position);

  hb_ot_layout_get_baseline_with_fallback (font, HB_OT_LAYOUT_BASELINE_TAG_HANGING, HB_DIRECTION_LTR,
					   HB_SCRIPT_TIBETAN,
					   HB_TAG ('E','N','G',' '),
					   &position);
  g_assert_cmpint (600, ==, position);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_layout_base_format4 (void)
{
  static const char BASE_data[] = {
    0, 1, 0, 0,			/* version */
    0, 8,				/* horizontal axis offset */
    0, 0,				/* vertical axis offset */
    0, 4,				/* base tag list offset */
    0, 10,				/* base script list offset */
    0, 1, 'r', 'o', 'm', 'n',		/* base tag list */
    0, 1, 'l', 'a', 't', 'n', 0, 8,	/* base script list */
    0, 6, 0, 0, 0, 0,			/* base script */
    0, 0, 0, 1, 0, 6,			/* base values */
    0, 4, 0, 123, 0, 0, 0, 0, 0,	/* BaseCoord format 4 */
  };
  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "BASE", BASE_data);
  hb_font_t *font = hb_font_create (face);
  hb_position_t position;

#ifndef HB_NO_BEYOND_64K
  g_assert_true (hb_ot_layout_get_baseline (font, HB_OT_LAYOUT_BASELINE_TAG_ROMAN,
					    HB_DIRECTION_LTR,
					    HB_TAG ('l','a','t','n'),
					    HB_TAG_NONE, &position));
  g_assert_cmpint (position, ==, 123);
#else
  g_assert_false (hb_ot_layout_get_baseline (font, HB_OT_LAYOUT_BASELINE_TAG_ROMAN,
					     HB_DIRECTION_LTR,
					     HB_TAG ('l','a','t','n'),
					     HB_TAG_NONE, &position));
#endif

  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_ot_layout_base);
  hb_test_add (test_ot_layout_base_with_fallback);
  hb_test_add (test_ot_layout_base_format4);

  return hb_test_run();
}
