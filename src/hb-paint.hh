/*
 * Copyright © 2022 Matthias Clasen
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef HB_PAINT_HH
#define HB_PAINT_HH

#include "hb.hh"
#include "hb-face.hh"
#include "hb-font.hh"
#include "hb-geometry.hh"

#define HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS \
  HB_PAINT_FUNC_IMPLEMENT (push_transform) \
  HB_PAINT_FUNC_IMPLEMENT (pop_transform) \
  HB_PAINT_FUNC_IMPLEMENT (color_glyph) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_glyph) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_rectangle) \
  HB_PAINT_FUNC_IMPLEMENT (pop_clip) \
  HB_PAINT_FUNC_IMPLEMENT (color) \
  HB_PAINT_FUNC_IMPLEMENT (image) \
  HB_PAINT_FUNC_IMPLEMENT (linear_gradient) \
  HB_PAINT_FUNC_IMPLEMENT (radial_gradient) \
  HB_PAINT_FUNC_IMPLEMENT (sweep_gradient) \
  HB_PAINT_FUNC_IMPLEMENT (push_group) \
  HB_PAINT_FUNC_IMPLEMENT (push_group_for) \
  HB_PAINT_FUNC_IMPLEMENT (pop_group) \
  HB_PAINT_FUNC_IMPLEMENT (custom_palette_color) \
  /* ^--- Add new callbacks here */

struct hb_paint_funcs_t
{
  hb_object_header_t header;

  struct {
#define HB_PAINT_FUNC_IMPLEMENT(name) hb_paint_##name##_func_t name;
    HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_PAINT_FUNC_IMPLEMENT
  } func;

  struct {
#define HB_PAINT_FUNC_IMPLEMENT(name) void *name;
    HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_PAINT_FUNC_IMPLEMENT
  } *user_data;

  struct {
#define HB_PAINT_FUNC_IMPLEMENT(name) hb_destroy_func_t name;
    HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_PAINT_FUNC_IMPLEMENT
  } *destroy;

  void push_transform (void *paint_data,
                       float xx, float yx,
                       float xy, float yy,
                       float dx, float dy)
  {
    // Handle -0.f to avoid -0.f == 0.f in the transform matrix.
    if (dx == -0.f) dx = 0.f;
    if (dy == -0.f) dy = 0.f;
    func.push_transform (this, paint_data,
                         xx, yx, xy, yy, dx, dy,
                         !user_data ? nullptr : user_data->push_transform); }
  void pop_transform (void *paint_data)
  { func.pop_transform (this, paint_data,
                        !user_data ? nullptr : user_data->pop_transform); }
  bool color_glyph (void *paint_data,
                    hb_codepoint_t glyph,
                    hb_font_t *font)
  { return func.color_glyph (this, paint_data,
                             glyph,
                             font,
                             !user_data ? nullptr : user_data->push_clip_glyph); }
  void push_clip_glyph (void *paint_data,
                        hb_codepoint_t glyph,
                        hb_font_t *font)
  { func.push_clip_glyph (this, paint_data,
                          glyph,
                          font,
                          !user_data ? nullptr : user_data->push_clip_glyph); }
  void push_clip_rectangle (void *paint_data,
                           float xmin, float ymin, float xmax, float ymax)
  { func.push_clip_rectangle (this, paint_data,
                              xmin, ymin, xmax, ymax,
                              !user_data ? nullptr : user_data->push_clip_rectangle); }
  void pop_clip (void *paint_data)
  { func.pop_clip (this, paint_data,
                   !user_data ? nullptr : user_data->pop_clip); }
  void color (void *paint_data,
              hb_bool_t is_foreground,
              hb_color_t color)
  { func.color (this, paint_data,
                is_foreground, color,
                !user_data ? nullptr : user_data->color); }
  bool image (void *paint_data,
              hb_blob_t *image,
              unsigned width, unsigned height,
              hb_tag_t format,
              float slant,
              hb_glyph_extents_t *extents)
  { return func.image (this, paint_data,
                       image, width, height, format, slant, extents,
                       !user_data ? nullptr : user_data->image); }
  void linear_gradient (void *paint_data,
                        hb_color_line_t *color_line,
                        float x0, float y0,
                        float x1, float y1,
                        float x2, float y2)
  { func.linear_gradient (this, paint_data,
                          color_line, x0, y0, x1, y1, x2, y2,
                          !user_data ? nullptr : user_data->linear_gradient); }
  void radial_gradient (void *paint_data,
                        hb_color_line_t *color_line,
                        float x0, float y0, float r0,
                        float x1, float y1, float r1)
  { func.radial_gradient (this, paint_data,
                          color_line, x0, y0, r0, x1, y1, r1,
                          !user_data ? nullptr : user_data->radial_gradient); }
  void sweep_gradient (void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0,
                       float start_angle,
                       float end_angle)
  { func.sweep_gradient (this, paint_data,
                         color_line, x0, y0, start_angle, end_angle,
                         !user_data ? nullptr : user_data->sweep_gradient); }
  void push_group (void *paint_data)
  { func.push_group (this, paint_data,
                     !user_data ? nullptr : user_data->push_group); }
  void push_group_for (void *paint_data,
                       hb_paint_composite_mode_t mode)
  { func.push_group_for (this, paint_data,
                         mode,
                         !user_data ? nullptr : user_data->push_group_for); }
  void pop_group (void *paint_data,
                  hb_paint_composite_mode_t mode)
  { func.pop_group (this, paint_data,
                    mode,
                    !user_data ? nullptr : user_data->pop_group); }
  bool custom_palette_color (void *paint_data,
                             unsigned int color_index,
                             hb_color_t *color)
  { return func.custom_palette_color (this, paint_data,
                                      color_index,
                                      color,
                                      !user_data ? nullptr : user_data->custom_palette_color); }


