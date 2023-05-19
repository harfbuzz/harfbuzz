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

  g_assert (hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
					    HB_SCRIPT_LATIN, HB_LANGUAGE_INVALID,
					    &extents));
  g_assert_cmpint (extents.ascender, ==, 2000);

  g_assert (hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
					    HB_SCRIPT_LATIN, hb_language_from_string ("xx", -1),
					    &extents));
  g_assert_cmpint (extents.ascender, ==, 2000);

  g_assert (!hb_ot_layout_get_font_extents2 (font, HB_DIRECTION_LTR,
					     HB_SCRIPT_ARABIC, HB_LANGUAGE_INVALID,
					     &extents));
  g_assert_cmpint (extents.ascender, ==, 3000);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_ot_layout_font_extents);

  return hb_test_run();
}
