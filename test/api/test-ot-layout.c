/*
 * Copyright © 2021  Khaled Hosny
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

#define STATIC_ARRAY_SIZE 255

static void
test_ot_layout_table_get_script_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  unsigned int total = 0;
  unsigned int count = STATIC_ARRAY_SIZE;
  unsigned int offset = 0;
  hb_tag_t tags[STATIC_ARRAY_SIZE];
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_table_get_script_tags (face, HB_OT_TAG_GSUB, offset, &count, tags);
    g_assert_cmpuint (3, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (3, ==, count);
      g_assert_cmpuint (HB_TAG ('a','r','a','b'), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('d','f','l','t'), ==, tags[1]);
      g_assert_cmpuint (HB_TAG ('l','a','t','n'), ==, tags[2]);
    }
  }
  count = STATIC_ARRAY_SIZE;
  offset = 0;
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_table_get_script_tags (face, HB_OT_TAG_GPOS, offset, &count, tags);
    g_assert_cmpuint (1, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (1, ==, count);
      g_assert_cmpuint (HB_TAG ('a','r','a','b'), ==, tags[0]);
    }
  }

  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_ot_layout_table_get_script_tags);
  return hb_test_run ();
}
