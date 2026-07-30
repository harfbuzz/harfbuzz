/*
 * Copyright © 2018  Google, Inc.
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
 * Google Author(s): Garret Rieger
 */

#include "hb-test.h"
#include "hb-subset-test.h"

static void
test_collect_unicodes_format4 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.abc.format4.ttf");
  hb_set_t *codepoints = hb_set_create();
  hb_codepoint_t cp;

  hb_face_collect_unicodes (face, codepoints);

  cp = HB_SET_VALUE_INVALID;
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x61, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x62, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x63, ==, cp);
  g_assert_true (!hb_set_next (codepoints, &cp));

  hb_set_destroy (codepoints);
  hb_face_destroy (face);
}

static void
test_collect_unicodes_format4_with_range_offsets_and_deltas (void)
{
  const char cmap_data[44] = {
    0, 0, 0, 1,		      /* cmap header */
    0, 0, 0, 3, 0, 0, 0, 12,  /* encoding records */
    /* cmap 4 */
    0, 4,          /* format */
    0, 44,          /* length */
    0, 0,          /* lang */
    0, 2,          /* segcountx2 */
    0, 0,          /* search range */
    0, 0,          /* entry selector */
    0, 0,          /* range shift */
    0, 73,         /* end code [0] */
    0, 0,          /* pad */
    0, 70,         /* start code [0] */
    0, 5,          /* id delta [0] */
    0, 2,          /* id range offset [0] */
    0, 8,          /* glyph id u70 */
    0, 5,          /* glyph id u71 */
    0, 0,          /* glyph id u72 */
    0, 12,          /* glyph id u73 */
  };

  hb_blob_t* cmap = hb_blob_create(cmap_data, 44, HB_MEMORY_MODE_READONLY, 0, 0);
  hb_face_t* face = hb_face_builder_create();
  hb_face_builder_add_table(face, HB_TAG('c', 'm', 'a', 'p'), cmap);
  hb_blob_destroy(cmap);

  hb_set_t *codepoints = hb_set_create();
  hb_map_t *mapping = hb_map_create();
  hb_codepoint_t cp;

  hb_face_collect_unicodes (face, codepoints);

  cp = HB_SET_VALUE_INVALID;
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (70, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (71, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (73, ==, cp);
  g_assert_false (hb_set_next (codepoints, &cp));

  hb_set_clear(codepoints);
  hb_face_collect_nominal_glyph_mapping(face, mapping, codepoints);

  g_assert_cmpuint (hb_map_get(mapping, 70), ==, 8 + 5);
  g_assert_cmpuint (hb_map_get(mapping, 71), ==, 5 + 5);
  g_assert_false (hb_map_has(mapping, 72));
  g_assert_cmpuint (hb_map_get(mapping, 73), ==, 12 + 5);

  hb_map_destroy(mapping);
  hb_set_destroy (codepoints);
  hb_face_destroy (face);
}

static void
test_collect_unicodes_format12_notdef (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cmunrm.otf");
  hb_set_t *codepoints = hb_set_create();
  hb_codepoint_t cp;

  hb_face_collect_unicodes (face, codepoints);

  cp = HB_SET_VALUE_INVALID;
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x20, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x21, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x22, ==, cp);

  hb_set_destroy (codepoints);
  hb_face_destroy (face);
}

static void
test_collect_unicodes_format12 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.abc.format12.ttf");
  hb_set_t *codepoints = hb_set_create();
  hb_codepoint_t cp;

  hb_face_collect_unicodes (face, codepoints);

  cp = HB_SET_VALUE_INVALID;
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x61, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x62, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_cmpuint (0x63, ==, cp);
  g_assert_true (!hb_set_next (codepoints, &cp));

  hb_set_destroy (codepoints);
  hb_face_destroy (face);
}

static void
test_collect_unicodes (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_set_t *codepoints = hb_set_create();
  hb_set_t *codepoints2 = hb_set_create();
  hb_map_t *mapping = hb_map_create();
  hb_codepoint_t cp;

  hb_face_collect_unicodes (face, codepoints);
  hb_face_collect_nominal_glyph_mapping (face, mapping, codepoints2);

  g_assert_true (hb_set_is_equal (codepoints, codepoints2));
  g_assert_cmpuint (hb_set_get_population (codepoints), ==, 3);
  g_assert_cmpuint (hb_map_get_population (mapping), ==, 3);

  cp = HB_SET_VALUE_INVALID;
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_true (hb_map_has (mapping, cp));
  g_assert_cmpuint (0x61, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_true (hb_map_has (mapping, cp));
  g_assert_cmpuint (0x62, ==, cp);
  g_assert_true (hb_set_next (codepoints, &cp));
  g_assert_true (hb_map_has (mapping, cp));
  g_assert_cmpuint (0x63, ==, cp);
  g_assert_true (!hb_set_next (codepoints, &cp));

  hb_set_destroy (codepoints);
  hb_set_destroy (codepoints2);
  hb_map_destroy (mapping);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_collect_unicodes);
  hb_test_add (test_collect_unicodes_format4);
  hb_test_add (test_collect_unicodes_format12);
  hb_test_add (test_collect_unicodes_format12_notdef);
  hb_test_add (test_collect_unicodes_format4_with_range_offsets_and_deltas);

  return hb_test_run();
}
