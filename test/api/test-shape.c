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

/* Unit tests for hb-shape.h */

/*
 * This test provides a framework to test aspects of hb_shape() that are
 * font-independent.  Please add tests for any feature that fits that
 * description.
 */

/* TODO Make this test data-driven and add some real test data */
/* TODO Test positions too. And test non-native direction.  Test commit 2e18c6dbdfb */


static const char test_data[] = "test\0data";

static hb_position_t
glyph_h_advance_func (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
		      hb_codepoint_t glyph,
		      void *user_data HB_UNUSED)
{
  switch (glyph) {
  case 1: return 10;
  case 2: return 6;
  case 3: return 5;
  }
  return 0;
}

static hb_bool_t
glyph_func (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
	    hb_codepoint_t unicode,
	    hb_codepoint_t *glyph,
	    void *user_data HB_UNUSED)
{
  switch (unicode) {
  case 'T': *glyph = 1; return TRUE;
  case 'e': *glyph = 2; return TRUE;
  case 's': *glyph = 3; return TRUE;
  }
  return FALSE;
}

static const char TesT[] = "TesT";

static void
test_font (hb_font_t *font)
{
  hb_buffer_t *buffer;
  unsigned int len;
  hb_glyph_info_t *glyphs;
  hb_glyph_position_t *positions;

  buffer =  hb_buffer_create ();
  hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
  hb_buffer_add_utf8 (buffer, TesT, 4, 0, 4);

  hb_shape (font, buffer, NULL, 0);

  len = hb_buffer_get_length (buffer);
  glyphs = hb_buffer_get_glyph_infos (buffer, NULL);
  positions = hb_buffer_get_glyph_positions (buffer, NULL);

  {
    const hb_codepoint_t output_glyphs[] = {1, 2, 3, 1};
    const hb_position_t output_x_advances[] = {10, 6, 5, 10};
    const hb_position_t output_x_offsets[] = {0, 0, 0, 0};
    unsigned int i;
    g_assert_cmpint (len, ==, 4);
    for (i = 0; i < len; i++) {
      g_assert_cmphex (glyphs[i].codepoint, ==, output_glyphs[i]);
      g_assert_cmphex (glyphs[i].cluster,   ==, i);
    }
    for (i = 0; i < len; i++) {
      g_assert_cmpint (output_x_advances[i], ==, positions[i].x_advance);
      g_assert_cmpint (output_x_offsets [i], ==, positions[i].x_offset);
      g_assert_cmpint (0, ==, positions[i].y_advance);
      g_assert_cmpint (0, ==, positions[i].y_offset);
    }
  }

  hb_buffer_destroy (buffer);
}

static void
test_shape (void)
{
  hb_blob_t *blob;
  hb_face_t *face;
  hb_font_funcs_t *ffuncs;
  hb_font_t *font, *sub_font;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font = hb_font_create (face);
  hb_face_destroy (face);
  hb_font_set_scale (font, 10, 10);

  ffuncs = hb_font_funcs_create ();
  hb_font_funcs_set_glyph_h_advance_func (ffuncs, glyph_h_advance_func, NULL, NULL);
  hb_font_funcs_set_nominal_glyph_func (ffuncs, glyph_func, malloc (10), free);
  hb_font_set_funcs (font, ffuncs, NULL, NULL);
  hb_font_funcs_destroy (ffuncs);

  test_font (font);

  sub_font = hb_font_create_sub_font (font);
  test_font (sub_font);

  hb_font_destroy (sub_font);
  hb_font_destroy (font);
}

static void
test_shape_clusters (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_buffer_t *buffer;
  unsigned int len;
  hb_glyph_info_t *glyphs;

  face = hb_face_create (NULL, 0);
  font = hb_font_create (face);
  hb_face_destroy (face);

  buffer =  hb_buffer_create ();
  hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
  {
    /* https://crbug.com/497578 */
    hb_codepoint_t test[] = {0xFFF1, 0xF0B6};
    hb_buffer_add_utf32 (buffer, test, 2, 0, 2);
  }

  hb_shape (font, buffer, NULL, 0);

  len = hb_buffer_get_length (buffer);
  glyphs = hb_buffer_get_glyph_infos (buffer, NULL);

  {
    const hb_codepoint_t output_glyphs[] = {0};
    const hb_position_t output_clusters[] = {0};
    unsigned int i;
    g_assert_cmpint (len, ==, 1);
    for (i = 0; i < len; i++) {
      g_assert_cmphex (glyphs[i].codepoint, ==, output_glyphs[i]);
      g_assert_cmphex (glyphs[i].cluster,   ==, output_clusters[i]);
    }
  }

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
}

