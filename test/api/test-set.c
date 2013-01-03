/*
 * Copyright © 2013  Google, Inc.
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

/* Unit tests for hb-set.h */


#if 0
static void
test_buffer_properties (fixture_t *fixture, gconstpointer user_data)
{
  hb_buffer_t *b = fixture->buffer;
  hb_unicode_funcs_t *ufuncs;

  /* test default properties */

  g_assert (hb_buffer_get_unicode_funcs (b) == hb_unicode_funcs_get_default ());
  g_assert (hb_buffer_get_direction (b) == HB_DIRECTION_INVALID);
  g_assert (hb_buffer_get_script (b) == HB_SCRIPT_INVALID);
  g_assert (hb_buffer_get_language (b) == NULL);
  g_assert (hb_buffer_get_flags (b) == HB_BUFFER_FLAGS_DEFAULT);


  /* test property changes are retained */
  ufuncs = hb_unicode_funcs_create (NULL);
  hb_buffer_set_unicode_funcs (b, ufuncs);
  hb_unicode_funcs_destroy (ufuncs);
  g_assert (hb_buffer_get_unicode_funcs (b) == ufuncs);

  hb_buffer_set_direction (b, HB_DIRECTION_RTL);
  g_assert (hb_buffer_get_direction (b) == HB_DIRECTION_RTL);

  hb_buffer_set_script (b, HB_SCRIPT_ARABIC);
  g_assert (hb_buffer_get_script (b) == HB_SCRIPT_ARABIC);

  hb_buffer_set_language (b, hb_language_from_string ("fa", -1));
  g_assert (hb_buffer_get_language (b) == hb_language_from_string ("Fa", -1));

  hb_buffer_set_flags (b, HB_BUFFER_FLAG_BOT);
  g_assert (hb_buffer_get_flags (b) == HB_BUFFER_FLAG_BOT);



  /* test clear clears all properties but unicode_funcs */

  hb_buffer_clear (b);

  g_assert (hb_buffer_get_unicode_funcs (b) == ufuncs);
  g_assert (hb_buffer_get_direction (b) == HB_DIRECTION_INVALID);
  g_assert (hb_buffer_get_script (b) == HB_SCRIPT_INVALID);
  g_assert (hb_buffer_get_language (b) == NULL);
  g_assert (hb_buffer_get_flags (b) == HB_BUFFER_FLAGS_DEFAULT);


  /* test reset clears all properties */

  hb_buffer_set_direction (b, HB_DIRECTION_RTL);
  g_assert (hb_buffer_get_direction (b) == HB_DIRECTION_RTL);

  hb_buffer_set_script (b, HB_SCRIPT_ARABIC);
  g_assert (hb_buffer_get_script (b) == HB_SCRIPT_ARABIC);

  hb_buffer_set_language (b, hb_language_from_string ("fa", -1));
  g_assert (hb_buffer_get_language (b) == hb_language_from_string ("Fa", -1));

  hb_buffer_set_flags (b, HB_BUFFER_FLAG_BOT);
  g_assert (hb_buffer_get_flags (b) == HB_BUFFER_FLAG_BOT);

  hb_buffer_reset (b);

  g_assert (hb_buffer_get_unicode_funcs (b) == hb_unicode_funcs_get_default ());
  g_assert (hb_buffer_get_direction (b) == HB_DIRECTION_INVALID);
  g_assert (hb_buffer_get_script (b) == HB_SCRIPT_INVALID);
  g_assert (hb_buffer_get_language (b) == NULL);
  g_assert (hb_buffer_get_flags (b) == HB_BUFFER_FLAGS_DEFAULT);
}

