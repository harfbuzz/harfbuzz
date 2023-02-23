/*
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#define HB_DEBUG_WASM 1

#include "hb-shaper-impl.hh"

#ifdef HAVE_WASM

#include <wasm_export.h>


/*
 * shaper face data
 */

#define HB_WASM_TAG_WASM HB_TAG('W','a','s','m')

struct hb_wasm_face_data_t {
  hb_blob_t *wasm_blob;
  wasm_module_t wasm_module;
};

static bool
init_wasm ()
{
  static bool initialized;
  if (initialized)
    return true;

  RuntimeInitArgs init_args;
  memset (&init_args, 0, sizeof (RuntimeInitArgs));

  // Define an array of NativeSymbol for the APIs to be exported.
  // Note: the array must be static defined since runtime
  //            will keep it after registration
  // For the function signature specifications, goto the link:
  // https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md

  static NativeSymbol native_symbols[] = {
#if 0
      {
	  "intToStr", // the name of WASM function name
	  intToStr,   // the native function pointer
	  "(i*~i)i",  // the function prototype signature, avoid to use i32
	  NULL        // attachment is NULL
      },
      {
	  "get_pow", // the name of WASM function name
	  get_pow,   // the native function pointer
	  "(ii)i",   // the function prototype signature, avoid to use i32
	  NULL       // attachment is NULL
      },
      { "calculate_native", calculate_native, "(iii)i", NULL }
#endif
  };

  init_args.mem_alloc_type = Alloc_With_Allocator;
  init_args.mem_alloc_option.allocator.malloc_func = (void *) hb_malloc;
  init_args.mem_alloc_option.allocator.realloc_func = (void *) hb_realloc;
  init_args.mem_alloc_option.allocator.free_func = (void *) hb_free;

  init_args.mem_alloc_type = Alloc_With_System_Allocator;

  // Native symbols need below registration phase
  init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
  init_args.native_module_name = "env";
  init_args.native_symbols = native_symbols;

  if (!wasm_runtime_full_init (&init_args))
  {
    DEBUG_MSG (WASM, nullptr, "Init runtime environment failed.");
    return false;
  }


  initialized = true;
  return true;
}

hb_wasm_face_data_t *
_hb_wasm_shaper_face_data_create (hb_face_t *face)
{
  hb_wasm_face_data_t *data = nullptr;
  hb_blob_t *wasm_blob = nullptr;
  wasm_module_t wasm_module = nullptr;

  wasm_blob = hb_face_reference_table (face, HB_WASM_TAG_WASM);
  unsigned length = hb_blob_get_length (wasm_blob);
  if (!length)
    goto fail;

  if (!init_wasm ())
    goto fail;

  wasm_module = wasm_runtime_load ((uint8_t *) hb_blob_get_data_writable (wasm_blob, nullptr),
				   length, nullptr, 0);
  if (!wasm_module)
  {
    DEBUG_MSG (WASM, nullptr, "Load wasm module failed.");
    goto fail;
  }


  data = (hb_wasm_face_data_t *) hb_calloc (1, sizeof (hb_wasm_face_data_t));
  if (unlikely (!data))
    goto fail;

  data->wasm_blob = wasm_blob;
  data->wasm_module = wasm_module;

  return data;

fail:
  if (wasm_module)
      wasm_runtime_unload (wasm_module);
  hb_blob_destroy (wasm_blob);
  hb_free (data);
  return nullptr;
}

void
_hb_wasm_shaper_face_data_destroy (hb_wasm_face_data_t *data)
{
  wasm_runtime_unload (data->wasm_module);
  hb_blob_destroy (data->wasm_blob);
  hb_free (data);
}


/*
 * shaper font data
 */

struct hb_wasm_font_data_t {};

hb_wasm_font_data_t *
_hb_wasm_shaper_font_data_create (hb_font_t *font HB_UNUSED)
{
  return (hb_wasm_font_data_t *) HB_SHAPER_DATA_SUCCEEDED;
}

void
_hb_wasm_shaper_font_data_destroy (hb_wasm_font_data_t *data HB_UNUSED)
{
}


/*
 * shaper
 */

hb_bool_t
_hb_wasm_shape (hb_shape_plan_t    *shape_plan,
		hb_font_t          *font,
		hb_buffer_t        *buffer,
		const hb_feature_t *features,
		unsigned int        num_features)
{
  bool ret = true;
  const hb_wasm_face_data_t *face_data = font->face->data.wasm;
  constexpr uint32_t stack_size = 8092, heap_size = 8092;

  wasm_module_inst_t module_inst = nullptr;
  wasm_exec_env_t exec_env = nullptr;
  wasm_function_inst_t shape_func = nullptr;

  module_inst = wasm_runtime_instantiate(face_data->wasm_module,
					 stack_size, heap_size,
					 nullptr, 0);
  if (!module_inst)
  {
    DEBUG_MSG (WASM, face_data->wasm_module, "Instantiate wasm module failed.");
    goto fail;
  }

  exec_env = wasm_runtime_create_exec_env (module_inst, stack_size);
  if (!exec_env) {
    DEBUG_MSG (WASM, module_inst, "Create wasm execution environment failed.");
    goto fail;
  }

  if (!(shape_func = wasm_runtime_lookup_function (module_inst, "shape", nullptr))) {
    DEBUG_MSG (WASM, module_inst, "Shape function not found.");
  }

  wasm_val_t results[1];
  wasm_val_t arguments[1];

  results[0].kind = WASM_I32;
  arguments[0].kind = WASM_I32;
  arguments[0].of.i32 = 41;

  if (!wasm_runtime_call_wasm_a (exec_env, shape_func, 1, results, 1, arguments))
  {
    DEBUG_MSG (WASM, module_inst, "Calling shape function failed: %s",
	       wasm_runtime_get_exception(module_inst));
    goto fail;
  }

  printf ("%i\n", results[0].of.i32);


  if (!face_data)
    goto fail;

  if (0)
  {
fail:
    ret = false;
  }

  if (exec_env)
    wasm_runtime_destroy_exec_env (exec_env);
  if (module_inst)
    wasm_runtime_deinstantiate (module_inst);

  return ret;
}

#endif
