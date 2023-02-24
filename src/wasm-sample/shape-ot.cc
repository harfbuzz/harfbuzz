#define HB_WASM_INTERFACE(ret_t, name) __attribute__((export_name(#name))) ret_t name

#include <hb-wasm-api.h>

extern "C" {
void debugprint1 (const char *s, int32_t);
void debugprint2 (const char *s, int32_t, int32_t);
}

bool_t
shape (font_t *font, buffer_t *buffer)
{
  return shape_with (font, buffer, "ot");
}
