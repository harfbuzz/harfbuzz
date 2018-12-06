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
 */

#include "hb-test.h"

/* Unit tests for hb-ot-style.h */

#include <hb-ot.h>


static void
test_face_empty (void)
{
  hb_face_t *empty = hb_face_get_empty ();

  unsigned int num_entries;
  hb_ot_style_get (empty, &num_entries);
//   g_assert_cmpuint (num_entries, ==, 5);
//   g_assert_cmpfloat (hb_ot_style_get (empty, HB_OT_STYLE_ITALIC), ==, 0);
//   g_assert_cmpfloat (hb_ot_style_get (empty, HB_OT_STYLE_OPTICAL_SIZE), ==, 12);
//   g_assert_cmpfloat (hb_ot_style_get (empty, HB_OT_STYLE_SLANT), ==, 0);
//   g_assert_cmpfloat (hb_ot_style_get (empty, HB_OT_STYLE_WIDTH), ==, 100);
//   g_assert_cmpfloat (hb_ot_style_get (empty, HB_OT_STYLE_WEIGHT), ==, 400);

  hb_face_destroy (empty);
}

static void
test_face_fdsc (void)
{
  hb_face_t *fdsc;
  fdsc = hb_test_open_font_file ("fonts/aat-fdsc.ttf");

  unsigned int num_entries;
  hb_ot_style_get (fdsc, &num_entries);
  g_assert_cmpuint (num_entries, ==, 5);
//   g_assert_cmpint ((int) hb_ot_style_get (fdsc, HB_OT_STYLE_ITALIC), ==, 0);
//   g_assert_cmpint ((int) hb_ot_style_get (fdsc, HB_OT_STYLE_OPTICAL_SIZE), ==, 24);
//   g_assert_cmpint ((int) hb_ot_style_get (fdsc, HB_OT_STYLE_SLANT), ==, 6);
//   g_assert_cmpint ((int) hb_ot_style_get (fdsc, HB_OT_STYLE_WIDTH), ==, 100);
//   g_assert_cmpint ((int) hb_ot_style_get (fdsc, HB_OT_STYLE_WEIGHT), ==, 400);

  hb_face_destroy (fdsc);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_face_empty);
  hb_test_add (test_face_fdsc);

  return hb_test_run();
}
