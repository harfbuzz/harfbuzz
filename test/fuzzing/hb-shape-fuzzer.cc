#include "hb-fuzzer.hh"

#include <hb-ot.h>
#include <string.h>

#define TEST_OT_FACE_NO_MAIN 1
#include "../api/test-ot-face.c"
#undef TEST_OT_FACE_NO_MAIN

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  hb_blob_t *blob = hb_blob_create ((const char *)data, size,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);
  hb_ot_font_set_funcs (font);
  hb_font_set_scale (font, 12, 12);

  {
    const char text[] = "ABCDEXYZ123@_%&)*$!";
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties (buffer);
    hb_shape (font, buffer, nullptr, 0);
    hb_buffer_destroy (buffer);
  }

  uint32_t text32[16] = {0};
  unsigned int len = sizeof (text32);
  if (size < len)
    len = size;
  memcpy(text32, data + size - len, len);

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf32 (buffer, text32, sizeof (text32) / sizeof (text32[0]), 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, nullptr, 0);
  hb_buffer_destroy (buffer);

  /* Misc calls on face. */
  test_face (face, text32[15]);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}
