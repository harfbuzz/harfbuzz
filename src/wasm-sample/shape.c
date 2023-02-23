#define HB_WASM_INTERFACE(ret_t, name) __attribute__((export_name(#name))) ret_t name

#include <hb-wasm-api.h>

void free (void*);

void debugprint1 (char *s, int32_t);
void debugprint2 (char *s, int32_t, int32_t);

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
    contents.info[i].codepoint++;
    contents.pos[i].x_advance = 256 * 64;
  }

  bool_t ret = buffer_set_contents (buffer, &contents);

  buffer_contents_free (&contents);

  return ret;
}
