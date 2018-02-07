/*
 * Copyright © 2018  Google, Inc.
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
 * Google Author(s): Garret Rieger, Roderick Sheeter
 */

#include "hb-subset-plan.hh"
#include "hb-subset-private.hh"

// TODO(Q1) map from old:new gid
hb_set_t *
glyph_ids_to_retain (hb_subset_face_t *face,
                     hb_set_t  *codepoints)
{
  hb_codepoint_t cp = -1;
  hb_set_t *gids = hb_set_create();
  while (hb_set_next(codepoints, &cp)) {
    hb_codepoint_t gid;
    if (face->cmap.get_nominal_glyph(cp, &gid)) {
      // TODO(Q1) a nice way to turn on/off logs
      fprintf(stderr, "gid for U+%04X is %d\n", cp, gid);
      hb_set_add(gids, cp);
    } else {
      fprintf(stderr, "Unable to resolve gid for U+%04X\n", cp);
    }
  }
  return gids;
}

/**
 * hb_subset_plan_create:
 * Computes a plan for subsetting the supplied face according
 * to a provide profile and input. The plan describes
 * which tables and glyphs should be retained.
 *
 * Return value: New subset plan.
 *
 * Since: 1.7.5
 **/
hb_subset_plan_t *
hb_subset_plan_create (hb_subset_face_t    *face,
                       hb_subset_profile_t *profile,
                       hb_subset_input_t   *input)
{
  hb_subset_plan_t *plan = hb_object_create<hb_subset_plan_t> ();
  plan->glyphs_to_retain = glyph_ids_to_retain (face, input->codepoints);
  return plan;
}

hb_subset_plan_t *
hb_subset_plan_get_empty ()
{
  hb_subset_plan_t *plan = hb_object_create<hb_subset_plan_t> ();
  plan->glyphs_to_retain = hb_set_get_empty();
  return plan;
}

/**
 * hb_subset_plan_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_plan_destroy (hb_subset_plan_t *plan)
{
  if (!hb_object_destroy (plan)) return;

  hb_set_destroy (plan->glyphs_to_retain);
  free (plan);
}
