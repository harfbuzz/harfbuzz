/*
 * Copyright © 2023  Behdad Esfahbod
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

/* Unit tests for hb-ot-layout.h font extents */

static void
test_ot_layout_font_extents (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/base-minmax.ttf");
  hb_font_t *font = hb_font_create (face);

  hb_font_extents_t extents;

  g_assert_true (hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
						 HB_SCRIPT_LATIN, HB_LANGUAGE_INVALID,
						 &extents));
  g_assert_cmpint (extents.ascender, ==, 2000);

  g_assert_true (hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
						 HB_SCRIPT_LATIN, hb_language_from_string ("xx", -1),
						 &extents));
  g_assert_cmpint (extents.ascender, ==, 2000);

  g_assert_true (!hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
						  HB_SCRIPT_ARABIC, HB_LANGUAGE_INVALID,
						  &extents));
  g_assert_cmpint (extents.ascender, ==, 3000);

  /* NULL extents is allowed on both the hit and the miss path. */
  g_assert_true (hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
						 HB_SCRIPT_LATIN, HB_LANGUAGE_INVALID,
						 NULL));
  g_assert_true (!hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
						  HB_SCRIPT_ARABIC, HB_LANGUAGE_INVALID,
						  NULL));

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_ot_layout_missing_font_extents (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/base-missing-minmax.ttf");
  hb_font_t *font = hb_font_create (face);
  const struct {
    hb_script_t script;
    const char *language;
    hb_bool_t found;
    int ascender, descender, line_gap;
  } cases[] = {
    {HB_SCRIPT_LATIN, NULL, false, 800, -200, 50},
    {HB_SCRIPT_LATIN, "fr", false, 800, -200, 50},
    {HB_SCRIPT_LATIN, "en", true, 950, -150, 0},
    {HB_SCRIPT_ARABIC, NULL, false, 800, -200, 50},
    {HB_SCRIPT_CYRILLIC, NULL, false, 800, -200, 50},
    {HB_SCRIPT_DEVANAGARI, NULL, true, 900, -100, 0},
    {HB_SCRIPT_DEVANAGARI, "en", true, 900, -100, 0},
    {HB_SCRIPT_DEVANAGARI, "fr", true, 1100, -100, 0},
    {HB_SCRIPT_DEVANAGARI, "ru", true, 900, 0, 0},
    {HB_SCRIPT_GREEK, NULL, true, 0, 0, 0},
    {HB_SCRIPT_HEBREW, NULL, false, 800, -200, 50},
  };

  for (unsigned scale = 1; scale <= 2; scale++)
  {
    hb_font_set_scale (font, scale * 1000, scale * 1000);
    for (unsigned i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      hb_language_t language = hb_language_from_string (cases[i].language, -1);
      hb_font_extents_t extents;
      g_assert_cmpint (hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
                                                      cases[i].script, language, &extents),
                       ==, cases[i].found);
      g_assert_cmpint (extents.ascender, ==, cases[i].ascender * (int) scale);
      g_assert_cmpint (extents.descender, ==, cases[i].descender * (int) scale);
      g_assert_cmpint (extents.line_gap, ==, cases[i].line_gap * (int) scale);
      g_assert_cmpint (hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
                                                      cases[i].script, language, NULL),
                       ==, cases[i].found);
    }
  }
  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_ot_layout_font_extents);
  hb_test_add (test_ot_layout_missing_font_extents);

  return hb_test_run();
}