  /* Internal specializations. */

  void push_font_transform (void *paint_data,
                            const hb_font_t *font)
  {
    float upem = font->face->get_upem ();
    int xscale = font->x_scale, yscale = font->y_scale;

    push_transform (paint_data,
		    xscale/upem, 0,
		    0, yscale/upem,
		    0, 0);
  }

  void push_inverse_font_transform (void *paint_data,
                                    const hb_font_t *font)
  {
    float upem = font->face->get_upem ();
    int xscale = font->x_scale ? font->x_scale : upem;
    int yscale = font->y_scale ? font->y_scale : upem;

    push_transform (paint_data,
		    upem/xscale, 0,
		    0, upem/yscale,
		    0, 0);
  }

  void push_transform (void *paint_data, hb_transform_t<float> t)
  {
    push_transform (paint_data, t.xx, t.yx, t.xy, t.yy, t.x0, t.y0);
  }

  void push_translate (void *paint_data,
                       float dx, float dy)
  {
    push_transform (paint_data,
		    hb_transform_t<float>::translation (dx, dy));
  }

  void push_scale (void *paint_data,
                   float sx, float sy)
  {
    push_transform (paint_data,
		    hb_transform_t<float>::scaling (sx, sy));
  }
  void push_scale_around_center (void *paint_data,
				 float sx, float sy,
				 float cx, float cy)
  {
    push_transform (paint_data,
		    hb_transform_t<float>::scaling_around_center (sx, sy, cx, cy));
  }

  void push_rotate (void *paint_data,
                    float a)
  {
    push_transform (paint_data,
		    hb_transform_t<float>::rotation (a * HB_PI));
  }

  void push_rotate_around_center (void *paint_data,
				  float a,
				  float cx, float cy)
  {
    push_transform (paint_data,
		    hb_transform_t<float>::rotation_around_center (a * HB_PI, cx, cy));
  }

  void push_skew (void *paint_data,
                  float sx, float sy)
  {
    push_transform (paint_data,
		    hb_transform_t<float>::skewing (-sx * HB_PI, sy * HB_PI));
  }
  void push_skew_around_center (void *paint_data,
				float sx, float sy,
				float cx, float cy)
  {
    push_transform (paint_data,
		    hb_transform_t<float>::skewing_around_center (-sx * HB_PI, sy * HB_PI, cx, cy));
  }
};
DECLARE_NULL_INSTANCE (hb_paint_funcs_t);


/* Linearly interpolate between two hb_color_t values, component-wise,
 * in byte-channel space with rounding. */
static inline hb_color_t
hb_color_lerp (hb_color_t c0, hb_color_t c1, float t)
{
  auto lerp = [&] (unsigned shift) -> unsigned {
    unsigned v0 = (c0 >> shift) & 0xFF;
    unsigned v1 = (c1 >> shift) & 0xFF;
    return (unsigned) (v0 + t * ((float) v1 - (float) v0) + 0.5f);
  };
  return HB_COLOR (lerp (0), lerp (8), lerp (16), lerp (24));
}

/* Iterate sweep gradient color stop tiling across the full 0..2π range,
 * calling emit_patch (a0, c0, a1, c1) for each sector.
 *
 * Handles pad, repeat, and reflect extend modes.  Stops must be
 * pre-sorted by offset.  Shared by the vector, raster, GPU and cairo
 * sweep-gradient implementations. */
