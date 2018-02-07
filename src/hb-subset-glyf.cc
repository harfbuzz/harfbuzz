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

#include "hb-open-type-private.hh"
#include "hb-ot-glyf-table.hh"
#include "hb-set.h"
#include "hb-subset-glyf.hh"

bool
calculate_glyf_prime_size (const OT::glyf::accelerator_t &glyf,
                           hb_set_t *glyph_ids,
                           unsigned int *size /* OUT */)
{
  unsigned int total = 0;
  hb_codepoint_t next_glyph = -1;
  while (hb_set_next(glyph_ids, &next_glyph)) {
    unsigned int start_offset, end_offset;
    if (!glyf.get_offsets (next_glyph, &start_offset, &end_offset)) {
      *size = 0;
      return false;
    }

    total += end_offset - start_offset;
  }

  *size = total;
  return true;
}

bool
write_glyf_prime (const OT::glyf::accelerator_t &glyf,
                  const char                    *glyf_data,
                  hb_set_t                      *glyph_ids,
                  int                            glyf_prime_size,
                  char                          *glyf_prime_data /* OUT */)
{
  char *glyf_prime_data_next = glyf_prime_data;

  hb_codepoint_t next_glyph = -1;
  while (hb_set_next(glyph_ids, &next_glyph)) {
    unsigned int start_offset, end_offset;
    if (!glyf.get_offsets (next_glyph, &start_offset, &end_offset)) {
      return false;
    }

    int length = end_offset - start_offset;
    memcpy (glyf_prime_data_next, glyf_data + start_offset, length);
    glyf_prime_data_next += length;
  }

  return true;
}

/**
 * hb_subset_glyf:
 * Subsets the glyph table according to a provided plan.
 *
 * Return value: subsetted glyf table.
 *
 * Since: 1.7.5
 **/
bool
hb_subset_glyf (hb_subset_plan_t *plan,
                hb_face_t        *face,
                hb_blob_t       **glyf_prime /* OUT */)
{
  // TODO(grieger): Sanity check writes to make sure they are in-bounds.
  // TODO(grieger): Sanity check allocation size for the new table.
  // TODO(grieger): Subset loca simultaneously.

  hb_blob_t *glyf_blob = OT::Sanitizer<OT::glyf>().sanitize (face->reference_table (HB_OT_TAG_glyf));
  const char *glyf_data = hb_blob_get_data(glyf_blob, nullptr);

  OT::glyf::accelerator_t glyf;
  glyf.init(face);

  unsigned int glyf_prime_size;
  if (!calculate_glyf_prime_size (glyf,
                                  plan->glyphs_to_retain,
                                  &glyf_prime_size)) {
    glyf.fini();
    return false;
  }

  char *glyf_prime_data = (char *) calloc (glyf_prime_size, 1);
  if (!write_glyf_prime (glyf, glyf_data, plan->glyphs_to_retain, glyf_prime_size,
                         glyf_prime_data)) {
    glyf.fini();
    free (glyf_prime_data);
    return false;
  }

  glyf.fini();
  *glyf_prime = hb_blob_create (glyf_prime_data,
                                glyf_prime_size,
                                HB_MEMORY_MODE_READONLY,
                                glyf_prime_data,
                                free);
  return true;
}
