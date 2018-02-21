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

#ifndef HB_SUBSET_PLAN_HH
#define HB_SUBSET_PLAN_HH
#include <unordered_map>
#include "hb-private.hh"
#include "hb-set.h"
#include "hb-subset.h"

#include "hb-subset-map.hh"
#include "hb-object-private.hh"

struct hb_subset_plan_t {
  hb_object_header_t header;

  hb_set_t *codepoints;

  /* codepoint => new gid based on cmap*/
  hb_map_t *dest_gid_by_codepoint;

  /* new gid by old gid lookup */
  hb_map_t *dest_gid_by_source_gid;

  /* Source gids to retain in the same order as the original font.
   * Re-ordering, such as to optimize for runs, makes subsetting G* harder, bad trade.
   * Position here is the dest gid.
   */
  hb_set_t *gids_to_retain;

  // Plan is only good for a specific source/dest so keep them with it
  hb_face_t *source;
  hb_face_t *dest;


};

typedef struct hb_subset_plan_t hb_subset_plan_t;

HB_INTERNAL hb_subset_plan_t *
hb_subset_plan_create (hb_face_t           *face,
                       hb_subset_profile_t *profile,
                       hb_subset_input_t   *input);

HB_INTERNAL hb_bool_t
hb_subset_plan_add_table (hb_subset_plan_t *plan,
                          hb_tag_t tag,
                          hb_blob_t *contents);

HB_INTERNAL hb_blob_t *
hb_subset_plan_ref_source_table (hb_subset_plan_t *plan, hb_tag_t tag);

HB_INTERNAL void
hb_subset_plan_destroy (hb_subset_plan_t *plan);

#endif /* HB_SUBSET_PLAN_HH */
