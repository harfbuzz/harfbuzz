/*
 * Copyright © 2020  Ebrahim Byagowi
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

#include "hb-ot.h"
#include "hb-ft.h"

/* Unit tests for hb-font.h */

static void
test_glyf_get_glyph_contour_point ()
{
  hb_face_t *face = hb_test_open_font_file ("fonts/OpenSans-Regular.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  hb_position_t x, y;

  /* ft */
  hb_ft_font_set_funcs (font);
  g_assert (hb_font_get_glyph_contour_point (font, 29, 3, &x, &y));
  g_assert_cmpint (x, ==, 270);
  g_assert_cmpint (y, ==, 242);

  /* ot */
  hb_ot_font_set_funcs (font);
  g_assert (hb_font_get_glyph_contour_point (font, 29, 3, &x, &y));
  g_assert_cmpint (x, ==, 270);
  g_assert_cmpint (y, ==, 242);

  hb_font_destroy (font);
}

static void
test_cff1_get_glyph_contour_point ()
{
  hb_face_t *face = hb_test_open_font_file ("fonts/RanaKufi-Regular.subset.otf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  hb_position_t x, y;

  /* ft */
  hb_ft_font_set_funcs (font);
  g_assert (hb_font_get_glyph_contour_point (font, 1, 3, &x, &y));
  g_assert_cmpint (x, ==, 650);
  g_assert_cmpint (y, ==, 274);

  /* ot */
  hb_ot_font_set_funcs (font);
  g_assert (hb_font_get_glyph_contour_point (font, 1, 3, &x, &y));
  g_assert_cmpint (x, ==, 650);
  g_assert_cmpint (y, ==, 274);

  hb_font_destroy (font);
}

static void
test_cff2_get_glyph_contour_point ()
{
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  hb_position_t x, y;

  /* ft */
  hb_ft_font_set_funcs (font);
  g_assert (hb_font_get_glyph_contour_point (font, 3, 2, &x, &y));
  g_assert_cmpint (x, ==, 337);
  g_assert_cmpint (y, ==, 435);

  /* ot */
  hb_ot_font_set_funcs (font);
  g_assert (hb_font_get_glyph_contour_point (font, 3, 2, &x, &y));
  g_assert_cmpint (x, ==, 337);
  g_assert_cmpint (y, ==, 435);

  hb_font_destroy (font);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_glyf_get_glyph_contour_point);
  hb_test_add (test_cff1_get_glyph_contour_point);
  hb_test_add (test_cff2_get_glyph_contour_point);

 return hb_test_run();
}
