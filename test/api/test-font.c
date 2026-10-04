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

#ifdef HAVE_FONTATIONS
#include <hb-fontations.h>
#endif

/* Unit tests for hb-font.h */


static const char test_data[] = "test\0data";


static void
test_face_empty (void)
{
  hb_face_t *created_from_empty;
  hb_face_t *created_from_null;

  g_assert_true (hb_face_get_empty ());

  created_from_empty = hb_face_create (hb_blob_get_empty (), 0);
  g_assert_true (hb_face_get_empty () != created_from_empty);

  created_from_null = hb_face_create (NULL, 0);
  g_assert_true (hb_face_get_empty () != created_from_null);

  g_assert_true (hb_face_reference_table (hb_face_get_empty (), HB_TAG ('h','e','a','d')) == hb_blob_get_empty ());

  g_assert_cmpint (hb_face_get_upem (hb_face_get_empty ()), ==, 1000);

  hb_face_destroy (created_from_null);
  hb_face_destroy (created_from_empty);
}

static void
test_face_create (void)
{
  hb_face_t *face;
  hb_blob_t *blob;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);

  g_assert_true (hb_face_reference_table (face, HB_TAG ('h','e','a','d')) == hb_blob_get_empty ());

  g_assert_cmpint (hb_face_get_upem (face), ==, 1000);

  hb_face_destroy (face);
}


static void
free_up (void *user_data)
{
  int *freed = (int *) user_data;

  g_assert_true (!*freed);

  (*freed)++;
}

static hb_tag_t test_tags[] = {
  HB_TAG ('a','b','c','d'),
  HB_TAG ('e','f','g','h'),
};

static hb_blob_t *
get_table (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data HB_UNUSED)
{
  for (unsigned i = 0; i < sizeof (test_tags) / sizeof (test_tags[0]); i++)
  {
    if (test_tags[i] == tag)
      return hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  }

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
  g_assert_true (!freed);

  g_assert_true (hb_face_reference_table (face, HB_TAG ('h','e','a','d')) == hb_blob_get_empty ());

  blob = hb_face_reference_table (face, HB_TAG ('a','b','c','d'));
  g_assert_true (blob != hb_blob_get_empty ());

  data = hb_blob_get_data (blob, &len);
  g_assert_cmpint (len, ==, sizeof (test_data));
  g_assert_true (0 == memcmp (data, test_data, sizeof (test_data)));
  hb_blob_destroy (blob);

  g_assert_cmpint (hb_face_get_upem (face), ==, 1000);

  hb_face_destroy (face);
  g_assert_true (freed);
}

static unsigned int
get_table_tags (const hb_face_t *face HB_UNUSED,
                unsigned int  start_offset,
                unsigned int *table_count,
                hb_tag_t     *table_tags,
                void         *user_data HB_UNUSED)
{
  unsigned count = sizeof (test_tags) / sizeof (test_tags[0]);
  unsigned end_offset;

  if (!table_count)
    return count;

  if (start_offset >= count)
  {
    *table_count = 0;
    return count;
  }

  end_offset = start_offset + *table_count;
  if (end_offset < start_offset)
  {
    *table_count = 0;
    return count;
  }

  end_offset = end_offset < count ? end_offset : count;

  *table_count = end_offset - start_offset;

  for (unsigned i = start_offset; i < end_offset; i++)
    table_tags[i - start_offset] = test_tags[i];

  return count;
}

static void
test_face_referenceblob (void)
{
  hb_blob_t *blob;
  hb_face_t *face;
  int freed = 0;

  face = hb_face_create_for_tables (get_table, &freed, free_up);
  hb_face_set_get_table_tags_func (face, get_table_tags, NULL, NULL);

  blob = hb_face_reference_blob (face);
  hb_face_destroy (face);

  g_assert_true (blob != hb_blob_get_empty ());

  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);

  g_assert_true (face != hb_face_get_empty ());
  g_assert_cmpuint (hb_face_get_table_tags (face, 0, NULL, NULL), ==, sizeof (test_tags) / sizeof (test_tags[0]));
  for (unsigned i = 0; i < sizeof (test_tags) / sizeof (test_tags[0]); i++)
  {
    hb_blob_t* table = hb_face_reference_table (face, test_tags[i]);
    g_assert_true (table != hb_blob_get_empty ());
    hb_blob_destroy (table);
  }

  hb_face_destroy (face);
}

