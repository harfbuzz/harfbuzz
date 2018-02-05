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
 * Google Author(s): Garret Rieger
 */

#include "hb-private.hh"

#include "hb-object-private.hh"


struct hb_subset_plan_t {
  hb_object_header_t header;
  ASSERT_POD ();
};

/**
 * hb_subset_plan_create:
 * Computes a plan for subsetting the supplied face according
 * to a provide profile and input. The plan describes
 * which tables and glyphs should be retained.
 *
 * Return value: New plan with default settings.
 *
 * Since: 1.7.5
 **/
hb_subset_plan_t *
hb_subset_plan_create (hb_face_t           *face,
                       hb_subset_profile_t *profile,
                       hb_subset_input_t   *input)
{
  return hb_object_create<hb_subset_plan_t>();
}

hb_subset_plan_t *
hb_subset_plan_create_empty ()
{
  return hb_object_create<hb_subset_plan_t>();
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

  free (plan);
}
