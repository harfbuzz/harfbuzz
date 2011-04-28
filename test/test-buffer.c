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
} fixture_t;

static void
fixture_init (fixture_t *fixture, gconstpointer user_data)
{
  unsigned int i;

  fixture->b = hb_buffer_create (0);

  switch (GPOINTER_TO_INT (user_data)) {
    case BUFFER_EMPTY:
      break;

    case BUFFER_ONE_BY_ONE:
      for (i = 1; i < G_N_ELEMENTS (utf32) - 1; i++)
	hb_buffer_add (fixture->b, utf32[i], 1, i);
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
fixture_finish (fixture_t *fixture, gconstpointer user_data)
{
  hb_buffer_destroy (fixture->b);
}


static void
test_buffer_properties (fixture_t *fixture, gconstpointer user_data)
{
  hb_unicode_funcs_t *ufuncs;

  /* test default properties */

  g_assert (hb_buffer_get_unicode_funcs (fixture->b) == hb_unicode_funcs_get_default ());
  g_assert (hb_buffer_get_direction (fixture->b) == HB_DIRECTION_INVALID);
  g_assert (hb_buffer_get_script (fixture->b) == HB_SCRIPT_INVALID);
  g_assert (hb_buffer_get_language (fixture->b) == NULL);


  /* test property changes are retained */
  ufuncs = hb_unicode_funcs_create (NULL);
  hb_buffer_set_unicode_funcs (fixture->b, ufuncs);
  hb_unicode_funcs_destroy (ufuncs);
  g_assert (hb_buffer_get_unicode_funcs (fixture->b) == ufuncs);

  hb_buffer_set_direction (fixture->b, HB_DIRECTION_RTL);
  g_assert (hb_buffer_get_direction (fixture->b) == HB_DIRECTION_RTL);

  hb_buffer_set_script (fixture->b, HB_SCRIPT_ARABIC);
  g_assert (hb_buffer_get_script (fixture->b) == HB_SCRIPT_ARABIC);

  hb_buffer_set_language (fixture->b, hb_language_from_string ("fa"));
  g_assert (hb_buffer_get_language (fixture->b) == hb_language_from_string ("Fa"));


  /* test reset clears properties */

  hb_buffer_reset (fixture->b);

  g_assert (hb_buffer_get_unicode_funcs (fixture->b) == hb_unicode_funcs_get_default ());
  g_assert (hb_buffer_get_direction (fixture->b) == HB_DIRECTION_INVALID);
  g_assert (hb_buffer_get_script (fixture->b) == HB_SCRIPT_INVALID);
  g_assert (hb_buffer_get_language (fixture->b) == NULL);
}

static void
test_buffer_contents (fixture_t *fixture, gconstpointer user_data)
{
  unsigned int i, len, len2;
  buffer_type_t buffer_type = GPOINTER_TO_INT (user_data);
  hb_glyph_info_t *glyphs;

  if (buffer_type == BUFFER_EMPTY) {
    g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);
    return;
  }

  len = hb_buffer_get_length (fixture->b);
  glyphs = hb_buffer_get_glyph_infos (fixture->b, NULL); /* test NULL */
  glyphs = hb_buffer_get_glyph_infos (fixture->b, &len2);
  g_assert_cmpint (len, ==, len2);
  g_assert_cmpint (len, ==, 5);

  for (i = 0; i < len; i++) {
    g_assert_cmphex (glyphs[i].mask,      ==, 1);
    g_assert_cmphex (glyphs[i].var1.u32,  ==, 0);
    g_assert_cmphex (glyphs[i].var2.u32,  ==, 0);
  }

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
  }

  /* reverse, test, and reverse back */

  hb_buffer_reverse (fixture->b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[len-i]);

  hb_buffer_reverse (fixture->b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);

  /* reverse_clusters works same as reverse for now since each codepoint is
   * in its own cluster */

  hb_buffer_reverse_clusters (fixture->b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[len-i]);

  hb_buffer_reverse_clusters (fixture->b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);

  /* now form a cluster and test again */
  glyphs[2].cluster = glyphs[1].cluster;

  /* reverse, test, and reverse back */

  hb_buffer_reverse (fixture->b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[len-i]);

  hb_buffer_reverse (fixture->b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);

  /* reverse_clusters twice still should return the original string,
   * but when applied once, the 1-2 cluster should be retained. */

  hb_buffer_reverse_clusters (fixture->b);
  for (i = 0; i < len; i++) {
    unsigned int j = len-1-i;
    if (j == 1)
      j = 2;
    else if (j == 2)
      j = 1;
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+j]);
  }

  hb_buffer_reverse_clusters (fixture->b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);


  /* test setting length */

  /* enlarge */
  g_assert (hb_buffer_set_length (fixture->b, 10));
  glyphs = hb_buffer_get_glyph_infos (fixture->b, NULL);
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 10);
  for (i = 0; i < 5; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);
  for (i = 5; i < 10; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, 0);
  /* shrink */
  g_assert (hb_buffer_set_length (fixture->b, 3));
  glyphs = hb_buffer_get_glyph_infos (fixture->b, NULL);
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 3);
  for (i = 0; i < 3; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);


  g_assert (hb_buffer_allocation_successful (fixture->b));


  /* test reset clears content */

  hb_buffer_reset (fixture->b);
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);
}

