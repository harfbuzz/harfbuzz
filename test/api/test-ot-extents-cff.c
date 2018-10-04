/*
 * Copyright © 2018 Adobe Systems Incorporated.
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
 * Adobe Author(s): Michiharu Ariza
 */

#include "hb-test.h"
#include <hb-ot.h>

/* Unit tests for CFF/CFF2 glyph extents */

static void
test_extents_cff1 (void)
{
  hb_blob_t *blob = hb_blob_create_from_file ("fonts/SourceSansPro-Regular.abc.otf");
  g_assert (blob);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  g_assert (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert (font);
  hb_ot_font_set_funcs (font);

  hb_glyph_extents_t  extents;
  hb_bool_t result = hb_font_get_glyph_extents (font, 1, &extents);
  g_assert (result);

  g_assert_cmpint (extents.x_bearing, ==, 52);
  g_assert_cmpint (extents.y_bearing, ==, 498);
  g_assert_cmpint (extents.width, ==, 381);
  g_assert_cmpint (extents.height, ==, -510);

  hb_font_destroy (font);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_extents_cff1);

  return hb_test_run ();
}
