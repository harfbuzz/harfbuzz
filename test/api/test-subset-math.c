/*
 * Copyright © 2025  Adobe, Inc.
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
 * Adobe Author(s): Skef Iterum
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for MATH table closure during subsetting.
 *
 * MATH glyph constructions (variants and assembly parts) can reference
 * glyphs that themselves have MATH constructions.  The closure must
 * follow these transitive dependencies; a single pass is not enough.
 *
 * STIXTwoMath U+21D4 (⇔) exercises a two-hop chain:
 *
 *   1582 (⇔) assembly includes 1579 (⇒) and 1574 (⇐)
 *   1579 (⇒) assembly includes 4827 (uni21D0.endl)
 *   1574 (⇐) assembly includes 4828 (uni21D0.endr)
 *
 * A single-pass closure retains 1579 and 1574 but misses 4827 and 4828,
 * causing the MATH table serializer to fail on absent glyph references.
 */

static void
test_subset_math_closure_transitive (void)
{
  hb_face_t *face = hb_test_open_font_file ("../subset/data/fonts/STIXTwoMath-Regular.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 0x21D4);  /* ⇔ */
  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);

  /* Create a subset plan and check retained glyphs. */
  hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
  g_assert_nonnull (plan);

  hb_map_t *glyph_map = hb_subset_plan_old_to_new_glyph_mapping (plan);
  hb_set_t *retained = hb_set_create ();
  {
    int idx = -1;
    hb_codepoint_t key, value;
    while (hb_map_next (glyph_map, &idx, &key, &value))
      hb_set_add (retained, key);
  }

  /* First-hop assembly parts of ⇔ */
  g_assert_true (hb_set_has (retained, 1574));  /* uni21D0 (⇐) */
  g_assert_true (hb_set_has (retained, 1575));  /* uni21D0.x  */
  g_assert_true (hb_set_has (retained, 1579));  /* uni21D2 (⇒) */

  /* Second-hop: assembly parts of ⇒ and ⇐ */
  g_assert_true (hb_set_has (retained, 4827));  /* uni21D0.endl */
  g_assert_true (hb_set_has (retained, 4828));  /* uni21D0.endr */

  hb_set_destroy (retained);
  hb_subset_plan_destroy (plan);

  /* Also verify that the full subset succeeds. */
  input = hb_subset_test_create_input (hb_set_create ());
  hb_set_add (hb_subset_input_unicode_set (input), 0x21D4);
  hb_face_t *subset = hb_subset_test_create_subset (face, input);
  g_assert_nonnull (subset);

  /* Subset should have MATH table. */
  hb_blob_t *math_blob = hb_face_reference_table (subset, HB_TAG ('M','A','T','H'));
  g_assert_cmpuint (hb_blob_get_length (math_blob), >, 0);
  hb_blob_destroy (math_blob);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_math_closure_transitive);

  return hb_test_run ();
}
