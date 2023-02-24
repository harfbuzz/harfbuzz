#define HB_WASM_INTERFACE(ret_t, name) __attribute__((export_name(#name))) ret_t name

#include <hb-wasm-api.h>

extern "C" {
void debugprint1 (const char *s, int32_t);
void debugprint2 (const char *s, int32_t, int32_t);
}

bool_t
shape (font_t *font, buffer_t *buffer)
{
  face_t *face = font_get_face (font);

  blob_t blob = face_reference_table (face, TAG ('c','m','a','p'));

  debugprint1 ("cmap length", blob.length);

  blob_free (&blob);

  buffer_contents_t contents = buffer_copy_contents (buffer);

  debugprint1 ("buffer length", contents.length);

  for (unsigned i = 0; i < contents.length; i++)
  {
    debugprint2 ("Codepoint", i, contents.info[i].codepoint);
    contents.info[i].codepoint = font_get_glyph (font, contents.info[i].codepoint, 0);
    contents.pos[i].x_advance = font_get_glyph_h_advance (font, contents.info[i].codepoint);
  }

  bool_t ret = buffer_set_contents (buffer, &contents);

  buffer_contents_free (&contents);

  return ret;
}
