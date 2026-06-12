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
 * Google Author(s): Roderick Sheeter
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for cmap subsetting */

static void
test_subset_cmap (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 99);
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('c','m','a','p'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cmap_non_consecutive_glyphs (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.D7,D8,D9,DA,DE.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0xD7);
  hb_set_add (codepoints, 0xD8);
  hb_set_add (codepoints, 0xD9);
  hb_set_add (codepoints, 0xDA);
  hb_set_add (codepoints, 0xDE);

  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face, face_subset, HB_TAG ('c','m','a','p'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cmap_noop (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('c','m','a','p'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
}

static void
test_subset_cmap4_no_exceeding_maximum_codepoint (void)
{
  hb_face_t *face_origin = hb_test_open_font_file ("fonts/Mplus1p-Regular.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/Mplus1p-Regular-cmap4-testing.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x20);
  hb_set_add (codepoints, 0x21);
  hb_set_add (codepoints, 0x1d542);
  hb_set_add (codepoints, 0x201a2);

  face_subset = hb_subset_test_create_subset (face_origin, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('c','m','a','p'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face_origin);
}

static void
test_subset_cmap_empty_tables (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_empty = hb_test_open_font_file ("fonts/Roboto-Regular.empty.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 100);
  hb_set_add (codepoints, 101);
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_empty, face_abc_subset, HB_TAG ('c','m','a','p'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_empty);
}

static void
test_subset_cmap_noto_color_emoji_noop (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.cmap.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x38);
  hb_set_add (codepoints, 0x39);
  hb_set_add (codepoints, 0xAE);
  hb_set_add (codepoints, 0x2049);
  hb_set_add (codepoints, 0x20E3);
  hb_set_add (codepoints, 0xfe0f);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face, face_subset, HB_TAG ('c','m','a','p'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cmap_noto_color_emoji_non_consecutive_glyphs (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.cmap.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/NotoColorEmoji.cmap.38,AE,2049.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x38);
  hb_set_add (codepoints, 0xAE);
  hb_set_add (codepoints, 0x2049);
  hb_set_add (codepoints, 0xfe0f);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('c','m','a','p'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

#ifndef HB_NO_BEYOND_64K
static void
test_subset_cmap_format15 (void)
{
  const char MAXP_data[] = "\x00\x00\x50\x00\x01\x00\x02";
  const char cmap_data[] = {
    0, 0, 0, 3,				/* cmap header */
    0, 0, 0, 4, 0, 0, 0, 28,		/* encoding records */
    0, 0, 0, 5, 0, 0, 0, 56,
    0, 0, 0, 5, 0, 0, 0, 86,
    0, 12, 0, 0, 0, 0, 0, 28,		/* format 12 header */
    0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 'A', 0, 0, 0, 'A', 0, 1, 0, 1, /* format 12 group */
    0, 14, 0, 0, 0, 30, 0, 0, 0, 1,	/* format 14 header */
    0, 0xFE, 0x0F, 0, 0, 0, 0, 0, 0, 0, 21, /* variation selector */
    0, 0, 0, 1, 0, 0, 'A', 0, 2,	/* non-default UVS */
    0, 15, 0, 0, 0, 31, 0, 0, 0, 1,	/* format 15 header */
    0, 0xFE, 0x0F, 0, 0, 0, 0, 0, 0, 0, 21, /* variation selector */
    0, 0, 0, 1, 0, 0, 'A', 1, 0, 1,	/* non-default UVS */
  };

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "MAXP", MAXP_data);
  HB_FACE_ADD_TABLE (face, "cmap", cmap_data);

  hb_font_t *font = hb_font_create (face);
  hb_codepoint_t glyph;
  g_assert_true (hb_font_get_variation_glyph (font, 'A', 0xFE0F, &glyph));
  g_assert_cmpuint (glyph, ==, 0x10001);
  hb_font_destroy (font);

  hb_set_t *set = hb_set_create ();
  hb_face_collect_variation_selectors (face, set);
  g_assert_true (hb_set_has (set, 0xFE0F));
  hb_set_clear (set);
  hb_face_collect_variation_unicodes (face, 0xFE0F, set);
  g_assert_true (hb_set_has (set, 'A'));
  hb_set_destroy (set);

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 'A');
  hb_set_add (codepoints, 0xFE0F);
  hb_face_t *subset = hb_subset_test_create_subset (face,
						    hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  font = hb_font_create (subset);
  g_assert_true (hb_font_get_variation_glyph (font, 'A', 0xFE0F, &glyph));
  g_assert_cmpuint (glyph, ==, 2);
  hb_font_destroy (font);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}
#endif

// TODO(rsheeter) test cmap to no codepoints

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_cmap);
  hb_test_add (test_subset_cmap_noop);
  hb_test_add (test_subset_cmap_non_consecutive_glyphs);
  hb_test_add (test_subset_cmap4_no_exceeding_maximum_codepoint);
  hb_test_add (test_subset_cmap_empty_tables);
  hb_test_add (test_subset_cmap_noto_color_emoji_noop);
  hb_test_add (test_subset_cmap_noto_color_emoji_non_consecutive_glyphs);
#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_subset_cmap_format15);
#endif

  return hb_test_run();
}
