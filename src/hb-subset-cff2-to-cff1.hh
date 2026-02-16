/*
 * Copyright © 2026 Behdad Esfahbod
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
 */

#ifndef HB_SUBSET_CFF2_TO_CFF1_HH
#define HB_SUBSET_CFF2_TO_CFF1_HH

#include "hb.hh"

#ifndef HB_NO_SUBSET_CFF

#include "hb-ot-cff1-table.hh"
#include "hb-ot-cff2-table.hh"
#include "hb-subset-cff-common.hh"

namespace OT {
  // Forward declarations - these are defined in hb-subset-cff2.cc
  struct cff2_subset_plan;
}

namespace CFF {

// Forward declaration
struct cff2_top_dict_values_t;

/*
 * CFF2 to CFF1 Converter
 *
 * Converts an instantiated CFF2 font to CFF1 format.
 * This is used when instantiating a variable font to a static instance.
 *
 * Key conversions:
 * - Version: 2 -> 1
 * - Add Name INDEX (required in CFF1)
 * - Wrap Top DICT in an INDEX (inline in CFF2, indexed in CFF1)
 * - Add String INDEX (empty for CID fonts)
 * - Add ROS operator to Top DICT (make it CID-keyed)
 * - Ensure FDSelect exists (optional in CFF2, required in CFF1)
 */

struct cff1_subset_plan_from_cff2_t
{
  // Inherits most data from cff2_subset_plan
  const OT::cff2_subset_plan *cff2_plan;

  // CFF1-specific additions
  hb_vector_t<unsigned char> fontName;  // Single font name for Name INDEX

  bool create (const OT::cff2_subset_plan &cff2_plan_)
  {
    cff2_plan = &cff2_plan_;

    // Create a simple font name (CFF1 requires a Name INDEX)
    // For subsets, we can use a generic name
    const char *name = "CFF1Font";
    fontName.resize (strlen (name));
    if (fontName.in_error ()) return false;
    memcpy (fontName.arrayZ, name, strlen (name));

    return true;
  }
};

/* CFF1 Top DICT operator serializer that adds ROS and removes CFF2-specific ops */
struct cff1_from_cff2_top_dict_op_serializer_t : cff_top_dict_op_serializer_t<>
{
  bool serialize (hb_serialize_context_t *c,
                  const op_str_t &opstr,
                  const cff_sub_table_info_t &info) const
  {
    TRACE_SERIALIZE (this);

    switch (opstr.op)
    {
      case OpCode_vstore:
        // CFF2-only operator, skip it
        return_trace (true);

      case OpCode_CharStrings:
        return_trace (FontDict::serialize_link4_op(c, opstr.op, info.char_strings_link, whence_t::Absolute));

      case OpCode_FDArray:
        return_trace (FontDict::serialize_link4_op(c, opstr.op, info.fd_array_link, whence_t::Absolute));

      case OpCode_FDSelect:
        return_trace (FontDict::serialize_link4_op(c, opstr.op, info.fd_select.link, whence_t::Absolute));

      default:
        return_trace (copy_opstr (c, opstr));
    }
  }

  // Serialize ROS operator to make this a CID-keyed font
  bool serialize_ros (hb_serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);

    // ROS = Registry-Ordering-Supplement
    // We use "Adobe", "Identity", 0 for maximum compatibility

    // Allocate space and encode directly
    // Registry: SID for "Adobe" (standard string #1038)
    // Ordering: SID for "Identity" (standard string #1039)
    // Supplement: 0

    str_buff_t buff;
    str_encoder_t encoder (buff);

    encoder.encode_int (1038);  // Registry SID
    encoder.encode_int (1039);  // Ordering SID
    encoder.encode_int (0);     // Supplement
    encoder.encode_op (OpCode_ROS);

    if (encoder.in_error ())
      return_trace (false);

    auto bytes = buff.as_bytes ();
    return_trace (c->embed (bytes.arrayZ, bytes.length));
  }
};

/* Main serialization function */
HB_INTERNAL bool
serialize_cff2_to_cff1 (hb_serialize_context_t *c,
                        OT::cff2_subset_plan &plan,
                        const cff2_top_dict_values_t &cff2_topDict,
                        const OT::cff2::accelerator_subset_t &acc);

} /* namespace CFF */

#endif /* HB_NO_SUBSET_CFF */

#endif /* HB_SUBSET_CFF2_TO_CFF1_HH */
