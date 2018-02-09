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
_calculate_glyf_and_loca_prime_size (const OT::glyf::accelerator_t &glyf,
                                     hb_set_t *glyph_ids,
                                     unsigned int *glyf_size /* OUT */,
                                     unsigned int *loca_size /* OUT */)
{
  unsigned int total = 0;
  unsigned int count = 0;
  hb_codepoint_t next_glyph = -1;
  while (hb_set_next(glyph_ids, &next_glyph)) {
    unsigned int start_offset, end_offset;
    if (unlikely (!glyf.get_offsets (next_glyph, &start_offset, &end_offset))) {
      *glyf_size = 0;
      *loca_size = sizeof(OT::HBUINT32);
      return false;
    }

    total += end_offset - start_offset;
    count++;
  }

  *glyf_size = total;
  *loca_size = (count + 1) * sizeof(OT::HBUINT32);
  return true;
}

bool
_write_glyf_and_loca_prime (const OT::glyf::accelerator_t &glyf,
                            const char                    *glyf_data,
                            const hb_set_t                *glyph_ids,
                            int                            glyf_prime_size,
                            char                          *glyf_prime_data /* OUT */,
                            int                            loca_prime_size,
                            char                          *loca_prime_data /* OUT */)
{
  // TODO(grieger): Handle the missing character glyf and outline.

  char *glyf_prime_data_next = glyf_prime_data;
  OT::HBUINT32 *loca_prime = (OT::HBUINT32*) loca_prime_data;

  hb_codepoint_t next_glyph = -1;
  hb_codepoint_t new_glyph_id = 0;

  while (hb_set_next(glyph_ids, &next_glyph)) {
    unsigned int start_offset, end_offset;
    if (unlikely (!glyf.get_offsets (next_glyph, &start_offset, &end_offset))) {
      return false;
    }

    int length = end_offset - start_offset;
    memcpy (glyf_prime_data_next, glyf_data + start_offset, length);
    loca_prime[new_glyph_id].set(start_offset);

    glyf_prime_data_next += length;
    new_glyph_id++;
  }

  return true;
}

bool
_hb_subset_glyf_and_loca (const OT::glyf::accelerator_t  &glyf,
                          const char                     *glyf_data,
                          hb_set_t                       *glyphs_to_retain,
                          hb_blob_t                     **glyf_prime /* OUT */,
                          hb_blob_t                     **loca_prime /* OUT */)
{
  // TODO(grieger): Sanity check writes to make sure they are in-bounds.
  // TODO(grieger): Sanity check allocation size for the new table.
  // TODO(grieger): Subset loca simultaneously.
  // TODO(grieger): Don't fail on bad offsets, just dump them.
  // TODO(grieger): Support short loca output.
  // TODO(grieger): Add a extra loca entry at the end.

  unsigned int glyf_prime_size;
  unsigned int loca_prime_size;
  if (unlikely (!_calculate_glyf_and_loca_prime_size (glyf,
                                                      glyphs_to_retain,
                                                      &glyf_prime_size,
                                                      &loca_prime_size))) {
    return false;
  }

  char *glyf_prime_data = (char *) calloc (glyf_prime_size, 1);
  char *loca_prime_data = (char *) calloc (loca_prime_size, 1);
  if (unlikely (!_write_glyf_and_loca_prime (glyf, glyf_data, glyphs_to_retain,
                                             glyf_prime_size, glyf_prime_data,
                                             loca_prime_size, loca_prime_data))) {
    free (glyf_prime_data);
    return false;
  }

  *glyf_prime = hb_blob_create (glyf_prime_data,
                                glyf_prime_size,
                                HB_MEMORY_MODE_READONLY,
                                glyf_prime_data,
                                free);
  *loca_prime = hb_blob_create (loca_prime_data,
                                loca_prime_size,
                                HB_MEMORY_MODE_READONLY,
                                loca_prime_data,
                                free);
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
hb_subset_glyf_and_loca (hb_subset_plan_t *plan,
                         hb_face_t        *face,
                         hb_blob_t       **glyf_prime /* OUT */,
                         hb_blob_t       **loca_prime /* OUT */)
{
  hb_blob_t *glyf_blob = OT::Sanitizer<OT::glyf>().sanitize (face->reference_table (HB_OT_TAG_glyf));
  const char *glyf_data = hb_blob_get_data(glyf_blob, nullptr);

  OT::glyf::accelerator_t glyf;
  glyf.init(face);
  bool result = _hb_subset_glyf_and_loca (glyf, glyf_data, plan->glyphs_to_retain, glyf_prime, loca_prime);
  glyf.fini();

  // TODO(grieger): Subset loca

  return result;
}
