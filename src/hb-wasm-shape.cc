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

#undef HB_DEBUG_WASM
#define HB_DEBUG_WASM 1

#include "hb-shaper-impl.hh"

#ifdef HAVE_WASM

/* Compile wasm-micro-runtime with:
 *
 * $ cmake -DWAMR_BUILD_MULTI_MODULE=1 -DWAMR_BUILD_INSTRUCTION_METERING=1
 * $ make
 *
 * If you manage to build a wasm shared module successfully and want to use it,
 * do the following:
 *
 *   - Add -DWAMR_BUILD_MULTI_MODULE=1 to your cmake build for wasm-micro-runtime,
 *
 *   - Remove the #define HB_WASM_NO_MODULES line below,
 *
 *   - Install your shared module with name ending in .wasm in
 *     $(prefix)/$(libdir)/harfbuzz/wasm/
 *
 *   - Build your font's wasm code importing the shared modules with the desired
 *     name. This can be done eg.: __attribute__((import_module("graphite2")))
 *     before each symbol in the shared-module's headers.
 *
 *   - Try shaping your font and hope for the best...
 *
 * I haven't been able to get this to work since emcc's support for shared libraries
 * requires support from the host that seems to be missing from wasm-micro-runtime?
 */

#include "hb-wasm-api.hh"
#include "hb-wasm-api-list.hh"

#if !defined(HB_NO_WASM_BUDGET) && !defined(HAVE_WASM_RUNTIME_SET_INSTRUCTION_COUNT_LIMIT)
#error "The Wasm shaper needs a wasm-micro-runtime built with -DWAMR_BUILD_INSTRUCTION_METERING=1, to limit what a font's module can run.  Define HB_NO_WASM_BUDGET to build without limits, for trusted fonts only."
#endif

#ifndef HB_WASM_NO_MODULES
#define HB_WASM_NO_MODULES
#endif


#ifndef HB_WASM_NO_MODULES
static bool HB_UNUSED
_hb_wasm_module_reader (const char *module_name,
		       uint8_t **p_buffer, uint32_t *p_size)
{
  char path[sizeof (HB_WASM_MODULE_DIR) + 64] = HB_WASM_MODULE_DIR "/";
  strncat (path, module_name, sizeof (path) - sizeof (HB_WASM_MODULE_DIR) - 16);
  strncat (path, ".wasm", 6);

  auto *blob = hb_blob_create_from_file (path);

  unsigned length;
  auto *data = hb_blob_get_data (blob, &length);

  *p_buffer = (uint8_t *) hb_malloc (length);

  if (length && !p_buffer)
    return false;

  memcpy (*p_buffer, data, length);
  *p_size = length;

  hb_blob_destroy (blob);

  return true;
}

static void HB_UNUSED
_hb_wasm_module_destroyer (uint8_t *buffer, uint32_t size)
{
  hb_free (buffer);
}
#endif

/*
 * shaper face data
 */

#define HB_WASM_TAG_WASM HB_TAG('W','a','s','m')

struct hb_wasm_face_data_t {
  hb_blob_t *wasm_blob;
  wasm_module_t wasm_module;
  /* The maximum the module declares for its memory, in pages. */
  uint32_t module_max_pages;
  /* The part of an instance's memory allowance that does not depend on
   * the buffer, in bytes. */
  uint64_t max_memory_base;
  /* The heap the runtime keeps in an instance for what we allocate in
   * it: the tables, outlines and buffer contents a module asks for. */
  uint32_t heap_size;
  mutable hb_atomic_t<hb_wasm_shape_plan_t *> plan;
};