static void
_test_font_nil_funcs (hb_font_t *font)
{
  hb_codepoint_t glyph;
  hb_position_t x, y;
  hb_glyph_extents_t extents;
  unsigned int upem = hb_face_get_upem (hb_font_get_face (font));

  x = y = 13;
  g_assert_true (!hb_font_get_glyph_contour_point (font, 17, 2, &x, &y));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);

  x = hb_font_get_glyph_h_advance (font, 17);
  g_assert_cmpint (x, ==, upem);

  extents.x_bearing = extents.y_bearing = 13;
  extents.width = extents.height = 15;
  hb_font_get_glyph_extents (font, 17, &extents);
  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 0);
  g_assert_cmpint (extents.width, ==, 0);
  g_assert_cmpint (extents.height, ==, 0);

  glyph = 3;
  g_assert_true (!hb_font_get_glyph (font, 17, 2, &glyph));
  g_assert_cmpint (glyph, ==, 0);
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
  g_assert_true (!hb_face_is_immutable (face));
  font = hb_font_create (face);
  g_assert_true (font);
  g_assert_true (hb_face_is_immutable (face));
  hb_face_destroy (face);


  hb_font_set_funcs (font, ffuncs, &freed, free_up);
  g_assert_cmpint (freed, ==, 0);

  _test_font_nil_funcs (font);

  subfont = hb_font_create_sub_font (font);
  g_assert_true (subfont);

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
  g_assert_true (hb_font_funcs_get_empty ());
  g_assert_true (hb_font_funcs_is_immutable (hb_font_funcs_get_empty ()));
  _test_fontfuncs_nil (hb_font_funcs_get_empty ());
}

static void
test_fontfuncs_nil (void)
{
  hb_font_funcs_t *ffuncs;

  ffuncs = hb_font_funcs_create ();

  g_assert_true (!hb_font_funcs_is_immutable (ffuncs));
  _test_fontfuncs_nil (hb_font_funcs_get_empty ());

  hb_font_funcs_destroy (ffuncs);
}

static hb_bool_t
contour_point_func1 (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
		     hb_codepoint_t glyph, unsigned int point_index HB_UNUSED,
		     hb_position_t *x, hb_position_t *y,
		     void *user_data HB_UNUSED)
{
  if (glyph == 1) {
    *x = 2;
    *y = 3;
    return TRUE;
  }
  if (glyph == 2) {
    *x = 4;
    *y = 5;
    return TRUE;
  }

  return FALSE;
}

static hb_bool_t
contour_point_func2 (hb_font_t *font, void *font_data HB_UNUSED,
		     hb_codepoint_t glyph, unsigned int point_index,
		     hb_position_t *x, hb_position_t *y,
		     void *user_data HB_UNUSED)
{
  if (glyph == 1) {
    *x = 6;
    *y = 7;
    return TRUE;
  }

  return hb_font_get_glyph_contour_point (hb_font_get_parent (font),
					  glyph, point_index, x, y);
}

static hb_position_t
glyph_h_advance_func1 (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
		       hb_codepoint_t glyph,
		       void *user_data HB_UNUSED)
{
  if (glyph == 1)
    return 8;

  return 0;
}

