#include "hb-shape-input.hh"

#include <hb-vector.h>

static void
shape_ascii_smoke (hb_font_t *font)
{
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, "ABCDEXYZ123@_%&)*$!", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, nullptr, 0);
  hb_buffer_destroy (buffer);
}

static void
draw_shaped_text (hb_font_t *font,
                  hb_buffer_t *buffer,
                  hb_vector_draw_t *draw,
                  hb_vector_paint_t *paint)
{
  float pen_x = 0.f;
  float pen_y = 0.f;
  unsigned count = hb_buffer_get_length (buffer);
  hb_glyph_info_t *infos = hb_buffer_get_glyph_infos (buffer, nullptr);
  hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (buffer, nullptr);

  for (unsigned i = 0; i < count; i++)
  {
    float x = pen_x + positions[i].x_offset;
    float y = pen_y + positions[i].y_offset;

    hb_vector_draw_glyph (draw, font, infos[i].codepoint, x, y, HB_VECTOR_EXTENTS_MODE_EXPAND);
    hb_vector_paint_glyph (paint, font, infos[i].codepoint, x, y, HB_VECTOR_EXTENTS_MODE_EXPAND);

    pen_x += positions[i].x_advance;
    pen_y += positions[i].y_advance;
  }
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  _fuzzing_skip_leading_comment (&data, &size);
  alloc_state = _fuzzing_alloc_state (data, size);

  _fuzzing_shape_input_t input;
  if (_fuzzing_prepare_shape_input (data, size, 30, 30, &input) == HB_FUZZING_SHAPE_INPUT_MALFORMED)
    return 0;

  shape_ascii_smoke (input.font);

  hb_buffer_t *buffer = hb_buffer_create ();
  if (!input.text.empty ())
    hb_buffer_add_codepoints (buffer, input.text.data (), input.text.size (), 0, input.text.size ());
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (input.font, buffer, nullptr, 0);

  hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (HB_VECTOR_FORMAT_SVG);
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);
  hb_vector_paint_set_foreground (paint, HB_COLOR (0, 0, 0, 255));

  unsigned precision = size ? data[size - 1] % 5 : 0;
  hb_vector_svg_set_precision (draw, precision);
  hb_vector_svg_paint_set_precision (paint, precision);
  hb_vector_svg_set_flat (draw, size > 1 ? !!(data[size - 2] & 1) : false);
  hb_vector_svg_paint_set_flat (paint, size > 2 ? !!(data[size - 3] & 1) : false);

  draw_shaped_text (input.font, buffer, draw, paint);

  unsigned glyph_count = hb_face_get_glyph_count (input.face);
  unsigned limit = glyph_count > 16 ? 16 : glyph_count;
  for (unsigned gid = 0; gid < limit; gid++)
  {
    float x = (float) ((gid % 4) * 40);
    float y = (float) ((gid / 4) * 40);
    hb_vector_draw_glyph (draw, input.font, gid, x, y, HB_VECTOR_EXTENTS_MODE_EXPAND);
    hb_vector_paint_glyph (paint, input.font, gid, x, y, HB_VECTOR_EXTENTS_MODE_EXPAND);
  }

  volatile unsigned counter = !glyph_count;

  hb_blob_t *draw_blob = hb_vector_draw_render (draw);
  if (draw_blob)
  {
    unsigned length = 0;
    const char *blob_data = hb_blob_get_data (draw_blob, &length);
    counter += length;
    if (blob_data && length)
      counter += (unsigned char) blob_data[0];
    hb_vector_draw_recycle_blob (draw, draw_blob);
  }

  hb_blob_t *paint_blob = hb_vector_paint_render (paint);
  if (paint_blob)
  {
    unsigned length = 0;
    const char *blob_data = hb_blob_get_data (paint_blob, &length);
    counter += length;
    if (blob_data && length)
      counter += (unsigned char) blob_data[0];
    hb_vector_paint_recycle_blob (paint, paint_blob);
  }

  hb_vector_draw_destroy (draw);
  hb_vector_paint_destroy (paint);
  hb_buffer_destroy (buffer);
  return counter ? 0 : 0;
}

