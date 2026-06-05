/*
 * Copyright © 2022  Behdad Esfahbod
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

#include <hb.h>

static void
test_maxp_and_loca (void)
{
  hb_face_t *face;

  const char maxp_data[] = "\x00\x00\x50\x00" // version
			   "\x00\x05" // numGlyphs
			   ;
#if 0
  /* These checks belonged to an old beyond-64k proposal that has since
   * changed. The library code needs to adapt before testing this behavior.
   */
  const char loca_data[18] = "";
#endif

  face = hb_face_builder_create ();
  g_assert_cmpuint (hb_face_get_glyph_count (face), ==, 0);
  hb_face_destroy (face);

  face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "maxp", maxp_data);
  g_assert_cmpuint (hb_face_get_glyph_count (face), ==, 5);
  hb_face_destroy (face);

#if 0
  /* These checks belonged to an old beyond-64k proposal that has since
   * changed. The library code needs to adapt before testing this behavior.
   */
  face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "maxp", maxp_data);
  HB_FACE_ADD_TABLE (face, "loca", loca_data);
  g_assert_cmpuint (hb_face_get_glyph_count (face), ==, 8);
  hb_face_destroy (face);

  face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "loca", loca_data);
  g_assert_cmpuint (hb_face_get_glyph_count (face), ==, 8);
  hb_face_destroy (face);
#endif
}

#ifndef HB_NO_BEYOND_64K
static hb_face_t *
create_glyf_face (hb_bool_t add_GLYF, hb_bool_t add_LOCA)
{
  static const char maxp_data[] =
    "\x00\x00\x50\x00" /* version */
    "\x00\x01" /* numGlyphs */
    ;
  static const char head_data[54] =
    "\x00\x01\x00\x00" /* version */
    "\x00\x00\x00\x00" /* fontRevision */
    "\x00\x00\x00\x00" /* checkSumAdjustment */
    "\x5F\x0F\x3C\xF5" /* magicNumber */
    "\x00\x00" /* flags */
    "\x03\xE8" /* unitsPerEm */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* created */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* modified */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* bounds */
    "\x00\x00" /* macStyle */
    "\x00\x00" /* lowestRecPPEM */
    "\x00\x00" /* fontDirectionHint */
    "\x00\x01" /* indexToLocFormat */
    "\x00\x00" /* glyphDataFormat */
    ;
  static const char loca_data[8] =
    "\x00\x00\x00\x00"
    "\x00\x00\x00\x15"
    ;
  static const char glyf_data[21] =
    "\x00\x01" /* numberOfContours */
    "\x00\x01\x00\x02\x00\x04\x00\x06" /* bounds */
    "\x00\x02" /* endPtsOfContours */
    "\x00\x00" /* instructionLength */
    "\x31\xB6\x96" /* flags */
    "\x0A\x0A" /* xCoordinates */
    "\x0A\x0A" /* yCoordinates */
    ;
  static const char GLYF_data[21] =
    "\x00\x01" /* numberOfContours */
    "\x00\x0B\x00\x0C\x00\x18\x00\x1A" /* bounds */
    "\x00\x02" /* endPtsOfContours */
    "\x00\x00" /* instructionLength */
    "\x31\xB6\x96" /* flags */
    "\x0A\x0A" /* xCoordinates */
    "\x0A\x0A" /* yCoordinates */
    ;

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "maxp", maxp_data);
  HB_FACE_ADD_TABLE (face, "head", head_data);
  HB_FACE_ADD_TABLE (face, "glyf", glyf_data);
  HB_FACE_ADD_TABLE (face, "loca", loca_data);
  if (add_GLYF)
    HB_FACE_ADD_TABLE (face, "GLYF", GLYF_data);
  if (add_LOCA)
    HB_FACE_ADD_TABLE (face, "LOCA", loca_data);
  return face;
}

static void
cubic_to (hb_draw_funcs_t *draw_funcs HB_UNUSED,
	  void *draw_data,
	  hb_draw_state_t *draw_state HB_UNUSED,
	  float control1_x HB_UNUSED, float control1_y HB_UNUSED,
	  float control2_x HB_UNUSED, float control2_y HB_UNUSED,
	  float to_x HB_UNUSED, float to_y HB_UNUSED,
	  void *user_data HB_UNUSED)
{
  (*(unsigned int *) draw_data)++;
}

static void
quadratic_to (hb_draw_funcs_t *draw_funcs HB_UNUSED,
	      void *draw_data HB_UNUSED,
	      hb_draw_state_t *draw_state HB_UNUSED,
	      float control_x HB_UNUSED, float control_y HB_UNUSED,
	      float to_x HB_UNUSED, float to_y HB_UNUSED,
	  void *user_data HB_UNUSED)
{}

static void
move_to (hb_draw_funcs_t *draw_funcs HB_UNUSED,
	 void *draw_data,
	 hb_draw_state_t *draw_state HB_UNUSED,
	 float to_x HB_UNUSED, float to_y HB_UNUSED,
	 void *user_data HB_UNUSED)
{
  (*(unsigned int *) draw_data)++;
}

