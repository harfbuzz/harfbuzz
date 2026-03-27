#include "hb-shape-input.hh"

#include <hb-gpu.h>

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  _fuzzing_skip_leading_comment (&data, &size);
  alloc_state = _fuzzing_alloc_state (data, size);

  _fuzzing_shape_input_t input;
  if (_fuzzing_prepare_shape_input (data, size, 30, 30, &input) == HB_FUZZING_SHAPE_INPUT_MALFORMED)
    return 0;

  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  if (!draw)
    return 0;

  unsigned glyph_count = hb_face_get_glyph_count (input.face);
  unsigned limit = glyph_count > 16 ? 16 : glyph_count;

  volatile unsigned counter = !glyph_count;

  for (unsigned gid = 0; gid < limit; gid++)
  {
    hb_gpu_draw_reset (draw);
    hb_gpu_draw_glyph (draw, input.font, gid);

    hb_glyph_extents_t extents;
    hb_gpu_draw_get_extents (draw, &extents);
    counter += extents.width;

    hb_blob_t *blob = hb_gpu_draw_encode (draw);
    if (blob)
    {
      unsigned length = 0;
      const char *blob_data = hb_blob_get_data (blob, &length);
      counter += length;
      if (blob_data && length)
	counter += (unsigned char) blob_data[0];
      hb_gpu_draw_recycle_blob (draw, blob);
    }
  }

  hb_gpu_draw_destroy (draw);
  return counter ? 0 : 0;
}
