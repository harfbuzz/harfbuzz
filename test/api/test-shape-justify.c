/*
 * Copyright © 2026  nussaiba shaikh
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

/* Unit tests for hb_shape_justify() */

static hb_buffer_t *
create_buffer (void)
{
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, "\xd8\xa7\xd8\xa8\xd8\xac", -1, 0, -1); /* ابج */
  hb_buffer_guess_segment_properties (buffer);
  return buffer;
}

/* Estedad-VF has a wdth axis (100..200, default 100).  At the default
 * instance the text advances 2565 units, at wdth=200 it advances 3565. */
static void
test_shape_justify_early_returns (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Estedad-VF.ttf");
  hb_font_t *font = hb_font_create (face);
  const char *bad_shapers[] = {"nonexistent", NULL};
  hb_buffer_t *buffer;
  float advance;
  hb_tag_t var_tag;
  float var_value;

  /* Default advance, computed internally, already inside the target range. */
  buffer = create_buffer ();
  advance = 0.f;
  g_assert_true (hb_shape_justify (font, buffer, NULL, 0, NULL,
				   2500.f, 2600.f,
				   &advance, &var_tag, &var_value));
  g_assert_cmpuint (var_tag, ==, HB_TAG_NONE);
  g_assert_cmpfloat (advance, ==, 2565.f);
  hb_buffer_destroy (buffer);

  /* Target above the maximum expansion: the axis maximum is returned. */
  buffer = create_buffer ();
  advance = 0.f;
  g_assert_true (hb_shape_justify (font, buffer, NULL, 0, NULL,
				   3600.f, 4000.f,
				   &advance, &var_tag, &var_value));
  g_assert_cmpuint (var_tag, ==, HB_TAG ('w','d','t','h'));
  g_assert_cmpfloat (var_value, ==, 200.f);
  g_assert_cmpfloat (advance, ==, 3565.f);
  hb_buffer_destroy (buffer);

  /* Target below the maximum shrink: the axis minimum is returned. */
  buffer = create_buffer ();
  advance = 0.f;
  g_assert_true (hb_shape_justify (font, buffer, NULL, 0, NULL,
				   2000.f, 2100.f,
				   &advance, &var_tag, &var_value));
  g_assert_cmpuint (var_tag, ==, HB_TAG ('w','d','t','h'));
  g_assert_cmpfloat (var_value, ==, 100.f);
  g_assert_cmpfloat (advance, ==, 2565.f);
  hb_buffer_destroy (buffer);

  /* Shaping failure. */
  buffer = create_buffer ();
  advance = 0.f;
  g_assert_false (hb_shape_justify (font, buffer, NULL, 0, bad_shapers,
				    3000.f, 3100.f,
				    &advance, &var_tag, &var_value));
  hb_buffer_destroy (buffer);

  /* Solver path. */
  buffer = create_buffer ();
  advance = 0.f;
  g_assert_true (hb_shape_justify (font, buffer, NULL, 0, NULL,
				   3000.f, 3100.f,
				   &advance, &var_tag, &var_value));
  g_assert_cmpuint (var_tag, ==, HB_TAG ('w','d','t','h'));
  g_assert_cmpfloat (advance, >=, 3000.f);
  g_assert_cmpfloat (advance, <=, 3100.f);
  hb_buffer_destroy (buffer);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_shape_justify_early_returns);

  return hb_test_run();
}