static void
test_GLYF_and_LOCA (void)
{
  hb_glyph_extents_t extents;
  hb_draw_funcs_t *draw_funcs = hb_draw_funcs_create ();
  hb_face_t *face;
  hb_font_t *font;
  unsigned int cubic_count;

  hb_draw_funcs_set_cubic_to_func (draw_funcs, cubic_to, NULL, NULL);
  hb_draw_funcs_set_quadratic_to_func (draw_funcs, quadratic_to, NULL, NULL);

  face = create_glyf_face (TRUE, TRUE);
  g_assert_cmpuint (hb_face_get_glyph_count (face), ==, 1);
  font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (hb_font_get_glyph_extents (font, 0, &extents));
  g_assert_cmpint (extents.width, ==, 13);
  cubic_count = 0;
  hb_font_draw_glyph (font, 0, draw_funcs, &cubic_count);
  g_assert_cmpuint (cubic_count, ==, 1);
  hb_font_destroy (font);

  face = create_glyf_face (TRUE, FALSE);
  font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (hb_font_get_glyph_extents (font, 0, &extents));
  g_assert_cmpint (extents.width, ==, 3);
  cubic_count = 0;
  hb_font_draw_glyph (font, 0, draw_funcs, &cubic_count);
  g_assert_cmpuint (cubic_count, ==, 0);
  hb_font_destroy (font);

  face = create_glyf_face (FALSE, TRUE);
  font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (hb_font_get_glyph_extents (font, 0, &extents));
  g_assert_cmpint (extents.width, ==, 3);
  hb_font_destroy (font);

  hb_draw_funcs_destroy (draw_funcs);
}

static hb_face_t *
create_composite_face (hb_bool_t extended)
{
  static const char maxp_data[] =
    "\x00\x00\x50\x00" /* version */
    "\x00\x02" /* numGlyphs */
    ;
  static const char head_data[54] =
    "\x00\x01\x00\x00" /* version */
    "\x00\x00\x00\x00" /* fontRevision */
    "\x00\x00\x00\x00" /* checkSumAdjustment */
    "\x5F\x0F\x3C\xF5" /* magicNumber */
    "\x00\x00" /* flags */
    "\x03\xE8" /* unitsPerEm */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* created */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* modified */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* bounds */
    "\x00\x00" /* macStyle */
    "\x00\x00" /* lowestRecPPEM */
    "\x00\x00" /* fontDirectionHint */
    "\x00\x01" /* indexToLocFormat */
    "\x00\x00" /* glyphDataFormat */
    ;
  static const char loca_data[12] =
    "\x00\x00\x00\x00"
    "\x00\x00\x00\x11"
    "\x00\x00\x00\x26"
    ;
  static const char glyf_data[38] =
    "\xFF\xFF" /* numberOfContours */
    "\x00\x00\x00\x00\x00\x14\x00\x14" /* bounds */
    "\x20\x02" /* GID_IS_24BIT | ARGS_ARE_XY_VALUES */
    "\x00\x00\x01" /* glyphIndex */
    "\x00\x00" /* arguments */
    "\x00\x01" /* numberOfContours */
    "\x00\x00\x00\x00\x00\x14\x00\x14" /* bounds */
    "\x00\x02" /* endPtsOfContours */
    "\x00\x00" /* instructionLength */
    "\x31\xB6\x96" /* flags */
    "\x0A\x0A" /* xCoordinates */
    "\x0A\x0A" /* yCoordinates */
    ;

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "maxp", maxp_data);
  HB_FACE_ADD_TABLE (face, "head", head_data);
  HB_FACE_ADD_TABLE (face, "glyf", glyf_data);
  HB_FACE_ADD_TABLE (face, "loca", loca_data);
  if (extended)
  {
    HB_FACE_ADD_TABLE (face, "GLYF", glyf_data);
    HB_FACE_ADD_TABLE (face, "LOCA", loca_data);
  }
  return face;
}

static void
test_GLYF_composite_gid (void)
{
  hb_draw_funcs_t *draw_funcs = hb_draw_funcs_create ();
  unsigned int move_count;

  hb_draw_funcs_set_move_to_func (draw_funcs, move_to, NULL, NULL);

  hb_face_t *face = create_composite_face (TRUE);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  move_count = 0;
  hb_font_draw_glyph (font, 0, draw_funcs, &move_count);
  g_assert_cmpuint (move_count, >, 0);
  hb_font_destroy (font);

  face = create_composite_face (FALSE);
  font = hb_font_create (face);
  hb_face_destroy (face);
  move_count = 0;
  hb_font_draw_glyph (font, 0, draw_funcs, &move_count);
  g_assert_cmpuint (move_count, ==, 0);
  hb_font_destroy (font);

  hb_draw_funcs_destroy (draw_funcs);
}
#endif


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_maxp_and_loca);
#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_GLYF_and_LOCA);
  hb_test_add (test_GLYF_composite_gid);
#endif

  return hb_test_run();
}
