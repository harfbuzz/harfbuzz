/*
 * Copyright © 2018 Adobe Systems Incorporated.
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
 * Adobe Author(s): Michiharu Ariza
 */

#include "hb-open-type-private.hh"
#include "hb-ot-cff2-table.hh"
#include "hb-set.h"
#include "hb-subset-cff2.hh"
#include "hb-subset-plan.hh"

static bool
_calculate_cff2_prime_size (const OT::cff2::accelerator_t &cff2,
                                     hb_vector_t<hb_codepoint_t> &glyph_ids,
                                     hb_bool_t drop_hints,
                                     unsigned int *cff2_size /* OUT */,
                                     hb_vector_t<unsigned int> *instruction_ranges /* OUT */)
{
  // XXX: TODO
  *cff2_size = 0;
  return true;
}

static bool
_write_cff2_prime (hb_subset_plan_t              *plan,
          const OT::cff2::accelerator_t &cff2,
                            const char                    *cff2_data,
                            hb_vector_t<unsigned int> &instruction_ranges,
                            unsigned int                   cff2_prime_size,
                            char                          *cff2_prime_data /* OUT */)
{
  // XXX: TODO
  return true;
}

static bool
_hb_subset_cff2 (const OT::cff2::accelerator_t  &cff2,
                          const char                     *cff2_data,
                          hb_subset_plan_t               *plan,
                          hb_blob_t                     **cff2_prime /* OUT */)
{
  hb_vector_t<hb_codepoint_t> &glyphs_to_retain = plan->glyphs;

  unsigned int cff2_prime_size;
  hb_vector_t<unsigned int> instruction_ranges;
  instruction_ranges.init();

  if (unlikely (!_calculate_cff2_prime_size (cff2,
                                                      glyphs_to_retain,
                                                      plan->drop_hints,
                                                      &cff2_prime_size,
                                                      &instruction_ranges))) {
    instruction_ranges.fini();
    return false;
  }

  char *cff2_prime_data = (char *) calloc (1, cff2_prime_size);
  if (unlikely (!_write_cff2_prime (plan, cff2, cff2_data,
                                             instruction_ranges,
                                             cff2_prime_size, cff2_prime_data))) {
    free (cff2_prime_data);
    instruction_ranges.fini();
    return false;
  }
  instruction_ranges.fini();

  *cff2_prime = hb_blob_create (cff2_prime_data,
                                cff2_prime_size,
                                HB_MEMORY_MODE_READONLY,
                                cff2_prime_data,
                                free);
  // XXX: TODO
  return true;
}

/**
 * hb_subset_cff2:
 * Subsets the CFF2 table according to a provided plan.
 *
 * Return value: subsetted cff2 table.
 **/
bool
hb_subset_cff2 (hb_subset_plan_t *plan,
                hb_blob_t       **cff2_prime /* OUT */)
{
  hb_blob_t *cff2_blob = OT::Sanitizer<OT::cff2>().sanitize (plan->source->reference_table (HB_OT_TAG_cff2));
  const char *cff2_data = hb_blob_get_data(cff2_blob, nullptr);

  OT::cff2::accelerator_t cff2;
  cff2.init(plan->source);
  bool result = _hb_subset_cff2 (cff2,
                                 cff2_data,
                                 plan,
                                 cff2_prime);

  hb_blob_destroy (cff2_blob);
  cff2.fini();

  return result;
}