static void
test_buffer_contents (fixture_t *fixture, gconstpointer user_data)
{
  hb_buffer_t *b = fixture->buffer;
  unsigned int i, len, len2;
  buffer_type_t buffer_type = GPOINTER_TO_INT (user_data);
  hb_glyph_info_t *glyphs;

  if (buffer_type == BUFFER_EMPTY) {
    g_assert_cmpint (hb_buffer_get_population (b), ==, 0);
    return;
  }

  len = hb_buffer_get_population (b);
  hb_buffer_get_glyph_infos (b, NULL); /* test NULL */
  glyphs = hb_buffer_get_glyph_infos (b, &len2);
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

  hb_buffer_reverse (b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[len-i]);

  hb_buffer_reverse (b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);

  /* reverse_clusters works same as reverse for now since each codepoint is
   * in its own cluster */

  hb_buffer_reverse_clusters (b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[len-i]);

  hb_buffer_reverse_clusters (b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);

  /* now form a cluster and test again */
  glyphs[2].cluster = glyphs[1].cluster;

  /* reverse, test, and reverse back */

  hb_buffer_reverse (b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[len-i]);

  hb_buffer_reverse (b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);

  /* reverse_clusters twice still should return the original string,
   * but when applied once, the 1-2 cluster should be retained. */

  hb_buffer_reverse_clusters (b);
  for (i = 0; i < len; i++) {
    unsigned int j = len-1-i;
    if (j == 1)
      j = 2;
    else if (j == 2)
      j = 1;
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+j]);
  }

  hb_buffer_reverse_clusters (b);
  for (i = 0; i < len; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);


  /* test setting length */

  /* enlarge */
  g_assert (hb_buffer_set_length (b, 10));
  glyphs = hb_buffer_get_glyph_infos (b, NULL);
  g_assert_cmpint (hb_buffer_get_population (b), ==, 10);
  for (i = 0; i < 5; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);
  for (i = 5; i < 10; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, 0);
  /* shrink */
  g_assert (hb_buffer_set_length (b, 3));
  glyphs = hb_buffer_get_glyph_infos (b, NULL);
  g_assert_cmpint (hb_buffer_get_population (b), ==, 3);
  for (i = 0; i < 3; i++)
    g_assert_cmphex (glyphs[i].codepoint, ==, utf32[1+i]);


  g_assert (hb_buffer_allocation_successful (b));


  /* test reset clears content */

  hb_buffer_reset (b);
  g_assert_cmpint (hb_buffer_get_population (b), ==, 0);
}

static void
test_buffer_allocation (fixture_t *fixture, gconstpointer user_data)
{
  hb_buffer_t *b = fixture->buffer;

  g_assert_cmpint (hb_buffer_get_population (b), ==, 0);

  g_assert (hb_buffer_pre_allocate (b, 100));
  g_assert_cmpint (hb_buffer_get_population (b), ==, 0);
  g_assert (hb_buffer_allocation_successful (b));

  /* lets try a huge allocation, make sure it fails */
  g_assert (!hb_buffer_pre_allocate (b, (unsigned int) -1));
  g_assert_cmpint (hb_buffer_get_population (b), ==, 0);
  g_assert (!hb_buffer_allocation_successful (b));

  /* small one again */
  g_assert (hb_buffer_pre_allocate (b, 50));
  g_assert_cmpint (hb_buffer_get_population (b), ==, 0);
  g_assert (!hb_buffer_allocation_successful (b));

  hb_buffer_reset (b);
  g_assert (hb_buffer_allocation_successful (b));

  /* all allocation and size  */
  g_assert (!hb_buffer_pre_allocate (b, ((unsigned int) -1) / 20 + 1));
  g_assert (!hb_buffer_allocation_successful (b));

  hb_buffer_reset (b);
  g_assert (hb_buffer_allocation_successful (b));

  /* technically, this one can actually pass on 64bit machines, but
   * I'm doubtful that any malloc allows 4GB allocations at a time.
   * But let's only enable it on a 32-bit machine. */
  if (sizeof (long) == 4) {
    g_assert (!hb_buffer_pre_allocate (b, ((unsigned int) -1) / 20 - 1));
    g_assert (!hb_buffer_allocation_successful (b));
  }

  hb_buffer_reset (b);
  g_assert (hb_buffer_allocation_successful (b));
}
#endif

static void
test_empty (hb_set_t *b)
{
  g_assert_cmpint (hb_set_get_population (b), ==, 0);
  g_assert_cmpint (hb_set_get_min (b), ==, (hb_codepoint_t) -1);
  g_assert_cmpint (hb_set_get_max (b), ==, (hb_codepoint_t) -1);
}

static void
test_set_empty (void)
{
  hb_set_t *b = hb_set_get_empty ();

  g_assert (hb_set_get_empty ());
  g_assert (hb_set_get_empty () == b);

  g_assert (!hb_set_allocation_successful (b));

  test_empty (b);

  hb_set_add (b, 13);

  test_empty (b);

  hb_set_invert (b);

  test_empty (b);

  g_assert (!hb_set_allocation_successful (b));

  hb_set_clear (b);

  test_empty (b);

  g_assert (!hb_set_allocation_successful (b));
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

#if 0
  hb_test_add (test_set_algebra);
  hb_test_add (test_set_iter);
#endif
  hb_test_add (test_set_empty);

  return hb_test_run();
}
