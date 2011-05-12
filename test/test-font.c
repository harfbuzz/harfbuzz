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

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_face_empty);

  hb_test_add (test_font_funcs_empty);

  hb_test_add (test_font_empty);

  return hb_test_run();
}
