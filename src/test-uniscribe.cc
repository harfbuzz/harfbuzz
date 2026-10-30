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
 */

#include "hb.hh"
#include "hb-uniscribe.h"

static void
test_overlapping_faces (bool overlap)
{
  /* Both directories are valid initially.  When they overlap, updating
   * face 0's name record changes face 1's table count in the output copy.
   * The renamer must continue reading directories from the original. */
  char data[128] = {};
  unsigned face0 = overlap ? 46 : 36;
  data[0] = 't'; data[1] = 't'; data[2] = 'c'; data[3] = 'f';
  data[5] = 1; /* TTC version 1 */
  data[11] = 2;
  data[15] = face0;
  data[19] = 64;
  data[face0 + 1] = 1; /* sfnt version 1 */
  data[face0 + 5] = 1; /* one table */
  data[face0 + 7] = 16;
  hb_memcpy (data + face0 + 12, "name", 4);

  for (unsigned i = 0; i < 2; i++)
  {
    hb_blob_t *blob = hb_blob_create (data, sizeof (data),
                                    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
    hb_always_assert (hb_face_count (blob) == 2);
    hb_face_t *face = hb_face_create (blob, i);
    hb_font_t *font = hb_font_create (face);
    /* GDI may reject these minimal fonts.  Either result is acceptable;
     * preparing the font must not access beyond the renamed blob. */
    hb_uniscribe_font_get_hfont (font);
    hb_font_destroy (font);
    hb_face_destroy (face);
    hb_blob_destroy (blob);
  }
}

int
main ()
{
  test_overlapping_faces (false);
  test_overlapping_faces (true);
  return 0;
}
