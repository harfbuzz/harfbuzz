#include "hb-shape-input.hh"

#include <hb-raster.h>

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
                  hb_raster_draw_t *draw,
                  hb_raster_paint_t *paint)
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

    hb_raster_draw_glyph (draw, font, infos[i].codepoint, x, y);
    hb_raster_paint_glyph (paint, font, infos[i].codepoint, x, y, 0, HB_COLOR (0, 0, 0, 255));

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

  hb_raster_draw_t *draw = hb_raster_draw_create_or_fail ();
  hb_raster_paint_t *paint = hb_raster_paint_create_or_fail ();
  hb_raster_paint_set_foreground (paint, HB_COLOR (0, 0, 0, 255));

  draw_shaped_text (input.font, buffer, draw, paint);

  unsigned glyph_count = hb_face_get_glyph_count (input.face);
  unsigned limit = glyph_count > 16 ? 16 : glyph_count;
  for (unsigned gid = 0; gid < limit; gid++)
  {
    float x = (float) ((gid % 4) * 40);
    float y = (float) ((gid / 4) * 40);
    hb_raster_draw_glyph (draw, input.font, gid, x, y);
    hb_raster_paint_glyph (paint, input.font, gid, x, y, 0, HB_COLOR (0, 0, 0, 255));
  }

  volatile unsigned counter = !glyph_count;

  hb_raster_image_t *draw_image = hb_raster_draw_render (draw);
  if (draw_image)
  {
    hb_raster_extents_t ext;
    hb_raster_image_get_extents (draw_image, &ext);
    counter += ext.width + ext.height + (unsigned) (ext.x_origin != 0) + (unsigned) (ext.y_origin != 0);
    const uint8_t *buf = hb_raster_image_get_buffer (draw_image);
    if (buf && ext.height && ext.stride)
      counter += buf[0];
    hb_raster_draw_recycle_image (draw, draw_image);
  }

  hb_raster_image_t *paint_image = hb_raster_paint_render (paint);
  if (paint_image)
  {
    hb_raster_extents_t ext;
    hb_raster_image_get_extents (paint_image, &ext);
    counter += ext.width + ext.height + (unsigned) (ext.x_origin != 0) + (unsigned) (ext.y_origin != 0);
    const uint8_t *buf = hb_raster_image_get_buffer (paint_image);
    if (buf && ext.height && ext.stride)
      counter += buf[0];
    hb_raster_paint_recycle_image (paint, paint_image);
  }

  hb_raster_draw_destroy (draw);
  hb_raster_paint_destroy (paint);
  hb_buffer_destroy (buffer);
  return counter ? 0 : 0;
}