static void
test_fontfuncs_subclassing (void)
{
  hb_blob_t *blob;
  hb_face_t *face;

  hb_font_funcs_t *ffuncs1;
  hb_font_funcs_t *ffuncs2;

  hb_font_t *font1;
  hb_font_t *font2;
  hb_font_t *font3;

  hb_position_t x;
  hb_position_t y;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font1 = hb_font_create (face);
  hb_face_destroy (face);
  hb_font_set_scale (font1, 10, 10);

  /* setup font1 */
  ffuncs1 = hb_font_funcs_create ();
  hb_font_funcs_set_glyph_contour_point_func (ffuncs1, contour_point_func1, NULL, NULL);
  hb_font_funcs_set_glyph_h_advance_func (ffuncs1, glyph_h_advance_func1, NULL, NULL);
  hb_font_set_funcs (font1, ffuncs1, NULL, NULL);
  hb_font_funcs_destroy (ffuncs1);

  x = y = 1;
  g_assert_true (hb_font_get_glyph_contour_point_for_origin (font1, 1, 2, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 2);
  g_assert_cmpint (y, ==, 3);
  g_assert_true (hb_font_get_glyph_contour_point_for_origin (font1, 2, 5, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 4);
  g_assert_cmpint (y, ==, 5);
  g_assert_true (!hb_font_get_glyph_contour_point_for_origin (font1, 3, 7, HB_DIRECTION_RTL, &x, &y));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
  x = hb_font_get_glyph_h_advance (font1, 1);
  g_assert_cmpint (x, ==, 8);
  x = hb_font_get_glyph_h_advance (font1, 2);
  g_assert_cmpint (x, ==, 0);

  /* creating sub-font doesn't make the parent font immutable;
   * making a font immutable however makes it's lineage immutable.
   */
  font2 = hb_font_create_sub_font (font1);
  font3 = hb_font_create_sub_font (font2);
  g_assert_true (!hb_font_is_immutable (font1));
  g_assert_true (!hb_font_is_immutable (font2));
  g_assert_true (!hb_font_is_immutable (font3));
  hb_font_make_immutable (font3);
  g_assert_true (hb_font_is_immutable (font1));
  g_assert_true (hb_font_is_immutable (font2));
  g_assert_true (hb_font_is_immutable (font3));
  hb_font_destroy (font2);
  hb_font_destroy (font3);

  font2 = hb_font_create_sub_font (font1);
  hb_font_destroy (font1);

  /* setup font2 to override some funcs */
  ffuncs2 = hb_font_funcs_create ();
  hb_font_funcs_set_glyph_contour_point_func (ffuncs2, contour_point_func2, NULL, NULL);
  hb_font_set_funcs (font2, ffuncs2, NULL, NULL);
  hb_font_funcs_destroy (ffuncs2);

  x = y = 1;
  g_assert_true (hb_font_get_glyph_contour_point_for_origin (font2, 1, 2, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 6);
  g_assert_cmpint (y, ==, 7);
  g_assert_true (hb_font_get_glyph_contour_point_for_origin (font2, 2, 5, HB_DIRECTION_RTL, &x, &y));
  g_assert_cmpint (x, ==, 4);
  g_assert_cmpint (y, ==, 5);
  g_assert_true (!hb_font_get_glyph_contour_point_for_origin (font2, 3, 7, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
  x = hb_font_get_glyph_h_advance (font2, 1);
  g_assert_cmpint (x, ==, 8);
  x = hb_font_get_glyph_h_advance (font2, 2);
  g_assert_cmpint (x, ==, 0);

  /* setup font3 to override scale */
  font3 = hb_font_create_sub_font (font2);
  hb_font_set_scale (font3, 20, 30);

  x = y = 1;
  g_assert_true (hb_font_get_glyph_contour_point_for_origin (font3, 1, 2, HB_DIRECTION_RTL, &x, &y));
  g_assert_cmpint (x, ==, 6*2);
  g_assert_cmpint (y, ==, 7*3);
  g_assert_true (hb_font_get_glyph_contour_point_for_origin (font3, 2, 5, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 4*2);
  g_assert_cmpint (y, ==, 5*3);
  g_assert_true (!hb_font_get_glyph_contour_point_for_origin (font3, 3, 7, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 0*2);
  g_assert_cmpint (y, ==, 0*3);
  x = hb_font_get_glyph_h_advance (font3, 1);
  g_assert_cmpint (x, ==, 8*2);
  x = hb_font_get_glyph_h_advance (font3, 2);
  g_assert_cmpint (x, ==, 0*2);


  hb_font_destroy (font3);
  hb_font_destroy (font2);
}

static hb_bool_t
nominal_glyph_func (hb_font_t *font HB_UNUSED,
		    void *font_data HB_UNUSED,
		    hb_codepoint_t unicode HB_UNUSED,
		    hb_codepoint_t *glyph,
		    void *user_data HB_UNUSED)
{
  *glyph = 0;
  return FALSE;
}

static unsigned int
nominal_glyphs_func (hb_font_t *font HB_UNUSED,
		     void *font_data HB_UNUSED,
		     unsigned int count HB_UNUSED,
		     const hb_codepoint_t *first_unicode HB_UNUSED,
		     unsigned int unicode_stride HB_UNUSED,
		     hb_codepoint_t *first_glyph HB_UNUSED,
		     unsigned int glyph_stride HB_UNUSED,
		     void *user_data HB_UNUSED)
{
  return 0;
}

static void
test_fontfuncs_parallels (void)
{
  hb_blob_t *blob;
  hb_face_t *face;

  hb_font_funcs_t *ffuncs1;
  hb_font_funcs_t *ffuncs2;

  hb_font_t *font0;
  hb_font_t *font1;
  hb_font_t *font2;
  hb_codepoint_t glyph;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font0 = hb_font_create (face);
  hb_face_destroy (face);

  /* setup sub-font1 */
  font1 = hb_font_create_sub_font (font0);
  hb_font_destroy (font0);
  ffuncs1 = hb_font_funcs_create ();
  hb_font_funcs_set_nominal_glyph_func (ffuncs1, nominal_glyph_func, NULL, NULL);
  hb_font_set_funcs (font1, ffuncs1, NULL, NULL);
  hb_font_funcs_destroy (ffuncs1);

  /* setup sub-font2 */
  font2 = hb_font_create_sub_font (font1);
  hb_font_destroy (font1);
  ffuncs2 = hb_font_funcs_create ();
  hb_font_funcs_set_nominal_glyphs_func (ffuncs1, nominal_glyphs_func, NULL, NULL);
  hb_font_set_funcs (font2, ffuncs2, NULL, NULL);
  hb_font_funcs_destroy (ffuncs2);

  /* Just test that calling get_nominal_glyph doesn't infinite-loop. */
  hb_font_get_nominal_glyph (font2, 0x0020u, &glyph);

  hb_font_destroy (font2);
}

static void
test_font_empty (void)
{
  hb_font_t *created_from_empty;
  hb_font_t *created_from_null;
  hb_font_t *created_sub_from_null;

  g_assert_true (hb_font_get_empty ());

  created_from_empty = hb_font_create (hb_face_get_empty ());
  g_assert_true (hb_font_get_empty () != created_from_empty);

  created_from_null = hb_font_create (NULL);
  g_assert_true (hb_font_get_empty () != created_from_null);

  created_sub_from_null = hb_font_create_sub_font (NULL);
  g_assert_true (hb_font_get_empty () != created_sub_from_null);

  g_assert_true (hb_font_is_immutable (hb_font_get_empty ()));

  g_assert_true (hb_font_get_face (hb_font_get_empty ()) == hb_face_get_empty ());
  g_assert_true (hb_font_get_parent (hb_font_get_empty ()) == NULL);

  hb_font_destroy (created_sub_from_null);
  hb_font_destroy (created_from_null);
  hb_font_destroy (created_from_empty);
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
  unsigned int upem;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font = hb_font_create (face);
  hb_face_destroy (face);


  g_assert_true (hb_font_get_face (font) == face);
  g_assert_true (hb_font_get_parent (font) == hb_font_get_empty ());
  subfont = hb_font_create_sub_font (font);
  g_assert_true (hb_font_get_parent (subfont) == font);
  hb_font_set_parent(subfont, NULL);
  g_assert_true (hb_font_get_parent (subfont) == hb_font_get_empty());
  hb_font_set_parent(subfont, font);
  g_assert_true (hb_font_get_parent (subfont) == font);
  hb_font_set_parent(subfont, NULL);
  hb_font_make_immutable (subfont);
  g_assert_true (hb_font_get_parent (subfont) == hb_font_get_empty());
  hb_font_set_parent(subfont, font);
  g_assert_true (hb_font_get_parent (subfont) == hb_font_get_empty());
  hb_font_destroy (subfont);


  /* Check scale */

  upem = hb_face_get_upem (hb_font_get_face (font));
  hb_font_get_scale (font, NULL, NULL);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, &x_scale, NULL);
  g_assert_cmpint (x_scale, ==, upem);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, NULL, &y_scale);
  g_assert_cmpint (y_scale, ==, upem);
  x_scale = y_scale = 13;
  hb_font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, upem);
  g_assert_cmpint (y_scale, ==, upem);

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

  /* Check ptem */
  g_assert_cmpint (hb_font_get_ptem (font), ==, 0);
  hb_font_set_ptem (font, 42);
  g_assert_cmpint (hb_font_get_ptem (font), ==, 42);


  /* Check immutable */

  g_assert_true (!hb_font_is_immutable (font));
  hb_font_make_immutable (font);
  g_assert_true (hb_font_is_immutable (font));

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

  g_assert_true (hb_font_get_parent (subfont) == font);
  g_assert_true (hb_font_get_face (subfont) == face);

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

static hb_bool_t
_extreme_glyph_extents_func (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
			     hb_codepoint_t glyph HB_UNUSED,
			     hb_glyph_extents_t *extents,
			     void *user_data HB_UNUSED)
{
  /* Mimic extents that the glyf/sbix producers can legitimately emit after
   * clamping each field independently to the full hb_position_t range. */
  extents->x_bearing = G_MAXINT;
  extents->y_bearing = G_MAXINT;
  extents->width = 100;
  extents->height = 100;
  return TRUE;
}

static void
test_synthetic_glyph_extents_overflow (void)
{
  hb_font_funcs_t *ffuncs = hb_font_funcs_create ();
  hb_font_funcs_set_glyph_extents_func (ffuncs, _extreme_glyph_extents_func, NULL, NULL);

  hb_face_t *face = hb_face_create (hb_blob_get_empty (), 0);
  hb_font_t *font = hb_font_create (face);
  hb_font_set_funcs (font, ffuncs, NULL, NULL);
  hb_font_set_scale (font, 2048, 2048);

  /* Both synthetic transforms operate on the extreme extents above; without
   * saturating arithmetic the int32 sums/differences overflow (UBSan
   * signed-integer-overflow in synthetic_glyph_extents). */
  hb_font_set_synthetic_slant (font, 0.25f);
  hb_font_set_synthetic_bold (font, 0.05f, 0.05f, TRUE);

  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 1, &extents));

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_font_funcs_destroy (ffuncs);
}

#ifdef HAVE_FONTATIONS
typedef struct {
  hb_font_t *font;
  GMutex mutex;
  GCond cond;
  unsigned waiting;
  hb_bool_t start;
  char name[128];
} fontations_name_test_t;

static gpointer
fontations_name_thread (gpointer user_data)
{
  fontations_name_test_t *data = user_data;
  g_mutex_lock (&data->mutex);
  data->waiting++;
  g_cond_broadcast (&data->cond);
  while (!data->start)
    g_cond_wait (&data->cond, &data->mutex);
  g_mutex_unlock (&data->mutex);

  for (unsigned i = 0; i < 100; i++)
  {
    hb_codepoint_t glyph = HB_CODEPOINT_INVALID;
    g_assert_true (hb_font_get_glyph_from_name (data->font, data->name, -1, &glyph));
    g_assert_cmpuint (glyph, ==, 1);
    g_assert_false (hb_font_get_glyph_from_name (data->font, "nonexistent-glyph", -1, &glyph));
  }
  return NULL;
}

static void
test_fontations_glyph_from_name_threads (void)
{
  for (unsigned run = 0; run < 8; run++)
  {
    hb_face_t *face = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
    fontations_name_test_t data = {0};
    data.font = hb_font_create (face);
    hb_fontations_font_set_funcs (data.font);
    hb_font_make_immutable (data.font);
    g_assert_true (hb_font_get_glyph_name (data.font, 1, data.name, sizeof (data.name)));
    g_mutex_init (&data.mutex);
    g_cond_init (&data.cond);

    GThread *threads[8];
    for (unsigned i = 0; i < G_N_ELEMENTS (threads); i++)
      threads[i] = g_thread_new ("fontations-names", fontations_name_thread, &data);
    g_mutex_lock (&data.mutex);
    while (data.waiting != G_N_ELEMENTS (threads))
      g_cond_wait (&data.cond, &data.mutex);
    data.start = true;
    g_cond_broadcast (&data.cond);
    g_mutex_unlock (&data.mutex);
    for (unsigned i = 0; i < G_N_ELEMENTS (threads); i++)
      g_thread_join (threads[i]);

    g_cond_clear (&data.cond);
    g_mutex_clear (&data.mutex);
    hb_font_destroy (data.font);
    hb_face_destroy (face);
  }
}

static void
test_fontations_scale_changes (void)
{
  const char *fonts[] = {
    "fonts/Roboto-Variable.abc.ttf",
    "fonts/SourceSerifVariable-Roman-VVAR.abc.ttf",
    "fonts/Roboto-Regular.abc.ttf",
  };
  const struct {
    int values[3];
    unsigned count;
  } locations[] = {
    {{0, 0, 0}, 0}, {{0, 0, 0}, 3},
    {{9830, 0, 0}, 1}, {{9830, 0, 0}, 3},
    {{9830, 16384, 0}, 2}, {{9830, 16384, 16384}, 3},
    {{0, 0, 16384}, 3}, {{0, 0, 0}, 0},
  };
  const int scales[][2] = {{2048, 4096}, {-1234, -5678}, {0, 0}, {1234, 5678}};

  for (unsigned f = 0; f < G_N_ELEMENTS (fonts); f++)
  {
    hb_face_t *face = hb_test_open_font_file (fonts[f]);
    hb_font_t *font = hb_font_create (face);
    hb_font_t *reference = hb_font_create (face);
    hb_fontations_font_set_funcs (font);

    for (unsigned c = 0; c < G_N_ELEMENTS (locations); c++)
    {
      hb_font_set_var_coords_normalized (font, locations[c].values, locations[c].count);
      hb_font_set_var_coords_normalized (reference, locations[c].values, locations[c].count);
      for (unsigned s = 0; s < G_N_ELEMENTS (scales); s++)
      {
        hb_font_set_scale (font, scales[s][0], scales[s][1]);
        hb_font_set_scale (reference, scales[s][0], scales[s][1]);
        /* Compare the reused instance against a freshly initialized one. */
        hb_fontations_font_set_funcs (reference);
        for (hb_codepoint_t glyph = 0; glyph < hb_face_get_glyph_count (face); glyph++)
        {
          g_assert_cmpint (hb_font_get_glyph_h_advance (font, glyph), ==,
                           hb_font_get_glyph_h_advance (reference, glyph));
          g_assert_cmpint (hb_font_get_glyph_v_advance (font, glyph), ==,
                           hb_font_get_glyph_v_advance (reference, glyph));
        }
      }
    }

    hb_font_destroy (reference);
    hb_font_destroy (font);
    hb_face_destroy (face);
  }
}

static void
test_fontations_strides (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_fontations_font_set_funcs (font);
  const hb_codepoint_t unicodes[] = {'a', 'b', 'c'};
  hb_codepoint_t ids[3];
  uint8_t unicode_bytes[16], glyph_bytes[16], advance_bytes[16];
  memset (unicode_bytes, 0xA5, sizeof (unicode_bytes));
  memset (glyph_bytes, 0xA5, sizeof (glyph_bytes));
  for (unsigned i = 0; i < G_N_ELEMENTS (unicodes); i++)
  {
    memcpy (unicode_bytes + 1 + 5 * i, &unicodes[i], sizeof (unicodes[i]));
    g_assert_true (hb_font_get_nominal_glyph (font, unicodes[i], &ids[i]));
  }
  g_assert_cmpuint (hb_font_get_nominal_glyphs (font, 3,
                   (const hb_codepoint_t *) (unicode_bytes + 1), 5,
                   (hb_codepoint_t *) (glyph_bytes + 1), 5), ==, 3);
  for (unsigned i = 0; i < 3; i++)
  {
    hb_codepoint_t glyph;
    memcpy (&glyph, glyph_bytes + 1 + 5 * i, sizeof (glyph));
    g_assert_cmpuint (glyph, ==, ids[i]);
    g_assert_cmpuint (glyph_bytes[5 * i + 5], ==, 0xA5);
  }
  g_assert_cmpuint (glyph_bytes[0], ==, 0xA5);

  /* hb-ot-normalize maps codepoint to var1.u32 in the same glyph-info array. */
  hb_glyph_info_t infos[3] = {0};
  for (unsigned i = 0; i < G_N_ELEMENTS (infos); i++)
  {
    infos[i].codepoint = unicodes[i];
    infos[i].mask = 123456;
    infos[i].cluster = i;
    infos[i].var1.u32 = HB_CODEPOINT_INVALID;
    infos[i].var2.u32 = 123456;
  }
  g_assert_cmpuint (hb_font_get_nominal_glyphs (font, G_N_ELEMENTS (infos),
                   &infos[0].codepoint, sizeof (infos[0]),
                   &infos[0].var1.u32, sizeof (infos[0])), ==, G_N_ELEMENTS (infos));
  for (unsigned i = 0; i < G_N_ELEMENTS (infos); i++)
  {
    g_assert_cmpuint (infos[i].codepoint, ==, unicodes[i]);
    g_assert_cmpuint (infos[i].var1.u32, ==, ids[i]);
    g_assert_cmpuint (infos[i].mask, ==, 123456);
    g_assert_cmpuint (infos[i].cluster, ==, i);
    g_assert_cmpuint (infos[i].var2.u32, ==, 123456);
  }

  void (*get_advances[])(hb_font_t *, unsigned, const hb_codepoint_t *, unsigned,
                        hb_position_t *, unsigned) = {
    hb_font_get_glyph_h_advances, hb_font_get_glyph_v_advances,
  };
  for (unsigned axis = 0; axis < G_N_ELEMENTS (get_advances); axis++)
  {
    hb_position_t expected[3];
    get_advances[axis] (font, 3, ids, sizeof (ids[0]), expected, sizeof (expected[0]));
    memset (advance_bytes, 0xA5, sizeof (advance_bytes));
    get_advances[axis] (font, 3, (const hb_codepoint_t *) (glyph_bytes + 1), 5,
                       (hb_position_t *) (advance_bytes + 1), 5);
    for (unsigned i = 0; i < 3; i++)
    {
      hb_position_t advance;
      memcpy (&advance, advance_bytes + 1 + 5 * i, sizeof (advance));
      g_assert_cmpint (advance, ==, expected[i]);
      g_assert_cmpuint (advance_bytes[5 * i + 5], ==, 0xA5);
    }
    g_assert_cmpuint (advance_bytes[0], ==, 0xA5);

    hb_position_t last = 0;
    get_advances[axis] (font, 3, ids, sizeof (ids[0]), &last, 0);
    g_assert_cmpint (last, ==, expected[2]);

    hb_codepoint_t in_place[3];
    memcpy (in_place, ids, sizeof (ids));
    get_advances[axis] (font, 3, in_place, sizeof (in_place[0]),
                       (hb_position_t *) in_place, sizeof (in_place[0]));
    for (unsigned i = 0; i < 3; i++)
      g_assert_cmpint ((hb_position_t) in_place[i], ==, expected[i]);
  }

  struct {
    hb_codepoint_t glyph;
    hb_position_t h_advance, v_advance, x, y, padding;
  } records[3];
  for (unsigned i = 0; i < G_N_ELEMENTS (records); i++)
  {
    records[i].glyph = ids[i];
    records[i].h_advance = records[i].v_advance = records[i].x = records[i].y = 0;
    records[i].padding = 123456;
  }
  hb_font_get_glyph_h_advances (font, G_N_ELEMENTS (records),
                               &records[0].glyph, sizeof (records[0]),
                               &records[0].h_advance, sizeof (records[0]));
  hb_font_get_glyph_v_advances (font, G_N_ELEMENTS (records),
                               &records[0].glyph, sizeof (records[0]),
                               &records[0].v_advance, sizeof (records[0]));
  g_assert_true (hb_font_get_glyph_v_origins (font, G_N_ELEMENTS (records),
                 &records[0].glyph, sizeof (records[0]),
                 &records[0].x, sizeof (records[0]),
                 &records[0].y, sizeof (records[0])));
  for (unsigned i = 0; i < G_N_ELEMENTS (records); i++)
  {
    hb_position_t x, y;
    g_assert_cmpuint (records[i].glyph, ==, ids[i]);
    g_assert_cmpint (records[i].h_advance, ==, hb_font_get_glyph_h_advance (font, ids[i]));
    g_assert_cmpint (records[i].v_advance, ==, hb_font_get_glyph_v_advance (font, ids[i]));
    g_assert_true (hb_font_get_glyph_v_origin (font, ids[i], &x, &y));
    g_assert_cmpint (records[i].x, ==, x);
    g_assert_cmpint (records[i].y, ==, y);
    g_assert_cmpint (records[i].padding, ==, 123456);
  }
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_fontations_v_origins (void)
{
  const char *fonts[] = {
    "fonts/Roboto-Regular.abc.ttf",
    "fonts/Roboto-Variable.abc.ttf",
    "fonts/SourceSerifVariable-Roman-VVAR.abc.ttf",
    "fonts/AdobeVFPrototype.abc.otf",
    "../shape/data/in-house/fonts/NotoSansCJK-VF.abc.ttf",
    "../shape/data/in-house/fonts/NotoSansCJK-VF.abc.otf",
  };
  const int coords[] = {0, 16384, -16384, 0};
  const hb_codepoint_t glyphs[] = {0, 1, 2, 3, HB_CODEPOINT_INVALID};
  for (unsigned f = 0; f < G_N_ELEMENTS (fonts); f++)
  {
    hb_face_t *face = hb_test_open_font_file (fonts[f]);
    hb_font_t *font = hb_font_create (face);
    hb_fontations_font_set_funcs (font);
    int upem = hb_face_get_upem (face);
    for (int sign = 1; sign >= -1; sign -= 2)
    {
      hb_font_set_scale (font, sign * upem * 2, sign * upem * 3);
      for (unsigned c = 0; c < G_N_ELEMENTS (coords); c++)
      {
        hb_font_set_var_coords_normalized (font, &coords[c], 1);
        hb_position_t expected_x[G_N_ELEMENTS (glyphs)], expected_y[G_N_ELEMENTS (glyphs)];
        for (unsigned i = 0; i < G_N_ELEMENTS (glyphs); i++)
          g_assert_true (hb_font_get_glyph_v_origin (font, glyphs[i], &expected_x[i], &expected_y[i]));

        hb_position_t origins[4 * G_N_ELEMENTS (glyphs)];
        for (unsigned i = 0; i < G_N_ELEMENTS (origins); i++)
          origins[i] = 123456;
        g_assert_true (hb_font_get_glyph_v_origins (font, G_N_ELEMENTS (glyphs),
                       glyphs, sizeof (glyphs[0]),
                       origins, 4 * sizeof (origins[0]),
                       origins + 1, 4 * sizeof (origins[0])));
        for (unsigned i = 0; i < G_N_ELEMENTS (glyphs); i++)
        {
          g_assert_cmpint (origins[4 * i], ==, expected_x[i]);
          g_assert_cmpint (origins[4 * i + 1], ==, expected_y[i]);
          g_assert_cmpint (origins[4 * i + 2], ==, 123456);
          g_assert_cmpint (origins[4 * i + 3], ==, 123456);
        }

        hb_position_t last_x = 0, last_y = 0;
        g_assert_true (hb_font_get_glyph_v_origins (font, G_N_ELEMENTS (glyphs),
                       glyphs, sizeof (glyphs[0]), &last_x, 0, &last_y, 0));
        g_assert_cmpint (last_x, ==, expected_x[G_N_ELEMENTS (glyphs) - 1]);
        g_assert_cmpint (last_y, ==, expected_y[G_N_ELEMENTS (glyphs) - 1]);

        hb_codepoint_t in_place[G_N_ELEMENTS (glyphs)];
        memcpy (in_place, glyphs, sizeof (glyphs));
        g_assert_true (hb_font_get_glyph_v_origins (font, G_N_ELEMENTS (glyphs),
                       in_place, sizeof (in_place[0]),
                       (hb_position_t *) in_place, sizeof (in_place[0]),
                       origins, sizeof (origins[0])));
        for (unsigned i = 0; i < G_N_ELEMENTS (glyphs); i++)
        {
          g_assert_cmpint ((hb_position_t) in_place[i], ==, expected_x[i]);
          g_assert_cmpint (origins[i], ==, expected_y[i]);
        }

        uint8_t x_bytes[1 + 5 * G_N_ELEMENTS (glyphs)], y_bytes[sizeof (x_bytes)];
        memset (x_bytes, 0xA5, sizeof (x_bytes));
        memset (y_bytes, 0xA5, sizeof (y_bytes));
        g_assert_true (hb_font_get_glyph_v_origins (font, G_N_ELEMENTS (glyphs),
                       glyphs, sizeof (glyphs[0]),
                       (hb_position_t *) (x_bytes + 1), 5,
                       (hb_position_t *) (y_bytes + 1), 5));
        for (unsigned i = 0; i < G_N_ELEMENTS (glyphs); i++)
        {
          hb_position_t x, y;
          memcpy (&x, x_bytes + 1 + 5 * i, sizeof (x));
          memcpy (&y, y_bytes + 1 + 5 * i, sizeof (y));
          g_assert_cmpint (x, ==, expected_x[i]);
          g_assert_cmpint (y, ==, expected_y[i]);
          g_assert_cmpuint (x_bytes[5 * i + 5], ==, 0xA5);
          g_assert_cmpuint (y_bytes[5 * i + 5], ==, 0xA5);
        }
        g_assert_true (hb_font_get_glyph_v_origins (font, 0, NULL, 0, NULL, 0, NULL, 0));
      }
    }
    hb_font_destroy (font);
    hb_face_destroy (face);
  }
}

static void
test_fontations_advances (void)
{
  const char *fonts[] = {
    "fonts/Roboto-Variable.abc.ttf",
    "fonts/SourceSerifVariable-Roman-VVAR.abc.ttf",
    "fonts/SourceSansVariable-Roman-nohvar-41,C1.ttf",
  };
  const int coords[] = {0, 16384, -16384, 0};
  const struct {
    hb_codepoint_t glyph;
    hb_codepoint_t padding[2];
  } glyphs[] = {{1, {0}}, {2, {0}}, {3, {0}}, {HB_CODEPOINT_INVALID, {0}}};

  for (unsigned f = 0; f < G_N_ELEMENTS (fonts); f++)
  {
    hb_face_t *face = hb_test_open_font_file (fonts[f]);
    hb_font_t *font = hb_font_create (face);
    hb_font_t *reference = hb_font_create (face);
    int upem = hb_face_get_upem (face);
    hb_fontations_font_set_funcs (font);

    for (int sign = 1; sign >= -1; sign -= 2)
    {
      hb_font_set_scale (font, sign * upem * 2, sign * upem * 3);
      hb_font_set_scale (reference, sign * upem * 2, sign * upem * 3);

      for (unsigned c = 0; c < G_N_ELEMENTS (coords); c++)
      {
        hb_font_set_var_coords_normalized (font, &coords[c], 1);
        hb_font_set_var_coords_normalized (reference, &coords[c], 1);

        hb_position_t h_advances[2 * G_N_ELEMENTS (glyphs)];
        hb_position_t v_advances[2 * G_N_ELEMENTS (glyphs)];
        for (unsigned i = 0; i < G_N_ELEMENTS (h_advances); i++)
          h_advances[i] = v_advances[i] = 123456;

        hb_font_get_glyph_h_advances (font, G_N_ELEMENTS (glyphs),
                                     &glyphs[0].glyph, sizeof (glyphs[0]),
                                     h_advances, 2 * sizeof (h_advances[0]));
        hb_font_get_glyph_v_advances (font, G_N_ELEMENTS (glyphs),
                                     &glyphs[0].glyph, sizeof (glyphs[0]),
                                     v_advances, 2 * sizeof (v_advances[0]));

        for (unsigned i = 0; i < G_N_ELEMENTS (glyphs); i++)
        {
          hb_codepoint_t glyph = glyphs[i].glyph;
          g_assert_cmpint (h_advances[2 * i], ==, hb_font_get_glyph_h_advance (font, glyph));
          g_assert_cmpint (v_advances[2 * i], ==, hb_font_get_glyph_v_advance (font, glyph));
          /* OT can apply variation deltas even to invalid glyph IDs. */
          if (glyph < hb_face_get_glyph_count (face))
          {
            g_assert_cmpint (h_advances[2 * i], ==, hb_font_get_glyph_h_advance (reference, glyph));
            g_assert_cmpint (v_advances[2 * i], ==, hb_font_get_glyph_v_advance (reference, glyph));
          }
          g_assert_cmpint (h_advances[2 * i + 1], ==, 123456);
          g_assert_cmpint (v_advances[2 * i + 1], ==, 123456);
        }

        hb_font_get_glyph_h_advances (font, 0, NULL, 0, NULL, 0);
        hb_font_get_glyph_v_advances (font, 0, NULL, 0, NULL, 0);
      }
    }

    hb_font_destroy (reference);
    hb_font_destroy (font);
    hb_face_destroy (face);
  }
}
#endif

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_face_empty);
  hb_test_add (test_face_create);
  hb_test_add (test_face_createfortables);
  hb_test_add (test_face_referenceblob);

  hb_test_add (test_fontfuncs_empty);
  hb_test_add (test_fontfuncs_nil);
  hb_test_add (test_fontfuncs_subclassing);
  hb_test_add (test_fontfuncs_parallels);

  hb_test_add (test_font_empty);
  hb_test_add (test_font_properties);
  hb_test_add (test_synthetic_glyph_extents_overflow);
#ifdef HAVE_FONTATIONS
  hb_test_add (test_fontations_glyph_from_name_threads);
  hb_test_add (test_fontations_scale_changes);
  hb_test_add (test_fontations_strides);
  hb_test_add (test_fontations_v_origins);
  hb_test_add (test_fontations_advances);
#endif

  return hb_test_run();
}
