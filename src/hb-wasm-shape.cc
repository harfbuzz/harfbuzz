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

#include "hb-shaper-impl.hh"

#ifdef HAVE_WASM

#include <wasm_c_api.h>

#define own // wasm-micro-runtime wasm-c-api/hello.c example has this; no idea why :))

static wasm_store_t *
get_wasm_store ()
{

  static wasm_store_t *store;
  if (!store)
  {
    static wasm_engine_t *engine = wasm_engine_new();
    store = wasm_store_new (engine);
  }

  return store;
}


/*
 * shaper face data
 */

#define HB_WASM_TAG_WASM HB_TAG('W','a','s','m')

struct hb_wasm_face_data_t {
  hb_blob_t *blob;
  wasm_module_t *mod;
  unsigned shape_func_idx;
};

hb_wasm_face_data_t *
_hb_wasm_shaper_face_data_create (hb_face_t *face)
{
  wasm_store_t *wasm_store = nullptr;
  hb_blob_t *wasm_blob = nullptr;
  own wasm_module_t *wasm_module = nullptr;
  hb_wasm_face_data_t *data = nullptr;
  wasm_exporttype_vec_t exports = {};
  unsigned shape_func_idx = (unsigned) -1;

  wasm_blob = hb_face_reference_table (face, HB_WASM_TAG_WASM);
  unsigned length = hb_blob_get_length (wasm_blob);
  if (!length)
    goto fail;

  wasm_store = get_wasm_store (); // Do before others to initialize

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized (&binary, length);
  memcpy (binary.data, hb_blob_get_data (wasm_blob, nullptr), length);
  wasm_module = wasm_module_new (wasm_store, &binary);
  if (!wasm_module)
    goto fail;
  wasm_byte_vec_delete(&binary);

  wasm_module_exports (wasm_module, &exports);
  for (unsigned i = 0; i < exports.size; i++)
  {
    auto *name = wasm_exporttype_name(exports.data[i]);
    if (0 == memcmp ("shape", name->data, name->size))
    {
      shape_func_idx = i;
      break;
    }
  }
  if (shape_func_idx == (unsigned) -1)
    goto fail;

  data = (hb_wasm_face_data_t *) hb_calloc (1, sizeof (hb_wasm_face_data_t));
  if (unlikely (!data))
    goto fail;

  data->blob = wasm_blob;
  data->mod = wasm_module;
  data->shape_func_idx = shape_func_idx;

  return data;

fail:
  wasm_exporttype_vec_delete (&exports);
  if (wasm_module)
    wasm_module_delete (wasm_module);
  hb_blob_destroy (wasm_blob);
  hb_free (data);
  return nullptr;
}

void
_hb_wasm_shaper_face_data_destroy (hb_wasm_face_data_t *data)
{
  wasm_module_delete (data->mod);
  hb_blob_destroy (data->blob);
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
  own wasm_instance_t *instance = nullptr;
  const wasm_func_t *run_func = nullptr;
  wasm_trap_t *trap = nullptr;
  own wasm_extern_vec_t exports = {};

  wasm_val_t as[1] = { WASM_I32_VAL(6) };
  wasm_val_t rs[1] = { WASM_INIT_VAL };
  wasm_val_vec_t args = WASM_ARRAY_VEC (as);
  wasm_val_vec_t results = WASM_ARRAY_VEC (rs);

  instance = wasm_instance_new (get_wasm_store (), face_data->mod, nullptr/*imports*/, nullptr);
  if (!instance)
    goto fail;

  wasm_instance_exports (instance, &exports);
  if (exports.size == 0)
    goto fail;

  run_func = wasm_extern_as_func (exports.data[face_data->shape_func_idx]);
  if (!run_func)
    goto fail;

  trap = wasm_func_call (run_func, &args, &results);
  if (trap)
  {
    goto fail;
  }

  ret = bool (rs[0].of.i32);

  if (0)
  {
fail:
    ret = false;
  }

  wasm_extern_vec_delete (&exports);
  if (instance)
    wasm_instance_delete(instance);
  if (trap)
    wasm_trap_delete (trap);

  return ret;
}

#endif
