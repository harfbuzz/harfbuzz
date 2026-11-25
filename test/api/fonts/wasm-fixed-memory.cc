/* The Wasm module of wasm-fixed-memory.ttf, which test-wasm.c shapes
 * with.  Its memory has a maximum, set when it is linked, that is well
 * within what Harfbuzz allows a module.  generate-wasm-api-font.py
 * compiles it. */

#define HB_WASM_INTERFACE(ret_t, name) \
  __attribute__((export_name(#name))) ret_t name

#include <hb-wasm-api.h>

/* How many times this instance was called. */
static unsigned calls;

bool_t
shape (void *shape_plan, font_t *font, buffer_t *buffer,
       const feature_t *features, uint32_t num_features)
{
  calls++;

  buffer_contents_t contents = BUFFER_CONTENTS_INIT;
  if (!buffer_copy_contents (buffer, &contents))
    return false;
  unsigned length = contents.length;
  buffer_contents_free (&contents);

  /* A long buffer is to come second, to the same instance. */
  return length <= 1024 || calls == 2;
}
