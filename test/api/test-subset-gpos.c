/*
 * Copyright © 2020 Adobe Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for GPOS subsetting */

static void
test_subset_gpos_lookup_subtable (void)
{
  hb_face_t *face_pwa = hb_test_open_font_file ("fonts/Roboto-Regular-gpos-.aw.ttf");
  hb_face_t *face_wa = hb_test_open_font_file ("fonts/Roboto-Regular-gpos-aw.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_pwa_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'w');

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);

  hb_set_del (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG),
              HB_TAG ('G', 'P', 'O', 'S'));

  face_pwa_subset = hb_subset_test_create_subset (face_pwa, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_wa, face_pwa_subset, HB_TAG ('G','P','O','S'));

  hb_face_destroy (face_pwa_subset);
  hb_face_destroy (face_pwa);
  hb_face_destroy (face_wa);
}

static void
test_subset_gpos_pairpos1_vf (void)
{
  hb_face_t *face_wav = hb_test_open_font_file ("fonts/AdobeVFPrototype.WAV.gpos.otf");
  hb_face_t *face_wa = hb_test_open_font_file ("fonts/AdobeVFPrototype.WA.gpos.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_wav_subset;
  hb_set_add (codepoints, 'W');
  hb_set_add (codepoints, 'A');

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);

  hb_set_del (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG),
              HB_TAG ('G', 'P', 'O', 'S'));

  face_wav_subset = hb_subset_test_create_subset (face_wav, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_wa, face_wav_subset, HB_TAG ('G','P','O','S'));

  hb_face_destroy (face_wav_subset);
  hb_face_destroy (face_wav);
  hb_face_destroy (face_wa);
}

static void
test_subset_gpos_device_only_pairpos (void)
{
  const char *filenames[] = {"fonts/device-only-pairpos1.ttf",
                             "fonts/device-only-pairpos2.ttf"};
  for (unsigned f = 0; f < G_N_ELEMENTS (filenames); f++)
  {
    hb_face_t *face = hb_test_open_font_file (filenames[f]);
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
    hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'D');
    g_assert_true (hb_subset_input_pin_axis_location (
        input, face, HB_TAG ('T', 'E', 'S', 'T'), .25f));
    hb_face_t *subset = hb_subset_or_fail (face, input);
    g_assert_nonnull (subset);
    const int remaining[] = {0, 8192, 16384};
    for (unsigned j = 0; j < G_N_ELEMENTS (remaining); j++)
    {
      hb_font_t *fonts[] = {hb_font_create (face), hb_font_create (subset)};
      const int coords[] = {4096, remaining[j]};
      hb_font_set_var_coords_normalized (fonts[0], coords, 2);
      hb_font_set_var_coords_normalized (fonts[1], &remaining[j], 1);
      hb_buffer_t *buffers[] = {hb_buffer_create (), hb_buffer_create ()};
      for (unsigned i = 0; i < 2; i++)
      {
        hb_buffer_add_utf8 (buffers[i], "ABCD", -1, 0, -1);
        hb_buffer_guess_segment_properties (buffers[i]);
        hb_shape (fonts[i], buffers[i], NULL, 0);
      }
      unsigned length, subset_length;
      const hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffers[0], &length);
      const hb_glyph_position_t *subset_pos = hb_buffer_get_glyph_positions (buffers[1], &subset_length);
      g_assert_cmpuint (length, ==, 4);
      g_assert_cmpuint (subset_length, ==, length);
      for (unsigned i = 0; i < length; i++)
      {
        g_assert_cmpint (subset_pos[i].x_advance, ==, pos[i].x_advance);
        g_assert_cmpint (subset_pos[i].x_offset, ==, pos[i].x_offset);
      }
      for (unsigned i = 0; i < 2; i++)
      {
        hb_buffer_destroy (buffers[i]);
        hb_font_destroy (fonts[i]);
      }
    }
    hb_face_destroy (subset);
    hb_subset_input_destroy (input);
    hb_face_destroy (face);
  }
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_gpos_lookup_subtable);
  hb_test_add (test_subset_gpos_pairpos1_vf);
  hb_test_add (test_subset_gpos_device_only_pairpos);

  return hb_test_run ();
}
