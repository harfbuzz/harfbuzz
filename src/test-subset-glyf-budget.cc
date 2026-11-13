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

#include "hb-subset.hh"
#include "hb-ot-glyf-table.hh"

static int64_t
subset_glyf (const char *path, int64_t budget, bool expected)
{
  hb_blob_t *font_blob = hb_blob_create_from_file_or_fail (path);
  hb_always_assert (font_blob);
  hb_face_t *face = hb_face_create (font_blob, 0);
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_subset_input_keep_everything (input);
  hb_always_assert (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','g','h','t'), 500));
  hb_always_assert (hb_subset_input_pin_axis_location (input, face, HB_TAG ('E','L','G','R'), 1.5));
  hb_always_assert (hb_subset_input_pin_axis_location (input, face, HB_TAG ('E','L','S','H'), 8));
  hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
  hb_always_assert (plan);
  hb_blob_t *blob = hb_face_reference_table (face, HB_TAG ('g','l','y','f'));
  auto *glyf = reinterpret_cast<const OT::glyf *> (hb_blob_get_data (blob, nullptr));
  char storage[65536];
  hb_serialize_context_t serializer (storage, sizeof storage);
  serializer.start_serialize<OT::glyf> ();
  hb_subset_context_t context (blob, plan, &serializer, HB_TAG ('g','l','y','f'));
  context.budget_remaining = budget;
  bool success = glyf->subset (&context);
  hb_always_assert (success == expected);
  hb_always_assert (expected ? context.budget_remaining >= 0 : context.budget_remaining < 0);
  int64_t cost = budget - context.budget_remaining;
  hb_blob_destroy (blob);
  hb_subset_plan_destroy (plan);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
  hb_blob_destroy (font_blob);
  return cost;
}
int main (int argc, char **argv)
{
  hb_always_assert (argc == 2);
  /* Exercise the real instancing path with a small, controlled budget.
   * Raising production limits must not turn exhaustion tests into billions
   * of decoded points, particularly under Valgrind. */
  int64_t cost = subset_glyf (argv[1], HB_BUDGET_GLYPH, true);
  hb_always_assert (cost > 0);
  subset_glyf (argv[1], cost, true);
  subset_glyf (argv[1], cost - 1, false);

}
