/*
 * Copyright © 2023 Red Hat, Inc.
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
 * Author(s): Matthias Clasen
 */

#include "hb-test.h"

static void
test_glyph_names_post (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char name [64];

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  ret = hb_font_get_glyph_name (font, 0, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, ".notdef");

  ret = hb_font_get_glyph_name (font, 1, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, ".space");

  ret = hb_font_get_glyph_name (font, 2, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, "icon0");

  ret = hb_font_get_glyph_name (font, 3, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, "icon0.0");

  ret = hb_font_get_glyph_name (font, 4, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, "icon0.1");

  ret = hb_font_get_glyph_name (font, 5, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, "icon0.2");

  /* beyond last glyph */
  ret = hb_font_get_glyph_name (font, 100, name, 64);
  g_assert_false (ret);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_cff (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char name [64];

  face = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");
  font = hb_font_create (face);

  ret = hb_font_get_glyph_name (font, 0, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, ".notdef");

  ret = hb_font_get_glyph_name (font, 1, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, "space");

  ret = hb_font_get_glyph_name (font, 2, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, "A");

  /* beyond last glyph */
  ret = hb_font_get_glyph_name (font, 2000, name, 64);
  g_assert_false (ret);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

/* Regression tests for issue #5946: hb_font_get_glyph_name (..., size=0)
 * must not underflow and write past a zero-capacity buffer, while preserving
 * the long-standing "return value only" probe behavior. */

static void
test_glyph_names_zero_size (void)
{
  hb_face_t *face;
  hb_font_t *font;
  char guard[4] = { 0x7f, 0x7f, 0x7f, 0x7f };

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  /* size == 0 must not write to the buffer. */
  g_assert_true (hb_font_get_glyph_name (font, 1, guard, 0));
  g_assert_cmpint (guard[0], ==, 0x7f);
  g_assert_cmpint (guard[1], ==, 0x7f);
  g_assert_cmpint (guard[2], ==, 0x7f);
  g_assert_cmpint (guard[3], ==, 0x7f);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_zero_size_post (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char guard[8] = { 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f };
  unsigned int i;

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  /* Named glyph, size == 0: keep the old return-value-only probe behavior
   * and leave the buffer untouched. */
  ret = hb_font_get_glyph_name (font, 1, guard, 0);
  g_assert_true (ret);
  for (i = 0; i < 8; i++)
    g_assert_cmpint (guard[i], ==, 0x7f);

  /* .notdef, size == 0: same invariant. Confirms it is not glyph-specific. */
  ret = hb_font_get_glyph_name (font, 0, guard, 0);
  g_assert_true (ret);
  for (i = 0; i < 8; i++)
    g_assert_cmpint (guard[i], ==, 0x7f);

  /* Perform a successful sized lookup first, then retry with size == 0.
   * Confirms no residual state corrupts the zero-size path. */
  {
    char name[64];
    ret = hb_font_get_glyph_name (font, 2, name, 64);
    g_assert_true (ret);
    g_assert_cmpstr (name, ==, "icon0");
  }
  ret = hb_font_get_glyph_name (font, 2, guard, 0);
  g_assert_true (ret);
  for (i = 0; i < 8; i++)
    g_assert_cmpint (guard[i], ==, 0x7f);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_zero_size_cff (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char guard[8] = { 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f };
  unsigned int i;

  face = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");
  font = hb_font_create (face);

  /* CFF-backed named glyph, size == 0: preserve the return-value-only probe
   * behavior without writing into the buffer. */
  ret = hb_font_get_glyph_name (font, 2, guard, 0);
  g_assert_true (ret);
  for (i = 0; i < 8; i++)
    g_assert_cmpint (guard[i], ==, 0x7f);

  /* .notdef on CFF. */
  ret = hb_font_get_glyph_name (font, 0, guard, 0);
  g_assert_true (ret);
  for (i = 0; i < 8; i++)
    g_assert_cmpint (guard[i], ==, 0x7f);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_zero_size_unnamed_glyph (void)
{
  /* size == 0 on a glyph beyond the font's range must also return false
   * and must not write. */
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char guard[4] = { 0x7f, 0x7f, 0x7f, 0x7f };
  unsigned int i;

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  ret = hb_font_get_glyph_name (font, 100, guard, 0);
  g_assert_false (ret);
  for (i = 0; i < 4; i++)
    g_assert_cmpint (guard[i], ==, 0x7f);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_zero_size_null_buffer (void)
{
  /* name == NULL with size == 0 is a valid return-value-only probe. */
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  /* Glyph with a name. */
  ret = hb_font_get_glyph_name (font, 1, NULL, 0);
  g_assert_true (ret);

  /* Glyph beyond the font's range. */
  ret = hb_font_get_glyph_name (font, 100, NULL, 0);
  g_assert_false (ret);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_size_one (void)
{
  /* Boundary: size == 1. The backend must produce exactly a trailing NUL
   * at name[0] (len = min(|name|, size-1) = 0 bytes of name copied) and
   * must not touch any byte past name[0]. */
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char buf[4] = { 0x7f, 0x7f, 0x7f, 0x7f };

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  ret = hb_font_get_glyph_name (font, 1, buf, 1);
  g_assert_true (ret);
  g_assert_cmpint (buf[0], ==, 0);
  /* Bytes beyond the caller-declared capacity must remain intact. */
  g_assert_cmpint (buf[1], ==, 0x7f);
  g_assert_cmpint (buf[2], ==, 0x7f);
  g_assert_cmpint (buf[3], ==, 0x7f);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_small_buffer_truncation (void)
{
  /* Non-regression sanity: a small-but-nonzero buffer must truncate to
   * size-1 bytes of the name plus a trailing NUL. */
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char buf[4] = { 0x7f, 0x7f, 0x7f, 0x7f };

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  /* ".notdef" truncated to capacity 4 -> ".no\0". */
  ret = hb_font_get_glyph_name (font, 0, buf, 4);
  g_assert_true (ret);
  g_assert_cmpint (buf[0], ==, '.');
  g_assert_cmpint (buf[1], ==, 'n');
  g_assert_cmpint (buf[2], ==, 'o');
  g_assert_cmpint (buf[3], ==, 0);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_exact_fit_buffer (void)
{
  /* Non-regression sanity: a buffer exactly sized to fit the full name
   * (including the trailing NUL) must hold the full name. For ".notdef"
   * that is 8 bytes. */
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char buf[8] = { 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f };

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  ret = hb_font_get_glyph_name (font, 0, buf, 8);
  g_assert_true (ret);
  g_assert_cmpstr (buf, ==, ".notdef");
  g_assert_cmpint (buf[7], ==, 0);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_nonzero_sizes_unchanged (void)
{
  /* Non-regression sanity: the existing behaviour at typical sizes must
   * be preserved by the fix. */
  hb_face_t *face;
  hb_font_t *font;
  hb_bool_t ret;
  char name[64];

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  font = hb_font_create (face);

  ret = hb_font_get_glyph_name (font, 0, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, ".notdef");

  ret = hb_font_get_glyph_name (font, 1, name, 64);
  g_assert_true (ret);
  g_assert_cmpstr (name, ==, ".space");

  /* Beyond last glyph still returns false. */
  ret = hb_font_get_glyph_name (font, 100, name, 64);
  g_assert_false (ret);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_glyph_names_zero_size_sub_font (void)
{
  /* Sub-fonts delegate to the parent's glyph-name funcs. Preserve the
   * return-value-only probe behavior through the parent chain as well. */
  hb_face_t *face;
  hb_font_t *parent;
  hb_font_t *font;
  hb_bool_t ret;
  char guard[4] = { 0x7f, 0x7f, 0x7f, 0x7f };
  unsigned int i;

  face = hb_test_open_font_file ("fonts/adwaita.ttf");
  parent = hb_font_create (face);
  font = hb_font_create_sub_font (parent);

  ret = hb_font_get_glyph_name (font, 1, guard, 0);
  g_assert_true (ret);
  for (i = 0; i < 4; i++)
    g_assert_cmpint (guard[i], ==, 0x7f);

  hb_font_destroy (font);
  hb_font_destroy (parent);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_glyph_names_post);
  hb_test_add (test_glyph_names_cff);
  hb_test_add (test_glyph_names_zero_size);
  hb_test_add (test_glyph_names_zero_size_post);
  hb_test_add (test_glyph_names_zero_size_cff);
  hb_test_add (test_glyph_names_zero_size_unnamed_glyph);
  hb_test_add (test_glyph_names_zero_size_null_buffer);
  hb_test_add (test_glyph_names_size_one);
  hb_test_add (test_glyph_names_small_buffer_truncation);
  hb_test_add (test_glyph_names_exact_fit_buffer);
  hb_test_add (test_glyph_names_nonzero_sizes_unchanged);
  hb_test_add (test_glyph_names_zero_size_sub_font);

  return hb_test_run();
}
