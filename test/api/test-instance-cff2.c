/*
 * Copyright © 2022  Google, Inc.
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
 * Google Author(s): Garret Rieger
 */

#include "hb-test.h"
#include "hb-subset-test.h"


static void
test_instance_cff2 (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *expected = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.static.otf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);

  hb_subset_input_t* input = hb_subset_test_create_input (codepoints);
  hb_subset_input_pin_axis_location (input,
                                     face_abc,
                                     HB_TAG ('w', 'g', 'h', 't'),
                                     250);
  hb_subset_input_pin_axis_location (input,
                                     face_abc,
                                     HB_TAG ('C', 'N', 'T', 'R'),
                                     0);

  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (expected, face_abc_subset, HB_TAG ('C','F','F', '2'));
  g_assert_true (hb_face_reference_table (face_abc_subset, HB_TAG ('f', 'v', 'a', 'r'))
		 == hb_blob_get_empty ());

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (expected);
}


static void
test_instance_cff2_private_blend_overflow (void)
{
  /* StdVW blend delta of 1e12; partial instancing must not convert the
   * transformed delta to int out of range. */
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.huge-blend.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 97);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  g_assert_true (hb_subset_input_set_axis_range (input, face,
						 HB_TAG ('w', 'g', 'h', 't'),
						 300, 600, 300));

  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_nonnull (subset);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}


static void
test_instance_cff2_empty_bounds (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2-empty.otf");
  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 32);
  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  g_assert_true (hb_subset_input_pin_axis_location (input, face,
                                                   HB_TAG ('w', 'g', 'h', 't'), 0.5f));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_nonnull (subset);

  hb_blob_t *head = hb_face_reference_table (subset, HB_TAG ('h', 'e', 'a', 'd'));
  unsigned length;
  const char *data = hb_blob_get_data (head, &length);
  const char zero_bounds[8] = {0};
  g_assert_cmpuint (length, >=, 44);
  g_assert_cmpmem (data + 36, 8, zero_bounds, 8);
  hb_blob_destroy (head);

  /* Empty outlines must still retain their advance and original bearing. */
  hb_blob_t *hmtx = hb_face_reference_table (subset, HB_TAG ('h', 'm', 't', 'x'));
  data = hb_blob_get_data (hmtx, &length);
  const char metric[4] = {1, (char) 0xF4, 0, 50};
  g_assert_cmpuint (length, >=, 4);
  g_assert_cmpmem (data, 4, metric, 4);
  hb_blob_destroy (hmtx);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}


static void
test_instance_cff2_large_store (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2-large-store.otf");
  hb_set_t *codepoints = hb_set_create ();
  hb_set_add_range (codepoints, 65, 264);
  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  g_assert_true (hb_subset_input_set_axis_range (input, face,
                                                HB_TAG ('w', 'g', 'h', 't'),
                                                0, 1, 0.1f));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_nonnull (subset);

  /* All 200 regions are used, so compaction cannot hide the large store. */
  hb_blob_t *cff2 = hb_face_reference_table (subset, HB_TAG ('C', 'F', 'F', '2'));
  g_assert_cmpuint (hb_blob_get_length (cff2), >, 65535);
  hb_blob_destroy (cff2);
  hb_font_t *font = hb_font_create (subset);
  hb_glyph_extents_t extents;
  hb_codepoint_t glyph;
  g_assert_true (hb_font_get_nominal_glyph (font, 65, &glyph));
  for (unsigned i = 0; i < 3; i++)
  {
    hb_variation_t variation = {HB_TAG ('w', 'g', 'h', 't'), i * 0.5f};
    hb_font_set_variations (font, &variation, 1);
    g_assert_true (hb_font_get_glyph_extents (font, glyph, &extents));
    g_assert_cmpint (extents.width, >, 0);
  }
  hb_font_destroy (font);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_instance_cff2);
  hb_test_add (test_instance_cff2_private_blend_overflow);
  hb_test_add (test_instance_cff2_empty_bounds);
  hb_test_add (test_instance_cff2_large_store);

  return hb_test_run();
}
