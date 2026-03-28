#include "hb-shape-input.hh"

#define TEST_OT_FACE_NO_MAIN 1
#include "../api/test-ot-face.c"
#undef TEST_OT_FACE_NO_MAIN

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  alloc_state = _fuzzing_alloc_state (data, size);

  _fuzzing_shape_input_t input;
  switch (_fuzzing_prepare_shape_input (data, size, 12, 12, &input))
  {
    case HB_FUZZING_SHAPE_INPUT_MALFORMED:
      return 0;
    case HB_FUZZING_SHAPE_INPUT_RAW:
    case HB_FUZZING_SHAPE_INPUT_EXTENDED:
      break;
  }

  {
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_buffer_set_flags (buffer, (hb_buffer_flags_t) (HB_BUFFER_FLAG_VERIFY));
    hb_buffer_add_utf8 (buffer, "ABCDEXYZ123@_%&)*$!", -1, 0, -1);
    hb_buffer_guess_segment_properties (buffer);
    hb_shape (input.font, buffer, nullptr, 0);
    hb_buffer_destroy (buffer);
  }

  uint32_t text32[16] = {0};
  unsigned int len = input.text.size () > 16 ? 16 : (unsigned int) input.text.size ();
  for (unsigned int i = 0; i < len; i++)
    text32[i] = input.text[i];

  text32[10] = test_font (input.font, text32[15]) % 256;

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf32 (buffer, text32, sizeof (text32) / sizeof (text32[0]), 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (input.font, buffer, nullptr, 0);
  hb_buffer_destroy (buffer);

  return 0;
}
