/*
 * Copyright © 2026  Khaled Hosny
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

#include <hb-ot.h>

/* Unit tests for hb-ot-fetch.h */

static void
test_ot_bits_fs_type (void)
{
  hb_face_t *adwaita = hb_test_open_font_file ("fonts/adwaita.ttf");
  hb_face_t *zycon = hb_test_open_font_file ("fonts/Zycon.ttf");
  hb_face_t *abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");

  g_assert_cmpuint (hb_ot_fetch_bits (adwaita, HB_OT_BITS_TAG_FS_TYPE), ==,
		    0x0004u);
  g_assert_cmpuint (hb_ot_fetch_bits (zycon, HB_OT_BITS_TAG_FS_TYPE), ==, 1);
  g_assert_cmpuint (hb_ot_fetch_bits (abc, HB_OT_BITS_TAG_FS_TYPE), ==,
		    0x0000u);

  hb_face_destroy (abc);
  hb_face_destroy (zycon);
  hb_face_destroy (adwaita);
}

static void
test_ot_bits_fs_selection (void)
{
  hb_face_t *adwaita = hb_test_open_font_file ("fonts/adwaita.ttf");
  hb_face_t *bold = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_face_t *italic = hb_test_open_font_file ("fonts/notosansitalic.ttf");
  hb_face_t *abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");

  g_assert_cmpuint (hb_ot_fetch_bits (adwaita, HB_OT_BITS_TAG_FS_SELECTION), ==,
		    0x0040u | 0x0080u);
  g_assert_cmpuint (hb_ot_fetch_bits (bold, HB_OT_BITS_TAG_FS_SELECTION), ==,
		    0x0020u | 0x0080u);
  g_assert_cmpuint (hb_ot_fetch_bits (italic, HB_OT_BITS_TAG_FS_SELECTION), ==,
		    0x0001u | 0x0100u);

  /* USE_TYPO_METRICS */
  g_assert_true (hb_ot_fetch_bits (adwaita, HB_OT_BITS_TAG_FS_SELECTION)
		 & 0x0080u);
  g_assert_false (hb_ot_fetch_bits (abc, HB_OT_BITS_TAG_FS_SELECTION)
		  & 0x0080u);

  hb_face_destroy (abc);
  hb_face_destroy (italic);
  hb_face_destroy (bold);
  hb_face_destroy (adwaita);
}

static void
test_ot_bits_mac_style (void)
{
  hb_face_t *bold = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_face_t *italic = hb_test_open_font_file ("fonts/notosansitalic.ttf");
  hb_face_t *adwaita = hb_test_open_font_file ("fonts/adwaita.ttf");

  g_assert_cmpuint (hb_ot_fetch_bits (bold, HB_OT_BITS_TAG_MAC_STYLE), ==,
		    0x0001u);
  g_assert_cmpuint (hb_ot_fetch_bits (italic, HB_OT_BITS_TAG_MAC_STYLE), ==,
		    0x0002u);
  g_assert_cmpuint (hb_ot_fetch_bits (adwaita, HB_OT_BITS_TAG_MAC_STYLE), ==, 0);

  hb_face_destroy (adwaita);
  hb_face_destroy (italic);
  hb_face_destroy (bold);
}

static void
test_ot_bits_is_fixed_pitch (void)
{
  hb_face_t *mono = hb_test_open_font_file ("fonts/Inconsolata-Regular.abc.ttf");
  hb_face_t *adwaita = hb_test_open_font_file ("fonts/adwaita.ttf");

  g_assert_cmpuint (hb_ot_fetch_bits (mono, HB_OT_BITS_TAG_IS_FIXED_PITCH), !=, 0);
  g_assert_cmpuint (hb_ot_fetch_bits (adwaita, HB_OT_BITS_TAG_IS_FIXED_PITCH), ==, 0);

  hb_face_destroy (adwaita);
  hb_face_destroy (mono);
}

