/*
 * Copyright (C) 2010  Google, Inc.
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

/* This file tests types defined in hb-buffer.h */


static const char utf8[10] = "ab\360\240\200\200defg";
static const uint16_t utf16[8] = {'a', 'b', 0xD840, 0xDC00, 'd', 'e', 'f', 'g'};
static const uint32_t utf32[7] = {'a', 'b', 0x20000, 'd', 'e', 'f', 'g'};

typedef enum {
  BUFFER_EMPTY,
  BUFFER_ONE_BY_ONE,
  BUFFER_UTF32,
  BUFFER_UTF16,
  BUFFER_UTF8,
  BUFFER_NUM_TYPES,
} buffer_type_t;

static const char *buffer_names[] = {
  "empty",
  "one-by-one",
  "utf32",
  "utf16",
  "utf8"
};

typedef struct
{
  hb_buffer_t *b;
} Fixture;

static void
fixture_init (Fixture *fixture, gconstpointer user_data)
{
  unsigned int i;

  fixture->b = hb_buffer_create (0);

  switch (GPOINTER_TO_INT (user_data)) {
    case BUFFER_EMPTY:
      break;

    case BUFFER_ONE_BY_ONE:
      for (i = 1; i < G_N_ELEMENTS (utf32) - 1; i++)
	hb_buffer_add_glyph (fixture->b, utf32[i], 1, i);
      break;

    case BUFFER_UTF32:
      hb_buffer_add_utf32 (fixture->b, utf32, G_N_ELEMENTS (utf32), 1, G_N_ELEMENTS (utf32) - 2);
      break;

    case BUFFER_UTF16:
      hb_buffer_add_utf16 (fixture->b, utf16, G_N_ELEMENTS (utf16), 1, G_N_ELEMENTS (utf16) - 2);
      break;

    case BUFFER_UTF8:
      hb_buffer_add_utf8  (fixture->b, utf8,  G_N_ELEMENTS (utf8),  1, G_N_ELEMENTS (utf8)  - 2);
      break;

    default:
      g_assert_not_reached ();
  }
}

static void
fixture_fini (Fixture *fixture, gconstpointer user_data)
{
  hb_buffer_destroy (fixture->b);
}


static void
test_buffer_properties (Fixture *fixture, gconstpointer user_data)
{
  /* TODO check unicode_funcs */

  g_assert (hb_buffer_get_direction (fixture->b) == HB_DIRECTION_INVALID);
  g_assert (hb_buffer_get_script (fixture->b) == HB_SCRIPT_INVALID);
  g_assert (hb_buffer_get_language (fixture->b) == NULL);

  hb_buffer_set_direction (fixture->b, HB_DIRECTION_RTL);
  g_assert (hb_buffer_get_direction (fixture->b) == HB_DIRECTION_RTL);

  hb_buffer_set_script (fixture->b, HB_SCRIPT_ARABIC);
  g_assert (hb_buffer_get_script (fixture->b) == HB_SCRIPT_ARABIC);

  hb_buffer_set_language (fixture->b, hb_language_from_string ("fa"));
  g_assert (hb_buffer_get_language (fixture->b) == hb_language_from_string ("Fa"));
}

static void
test_buffer_contents (Fixture *fixture, gconstpointer user_data)
{
  unsigned int i, len;
  buffer_type_t buffer_type = GPOINTER_TO_INT (user_data);
  hb_glyph_info_t *glyphs;

  if (buffer_type == BUFFER_EMPTY) {
    g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);
    return;
  }

  glyphs = hb_buffer_get_glyph_infos (fixture->b, &len);
  g_assert_cmpint (len, ==, 5);

  for (i = 0; i < len; i++) {
    unsigned int cluster;
    cluster = 1+i;
    if (i >= 2) {
      if (buffer_type == BUFFER_UTF16)
	cluster++;
      else if (buffer_type == BUFFER_UTF8)
        cluster += 3;
    }
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);
    g_assert_cmphex (glyphs[i].cluster,   ==, cluster);
    g_assert_cmphex (glyphs[i].mask,      ==, 1);
    g_assert_cmphex (glyphs[i].var1.u32,  ==, 0);
    g_assert_cmphex (glyphs[i].var2.u32,  ==, 0);
  }
}

static void
test_buffer_positions (Fixture *fixture, gconstpointer user_data)
{
  unsigned int i, len;
  hb_glyph_position_t *positions;

  /* Without shaping, positions should all be zero */
  positions = hb_buffer_get_glyph_positions (fixture->b, &len);
  for (i = 0; i < len; i++) {
    g_assert_cmpint (0, ==, positions[i].x_advance);
    g_assert_cmpint (0, ==, positions[i].y_advance);
    g_assert_cmpint (0, ==, positions[i].x_offset);
    g_assert_cmpint (0, ==, positions[i].y_offset);
    g_assert_cmpint (0, ==, positions[i].var.i32);
  }
}

int
main (int argc, char **argv)
{
  int i;

  g_test_init (&argc, &argv, NULL);

  for (i = 0; i < BUFFER_NUM_TYPES; i++) {
#define TEST_ADD(path, func) \
    G_STMT_START { \
      char *s = g_strdup_printf ("%s/%s", path, buffer_names[i]); \
      g_test_add (s, Fixture, GINT_TO_POINTER (i), fixture_init, func, fixture_fini); \
      g_free (s); \
    } G_STMT_END
    TEST_ADD ("/buffer/properties", test_buffer_properties);
    TEST_ADD ("/buffer/contents", test_buffer_contents);
    TEST_ADD ("/buffer/positions", test_buffer_positions);
#undef TEST_ADD
  }

  /* XXX test invalid UTF-8 / UTF-16 text input (also overlong sequences) */
  /* XXX test reverse() and reverse_clusters() */
  /* XXX test ensure() and memory management */
  /* XXX test buffer reset */
  /* XXX test buffer set length */

  return g_test_run();
}
