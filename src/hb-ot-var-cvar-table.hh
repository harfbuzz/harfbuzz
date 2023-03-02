/*
 * Copyright © 2023  Google, Inc.
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
 */

#ifndef HB_OT_VAR_CVAR_TABLE_HH
#define HB_OT_VAR_CVAR_TABLE_HH

#include "hb-ot-var-common.hh"


namespace OT {
/*
 * cvar -- control value table (CVT) Variations
 * https://docs.microsoft.com/en-us/typography/opentype/spec/cvar
 */
#define HB_OT_TAG_cvar HB_TAG('c','v','a','r')

struct cvar
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_cvar;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  version.sanitize (c) && likely (version.major == 1) &&
		  tupleVariationData.sanitize (c));
  }

  const TupleVariationData* get_tuple_var_data (void) const
  { return &tupleVariationData; }

  static bool calculate_cvt_deltas (unsigned axis_count,
                                    hb_array_t<int> coords,
                                    unsigned num_cvt_item,
                                    const TupleVariationData *tuple_var_data,
                                    const void *base,
                                    hb_vector_t<float>& cvt_deltas /* OUT */)
  {
    if (!coords) return true;
    hb_vector_t<unsigned> shared_indices;
    TupleVariationData::tuple_iterator_t iterator;
    unsigned var_data_length = tuple_var_data->get_size (axis_count);
    hb_bytes_t var_data_bytes = hb_bytes_t (reinterpret_cast<const char*> (tuple_var_data), var_data_length);
    if (!TupleVariationData::get_tuple_iterator (var_data_bytes, axis_count, base,
                                                 shared_indices, &iterator))
      return true; /* isn't applied at all */

    hb_array_t<const F2DOT14> shared_tuples = hb_array<F2DOT14> ();
    hb_vector_t<unsigned> private_indices;
    hb_vector_t<int> unpacked_deltas;

    do
    {
      float scalar = iterator.current_tuple->calculate_scalar (coords, axis_count, shared_tuples);
      if (scalar == 0.f) continue;
      const HBUINT8 *p = iterator.get_serialized_data ();
      unsigned int length = iterator.current_tuple->get_data_size ();
      if (unlikely (!iterator.var_data_bytes.check_range (p, length)))
        return false;

      const HBUINT8 *end = p + length;

      bool has_private_points = iterator.current_tuple->has_private_points ();
      if (has_private_points &&
          !TupleVariationData::unpack_points (p, private_indices, end))
        return false;
      const hb_vector_t<unsigned int> &indices = has_private_points ? private_indices : shared_indices;

      bool apply_to_all = (indices.length == 0);
      unsigned num_deltas = apply_to_all ? num_cvt_item : indices.length;
      if (unlikely (!unpacked_deltas.resize (num_deltas, false))) return false;
      if (unlikely (!TupleVariationData::unpack_deltas (p, unpacked_deltas, end))) return false;

      for (unsigned int i = 0; i < num_deltas; i++)
      {
        unsigned int idx = apply_to_all ? i : indices[i];
        if (unlikely (idx >= num_cvt_item)) continue;
        if (scalar != 1.0f) cvt_deltas[idx] += unpacked_deltas[i] * scalar ;
        else cvt_deltas[idx] += unpacked_deltas[i];
      }
    } while (iterator.move_to_next ());

    return true;
  }

  static bool add_cvt_and_apply_deltas (hb_subset_plan_t *plan,
                                        const TupleVariationData *tuple_var_data,
                                        const void *base)
  {
    const hb_tag_t cvt = HB_TAG('c','v','t',' ');
    hb_blob_t *cvt_blob = hb_face_reference_table (plan->source, cvt);
    hb_blob_t *cvt_prime_blob = hb_blob_copy_writable_or_fail (cvt_blob);
    hb_blob_destroy (cvt_blob);
  
    if (unlikely (!cvt_prime_blob))
      return false;
 
    unsigned cvt_blob_length = hb_blob_get_length (cvt_prime_blob);
    unsigned num_cvt_item = cvt_blob_length / FWORD::static_size;

    hb_vector_t<float> cvt_deltas;
    if (unlikely (!cvt_deltas.resize (num_cvt_item)))
    {
      hb_blob_destroy (cvt_prime_blob);
      return false;
    }
    hb_memset (cvt_deltas.arrayZ, 0, cvt_deltas.get_size ());

    if (!calculate_cvt_deltas (plan->normalized_coords.length, plan->normalized_coords.as_array (),
                               num_cvt_item, tuple_var_data, base, cvt_deltas))
    {
      hb_blob_destroy (cvt_prime_blob);
      return false;
    }

    FWORD *cvt_prime = (FWORD *) hb_blob_get_data_writable (cvt_prime_blob, nullptr);
    for (unsigned i = 0; i < num_cvt_item; i++)
      cvt_prime[i] += (int) roundf (cvt_deltas[i]);
    
    bool success = plan->add_table (cvt, cvt_prime_blob);
    hb_blob_destroy (cvt_prime_blob);
    return success;
  }

  protected:
  FixedVersion<>version;		/* Version of the CVT variation table
					 * initially set to 0x00010000u */
  TupleVariationData tupleVariationData; /* TupleVariationDate for cvar table */
  public:
  DEFINE_SIZE_MIN (8);
};

} /* namespace OT */


#endif /* HB_OT_VAR_CVAR_TABLE_HH */
