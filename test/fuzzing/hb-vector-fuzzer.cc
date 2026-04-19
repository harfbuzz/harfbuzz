#include "hb-shape-input.hh"

#include <hb-vector.h>

static void
exercise_format (const _fuzzing_shape_input_t *input,
		 hb_vector_format_t format,
		 unsigned precision,
		 volatile unsigned *counter)
{
  hb_vector_draw_t  *draw  = hb_vector_draw_create_or_fail  (format);
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  if (!draw || !paint)
  {
    hb_vector_draw_destroy (draw);
    hb_vector_paint_destroy (paint);
    return;
  }
  hb_vector_paint_set_foreground (paint, HB_COLOR (0, 0, 0, 255));

  hb_vector_draw_set_precision  (draw,  precision);
  hb_vector_paint_set_precision (paint, precision);

  unsigned glyph_count = hb_face_get_glyph_count (input->face);
  unsigned limit = glyph_count > 16 ? 16 : glyph_count;

  for (unsigned gid = 0; gid < limit; gid++)
  {
    float x = (float) ((gid % 4) * 40);
    float y = (float) ((gid / 4) * 40);
    hb_vector_draw_set_transform (draw, 1.f, 0.f, 0.f, 1.f, x, y);
    hb_vector_draw_glyph  (draw,  input->font, gid, HB_VECTOR_EXTENTS_MODE_EXPAND);
    hb_vector_paint_set_transform (paint, 1.f, 0.f, 0.f, 1.f, x, y);
    hb_vector_paint_glyph (paint, input->font, gid, HB_VECTOR_EXTENTS_MODE_EXPAND);
  }

  hb_blob_t *draw_blob = hb_vector_draw_render (draw);
  if (draw_blob)
  {
    unsigned length = 0;
    const char *blob_data = hb_blob_get_data (draw_blob, &length);
    *counter += length;
    if (blob_data && length)
      *counter += (unsigned char) blob_data[0];
    hb_vector_draw_recycle_blob (draw, draw_blob);
  }

  hb_blob_t *paint_blob = hb_vector_paint_render (paint);
  if (paint_blob)
  {
    unsigned length = 0;
    const char *blob_data = hb_blob_get_data (paint_blob, &length);
    *counter += length;
    if (blob_data && length)
      *counter += (unsigned char) blob_data[0];
    hb_vector_paint_recycle_blob (paint, paint_blob);
  }

  hb_vector_draw_destroy (draw);
  hb_vector_paint_destroy (paint);
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  alloc_state = _fuzzing_alloc_state (data, size);

  _fuzzing_shape_input_t input;
  if (_fuzzing_prepare_shape_input (data, size, 30, 30, &input) == HB_FUZZING_SHAPE_INPUT_MALFORMED)
    return 0;

  unsigned precision = size ? data[size - 1] % 5 : 0;

  unsigned glyph_count = hb_face_get_glyph_count (input.face);
  volatile unsigned counter = !glyph_count;

  /* Exercise both output formats.  Each has its own serializer, path
   * encoder, gradient machinery, and (for PDF) indexed-PNG SMask +
   * Type 6 Coons-patch encoder. */
  exercise_format (&input, HB_VECTOR_FORMAT_SVG, precision, &counter);
  exercise_format (&input, HB_VECTOR_FORMAT_PDF, precision, &counter);

  return counter ? 0 : 0;
}
