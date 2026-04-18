#include "hb-shape-input.hh"

#include <hb-raster.h>

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  alloc_state = _fuzzing_alloc_state (data, size);

  _fuzzing_shape_input_t input;
  if (_fuzzing_prepare_shape_input (data, size, 30, 30, &input) == HB_FUZZING_SHAPE_INPUT_MALFORMED)
    return 0;

  hb_raster_draw_t *draw = hb_raster_draw_create_or_fail ();
  hb_raster_paint_t *paint = hb_raster_paint_create_or_fail ();
  if (!draw || !paint)
  {
    hb_raster_draw_destroy (draw);
    hb_raster_paint_destroy (paint);
    return 0;
  }
  hb_raster_paint_set_foreground (paint, HB_COLOR (0, 0, 0, 255));

  /* Cap font scale so malicious fonts don't produce huge surfaces. */
  int x_scale, y_scale;
  hb_font_get_scale (input.font, &x_scale, &y_scale);
  if (x_scale > 1000 || x_scale < -1000) x_scale = 1000;
  if (y_scale > 1000 || y_scale < -1000) y_scale = 1000;
  hb_font_set_scale (input.font, x_scale, y_scale);

  unsigned glyph_count = hb_face_get_glyph_count (input.face);
  unsigned limit = glyph_count > 16 ? 16 : glyph_count;

  volatile unsigned counter = !glyph_count;

  for (unsigned gid = 0; gid < limit; gid++)
  {
    float x = (float) ((gid % 4) * 40);
    float y = (float) ((gid / 4) * 40);
    hb_raster_draw_set_transform (draw, 1.f, 0.f, 0.f, 1.f, x, y);
    hb_raster_draw_glyph (draw, input.font, gid);
    hb_raster_paint_set_transform (paint, 1.f, 0.f, 0.f, 1.f, x, y);
    hb_raster_paint_glyph (paint, input.font, gid);
  }

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
  return counter ? 0 : 0;
}
