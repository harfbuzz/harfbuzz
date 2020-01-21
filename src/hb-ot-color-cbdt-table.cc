/*
 * Copyright © 2020  Google, Inc.
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
 * Google Author(s): Calder Kitagawa
 */

#include "hb-open-type.hh"

#include "hb-ot-color-cbdt-table.hh"

namespace OT {

namespace CBDT_internal {

bool copy_data_to_cbdt (hb_vector_t<char> *cbdt_prime,
                      const void *data,
                      unsigned int length)
{
  unsigned int new_len = cbdt_prime->length + length;
  if (unlikely (!cbdt_prime->alloc(new_len))) return false;
  memcpy (cbdt_prime->arrayZ + cbdt_prime->length, data, length);
  cbdt_prime->length = new_len;
  return true;
}

}

bool CBLC::subset (hb_subset_context_t *c) const
{
  TRACE_SUBSET (this);

  auto* cblc_prime = c->serializer->start_embed<CBLC> ();

  // Use a vector as a secondary buffer as the tables need to be built in parallel.
  hb_vector_t<char> cbdt_prime;

  if (unlikely (!cblc_prime)) return_trace (false);
  if (unlikely (!c->serializer->extend_min(cblc_prime))) return_trace (false);
  cblc_prime->version = version;

  hb_blob_t* cbdt_blob = hb_sanitize_context_t ().reference_table<CBDT> (c->plan->source);
  unsigned int cbdt_length;
  CBDT* cbdt = (CBDT *) hb_blob_get_data (cbdt_blob, &cbdt_length);
  if (unlikely (cbdt_length < CBDT::min_size)) return_trace (false);
  CBDT_internal::copy_data_to_cbdt (&cbdt_prime, cbdt, CBDT::min_size);

  for (const BitmapSizeTable& table : + sizeTables.iter ())
    subset_size_table (c, table, (const char *) cbdt, cblc_prime, &cbdt_prime);

  hb_blob_destroy (cbdt_blob);

  return_trace (CBLC::sink_cbdt (c, &cbdt_prime));
}

} /* namespace OT */
