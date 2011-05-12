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


static const char test_data[] = "test\0data";


static void
test_face_empty (void)
{
  g_assert (hb_face_get_empty ());
  g_assert (hb_face_get_empty () == hb_face_create (hb_blob_get_empty (), 0));
  g_assert (hb_face_get_empty () == hb_face_create (NULL, 0));

  g_assert (hb_face_reference_table (hb_face_get_empty (), HB_TAG ('h','e','a','d')) == hb_blob_get_empty ());

  g_assert_cmpint (hb_face_get_upem (hb_face_get_empty ()), ==, 1000);
}

static void
test_face_create (void)
{
  hb_face_t *face;
  hb_blob_t *blob;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);

  g_assert (hb_face_reference_table (face, HB_TAG ('h','e','a','d')) == hb_blob_get_empty ());

  g_assert_cmpint (hb_face_get_upem (face), ==, 1000);

  hb_face_destroy (face);
}


static void
free_up (void *user_data)
{
  int *freed = (int *) user_data;

  g_assert (!*freed);

  (*freed)++;
}

static hb_blob_t *
get_table (hb_face_t *face, hb_tag_t tag, void *user_data)
{
  if (tag == HB_TAG ('a','b','c','d'))
    return hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);

  return hb_blob_get_empty ();
}

static void
test_face_createfortables (void)
{
  hb_face_t *face;
  hb_blob_t *blob;
  const char *data;
  unsigned int len;
  int freed = 0;

  face = hb_face_create_for_tables (get_table, &freed, free_up);
  g_assert (!freed);

  g_assert (hb_face_reference_table (face, HB_TAG ('h','e','a','d')) == hb_blob_get_empty ());

  blob = hb_face_reference_table (face, HB_TAG ('a','b','c','d'));
  g_assert (blob != hb_blob_get_empty ());

  data = hb_blob_get_data (blob, &len);
  g_assert_cmpint (len, ==, sizeof (test_data));
  g_assert (0 == memcmp (data, test_data, sizeof (test_data)));
  hb_blob_destroy (blob);

  g_assert_cmpint (hb_face_get_upem (face), ==, 1000);

  hb_face_destroy (face);
  g_assert (freed);
}

static void
_test_font_nil_funcs (hb_font_t *font)
{
  hb_position_t x, y;
  hb_glyph_extents_t extents;

  x = y = 13;
  g_assert (!hb_font_get_contour_point (font, 17, 2, &x, &y));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);

  x = y = 13;
  hb_font_get_glyph_advance (font, 17, &x, &y);
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);

  extents.x_bearing = extents.y_bearing = 13;
  extents.width = extents.height = 15;
  hb_font_get_glyph_extents (font, 17, &extents);
  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 0);
  g_assert_cmpint (extents.width, ==, 0);
  g_assert_cmpint (extents.height, ==, 0);

  g_assert (0 == hb_font_get_glyph (font, 17, 2));

  x = y = 13;
  hb_font_get_kerning (font, 17, 19, &x, &y);
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
}

static void
_test_fontfuncs_nil (hb_font_funcs_t *ffuncs)
{
  hb_blob_t *blob;
  hb_face_t *face;
  hb_font_t *font;
  hb_font_t *subfont;
  int freed = 0;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font = hb_font_create (face);
  hb_face_destroy (face);


  hb_font_set_funcs (font, ffuncs, &freed, free_up);
  g_assert_cmpint (freed, ==, 0);

  _test_font_nil_funcs (font);

  subfont = hb_font_create_sub_font (font);

  g_assert_cmpint (freed, ==, 0);
  hb_font_destroy (font);
  g_assert_cmpint (freed, ==, 0);

  _test_font_nil_funcs (subfont);

  hb_font_destroy (subfont);
  g_assert_cmpint (freed, ==, 1);
}

static void
test_fontfuncs_empty (void)
{
  g_assert (hb_font_funcs_get_empty ());
  g_assert (hb_font_funcs_is_immutable (hb_font_funcs_get_empty ()));
  _test_fontfuncs_nil (hb_font_funcs_get_empty ());
}

static void
test_fontfuncs_custom (void)
{
  hb_font_funcs_t *ffuncs;

  ffuncs = hb_font_funcs_create ();

  g_assert (!hb_font_funcs_is_immutable (ffuncs));
  _test_fontfuncs_nil (hb_font_funcs_get_empty ());

  hb_font_funcs_destroy (ffuncs);
}


static void
test_font_empty (void)
{
  g_assert (hb_font_get_empty ());
  g_assert (hb_font_get_empty () == hb_font_create (hb_face_get_empty ()));
  g_assert (hb_font_get_empty () == hb_font_create (NULL));
  g_assert (hb_font_get_empty () == hb_font_create_sub_font (NULL));
  g_assert (hb_font_is_immutable (hb_font_get_empty ()));

  g_assert (hb_font_get_face (hb_font_get_empty ()) == hb_face_get_empty ());
  g_assert (hb_font_get_parent (hb_font_get_empty ()) == NULL);
}

static void
test_font_properties (void)
{
  hb_blob_t *blob;
  hb_face_t *face;
  hb_font_t *font;
  hb_font_t *subfont;
  int x_scale, y_scale;
  unsigned int x_ppem, y_ppem;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font = hb_font_create (face);
  hb_face_destroy (face);


  g_assert (hb_font_get_face (font) == face);
  g_assert (hb_font_get_parent (font) == NULL);


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


  /* sub_font now */
  subfont = hb_font_create_sub_font (font);
  hb_font_destroy (font);

  g_assert (hb_font_get_parent (subfont) == font);
  g_assert (hb_font_get_face (subfont) == face);

  /* scale */
  x_scale = y_scale = 13;
  hb_font_get_scale (subfont, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);
  hb_font_set_scale (subfont, 10, 12);
  x_scale = y_scale = 13;
  hb_font_get_scale (subfont, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 10);
  g_assert_cmpint (y_scale, ==, 12);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);

  /* ppem */
  x_ppem = y_ppem = 13;
  hb_font_get_ppem (subfont, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);
  hb_font_set_ppem (subfont, 10, 12);
  x_ppem = y_ppem = 13;
  hb_font_get_ppem (subfont, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 10);
  g_assert_cmpint (y_ppem, ==, 12);
  x_ppem = y_ppem = 13;
  hb_font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);

  hb_font_destroy (subfont);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_face_empty);
  hb_test_add (test_face_create);
  hb_test_add (test_face_createfortables);

  hb_test_add (test_fontfuncs_empty);
  hb_test_add (test_fontfuncs_custom);

  hb_test_add (test_font_empty);
  hb_test_add (test_font_properties);


  return hb_test_run();
}
