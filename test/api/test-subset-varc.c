/*
 * Copyright © 2026  Google, Inc.
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

#include "hb-test.h"
#include "hb-subset-test.h"

static void
assert_has_varc (hb_face_t *face, hb_bool_t expected)
{
  hb_blob_t *blob = hb_face_reference_table (face, HB_TAG ('V','A','R','C'));
  g_assert_cmpint (hb_blob_get_length (blob) != 0, ==, expected);
  hb_blob_destroy (blob);
}

static hb_face_t *
subset_varc (hb_face_t *face, hb_codepoint_t unicode)
{
  hb_set_t *unicodes = hb_set_create ();
  hb_set_add (unicodes, unicode);
  hb_face_t *subset = hb_subset_test_create_subset (
    face, hb_subset_test_create_input (unicodes));
  hb_set_destroy (unicodes);
  return subset;
}

static void
test_subset_varc_closure_and_remap (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac00-ac01.ttf");

  hb_face_t *ac00 = subset_varc (face, 0xAC00);
  g_assert_cmpuint (hb_face_get_glyph_count (ac00), ==, 6);
  assert_has_varc (ac00, true);

  hb_face_t *ac01 = subset_varc (face, 0xAC01);
  g_assert_cmpuint (hb_face_get_glyph_count (ac01), ==, 8);
  assert_has_varc (ac01, true);

  hb_face_destroy (ac01);
  hb_face_destroy (ac00);
  hb_face_destroy (face);
}

static void
test_subset_varc_condition_closure (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac01-conditional.ttf");
  hb_face_t *subset = subset_varc (face, 0xAC01);

  /* Closure conservatively retains components regardless of their condition. */
  g_assert_cmpuint (hb_face_get_glyph_count (subset), ==, 8);
  assert_has_varc (subset, true);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_varc_retain_gids (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac00-ac01.ttf");
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add (hb_subset_input_unicode_set (input), 0xAC00);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);

  hb_face_t *subset = hb_subset_test_create_subset (face, input);
  g_assert_cmpuint (hb_face_get_glyph_count (subset), ==, 7);
  assert_has_varc (subset, true);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_varc_fails_when_instancing (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac00-ac01.ttf");
  hb_set_t *unicodes = hb_set_create ();
  hb_set_add (unicodes, 0xAC00);
  hb_subset_input_t *input = hb_subset_test_create_input (unicodes);
  hb_set_destroy (unicodes);

  g_assert_true (hb_subset_input_pin_axis_location (input, face,
						    HB_TAG ('w','g','h','t'), 400));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_null (subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

static void
test_subset_varc_can_be_explicitly_dropped_when_instancing (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac00-ac01.ttf");
  hb_set_t *unicodes = hb_set_create ();
  hb_set_add (unicodes, 0xAC00);
  hb_subset_input_t *input = hb_subset_test_create_input (unicodes);
  hb_set_destroy (unicodes);

  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG),
	      HB_TAG ('V','A','R','C'));
  g_assert_true (hb_subset_input_pin_axis_location (input, face,
						    HB_TAG ('w','g','h','t'), 400));
  hb_face_t *subset = hb_subset_test_create_subset (face, input);
  assert_has_varc (subset, false);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static hb_subset_input_t *
create_varc_no_subset_input (hb_bool_t retain_gids)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add (hb_subset_input_unicode_set (input), 0xAC00);
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_NO_SUBSET_TABLE_TAG),
	      HB_TAG ('V','A','R','C'));
  if (retain_gids)
    hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  return input;
}

static void
test_subset_varc_passthrough_requires_retained_gids (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/varc-ac00-ac01.ttf");

  hb_subset_input_t *input = create_varc_no_subset_input (false);
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_null (subset);
  hb_subset_input_destroy (input);

  input = create_varc_no_subset_input (true);
  subset = hb_subset_test_create_subset (face, input);
  assert_has_varc (subset, true);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_subset_varc_closure_and_remap);
  hb_test_add (test_subset_varc_condition_closure);
  hb_test_add (test_subset_varc_retain_gids);
  hb_test_add (test_subset_varc_fails_when_instancing);
  hb_test_add (test_subset_varc_can_be_explicitly_dropped_when_instancing);
  hb_test_add (test_subset_varc_passthrough_requires_retained_gids);
  return hb_test_run ();
}
