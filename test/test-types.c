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

/* This file tests types defined in hb-common.h */

static void
test_types_int (void)
{
  /* We already ASSERT_STATIC these in hb-common.h, but anyway */
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
  g_assert_cmpint ((signed) HB_DIRECTION_INVALID, <, 0);
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
}

static void
test_types_tag (void)
{
  g_assert_cmphex (HB_TAG_NONE, ==, 0);
  g_assert_cmphex (HB_TAG ('a','B','c','D'), ==, 0x61426344);

  g_assert_cmphex (HB_TAG_CHAR4 ("aBcD"), ==, 0x61426344);

  g_assert_cmphex (hb_tag_from_string ("aBcDe"), ==, 0x61426344);
  g_assert_cmphex (hb_tag_from_string ("aBcD"),  ==, 0x61426344);
  g_assert_cmphex (hb_tag_from_string ("aBc"),   ==, 0x61426320);
  g_assert_cmphex (hb_tag_from_string ("aB"),    ==, 0x61422020);
  g_assert_cmphex (hb_tag_from_string ("a"),     ==, 0x61202020);
  g_assert_cmphex (hb_tag_from_string (""),      ==, 0x20202020);
}

int
main (int   argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/types/direction", test_types_direction);
  g_test_add_func ("/types/int", test_types_int);
  g_test_add_func ("/types/tag", test_types_tag);

  return g_test_run();
}
