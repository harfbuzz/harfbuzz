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

/* Unit tests for hb-common.h */


static void
test_types_int (void)
{
  /* We already ASSERT_STATIC these in hb-private.h, but anyway */
  g_assert_cmpint (sizeof (int8_t), ==, 1);
  g_assert_cmpint (sizeof (uint8_t), ==, 1);
  g_assert_cmpint (sizeof (int16_t), ==, 2);
  g_assert_cmpint (sizeof (uint16_t), ==, 2);
  g_assert_cmpint (sizeof (int32_t), ==, 4);
  g_assert_cmpint (sizeof (uint32_t), ==, 4);
  g_assert_cmpint (sizeof (int64_t), ==, 8);
  g_assert_cmpint (sizeof (uint64_t), ==, 8);

  g_assert_cmpint (sizeof (hb_codepoint_t), ==, 4);
  g_assert_cmpint (sizeof (hb_position_t), ==, 4);
  g_assert_cmpint (sizeof (hb_mask_t), ==, 4);
  g_assert_cmpint (sizeof (hb_var_int_t), ==, 4);
}

static void
test_types_direction (void)
{
  g_assert_cmpint ((signed) HB_DIRECTION_INVALID, ==, -1);
  g_assert_cmpint (HB_DIRECTION_LTR, ==, 0);

  g_assert (HB_DIRECTION_IS_HORIZONTAL (HB_DIRECTION_LTR));
  g_assert (HB_DIRECTION_IS_HORIZONTAL (HB_DIRECTION_RTL));
  g_assert (!HB_DIRECTION_IS_HORIZONTAL (HB_DIRECTION_TTB));
  g_assert (!HB_DIRECTION_IS_HORIZONTAL (HB_DIRECTION_BTT));

  g_assert (!HB_DIRECTION_IS_VERTICAL (HB_DIRECTION_LTR));
  g_assert (!HB_DIRECTION_IS_VERTICAL (HB_DIRECTION_RTL));
  g_assert (HB_DIRECTION_IS_VERTICAL (HB_DIRECTION_TTB));
  g_assert (HB_DIRECTION_IS_VERTICAL (HB_DIRECTION_BTT));

  g_assert (HB_DIRECTION_IS_FORWARD (HB_DIRECTION_LTR));
  g_assert (HB_DIRECTION_IS_FORWARD (HB_DIRECTION_TTB));
  g_assert (!HB_DIRECTION_IS_FORWARD (HB_DIRECTION_RTL));
  g_assert (!HB_DIRECTION_IS_FORWARD (HB_DIRECTION_BTT));

  g_assert (!HB_DIRECTION_IS_BACKWARD (HB_DIRECTION_LTR));
  g_assert (!HB_DIRECTION_IS_BACKWARD (HB_DIRECTION_TTB));
  g_assert (HB_DIRECTION_IS_BACKWARD (HB_DIRECTION_RTL));
  g_assert (HB_DIRECTION_IS_BACKWARD (HB_DIRECTION_BTT));

  g_assert_cmpint (HB_DIRECTION_REVERSE (HB_DIRECTION_LTR), ==, HB_DIRECTION_RTL);
  g_assert_cmpint (HB_DIRECTION_REVERSE (HB_DIRECTION_RTL), ==, HB_DIRECTION_LTR);
  g_assert_cmpint (HB_DIRECTION_REVERSE (HB_DIRECTION_TTB), ==, HB_DIRECTION_BTT);
  g_assert_cmpint (HB_DIRECTION_REVERSE (HB_DIRECTION_BTT), ==, HB_DIRECTION_TTB);

  g_assert_cmpint (HB_DIRECTION_INVALID, ==, hb_direction_from_string (NULL));
  g_assert_cmpint (HB_DIRECTION_INVALID, ==, hb_direction_from_string (""));
  g_assert_cmpint (HB_DIRECTION_INVALID, ==, hb_direction_from_string ("x"));
  g_assert_cmpint (HB_DIRECTION_RTL, ==, hb_direction_from_string ("r"));
  g_assert_cmpint (HB_DIRECTION_RTL, ==, hb_direction_from_string ("rtl"));
  g_assert_cmpint (HB_DIRECTION_RTL, ==, hb_direction_from_string ("RtL"));
  g_assert_cmpint (HB_DIRECTION_RTL, ==, hb_direction_from_string ("right-to-left"));
  g_assert_cmpint (HB_DIRECTION_TTB, ==, hb_direction_from_string ("ttb"));
}