static bool
_hb_wasm_init ()
{
  /* XXX
   *
   * Umm. Make this threadsafe. How?!
   * It's clunky that we can't allocate a static mutex.
   * So we have to first allocate one on the heap atomically...
   *
   * Do we also need to lock around module creation?
   *
   * Also, wasm-micro-runtime uses a singleton instance. So if
   * another library or client uses it, all bets are off. :-(
   */

  static bool initialized;
  if (initialized)
    return true;

  RuntimeInitArgs init_args;
  hb_memset (&init_args, 0, sizeof (RuntimeInitArgs));

  init_args.mem_alloc_type = Alloc_With_Allocator;
  init_args.mem_alloc_option.allocator.malloc_func = (void *) hb_malloc;
  init_args.mem_alloc_option.allocator.realloc_func = (void *) hb_realloc;
  init_args.mem_alloc_option.allocator.free_func = (void *) hb_free;

  // Native symbols need below registration phase
  init_args.n_native_symbols = ARRAY_LENGTH (_hb_wasm_native_symbols);
  init_args.native_module_name = "env";
  init_args.native_symbols = _hb_wasm_native_symbols;

  if (unlikely (!wasm_runtime_full_init (&init_args)))
  {
    DEBUG_MSG (WASM, nullptr, "Init runtime environment failed.");
    return false;
  }

#ifndef HB_WASM_NO_MODULES
  wasm_runtime_set_module_reader (_hb_wasm_module_reader,
				  _hb_wasm_module_destroyer);
#endif

  initialized = true;
  return true;
}

#ifndef HB_NO_WASM_BUDGET

/* Whether the module has a start section: a function that the runtime
 * runs when it makes an instance. */
static bool
_hb_wasm_module_has_start (const uint8_t *data, unsigned length)
{
  /* The magic number and the version, then the sections: an id, a size
   * in LEB128, and that many bytes.  The runtime has accepted the
   * module, so this only fails safe. */
  unsigned i = 8;
  while (i < length)
  {
    unsigned id = data[i++];
    uint32_t size = 0;
    for (unsigned shift = 0;; shift += 7)
    {
      if (unlikely (i >= length || shift > 28))
	return true;
      unsigned byte = data[i++];
      size |= (uint32_t) (byte & 0x7F) << shift;
      if (!(byte & 0x80))
	break;
    }
    if (id == 8 || unlikely (size > length - i))
      return true;
    i += size;
  }
  return false;
}

/* Returns why a module is not to run, or nullptr.
 *
 * What we can limit is what a module does in the calls we make into it,
 * and what our own functions do for it.  A module can have the runtime
 * run more than that, in three ways:
 *
 *   - It imports a function that is not ours and that the runtime
 *     has for it: a runtime built with its defaults has a C library
 *     and WASI of its own.  An import that nothing provides is left
 *     alone; calling it ends the call.  Modules made with wasm-bindgen
 *     have one, for JavaScript glue that they never call here.
 *
 *   - It has a function that the runtime runs when it makes an
 *     instance: the start function, or one of the three the runtime
 *     calls by name.
 *
 *   - It exports an allocator.  The runtime then leaves the instance
 *     without a heap of its own, and runs the module's allocator for
 *     every allocation we make in it.
 *
 * The names are those wasm-micro-runtime looks for, as of 2.4.6. */
static const char *
_hb_wasm_module_refusal (wasm_module_t wasm_module,
			 const uint8_t *data, unsigned length)
{
  for (int32_t i = 0; i < wasm_runtime_get_import_count (wasm_module); i++)
  {
    wasm_import_t import;
    wasm_runtime_get_import_type (wasm_module, i, &import);
    if (import.kind != WASM_IMPORT_EXPORT_KIND_FUNC || !import.linked)
      continue;

    bool ours = false;
    if (0 == strcmp (import.module_name, "env"))
      for (unsigned j = 0; j < ARRAY_LENGTH (_hb_wasm_native_symbols); j++)
	if (0 == strcmp (import.name, _hb_wasm_native_symbols[j].symbol))
	{
	  ours = true;
	  break;
	}
    if (!ours)
      return "imports a function of the runtime's";
  }

  if (_hb_wasm_module_has_start (data, length))
    return "has a start function";

  bool has_malloc = false, has_free = false;
  for (int32_t i = 0; i < wasm_runtime_get_export_count (wasm_module); i++)
  {
    wasm_export_t export_type;
    wasm_runtime_get_export_type (wasm_module, i, &export_type);
    if (export_type.kind != WASM_IMPORT_EXPORT_KIND_FUNC)
      continue;

    const char *name = export_type.name;
    if (0 == strcmp (name, "_initialize") ||
	0 == strcmp (name, "__post_instantiate") ||
	0 == strcmp (name, "__wasm_call_ctors"))
      return "exports a function that the runtime runs by itself";

    if (0 == strcmp (name, "malloc") ||
	0 == strcmp (name, "__new"))
      has_malloc = true;
    if (0 == strcmp (name, "free") ||
	0 == strcmp (name, "__release") ||
	0 == strcmp (name, "__unpin"))
      has_free = true;
  }
  if (has_malloc && has_free)
    return "exports an allocator";

  return nullptr;
}

