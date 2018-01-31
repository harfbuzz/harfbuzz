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

#include "hb-private.hh"

#include "hb-object-private.hh"


struct hb_subset_profile_t {
  hb_object_header_t header;
  ASSERT_POD ();
};

struct hb_subset_input_t {
  hb_object_header_t header;
  ASSERT_POD ();
};

struct hb_subset_face_t {
  hb_object_header_t header;
  ASSERT_POD ();

  hb_face_t *face;
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
hb_subset_input_create()
{
  return hb_object_create<hb_subset_input_t>();
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
hb_subset_face_create(hb_face_t *face)
{
  if (unlikely (!face))
    face = hb_face_get_empty();

  hb_subset_face_t *subset_face = hb_object_create<hb_subset_face_t> ();
  subset_face->face = hb_face_reference (face);

  return subset_face;
}

/**
 * hb_subset_face_destroy:
 *
 * Since: 1.7.5
 **/
void
hb_subset_face_destroy(hb_subset_face_t *subset_face)
{
  if (!hb_object_destroy (subset_face)) return;

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
hb_subset(hb_subset_profile_t *profile,
          hb_subset_input_t *input,
          hb_subset_face_t *face,
          hb_blob_t **result /* OUT */)
{
  if (!profile || !input || !face) return false;

  *result = hb_blob_get_empty();
  return true;
}
