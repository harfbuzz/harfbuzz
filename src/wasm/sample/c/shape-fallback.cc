#define HB_WASM_INTERFACE(ret_t, name) __attribute__((export_name(#name))) ret_t name

#include <hb-wasm-api.h>

extern "C" {
void debugprint (const char *s);
void debugprint1 (const char *s, int32_t);
void debugprint2 (const char *s, int32_t, int32_t);
}

bool_t
shape (void *shape_plan,
       font_t *font,
       buffer_t *buffer,
       const feature_t *features,
       uint32_t num_features)
{
  face_t *face = font_get_face (font);

  blob_t blob = face_copy_table (face, TAG ('c','m','a','p'));

  debugprint1 ("cmap length", blob.length);

  blob_free (&blob);

  buffer_contents_t contents;
  if (!buffer_copy_contents (buffer, &contents))
    return false;

  debugprint1 ("buffer length", contents.length);

  for (unsigned i = 0; i < contents.length; i++)
  {
    char name[64];

    font_glyph_to_string (font, contents.info[i].codepoint, name, sizeof (name));

    debugprint (name);

    contents.info[i].codepoint = font_get_glyph (font, contents.info[i].codepoint, 0);
    contents.pos[i].x_advance = font_get_glyph_h_advance (font, contents.info[i].codepoint);
  }

  bool_t ret = buffer_set_contents (buffer, &contents);

  buffer_contents_free (&contents);

  return ret;
}