static void
test_shape_overlong_mark_cluster (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_buffer_t *buffer = hb_buffer_create ();

  hb_font_set_scale (font, 12 * 65536, 12 * 65536);
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  hb_buffer_add (buffer, 'e', 0);
  for (unsigned i = 0; i < 35000; i++)
    hb_buffer_add (buffer, 0x0301, i + 1);
  hb_buffer_add (buffer, 'X', 35001);
  hb_buffer_guess_segment_properties (buffer);

  hb_shape (font, buffer, NULL, 0);

  unsigned len;
  hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (buffer, &len);
  g_assert_cmpuint (len, ==, 35001);
  g_assert_cmpint (positions[len - 2].y_offset, ==, INT32_MAX);

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
}



#ifndef HB_NO_AAT
static hb_bool_t
count_contour_point_calls (hb_font_t      *font HB_UNUSED,
			   void           *font_data,
			   hb_codepoint_t  glyph HB_UNUSED,
			   unsigned int    point_index HB_UNUSED,
			   hb_position_t  *x,
			   hb_position_t  *y,
			   void           *user_data HB_UNUSED)
{
  (*(unsigned *) font_data)++;
  *x = *y = 0;
  return TRUE;
}

static void
test_shape_kerx_control_point_actions_are_charged (void)
{
  /* A two-glyph font whose kerx format-4 subtable loops on the letter 'a'
   * in state 1 without advancing, performing a control-point action on
   * every transition.  Each action asks the font for two contour points,
   * which may load a whole glyph outline, so shaping must charge them to
   * the buffer's work budget. */
  static const char maxp[] = {0x00, 0x00, 0x50, 0x00,  0x00, 0x02};
  static const char cmap[] = {0, 0,  0, 1,  0, 3,  0, 10,  0, 0, 0, 12,
			      0, 12,  0, 0,  0, 0, 0, 28,  0, 0, 0, 0,  0, 0, 0, 1,
			      0, 0, 0, 'a',  0, 0, 0, 'a',  0, 0, 0, 1};
  static const unsigned char kerx[] = {
    0, 2, 0, 0,  0, 0, 0, 1,			/* version 2, one subtable */
    0, 0, 0, 82,  0, 0, 0, 4,  0, 0, 0, 0,	/* length, format 4, tupleCount */
    0, 0, 0, 5,  0, 0, 0, 20,  0, 0, 0, 28,  0, 0, 0, 48, /* nClasses, class, state, entry offsets */
    0, 0, 0, 66,				/* action type 0, control points at 66 */
    0, 8,  0, 1,  0, 1,  0, 4,			/* class lookup format 8: glyph 1 is class 4 */
    0, 0,  0, 0,  0, 0,  0, 0,  0, 1,		/* state 0: class 4 takes entry 1 */
    0, 0,  0, 0,  0, 0,  0, 0,  0, 2,		/* state 1: class 4 takes entry 2 */
    0, 0,  0x00, 0,  0xFF, 0xFF,		/* entry 0: to state 0 */
    0, 1,  0xC0, 0,  0xFF, 0xFF,		/* entry 1: to state 1, Mark, DontAdvance */
    0, 1,  0x40, 0,  0, 0,			/* entry 2: to state 1, DontAdvance, action 0 */
    0, 0,  0, 0					/* action 0: mark point 0, current point 0 */
  };
  static const char *shapers[] = {"ot", NULL};

  hb_face_t *builder = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (maxp, sizeof (maxp), HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (builder, HB_TAG ('m','a','x','p'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create (cmap, sizeof (cmap), HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (builder, HB_TAG ('c','m','a','p'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create ((const char *) kerx, sizeof (kerx), HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (builder, HB_TAG ('k','e','r','x'), blob);
  hb_blob_destroy (blob);
  blob = hb_face_reference_blob (builder);
  hb_face_destroy (builder);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);

  unsigned calls = 0;
  hb_font_t *parent = hb_font_create (face);
  hb_font_t *font = hb_font_create_sub_font (parent);
  hb_font_funcs_t *funcs = hb_font_funcs_create ();
  hb_font_funcs_set_glyph_contour_point_func (funcs, count_contour_point_calls, NULL, NULL);
  hb_font_set_funcs (font, funcs, &calls, NULL);
  hb_font_funcs_destroy (funcs);

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, "aa", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape_full (font, buffer, NULL, 0, shapers);

  /* 131074 before the budget was charged. */
  g_assert_cmpuint (calls, >=, 2);
  g_assert_cmpuint (calls, <=, 256);

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_font_destroy (parent);
  hb_face_destroy (face);
}
#endif

static void
test_shape_list (void)
{
  const char **shapers = hb_shape_list_shapers ();

  unsigned int i;
  for (i = 0; shapers[i]; i++)
    ;

  g_assert_cmpint (i, >, 1);
  g_assert_true (!strcmp (shapers[i - 1], "fallback"));
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_shape);
  hb_test_add (test_shape_clusters);
  hb_test_add (test_shape_overlong_mark_cluster);
#ifndef HB_NO_AAT
  hb_test_add (test_shape_kerx_control_point_actions_are_charged);
#endif
  /* TODO test fallback shaper */
  /* TODO test shaper_full */
  hb_test_add (test_shape_list);

  return hb_test_run();
}