static void
test_ot_bits_ranges (void)
{
  hb_face_t *bold = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_face_t *mplus = hb_test_open_font_file ("fonts/Mplus1p-Regular.ttf");
  hb_face_t *zycon = hb_test_open_font_file ("fonts/Zycon.ttf");

  g_assert_cmpuint (hb_ot_fetch_bits (bold, HB_OT_BITS_TAG_UNICODE_RANGE_1), ==, 0xE00002FFu);
  g_assert_cmpuint (hb_ot_fetch_bits (bold, HB_OT_BITS_TAG_UNICODE_RANGE_2), ==, 0x4000201Fu);
  g_assert_cmpuint (hb_ot_fetch_bits (bold, HB_OT_BITS_TAG_UNICODE_RANGE_3), ==, 0x08000029u);
  g_assert_cmpuint (hb_ot_fetch_bits (bold, HB_OT_BITS_TAG_UNICODE_RANGE_4), ==, 0x00100000u);

  g_assert_cmpuint (hb_ot_fetch_bits (mplus, HB_OT_BITS_TAG_CODE_PAGE_RANGE_1), ==, 0x601201BFu);
  g_assert_cmpuint (hb_ot_fetch_bits (mplus, HB_OT_BITS_TAG_CODE_PAGE_RANGE_2), ==, 0xDFF70000u);

  /* OS/2 v0 does not have code page ranges */
  g_assert_cmpuint (hb_ot_fetch_bits (zycon, HB_OT_BITS_TAG_CODE_PAGE_RANGE_1), ==, 0);
  g_assert_cmpuint (hb_ot_fetch_bits (zycon, HB_OT_BITS_TAG_CODE_PAGE_RANGE_2), ==, 0);

  hb_face_destroy (zycon);
  hb_face_destroy (mplus);
  hb_face_destroy (bold);
}

static void
test_ot_bits_missing_tables (void)
{
  /* No OS/2 or post tables */
  hb_face_t *face = hb_test_open_font_file ("fonts/meta.ttf");

  g_assert_cmpuint (hb_ot_fetch_bits (face, HB_OT_BITS_TAG_FS_TYPE), ==, 0);
  g_assert_cmpuint (hb_ot_fetch_bits (face, HB_OT_BITS_TAG_FS_SELECTION), ==, 0);
  g_assert_cmpuint (hb_ot_fetch_bits (face, HB_OT_BITS_TAG_IS_FIXED_PITCH), ==, 0);
  g_assert_cmpuint (hb_ot_fetch_bits (face, HB_OT_BITS_TAG_UNICODE_RANGE_1), ==, 0);
  g_assert_cmpuint (hb_ot_fetch_bits (face, HB_OT_BITS_TAG_CODE_PAGE_RANGE_1), ==, 0);

  hb_face_destroy (face);
}

static void
test_ot_bits_invalid (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/adwaita.ttf");

  g_assert_cmpuint (hb_ot_fetch_bits (face, (hb_ot_bits_tag_t) HB_TAG ('x','x','x','x')), ==, 0);
  g_assert_cmpuint (hb_ot_fetch_bits (hb_face_get_empty (), HB_OT_BITS_TAG_FS_TYPE), ==, 0);

  hb_face_destroy (face);
}

static void
test_ot_number_bounding_box (void)
{
  hb_face_t *adwaita = hb_test_open_font_file ("fonts/adwaita.ttf");
  hb_face_t *abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");

  g_assert_cmpint (hb_ot_fetch_number (adwaita, HB_OT_NUMBER_TAG_FONT_X_MIN), ==, 51);
  g_assert_cmpint (hb_ot_fetch_number (adwaita, HB_OT_NUMBER_TAG_FONT_Y_MIN), ==, -250);
  g_assert_cmpint (hb_ot_fetch_number (adwaita, HB_OT_NUMBER_TAG_FONT_X_MAX), ==, 1238);
  g_assert_cmpint (hb_ot_fetch_number (adwaita, HB_OT_NUMBER_TAG_FONT_Y_MAX), ==, 950);

  g_assert_cmpint (hb_ot_fetch_number (abc, HB_OT_NUMBER_TAG_FONT_X_MIN), ==, -454);
  g_assert_cmpint (hb_ot_fetch_number (abc, HB_OT_NUMBER_TAG_FONT_Y_MIN), ==, -293);
  g_assert_cmpint (hb_ot_fetch_number (abc, HB_OT_NUMBER_TAG_FONT_X_MAX), ==, 2159);
  g_assert_cmpint (hb_ot_fetch_number (abc, HB_OT_NUMBER_TAG_FONT_Y_MAX), ==, 968);

  hb_face_destroy (adwaita);
  hb_face_destroy (abc);
}

static void
test_ot_number_invalid (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/adwaita.ttf");

  g_assert_cmpint (hb_ot_fetch_number (face, (hb_ot_number_tag_t) HB_TAG ('x','x','x','x')), ==, 0);
  g_assert_cmpint (hb_ot_fetch_number (hb_face_get_empty (), HB_OT_NUMBER_TAG_FONT_X_MIN), ==, 0);

  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_ot_bits_fs_type);
  hb_test_add (test_ot_bits_fs_selection);
  hb_test_add (test_ot_bits_mac_style);
  hb_test_add (test_ot_bits_is_fixed_pitch);
  hb_test_add (test_ot_bits_ranges);
  hb_test_add (test_ot_bits_missing_tables);
  hb_test_add (test_ot_bits_invalid);
  hb_test_add (test_ot_number_bounding_box);
  hb_test_add (test_ot_number_invalid);

  return hb_test_run ();
}
