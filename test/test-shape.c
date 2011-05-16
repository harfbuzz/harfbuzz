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


static const char test_data[] = "test\0data";

static void
glyph_advance_func (hb_font_t *font, void *font_data,
		    hb_codepoint_t glyph,
		    hb_position_t *x_advance, hb_position_t *y_advance,
		    void *user_data)
{
  switch (glyph) {
  case 1: *x_advance = 10; return;
  case 2: *x_advance =  6; return;
  case 3: *x_advance =  5; return;
  }
}

static hb_bool_t
glyph_func (hb_font_t *font, void *font_data,
	    hb_codepoint_t unicode, hb_codepoint_t variant_selector,
	    hb_codepoint_t *glyph,
	    void *user_data)
{
  switch (unicode) {
  case 'T': *glyph = 1; return TRUE;
  case 'e': *glyph = 2; return TRUE;
  case 's': *glyph = 3; return TRUE;
  }

  return FALSE;
}

static void
kerning_func (hb_font_t *font, void *font_data,
		    hb_codepoint_t left, hb_codepoint_t right,
		    hb_position_t *x_kern, hb_position_t *y_kern,
		    void *user_data)
{
  if (left == 1 && right == 2) {
    *x_kern = -2;
  }
}

static const char TesT[] = "TesT";

static void
test_shape (void)
{
  hb_blob_t *blob;
  hb_face_t *face;
  hb_font_funcs_t *ffuncs;
  hb_font_t *font;
  hb_buffer_t *buffer;
  unsigned int len;
  hb_glyph_info_t *glyphs;
  hb_glyph_position_t *positions;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font = hb_font_create (face);
  hb_face_destroy (face);
  hb_font_set_scale (font, 10, 10);

  ffuncs = hb_font_funcs_create ();
  hb_font_funcs_set_glyph_advance_func (ffuncs, glyph_advance_func, NULL, NULL);
  hb_font_funcs_set_glyph_func (ffuncs, glyph_func, NULL, NULL);
  hb_font_funcs_set_kerning_func (ffuncs, kerning_func, NULL, NULL);
  hb_font_set_funcs (font, ffuncs, NULL, NULL);
  hb_font_funcs_destroy (ffuncs);

  buffer =  hb_buffer_create (0);
  hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
  hb_buffer_add_utf8 (buffer, TesT, 4, 0, 4);

  hb_shape (font, buffer, NULL, 0);

  len = hb_buffer_get_length (buffer);
  glyphs = hb_buffer_get_glyph_infos (buffer, NULL);
  positions = hb_buffer_get_glyph_positions (buffer, NULL);

  {
    const hb_codepoint_t output_glyphs[] = {1, 2, 3, 1};
    const hb_position_t output_x_advances[] = {9, 5, 5, 10};
    const hb_position_t output_x_offsets[] = {0, -1, 0, 0};
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
  hb_font_destroy (font);
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_shape);

  return hb_test_run();
}