static void
test_buffer_positions (fixture_t *fixture, gconstpointer user_data)
{
  unsigned int i, len, len2;
  hb_glyph_position_t *positions;

  /* Without shaping, positions should all be zero */
  len = hb_buffer_get_length (fixture->b);
  positions = hb_buffer_get_glyph_positions (fixture->b, NULL); /* test NULL */
  positions = hb_buffer_get_glyph_positions (fixture->b, &len2);
  g_assert_cmpint (len, ==, len2);
  for (i = 0; i < len; i++) {
    g_assert_cmpint (0, ==, positions[i].x_advance);
    g_assert_cmpint (0, ==, positions[i].y_advance);
    g_assert_cmpint (0, ==, positions[i].x_offset);
    g_assert_cmpint (0, ==, positions[i].y_offset);
    g_assert_cmpint (0, ==, positions[i].var.i32);
  }

  /* test reset clears content */
  hb_buffer_reset (fixture->b);
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);
}

static void
test_buffer_allocation (fixture_t *fixture, gconstpointer user_data)
{
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);

  g_assert (hb_buffer_pre_allocate (fixture->b, 100));
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);
  g_assert (hb_buffer_allocation_successful (fixture->b));

  /* lets try a huge allocation, make sure it fails */
  g_assert (!hb_buffer_pre_allocate (fixture->b, (unsigned int) -1));
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);
  g_assert (!hb_buffer_allocation_successful (fixture->b));

  /* small one again */
  g_assert (hb_buffer_pre_allocate (fixture->b, 50));
  g_assert_cmpint (hb_buffer_get_length (fixture->b), ==, 0);
  g_assert (!hb_buffer_allocation_successful (fixture->b));

  hb_buffer_reset (fixture->b);
  g_assert (hb_buffer_allocation_successful (fixture->b));

  /* all allocation and size  */
  g_assert (!hb_buffer_pre_allocate (fixture->b, ((unsigned int) -1) / 20 + 1));
  g_assert (!hb_buffer_allocation_successful (fixture->b));

  hb_buffer_reset (fixture->b);
  g_assert (hb_buffer_allocation_successful (fixture->b));

  /* technically, this one can actually pass on 64bit machines, but
   * I'm doubtful that any malloc allows 4GB allocations at a time.
   * But let's only enable it on a 32-bit machine. */
  if (sizeof (long) == 4) {
    g_assert (!hb_buffer_pre_allocate (fixture->b, ((unsigned int) -1) / 20 - 1));
    g_assert (!hb_buffer_allocation_successful (fixture->b));
  }

  hb_buffer_reset (fixture->b);
  g_assert (hb_buffer_allocation_successful (fixture->b));
}


int
main (int argc, char **argv)
{
  int i;

  hb_test_init (&argc, &argv);

  for (i = 0; i < BUFFER_NUM_TYPES; i++)
  {
    const void *buffer_type = GINT_TO_POINTER (i);
    const char *buffer_name = buffer_names[i];

    hb_test_add_fixture_flavor (fixture, buffer_type, buffer_name, test_buffer_properties);
    hb_test_add_fixture_flavor (fixture, buffer_type, buffer_name, test_buffer_contents);
    hb_test_add_fixture_flavor (fixture, buffer_type, buffer_name, test_buffer_positions);
  }

  hb_test_add_fixture (fixture, GINT_TO_POINTER (BUFFER_EMPTY), test_buffer_allocation);

  /* XXX test invalid UTF-8 / UTF-16 text input (also overlong sequences) */

  return hb_test_run();
}
