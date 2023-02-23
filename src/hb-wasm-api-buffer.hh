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

#ifndef HB_WASM_API_BUFFER_HH
#define HB_WASM_API_BUFFER_HH

#include "hb-wasm-api.hh"

#include "hb-buffer.hh"

namespace hb {
namespace wasm {


void
buffer_copy_contents (HB_WASM_EXEC_ENV_COMPOUND
		      buffer_t bufferref)
{
  HB_RETURN_TYPE (buffer_contents_t, ret);
  HB_REF2OBJ (buffer);

  if (buffer->have_output)
    buffer->sync ();

  static_assert (sizeof (glyph_info_t) == sizeof (hb_glyph_info_t), "");
  static_assert (sizeof (glyph_position_t) == sizeof (hb_glyph_position_t), "");

  unsigned length = buffer->len;
  ret.length = length;
  ret.info = wasm_runtime_module_dup_data (module_inst, (const char *) buffer->info, length * sizeof (buffer->info[0]));
  ret.pos = wasm_runtime_module_dup_data (module_inst, (const char *) buffer->pos, length * sizeof (buffer->pos[0]));
}


}}

#endif /* HB_WASM_API_BUFFER_HH */