#endif

hb_wasm_face_data_t *
_hb_wasm_shaper_face_data_create (hb_face_t *face)
{
  char error[128];
  hb_wasm_face_data_t *data = nullptr;
  wasm_module_t wasm_module = nullptr;

  hb_unique_ptr_t<hb_blob_t> wasm_blob (hb_face_reference_table (face, HB_WASM_TAG_WASM));

  unsigned length = hb_blob_get_length (wasm_blob);
  if (!length)
    return nullptr;

  uint8_t *wasm_data = (uint8_t *) hb_blob_get_data_writable (wasm_blob, nullptr);

  /* The runtime also loads modules that were compiled ahead of time,
   * which are native code, and runs them as they are.  Take nothing
   * from a font but WebAssembly itself. */
  if (unlikely (!wasm_data || length < 4 ||
		0 != hb_memcmp (wasm_data, "\0asm", 4)))
  {
    DEBUG_MSG (WASM, nullptr, "Wasm table is not a WebAssembly module.");
    return nullptr;
  }

  if (!_hb_wasm_init ())
    return nullptr;

  wasm_module = wasm_runtime_load (wasm_data, length, error, sizeof (error));
  if (unlikely (!wasm_module))
  {
    DEBUG_MSG (WASM, nullptr, "Load wasm module failed: %s", error);
    return nullptr;
  }
  auto module_guard = hb_make_scope_guard ([&]() { wasm_runtime_unload (wasm_module); });

#ifndef HB_NO_WASM_BUDGET
  const char *refusal = _hb_wasm_module_refusal (wasm_module, wasm_data, length);
  if (unlikely (refusal))
  {
    DEBUG_MSG (WASM, nullptr, "Not running the font's module: it %s.", refusal);
    return nullptr;
  }
#endif

  data = (hb_wasm_face_data_t *) hb_calloc (1, sizeof (hb_wasm_face_data_t));
  if (unlikely (!data))
    return nullptr;

  data->wasm_blob = wasm_blob.release ();
  data->wasm_module = wasm_module;

  /* The runtime complains, each time, when told to hold a module to more
   * than the module's own maximum.  Modules export their memory, so look
   * that maximum up. */
  data->module_max_pages = 65536;
  for (int32_t i = 0; i < wasm_runtime_get_export_count (wasm_module); i++)
  {
    wasm_export_t export_type;
    wasm_runtime_get_export_type (wasm_module, i, &export_type);
    if (export_type.kind == WASM_IMPORT_EXPORT_KIND_MEMORY)
      data->module_max_pages = wasm_memory_type_get_max_page_count (export_type.u.memory_type);
  }

  hb_unique_ptr_t<hb_blob_t> face_blob (hb_face_reference_blob (face));
  data->max_memory_base = HB_WASM_MAX_MEMORY_MIN +
			  (uint64_t) HB_WASM_MAX_MEMORY_FACE_FACTOR * hb_blob_get_length (face_blob);
  /* Room for a module to hold every table of its font at once. */
  data->heap_size = 2 * 1024 * 1024 +
		    hb_min (hb_blob_get_length (face_blob), 1u << 30);

  module_guard.release ();
  return data;
}

/* Sets how many instructions the next call into the module may run,
 * for a buffer of the given length, and how much work host functions
 * may do for it in that call: as much again. */
static void
limit_instructions (hb_wasm_shape_plan_t *plan HB_UNUSED,
		    wasm_exec_env_t exec_env HB_UNUSED,
		    unsigned buffer_len HB_UNUSED)
{
#ifndef HB_NO_WASM_BUDGET
  uint64_t limit = (uint64_t) HB_WASM_MAX_INSTRUCTIONS_MIN +
		   (uint64_t) HB_WASM_MAX_INSTRUCTIONS_FACTOR * buffer_len;
  wasm_runtime_set_instruction_count_limit (exec_env,
					    (int) hb_min (limit, (uint64_t) INT_MAX));
  plan->work_left = limit;
#endif
}

