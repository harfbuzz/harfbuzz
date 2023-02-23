/*
 * Copyright © 2023  Behdad Esfahbod
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

#ifndef HB_WASM_FACE_HH
#define HB_WASM_FACE_HH

#include <hb-wasm-api.hh>

namespace hb {
namespace wasm {


void
face_reference_table (HB_WASM_EXEC_ENV_COMPOUND
		      face_t faceref,
		      tag_t table_tag)
{
  HB_RETURN_TYPE (blob_t, ret);
  HB_REF2OBJ (face);

  hb_blob_t *blob = hb_face_reference_table (face, table_tag);

  unsigned length;
  const char *data = hb_blob_get_data (blob, &length);

  ret.data = wasm_runtime_module_dup_data (module_inst, data, length);
  ret.length = length;

  hb_blob_destroy (blob);
}


}}

#endif /* HB_WASM_FACE_HH */
