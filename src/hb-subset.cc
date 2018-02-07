/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2012  Google, Inc.
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
 * Google Author(s): Garret Rieger, Rod Sheeter
 */

#include "hb-object-private.hh"
#include "hb-private.hh"

#include "hb-subset-glyf.hh"
#include "hb-subset-private.hh"
#include "hb-subset-plan.hh"

#include "hb-ot-glyf-table.hh"

struct hb_subset_profile_t {
  hb_object_header_t header;
  ASSERT_POD ();
};

/**
 * hb_subset_profile_create:
 *
 * Return value: New profile with default settings.
 *
 * Since: 1.7.5
 **/
hb_subset_profile_t *
hb_subset_profile_create ()
{
  return hb_object_create<hb_subset_profile_t>();
}

/**
 * hb_subset_profile_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_profile_destroy (hb_subset_profile_t *profile)
{
  if (!hb_object_destroy (profile)) return;

  free (profile);
}

/**
 * hb_subset_input_create:
 *
 * Return value: New subset input.
 *
 * Since: 1.7.5
 **/
hb_subset_input_t *
hb_subset_input_create (hb_set_t *codepoints)
{
  if (unlikely (!codepoints))
    codepoints = hb_set_get_empty();

  hb_subset_input_t *input = hb_object_create<hb_subset_input_t>();
  input->codepoints = hb_set_reference(codepoints);
  return input;
}

/**
 * hb_subset_input_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_input_destroy(hb_subset_input_t *subset_input)
{
  if (!hb_object_destroy (subset_input)) return;

  hb_set_destroy (subset_input->codepoints);
  free (subset_input);
}

/**
 * hb_subset_face_create:
 *
 * Return value: New subset face.
 *
 * Since: 1.7.5
 **/
hb_subset_face_t *
hb_subset_face_create (hb_face_t *face)
{
  if (unlikely (!face))
    face = hb_face_get_empty();

  hb_subset_face_t *subset_face = hb_object_create<hb_subset_face_t> ();
  subset_face->face = hb_face_reference (face);
  subset_face->cmap.init(face);

  return subset_face;
}

/**
 * hb_subset_face_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_face_destroy (hb_subset_face_t *subset_face)
{
  if (!hb_object_destroy (subset_face)) return;

  subset_face->cmap.fini();
  hb_face_destroy(subset_face->face);
  free (subset_face);
}

/**
 * hb_subset:
 * @profile: profile to use for the subsetting.
 * @input: input to use for the subsetting.
 * @face: font face data to be subset.
 * @result: subsetting result.
 *
 * Subsets a font according to provided profile and input.
 **/
hb_bool_t
hb_subset (hb_subset_profile_t *profile,
           hb_subset_input_t *input,
           hb_subset_face_t *face,
           hb_blob_t **result /* OUT */)
{
  if (!profile || !input || !face) return false;

  hb_subset_plan_t *plan = hb_subset_plan_create (face, profile, input);

  hb_codepoint_t old_gid = -1;
  while (hb_set_next(plan->glyphs_to_retain, &old_gid)) {
    hb_codepoint_t new_gid;
    if (hb_subset_plan_new_gid_for_old_id(plan, old_gid, &new_gid)) {
      DEBUG_MSG(SUBSET, nullptr, "Remap %d : %d\n", old_gid, new_gid);
    } else {
      DEBUG_MSG(SUBSET, nullptr, "Remap %d : DOOM! No new ID\n", old_gid);
    }
  }
  // TODO:
  // - Create initial header + table directory
  // - Loop through the set of tables to be kept:
  //   - Perform table specific subsetting if defined.
  //   - copy the table into the output.
  // - Fix header + table directory.

  bool success = true;

  hb_blob_t *glyf_prime = nullptr;
  if (hb_subset_glyf (plan, face->face, &glyf_prime)) {
    // TODO: write new glyf to new face.
  } else {
    success = false;
  }
  hb_blob_destroy (glyf_prime);

  *result = hb_face_reference_blob(face->face);
  hb_subset_plan_destroy (plan);
  return success;
}
