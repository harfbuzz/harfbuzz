/*
 * Copyright © 2020  Ebrahim Byagowi
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
#include <hb-ot.h>

static void
test_ot_layout_lookup_get_glyph_alternates (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");

  hb_codepoint_t alternates[3];
  unsigned alternates_count = 3;
  g_assert_cmpuint (7, ==, hb_ot_layout_lookup_get_glyph_alternates (face, 1, 1091, 2, &alternates_count, alternates));

  g_assert_cmpuint (3, ==, alternates_count);
  g_assert_cmpuint (1606, ==, alternates[0]);
  g_assert_cmpuint (1578, ==, alternates[1]);
  g_assert_cmpuint (1592, ==, alternates[2]);

  hb_face_destroy (face);
}

/* Collect one lookup and cross-check every entry against the per-glyph getter. */
static void
test_ot_layout_lookup_collect_glyph_alternates (gconstpointer user_data)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");
  unsigned lookup_index = GPOINTER_TO_UINT (user_data);
  hb_map_t *alternate_count = hb_map_create ();
  hb_map_t *alternate_glyphs = hb_map_create ();
  unsigned glyph_count = hb_face_get_glyph_count (face);
  unsigned collected = 0;
  unsigned covered = 0;

  hb_ot_layout_lookup_collect_glyph_alternates (face, lookup_index,
						alternate_count, alternate_glyphs);

  for (hb_codepoint_t gid = 0; gid < glyph_count; gid++)
  {
    hb_codepoint_t alternates[16];
    unsigned count = 16;
    unsigned total = hb_ot_layout_lookup_get_glyph_alternates (face, lookup_index, gid, 0, &count, alternates);

    g_assert_cmpuint (total, ==, count);
    if (!count)
    {
      g_assert_false (hb_map_has (alternate_count, gid));
      continue;
    }
    g_assert_cmpuint (count, ==, hb_map_get (alternate_count, gid));
    for (unsigned i = 0; i < count; i++)
      g_assert_cmpuint (alternates[i], ==, hb_map_get (alternate_glyphs, gid + (i << 24)));
    collected += count;
    covered++;
  }

  g_assert_cmpuint (collected, ==, hb_map_get_population (alternate_glyphs));
  g_assert_cmpuint (covered, ==, hb_map_get_population (alternate_count));

  hb_map_destroy (alternate_count);
  hb_map_destroy (alternate_glyphs);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_ot_layout_lookup_get_glyph_alternates);
  hb_test_add_data_flavor (GUINT_TO_POINTER (0), "single-format2", test_ot_layout_lookup_collect_glyph_alternates);
  hb_test_add_data_flavor (GUINT_TO_POINTER (1), "alternate", test_ot_layout_lookup_collect_glyph_alternates);
  hb_test_add_data_flavor (GUINT_TO_POINTER (2), "single-format1", test_ot_layout_lookup_collect_glyph_alternates);
  hb_test_add_data_flavor (GUINT_TO_POINTER (18), "single-format1-many", test_ot_layout_lookup_collect_glyph_alternates);
  return hb_test_run ();
}