static void
release_shape_plan (const hb_wasm_face_data_t *face_data,
		    hb_wasm_shape_plan_t *plan,
		    bool cache = false);

#ifndef HB_NO_WASM_BUDGET
/* The memory an instance may use for a buffer of this length, in bytes. */
static uint64_t
max_memory_for (const hb_wasm_face_data_t *face_data, unsigned buffer_len)
{
  return face_data->max_memory_base +
	 (uint64_t) HB_WASM_MAX_MEMORY_BUFFER_FACTOR *
	 (sizeof (hb_glyph_info_t) + sizeof (hb_glyph_position_t)) *
	 buffer_len;
}
#endif

static hb_wasm_shape_plan_t *
acquire_shape_plan (hb_face_t *face,
		    const hb_wasm_face_data_t *face_data,
		    unsigned buffer_len)
{
  char error[128];

  /* Fetch cached one if available. */
  hb_wasm_shape_plan_t *plan = face_data->plan.get_acquire ();
  if (likely (plan && face_data->plan.cmpexch (plan, nullptr)))
  {
    if (likely (buffer_len <= plan->max_buffer_len))
    {
#ifndef HB_NO_WASM_BUDGET
      /* An instance that serves buffers of any length was made for a
       * shorter one; see below. */
      plan->max_memory = hb_max (plan->max_memory,
				 max_memory_for (face_data, buffer_len));
#endif
      return plan;
    }

    /* How much memory an instance may use is fixed when it is created,
     * and this one was created for shorter buffers. */
    release_shape_plan (face_data, plan);
  }

  plan = (hb_wasm_shape_plan_t *) hb_calloc (1, sizeof (hb_wasm_shape_plan_t));
  if (unlikely (!plan))
    return nullptr;
  auto plan_guard = hb_make_scope_guard ([&]() {
    plan->drop_refs ();
    plan->refs.fini ();
    hb_free (plan);
  });

  constexpr uint32_t stack_size = 32 * 1024;
  uint32_t heap_size = face_data->heap_size;

  InstantiationArgs args;
  hb_memset (&args, 0, sizeof (args));
  args.default_stack_size = stack_size;
  args.host_managed_heap_size = heap_size;

#ifndef HB_NO_WASM_BUDGET
  /* Leave room for buffers longer than this one, so that the instance
   * can be kept for them. */
  plan->max_buffer_len = 1024;
  while (plan->max_buffer_len < buffer_len)
    plan->max_buffer_len *= 2;
  plan->max_memory = max_memory_for (face_data, plan->max_buffer_len);
  uint32_t max_pages = (uint32_t) hb_min ((plan->max_memory + 65535) / 65536, (uint64_t) 65536);
  /* Zero leaves the module its own maximum. */
  args.max_memory_pages = face_data->module_max_pages <= max_pages ? 0 : max_pages;
  /* A module whose own maximum is within what we allow is not held by
   * our limit, now or for a longer buffer.  Its instance serves them
   * all; making another would only have the module set itself up
   * again, which for some is most of their work. */
  if (!args.max_memory_pages)
    plan->max_buffer_len = (unsigned) -1;
#else
  plan->max_buffer_len = (unsigned) -1;
  plan->max_memory = (uint64_t) -1;
#endif

  wasm_module_inst_t module_inst = plan->module_inst = wasm_runtime_instantiate_ex (
      face_data->wasm_module, &args, error, sizeof (error));
  if (unlikely (!module_inst))
  {
    DEBUG_MSG (WASM, face_data, "Create wasm module instance failed: %s", error);
    return nullptr;
  }
  auto module_guard = hb_make_scope_guard ([&]() { wasm_runtime_deinstantiate (module_inst); });

#ifndef HB_NO_WASM_BUDGET
  /* The runtime holds a module to the limit as it grows, but not below
   * the memory it starts with.  The heap we asked for comes on top of
   * the module's own pages, rounded up to one more. */
  uint64_t memory_start, memory_end;
  if (unlikely (!wasm_runtime_get_app_addr_range (module_inst, 0, &memory_start, &memory_end) ||
		memory_end > (uint64_t) max_pages * 65536 + heap_size + 65536))
  {
    DEBUG_MSG (WASM, face_data, "Wasm module starts with too much memory.");
    return nullptr;
  }

  /* Instructions are only counted in the interpreter. */
  if (unlikely (!wasm_runtime_set_running_mode (module_inst, Mode_Interp)))
  {
    DEBUG_MSG (WASM, face_data, "The runtime has no interpreter to count instructions in.");
    return nullptr;
  }
#endif

  wasm_runtime_set_custom_data (module_inst, plan);

  /* The face keeps one handle for as long as the instance lives. */
  HB_OBJ2REF (face);
  if (unlikely (!faceref))
  {
    DEBUG_MSG (WASM, face_data, "Failed to register face object.");
    return nullptr;
  }

  wasm_exec_env_t exec_env = plan->exec_env = wasm_runtime_create_exec_env (module_inst, stack_size);
  if (unlikely (!exec_env)) {
    DEBUG_MSG (WASM, face_data, "Create wasm execution environment failed.");
    return nullptr;
  }
  auto exec_guard = hb_make_scope_guard ([&]() { wasm_runtime_destroy_exec_env (exec_env); });

  wasm_function_inst_t func = wasm_runtime_lookup_function (module_inst, "shape_plan_create");
  if (func)
  {
    wasm_val_t results[1];
    wasm_val_t arguments[1];

    results[0].kind = WASM_I32;
    arguments[0].kind = WASM_I32;
    arguments[0].of.i32 = faceref;
    limit_instructions (plan, exec_env, 0);
    bool ret = wasm_runtime_call_wasm_a (exec_env, func,
					 ARRAY_LENGTH (results), results,
					 ARRAY_LENGTH (arguments), arguments);

    if (unlikely (!ret))
    {
      DEBUG_MSG (WASM, module_inst, "Calling shape_plan_create() failed: %s",
		 wasm_runtime_get_exception (module_inst));
      return nullptr;
    }
    plan->wasm_shape_planptr = results[0].of.i32;
  }

  plan_guard.release ();
  module_guard.release ();
  exec_guard.release ();
  return plan;
}

