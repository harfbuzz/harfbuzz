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

static_assert (sizeof (glyph_info_t) == sizeof (hb_glyph_info_t), "");
static_assert (sizeof (glyph_position_t) == sizeof (hb_glyph_position_t), "");

void
buffer_contents_free (HB_WASM_EXEC_ENV
		      ptr_t(buffer_contents_t) contentsptr)
{
  HB_OUT_PARAM (buffer_contents_t, contents);
  if (unlikely (!contents))
    return;

  module_free (contents->info);
  module_free (contents->pos);

  contents->info = nullref;
  contents->pos = nullref;
  contents->length = 0;
}

void
 buffer_contents_realloc (HB_WASM_EXEC_ENV
			  ptr_t(buffer_contents_t) contentsptr,
			  uint32_t size)
{
  HB_OUT_PARAM (buffer_contents_t, contents);
  if (unlikely (!contents))
    return;

  if (size <= contents->length)
    return;

  unsigned bytes;
  if (hb_unsigned_mul_overflows (size, sizeof (glyph_info_t), &bytes))
    return;

  // TODO bounds check?
  uint32_t infoptr = contents->info;
  uint32_t posptr = contents->pos;
  const char *info = (const char *) addr_app_to_native (infoptr);
  const char *pos = (const char *) addr_app_to_native (posptr);

  contents->info = wasm_runtime_module_dup_data (module_inst, info, bytes);
  contents->pos = wasm_runtime_module_dup_data (module_inst, pos, bytes);

  module_free (infoptr);
  module_free (posptr);

  // TODO Check success
  contents->length = size;
}

void
buffer_copy_contents (HB_WASM_EXEC_ENV_COMPOUND
		      ptr_t(buffer_t) bufferref)
{
  HB_RETURN_STRUCT (buffer_contents_t, ret);
  HB_REF2OBJ (buffer);

  if (buffer->have_output)
    buffer->sync ();

  unsigned length = buffer->len;
  ret.length = length;
  ret.info = wasm_runtime_module_dup_data (module_inst, (const char *) buffer->info, length * sizeof (buffer->info[0]));
  ret.pos = wasm_runtime_module_dup_data (module_inst, (const char *) buffer->pos, length * sizeof (buffer->pos[0]));
}

bool_t
buffer_set_contents (HB_WASM_EXEC_ENV
		     ptr_t(buffer_t) bufferref,
		     ptr_t(const buffer_contents_t) contentsptr)
{
  HB_REF2OBJ (buffer);
  HB_OUT_PARAM (buffer_contents_t, contents);
  if (unlikely (!contents))
    return false;

  unsigned length = contents->length;
  unsigned bytes;
  if (unlikely (hb_unsigned_mul_overflows (length, sizeof (buffer->info[0]), &bytes)))
    return false;

  if (unlikely (!buffer->resize (length)))
    return false;

  glyph_info_t *info = (glyph_info_t *) (validate_app_addr (contents->info, bytes) ? addr_app_to_native (contents->info) : nullptr);
  glyph_position_t *pos = (glyph_position_t *) (validate_app_addr (contents->pos, bytes) ? addr_app_to_native (contents->pos) : nullptr);

  buffer->clear_positions (); /* This is wasteful, but we need it to set have_positions=true. */
  memcpy (buffer->info, info, bytes);
  memcpy (buffer->pos, pos, bytes);
  buffer->len = length;

  return true;
}

direction_t
buffer_get_direction (HB_WASM_EXEC_ENV
		      ptr_t(buffer_t) bufferref)
{
  HB_REF2OBJ (buffer);

  return (direction_t) hb_buffer_get_direction (buffer);
}


}}

#endif /* HB_WASM_API_BUFFER_HH */
