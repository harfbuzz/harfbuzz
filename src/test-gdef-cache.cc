/*
 * Copyright © 2026  Behdad Esfahbod
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
 * Author(s): Behdad Esfahbod
 */

#include "hb-ot-layout-gdef-table.hh"

int
main ()
{
#if !defined(HB_NO_GDEF_CACHE) && !defined(HB_NO_OT_LAYOUT)
  /* Empty, singleton, sparse, unaligned ranges, all glyphs, and spans
   * immediately below and above the exact-cache limit. */
  static const unsigned char data[] = {
    0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14,
    0, 1, 0, 7,
    0, 0, 0, 32, 0, 0, 0, 36, 0, 0, 0, 42, 0, 0, 0, 56,
    0, 0, 0, 72, 0, 0, 0, 82, 0, 0, 0, 92,
    0, 1, 0, 0,
    0, 1, 0, 1, 0, 37,
    0, 1, 0, 5, 0, 37, 0, 63, 0, 64, 0, 65, 0, 100,
    0, 2, 0, 2, 0, 37, 0, 102, 0, 0, 0, 150, 0, 159, 0, 66,
    0, 2, 0, 1, 0, 0, 255, 255, 0, 0,
    0, 2, 0, 1, 0, 37, 2, 36, 0, 0,
    0, 2, 0, 1, 0, 37, 2, 37, 0, 0,
  };
  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create ((const char *) data, sizeof (data),
                                  HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_always_assert (hb_face_builder_add_table (face, HB_OT_TAG_GDEF, blob));
  hb_blob_destroy (blob);
  hb_ot_layout_has_glyph_classes (face);
  const auto *accel = face->table.GDEF.get_relaxed ();
  hb_always_assert (accel && accel->mark_glyph_set_bitmaps.length == 7);
  for (unsigned set = 0; set < 7; set++)
  {
    bool exact = set == 1 || set == 2 || set == 3 || set == 5;
    hb_always_assert (accel->mark_glyph_set_bitmaps[set].glyphs.in_error () == !exact);
    for (unsigned glyph = 0; glyph <= 65536; glyph++)
      hb_always_assert (accel->mark_set_covers (set, glyph) ==
                        accel->table->mark_set_covers (set, glyph));
    hb_always_assert (!accel->mark_set_covers (set, UINT_MAX));
  }
  hb_face_destroy (face);
#endif
}