static void
release_shape_plan (const hb_wasm_face_data_t *face_data,
		    hb_wasm_shape_plan_t *plan,
		    bool cache)
{
  /* The faces and fonts a module creates and does not destroy live as
   * long as its instance.  Kept on the face, an instance that owns any
   * would collect more of them with every call, and a font created on
   * our face would keep the face itself alive. */
  if (cache && !plan->num_owned && !plan->low_on_refs () &&
      face_data->plan.cmpexch (nullptr, plan))
    return;

  auto *module_inst = plan->module_inst;
  auto *exec_env = plan->exec_env;

  /* The module's handles go before its shape_plan_destroy() runs.  We
   * may be here because the face is being destroyed, and a handle to
   * that face must not reach a host function. */
  plan->drop_refs ();

  /* Is there even any point to having a shape_plan_destroy function
   * and calling it? */
  if (plan->wasm_shape_planptr)
  {

    auto *func = wasm_runtime_lookup_function (module_inst, "shape_plan_destroy");
    if (func)
    {
      wasm_val_t arguments[1];

      arguments[0].kind = WASM_I32;
      arguments[0].of.i32 = plan->wasm_shape_planptr;
      limit_instructions (plan, exec_env, 0);
      bool ret = wasm_runtime_call_wasm_a (exec_env, func,
					   0, nullptr,
					   ARRAY_LENGTH (arguments), arguments);

      if (unlikely (!ret))
      {
	DEBUG_MSG (WASM, module_inst, "Calling shape_plan_destroy() failed: %s",
		   wasm_runtime_get_exception (module_inst));
      }
    }
  }

  wasm_runtime_destroy_exec_env (exec_env);
  wasm_runtime_deinstantiate (module_inst);
  plan->drop_refs ();
  plan->refs.fini ();
  hb_free (plan);
}

