/*
 * Copyright © 2026  Behdad Esfahbod
 *
 * This is part of HarfBuzz, a text shaping library.
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

#include "hb-buffer.hh"

int
main (int argc, char **argv)
{
  hb_always_assert (argc == 2);
  hb_face_t *face = hb_face_create_from_file_or_fail (argv[1], 0);
  hb_always_assert (face);
  hb_font_t *font = hb_font_create (face);
  const char *shapers[] = {"graphite2", nullptr};

  for (unsigned fail = 0; fail < 2; fail++)
  {
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_always_assert (hb_buffer_pre_allocate (buffer, 128));
    hb_buffer_add_utf8 (buffer, "aaaa", -1, 0, -1);
    hb_buffer_guess_segment_properties (buffer);
    hb_segment_properties_t props;
    hb_buffer_get_segment_properties (buffer, &props);
    hb_shape_plan_t *plan = hb_shape_plan_create (face, &props, nullptr, 0, shapers);
    hb_always_assert (!strcmp (hb_shape_plan_get_shaper (plan), "graphite2"));

    /* The font expands each character to 41 glyphs.  Scratch storage fits
     * in the existing allocation, but the 164 output glyphs do not.
     * Refuse output growth without depending on system memory pressure. */
    hb_always_assert (buffer->allocated < 164);
    hb_always_assert (buffer->allocated * sizeof (hb_glyph_position_t) >= 4 * 24 + 164 * 8);
    if (fail)
      buffer->max_len = buffer->allocated;

    hb_always_assert (bool (hb_shape_plan_execute (plan, font, buffer, nullptr, 0)) == !fail);
    hb_always_assert (bool (hb_buffer_allocation_successful (buffer)) == !fail);
    hb_always_assert (hb_buffer_get_length (buffer) == (fail ? 4u : 164u));

    hb_shape_plan_destroy (plan);
    hb_buffer_destroy (buffer);
  }

  hb_font_destroy (font);
  hb_face_destroy (face);
}