template <typename Func>
static inline void
hb_sweep_gradient_tiles (hb_color_stop_t *stops,
			 unsigned n_stops,
			 hb_paint_extend_t extend,
			 float start_angle,
			 float end_angle,
			 Func emit_patch)
{
  if (!n_stops) return;

  if (start_angle == end_angle)
  {
    if (extend == HB_PAINT_EXTEND_PAD)
    {
      if (start_angle > 0.f)
	emit_patch (0.f, stops[0].color, start_angle, stops[0].color);
      if (end_angle < HB_2_PI)
	emit_patch (end_angle, stops[n_stops - 1].color, HB_2_PI, stops[n_stops - 1].color);
    }
    return;
  }

  if (end_angle < start_angle)
  {
    float tmp = start_angle; start_angle = end_angle; end_angle = tmp;
    for (unsigned i = 0; i < n_stops - 1 - i; i++)
    {
      hb_color_stop_t t = stops[i];
      stops[i] = stops[n_stops - 1 - i];
      stops[n_stops - 1 - i] = t;
    }
    for (unsigned i = 0; i < n_stops; i++)
      stops[i].offset = 1.f - stops[i].offset;
  }

  /* Map stop offsets to angles. */
  float angles_buf[16];
  hb_color_t colors_buf[16];
  float *angles = angles_buf;
  hb_color_t *colors = colors_buf;
  bool dynamic = false;

  if (n_stops > 16)
  {
    angles = (float *) hb_malloc (sizeof (float) * n_stops);
    colors = (hb_color_t *) hb_malloc (sizeof (hb_color_t) * n_stops);
    if (!angles || !colors)
    {
      hb_free (angles);
      hb_free (colors);
      return;
    }
    dynamic = true;
  }

  for (unsigned i = 0; i < n_stops; i++)
  {
    angles[i] = start_angle + stops[i].offset * (end_angle - start_angle);
    colors[i] = stops[i].color;
  }

  if (extend == HB_PAINT_EXTEND_PAD)
  {
    unsigned pos;
    hb_color_t color0 = colors[0];
    for (pos = 0; pos < n_stops; pos++)
    {
      if (angles[pos] >= 0)
      {
	if (pos > 0)
	{
	  float f = (0.f - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
	  color0 = hb_color_lerp (colors[pos - 1], colors[pos], f);
	}
	break;
      }
    }
    if (pos == n_stops)
    {
      color0 = colors[n_stops - 1];
      emit_patch (0.f, color0, HB_2_PI, color0);
      goto done;
    }
    emit_patch (0.f, color0, angles[pos], colors[pos]);
    for (pos++; pos < n_stops; pos++)
    {
      if (angles[pos] <= HB_2_PI)
	emit_patch (angles[pos - 1], colors[pos - 1], angles[pos], colors[pos]);
      else
      {
	float f = (HB_2_PI - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
	hb_color_t color1 = hb_color_lerp (colors[pos - 1], colors[pos], f);
	emit_patch (angles[pos - 1], colors[pos - 1], HB_2_PI, color1);
	break;
      }
    }
    if (pos == n_stops)
    {
      color0 = colors[n_stops - 1];
      emit_patch (angles[n_stops - 1], color0, HB_2_PI, color0);
      goto done;
    }
  }
  else
  {
    float span = angles[n_stops - 1] - angles[0];
    if (fabsf (span) < 1e-6f)
      goto done;

    int k = 0;
    if (angles[0] >= 0)
    {
      float ss = angles[0];
      while (ss > 0)
      {
	if (span > 0) { ss -= span; k--; }
	else          { ss += span; k++; }
      }
    }
    else
    {
      float ee = angles[n_stops - 1];
      while (ee < 0)
      {
	if (span > 0) { ee += span; k++; }
	else          { ee -= span; k--; }
      }
    }

    span = fabsf (span);
    for (int l = k; l < 1000; l++)
    {
      for (unsigned i = 1; i < n_stops; i++)
      {
	float a0_l, a1_l;
	hb_color_t col0, col1;
	if ((l % 2 != 0) && (extend == HB_PAINT_EXTEND_REFLECT))
	{
	  a0_l = angles[0] + angles[n_stops - 1] - angles[n_stops - i] + l * span;
	  a1_l = angles[0] + angles[n_stops - 1] - angles[n_stops - 1 - i] + l * span;
	  col0 = colors[n_stops - i];
	  col1 = colors[n_stops - 1 - i];
	}
	else
	{
	  a0_l = angles[i - 1] + l * span;
	  a1_l = angles[i] + l * span;
	  col0 = colors[i - 1];
	  col1 = colors[i];
	}

	if (a1_l < 0.f) continue;
	if (a0_l < 0.f)
	{
	  float f = (0.f - a0_l) / (a1_l - a0_l);
	  hb_color_t c = hb_color_lerp (col0, col1, f);
	  emit_patch (0.f, c, a1_l, col1);
	}
	else if (a1_l >= HB_2_PI)
	{
	  float f = (HB_2_PI - a0_l) / (a1_l - a0_l);
	  hb_color_t c = hb_color_lerp (col0, col1, f);
	  emit_patch (a0_l, col0, HB_2_PI, c);
	  goto done;
	}
	else
	  emit_patch (a0_l, col0, a1_l, col1);
      }
    }
  }

done:
  if (dynamic)
  {
    hb_free (angles);
    hb_free (colors);
  }
}


#endif /* HB_PAINT_HH */
