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
 *
 */

#include "hb-test.h"

#include <hb-ot.h>

static void
test_ot_layout_feature_get_name_ids_and_characters (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/lcar.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_font_set_scale (font, hb_face_get_upem (face) * 2, hb_face_get_upem (face) * 4);

  hb_position_t caret_array[2];
  unsigned int caret_count = 2;
  g_assert_cmpuint (2, ==, hb_ot_layout_get_ligature_carets (font, HB_DIRECTION_RTL,
							     98, 0, &caret_count,
							     caret_array));

  g_assert_cmpuint (2, ==, caret_count);
  g_assert_cmpuint (1130, ==, caret_array[0]);
  g_assert_cmpuint (2344, ==, caret_array[1]);

  g_assert_cmpuint (2, ==, hb_ot_layout_get_ligature_carets (font, HB_DIRECTION_BTT,
							     98, 0, &caret_count,
							     caret_array));

  g_assert_cmpuint (2, ==, caret_count);
  g_assert_cmpuint (2260, ==, caret_array[0]);
  g_assert_cmpuint (4688, ==, caret_array[1]);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  hb_test_add (test_ot_layout_feature_get_name_ids_and_characters);

  return hb_test_run ();
}
