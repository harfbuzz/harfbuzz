/*
 * Copyright © 2026  Google, Inc.
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

static void
test_graphite2_repeated_clusters (void)
{
  const char *shapers[] = {"graphite2", NULL};
  hb_face_t *face;
  hb_font_t *font;
  hb_buffer_t *buffer;
  hb_glyph_info_t *info;
  hb_glyph_position_t *position;
  unsigned int length;
  hb_position_t advance = 0;

  face = hb_test_open_font_file ("../shape/data/in-house/fonts/2f652d30fd15eea6be23fd9bdc1099836975aa83.ttf");
  font = hb_font_create (face);
  hb_face_destroy (face);

  buffer = hb_buffer_create ();
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  hb_buffer_set_direction (buffer, HB_DIRECTION_RTL);
  hb_buffer_set_script (buffer, HB_SCRIPT_ARABIC);
  hb_buffer_add (buffer, 0x0660, 0);
  hb_buffer_add (buffer, 0x0600, 3);
  hb_buffer_add (buffer, 0x0600, 3);

  g_assert_true (hb_shape_full (font, buffer, NULL, 0, shapers));

  info = hb_buffer_get_glyph_infos (buffer, &length);
  position = hb_buffer_get_glyph_positions (buffer, NULL);
  g_assert_cmpuint (length, ==, 3);
  for (unsigned int i = 0; i < length; i++)
  {
    g_assert_cmpuint (info[i].cluster, ==, 0);
    advance += position[i].x_advance;
  }
  g_assert_cmpint (advance, ==, 3200);

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_graphite2_repeated_clusters);

  return hb_test_run ();
}
