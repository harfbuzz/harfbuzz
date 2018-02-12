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

static bool
_calculate_glyf_and_loca_prime_size (const OT::glyf::accelerator_t &glyf,
                                     hb_prealloced_array_t<hb_codepoint_t> &glyph_ids,
                                     bool *use_short_loca, /* OUT */
                                     unsigned int *glyf_size, /* OUT */
                                     unsigned int *loca_size /* OUT */)
{
  unsigned int total = 0;
  unsigned int count = 0;
  for (unsigned int i = 0; i < glyph_ids.len; i++) {
    hb_codepoint_t next_glyph = glyph_ids[i];
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
  *use_short_loca = (total <= 131070);
  *loca_size = (count + 1)
      * (*use_short_loca ? sizeof(OT::HBUINT16) : sizeof(OT::HBUINT32));

  DEBUG_MSG(SUBSET, nullptr, "preparing to subset glyf: final size %d, loca size %d, using %s loca",
            total,
            *loca_size,
            *use_short_loca ? "short" : "long");
  return true;
}

static void
_write_loca_entry (unsigned int id, unsigned int offset, bool is_short, void *loca_prime) {
  if (is_short) {
    ((OT::HBUINT16*) loca_prime) [id].set (offset / 2);
  } else {
    ((OT::HBUINT32*) loca_prime) [id].set (offset);
  }
}

static bool
_write_glyf_and_loca_prime (const OT::glyf::accelerator_t &glyf,
                            const char                    *glyf_data,
                            hb_prealloced_array_t<hb_codepoint_t> &glyph_ids,
                            bool                           use_short_loca,
                            int                            glyf_prime_size,
                            char                          *glyf_prime_data /* OUT */,
                            int                            loca_prime_size,
                            char                          *loca_prime_data /* OUT */)
{
  char *glyf_prime_data_next = glyf_prime_data;

  hb_codepoint_t new_glyph_id = 0;

  unsigned int current_offset = 0;
  unsigned int end_offset = 0;
  for (unsigned int i = 0; i < glyph_ids.len; i++) {
    unsigned int start_offset;
    if (unlikely (!glyf.get_offsets (glyph_ids[i], &start_offset, &end_offset))) {
      return false;
    }

    int length = end_offset - start_offset;
    memcpy (glyf_prime_data_next, glyf_data + start_offset, length);

    _write_loca_entry (i, current_offset, use_short_loca, loca_prime_data);

    glyf_prime_data_next += length;
    current_offset += length;
    new_glyph_id++;
  }

  // Add the last loca entry which doesn't correspond to a specific glyph
  // but identifies the end of the last glyphs data.
  _write_loca_entry (new_glyph_id, current_offset, use_short_loca, loca_prime_data);

  return true;
}

static bool
_hb_subset_glyf_and_loca (const OT::glyf::accelerator_t  &glyf,
                          const char                     *glyf_data,
                          hb_prealloced_array_t<hb_codepoint_t>&glyphs_to_retain,
                          bool                           *use_short_loca,
                          hb_blob_t                     **glyf_prime /* OUT */,
                          hb_blob_t                     **loca_prime /* OUT */)
{
  // TODO(grieger): Sanity check writes to make sure they are in-bounds.
  // TODO(grieger): Sanity check allocation size for the new table.
  // TODO(grieger): Don't fail on bad offsets, just dump them.

  unsigned int glyf_prime_size;
  unsigned int loca_prime_size;

  if (unlikely (!_calculate_glyf_and_loca_prime_size (glyf,
                                                      glyphs_to_retain,
                                                      use_short_loca,
                                                      &glyf_prime_size,
                                                      &loca_prime_size))) {
    return false;
  }

  char *glyf_prime_data = (char *) calloc (glyf_prime_size, 1);
  char *loca_prime_data = (char *) calloc (loca_prime_size, 1);
  if (unlikely (!_write_glyf_and_loca_prime (glyf, glyf_data, glyphs_to_retain,
                                             *use_short_loca,
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
                         bool             *use_short_loca, /* OUT */
                         hb_blob_t       **glyf_prime, /* OUT */
                         hb_blob_t       **loca_prime /* OUT */)
{
  hb_blob_t *glyf_blob = OT::Sanitizer<OT::glyf>().sanitize (face->reference_table (HB_OT_TAG_glyf));
  const char *glyf_data = hb_blob_get_data(glyf_blob, nullptr);

  OT::glyf::accelerator_t glyf;
  glyf.init(face);
  bool result = _hb_subset_glyf_and_loca (glyf,
                                          glyf_data,
                                          plan->gids_to_retain_sorted,
                                          use_short_loca,
                                          glyf_prime,
                                          loca_prime);
  glyf.fini();

  return result;
}
