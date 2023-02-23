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

#ifndef HB_WASM_API_HH
#define HB_WASM_API_HH

#include "hb.hh"

#include <wasm_export.h>

#define HB_WASM_API(x) HB_INTERNAL x
#define HB_WASM_BEGIN_DECLS namespace hb { namespace wasm {
#define HB_WASM_END_DECLS }}

#define HB_WASM_EXEC_ENV wasm_exec_env_t exec_env,

#include "hb-wasm-api.h"

#undef HB_WASM_API
#undef HB_WASM_BEGIN_DECLS
#undef HB_WASM_END_DECLS

#define module_inst wasm_runtime_get_module_inst (exec_env)


#include "hb-wasm-font.hh"


#undef module_inst

static void debugprint(wasm_exec_env_t exec_env, char *the_string, uint8_t len) {
	printf("%*s", len, the_string);
}


  /* Define an array of NativeSymbol for the APIs to be exported.
   * Note: the array must be static defined since runtime will keep it after registration
   * For the function signature specifications, goto the link:
   * https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md
   */

#define NATIVE_SYMBOL(signature, name) {#name, (void *) hb::wasm::name, signature, NULL}
  static NativeSymbol _hb_wasm_native_symbols[] =
  {
    NATIVE_SYMBOL ("(r)r",	font_get_face),
    {
	    "debugprint",
	    (void *) debugprint,
	    "($i)",
	    NULL
    }
  };
#undef NATIVE_SYMBOL


#endif /* HB_WASM_API_HH */
