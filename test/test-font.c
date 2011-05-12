/*
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-test.h"

/* Unit tests for hb-font.h */


static void
test_face_empty (void)
{
  g_assert (hb_face_get_empty ());
  g_assert (hb_face_get_empty () == hb_face_create (hb_blob_get_empty (), 0));
  g_assert (hb_face_get_empty () == hb_face_create (NULL, 0));
}

static void
test_font_funcs_empty (void)
{
  g_assert (hb_font_funcs_get_empty ());
  g_assert (hb_font_funcs_is_immutable (hb_font_funcs_get_empty ()));
}

static void
test_font_empty (void)
{
  g_assert (hb_font_get_empty ());
  g_assert (hb_font_get_empty () == hb_font_create (hb_face_get_empty ()));
  g_assert (hb_font_get_empty () == hb_font_create (NULL));
  g_assert (hb_font_get_empty () == hb_font_create_sub_font (NULL));
  g_assert (hb_font_is_immutable (hb_font_get_empty ()));
}

static const char test_data[] = "test\0data";

static void
test_font_properties (void)
{
  hb_blob_t *blob;
  hb_face_t *face;
  hb_font_t *font;
  int x_scale, y_scale;
  unsigned int x_ppem, y_ppem;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font = hb_font_create (face);
  hb_face_destroy (face);


  /* Check scale */

  hb_font_get_scale (font, NULL, NULL);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, &x_scale, NULL);
  g_assert_cmpint (x_scale, ==, 0);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, NULL, &y_scale);
  g_assert_cmpint (y_scale, ==, 0);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 0);
  g_assert_cmpint (y_scale, ==, 0);

  hb_font_set_scale (font, 17, 19);

  x_scale = y_scale = 13;
  hb_font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);


  /* Check ppem */

  hb_font_get_ppem (font, NULL, NULL);
  x_ppem = y_ppem = 13;
  hb_font_get_ppem (font, &x_ppem, NULL);
  g_assert_cmpint (x_ppem, ==, 0);
  x_ppem = y_ppem = 13;
  hb_font_get_ppem (font, NULL, &y_ppem);
  g_assert_cmpint (y_ppem, ==, 0);
  x_ppem = y_ppem = 13;
  hb_font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 0);
  g_assert_cmpint (y_ppem, ==, 0);

  hb_font_set_ppem (font, 17, 19);

  x_ppem = y_ppem = 13;
  hb_font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);


  /* Check immutable */

  g_assert (!hb_font_is_immutable (font));
  hb_font_make_immutable (font);
  g_assert (hb_font_is_immutable (font));

  hb_font_set_scale (font, 10, 12);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);

  hb_font_set_ppem (font, 10, 12);
  x_ppem = y_ppem = 13;
  hb_font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);


  hb_font_destroy (font);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_face_empty);

  hb_test_add (test_font_funcs_empty);

  hb_test_add (test_font_empty);
  hb_test_add (test_font_properties);

  return hb_test_run();
}
