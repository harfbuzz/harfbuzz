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

#ifndef HB_WASM_API_LIST_HH
#define HB_WASM_API_LIST_HH

#include "hb-wasm-api.hh"

#include "hb-wasm-font.hh"

#ifdef HB_DEBUG_WASM
namespace hb { namespace wasm {
static void
debugprint (HB_WASM_EXEC_ENV
	    char *the_string)
{
  DEBUG_MSG (WASM, exec_env, "%s", the_string);
}
}}
#endif

#define NATIVE_SYMBOL(signature, name) {#name, (void *) hb::wasm::name, signature, NULL}
/* Note: the array must be static defined since runtime will keep it after registration
 * https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md */
static NativeSymbol _hb_wasm_native_symbols[] =
{

  /* font */
  NATIVE_SYMBOL ("(i)i",	font_get_face),

  /* debug */
#ifdef HB_DEBUG_WASM
  NATIVE_SYMBOL ("($)",		debugprint),
#endif

};
#undef NATIVE_SYMBOL


#endif /* HB_WASM_API_LIST_HH */