void
_hb_wasm_shaper_face_data_destroy (hb_wasm_face_data_t *data)
{
  if (data->plan.get_relaxed ())
    release_shape_plan (data, data->plan);
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
  if (unlikely (buffer->in_error ()))
    return false;

  bool ret = true;
  hb_face_t *face = font->face;
  const hb_wasm_face_data_t *face_data = face->data.wasm;

  bool retried = false;
  if (0)
  {
retry:
    DEBUG_MSG (WASM, font, "Retrying...");
  }

  wasm_function_inst_t func = nullptr;

  hb_wasm_shape_plan_t *plan = acquire_shape_plan (face, face_data, buffer->len);
  if (unlikely (!plan))
  {
    DEBUG_MSG (WASM, face_data, "Acquiring shape-plan failed.");
    return false;
  }

  auto *module_inst = plan->module_inst;
  auto *exec_env = plan->exec_env;
  unsigned num_refs = plan->refs.length;

  /* What the faces a module creates hold counts against its memory, from
   * when it last owned nothing. */
  if (!plan->num_owned)
    plan->face_bytes = 0;

  HB_OBJ2REF (font);
  HB_OBJ2REF (buffer);
  if (unlikely (!fontref || !bufferref))
  {
    DEBUG_MSG (WASM, module_inst, "Failed to register objects.");
    goto fail;
  }

  func = wasm_runtime_lookup_function (module_inst, "shape");
  if (unlikely (!func))
  {
    DEBUG_MSG (WASM, module_inst, "Shape function not found.");
    goto fail;
  }

  wasm_val_t results[1];
  wasm_val_t arguments[5];

  uint32_t featuresptr;
  featuresptr = num_features ? HB_MODULE_DUP (features,
					      num_features * sizeof (features[0])) : 0;

  results[0].kind = WASM_I32;
  arguments[0].kind = WASM_I32;
  arguments[0].of.i32 = plan->wasm_shape_planptr;
  arguments[1].kind = WASM_I32;
  arguments[1].of.i32 = fontref;
  arguments[2].kind = WASM_I32;
  arguments[2].of.i32 = bufferref;
  arguments[3].kind = WASM_I32;
  arguments[3].of.i32 = featuresptr;
  arguments[4].kind = WASM_I32;
  arguments[4].of.i32 = num_features;

  limit_instructions (plan, exec_env, buffer->len);
  ret = wasm_runtime_call_wasm_a (exec_env, func,
				  ARRAY_LENGTH (results), results,
				  ARRAY_LENGTH (arguments), arguments);

  if (num_features)
    HB_MODULE_FREE (featuresptr);

  if (unlikely (!ret || !results[0].of.i32))
  {
    DEBUG_MSG (WASM, module_inst, "Calling shape() failed: %s",
	       wasm_runtime_get_exception (module_inst));
    if (!buffer->ensure_unicode ())
    {
      DEBUG_MSG (WASM, font, "Shape failed but buffer is not in Unicode; failing...");
      goto fail;
    }
#ifndef HB_NO_WASM_BUDGET
    /* A module that ran out of instructions would do so again.  This is
     * the runtime's message; if it ever changes, we retry needlessly. */
    const char *exception;
    exception = wasm_runtime_get_exception (module_inst);
    if (exception && strstr (exception, "limit exceeded"))
      goto fail;
#endif
    if (retried)
    {
      DEBUG_MSG (WASM, font, "Giving up...");
      goto fail;
    }
    buffer->successful = true;
    retried = true;
    release_shape_plan (face_data, plan);
    plan = nullptr;
    goto retry;
  }

  /* TODO Regularize clusters according to direction & cluster level,
   * such that client doesn't crash with unmet expectations. */

  if (!results[0].of.i32)
  {
fail:
    ret = false;
  }

  /* A module that still owns an object expects it to be there in the
   * next call.  It gets a fresh instance instead, and starts over. */
  bool keep = ret && !plan->num_owned;

  /* The instance may be kept for the next call, and the font and the
   * buffer may be gone by then.  Drop the handles made during this
   * call, so that a module that passes one again gets the empty
   * object. */
  plan->drop_refs (num_refs);

  release_shape_plan (face_data, plan, keep);

  if (ret)
  {
    buffer->clear_glyph_flags ();
    buffer->unsafe_to_break ();
  }

  return ret;
}

#endif