static void
test_types_tag (void)
{
  g_assert_cmphex (HB_TAG_NONE, ==, 0);

  g_assert_cmphex (HB_TAG ('a','B','c','D'), ==, 0x61426344);

  g_assert_cmphex (hb_tag_from_string ("aBcDe"), ==, 0x61426344);
  g_assert_cmphex (hb_tag_from_string ("aBcD"),  ==, 0x61426344);
  g_assert_cmphex (hb_tag_from_string ("aBc"),   ==, 0x61426320);
  g_assert_cmphex (hb_tag_from_string ("aB"),    ==, 0x61422020);
  g_assert_cmphex (hb_tag_from_string ("a"),     ==, 0x61202020);

  g_assert_cmphex (hb_tag_from_string (""),      ==, HB_TAG_NONE);
  g_assert_cmphex (hb_tag_from_string (NULL),    ==, HB_TAG_NONE);
}

static void
test_types_script (void)
{
  hb_tag_t arab = HB_TAG_CHAR4 ("arab");
  hb_tag_t Arab = HB_TAG_CHAR4 ("Arab");
  hb_tag_t ARAB = HB_TAG_CHAR4 ("ARAB");

  hb_tag_t wWyZ = HB_TAG_CHAR4 ("wWyZ");
  hb_tag_t Wwyz = HB_TAG_CHAR4 ("Wwyz");

  hb_tag_t x123 = HB_TAG_CHAR4 ("x123");

  g_assert_cmpint (HB_SCRIPT_INVALID, ==, (hb_script_t) HB_TAG_NONE);
  g_assert_cmphex (HB_SCRIPT_ARABIC, !=, HB_SCRIPT_LATIN);

  g_assert_cmphex (HB_SCRIPT_INVALID, ==, hb_script_from_string (NULL));
  g_assert_cmphex (HB_SCRIPT_INVALID, ==, hb_script_from_string (""));
  g_assert_cmphex (HB_SCRIPT_UNKNOWN, ==, hb_script_from_string ("x"));

  g_assert_cmphex (HB_SCRIPT_ARABIC, ==, hb_script_from_string ("arab"));
  g_assert_cmphex (HB_SCRIPT_ARABIC, ==, hb_script_from_string ("Arab"));
  g_assert_cmphex (HB_SCRIPT_ARABIC, ==, hb_script_from_string ("ARAB"));

  g_assert_cmphex (HB_SCRIPT_ARABIC, ==, hb_script_from_iso15924_tag (arab));
  g_assert_cmphex (HB_SCRIPT_ARABIC, ==, hb_script_from_iso15924_tag (Arab));
  g_assert_cmphex (HB_SCRIPT_ARABIC, ==, hb_script_from_iso15924_tag (ARAB));

  /* Arbitrary tags that look like may be valid ISO 15924 should be preserved. */
  g_assert_cmphex (HB_SCRIPT_UNKNOWN, !=, hb_script_from_string ("wWyZ"));
  g_assert_cmphex (HB_SCRIPT_UNKNOWN, !=, hb_script_from_iso15924_tag (wWyZ));
  /* Otherwise, UNKNOWN should be returned. */
  g_assert_cmphex (HB_SCRIPT_UNKNOWN, ==, hb_script_from_string ("x123"));
  g_assert_cmphex (HB_SCRIPT_UNKNOWN, ==, hb_script_from_iso15924_tag (x123));

  g_assert_cmphex (hb_script_to_iso15924_tag (HB_SCRIPT_ARABIC), ==, Arab);
  g_assert_cmphex (hb_script_to_iso15924_tag (hb_script_from_iso15924_tag (wWyZ)), ==, Wwyz);

  g_assert_cmpint (hb_script_get_horizontal_direction (HB_SCRIPT_LATIN), ==, HB_DIRECTION_LTR);
  g_assert_cmpint (hb_script_get_horizontal_direction (HB_SCRIPT_ARABIC), ==, HB_DIRECTION_RTL);
  g_assert_cmpint (hb_script_get_horizontal_direction (hb_script_from_iso15924_tag (wWyZ)), ==, HB_DIRECTION_LTR);
}

static void
test_types_language (void)
{
  hb_language_t fa = hb_language_from_string ("fa");
  hb_language_t fa_IR = hb_language_from_string ("fa_IR");
  hb_language_t fa_ir = hb_language_from_string ("fa-ir");
  hb_language_t en = hb_language_from_string ("en");

  g_assert (fa != NULL);
  g_assert (fa_IR != NULL);
  g_assert (fa_IR == fa_ir);

  g_assert (en != NULL);
  g_assert (en != fa);

  /* Test recall */
  g_assert (en == hb_language_from_string ("en"));
  g_assert (en == hb_language_from_string ("eN"));

  g_assert (NULL == hb_language_from_string (NULL));
  g_assert (NULL == hb_language_from_string (""));
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_types_int);
  hb_test_add (test_types_direction);
  hb_test_add (test_types_tag);
  hb_test_add (test_types_script);
  hb_test_add (test_types_language);

  return hb_test_run();
}
