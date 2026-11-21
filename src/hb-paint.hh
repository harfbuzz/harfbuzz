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
#include "hb-vector.hh"

#define HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS \
  HB_PAINT_FUNC_IMPLEMENT (push_transform) \
  HB_PAINT_FUNC_IMPLEMENT (pop_transform) \
  HB_PAINT_FUNC_IMPLEMENT (color_glyph) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_glyph) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_rectangle) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_path_start) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_path_end) \
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
  HB_PAINT_FUNC_IMPLEMENT (fill_glyph) \
  HB_PAINT_FUNC_IMPLEMENT (set_budget) \
  HB_PAINT_FUNC_IMPLEMENT (get_budget) \
  HB_PAINT_FUNC_IMPLEMENT (get_budget_remaining) \
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
                             !user_data ? nullptr : user_data->color_glyph); }
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
  hb_draw_funcs_t *push_clip_path_start (void *paint_data,
                                         void **draw_data)
  { return func.push_clip_path_start (this, paint_data, draw_data,
                                      !user_data ? nullptr : user_data->push_clip_path_start); }
  void push_clip_path_end (void *paint_data)
  { func.push_clip_path_end (this, paint_data,
                             !user_data ? nullptr : user_data->push_clip_path_end); }
  void pop_clip (void *paint_data)
  { func.pop_clip (this, paint_data,
                   !user_data ? nullptr : user_data->pop_clip); }
  void color (void *paint_data,
              hb_bool_t is_foreground,
              hb_color_t color)
  { func.color (this, paint_data,
                is_foreground, color,
                !user_data ? nullptr : user_data->color); }
  void fill_glyph (void *paint_data,
                   hb_codepoint_t glyph,
                   hb_font_t *font,
                   hb_bool_t is_foreground,
                   hb_color_t color)
  { func.fill_glyph (this, paint_data,
                     glyph, font,
                     is_foreground, color,
                     !user_data ? nullptr : user_data->fill_glyph); }
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
  bool set_budget (void *paint_data, int64_t budget)
  {
    if (budget < 0 && budget != HB_BUDGET_DEFAULT)
      budget = 0;
    return func.set_budget (this, paint_data, budget,
			    !user_data ? nullptr : user_data->set_budget);
  }
  int64_t get_budget (void *paint_data)
  { return func.get_budget (this, paint_data,
			    !user_data ? nullptr : user_data->get_budget); }
  int64_t *get_budget_remaining_ptr (void *paint_data)
  { return func.get_budget_remaining (this, paint_data,
				      !user_data ? nullptr : user_data->get_budget_remaining); }


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

/* COLR interpolates the colors of a gradient with alpha multiplied
 * into them.  SVG and PDF viewers, and cairo, interpolate the color
 * and the alpha of the stops as given, which is not the same between
 * two stops that differ in alpha: a color fading out to transparent
 * black would turn dark on its way.
 *
 * Rewrites @stops, which have to be sorted, into @out, so that
 * interpolating those that way gives what COLR asks for.  A
 * transparent stop has no color of its own, so it takes its
 * neighbor's, on each side.  Between two other stops that differ in
 * alpha, the color is not linear in the offset; stops are added along
 * its curve, as many as it takes to stay within a level or two of it.
 * Returns false if @stops are to be used as they are. */
static inline bool
hb_paint_premultiply_color_stops (const hb_color_stop_t *stops, unsigned count,
				  hb_vector_t<hb_color_stop_t> &out)
{
  bool differ = false;
  for (unsigned i = 1; i < count && !differ; i++)
    differ = hb_color_get_alpha (stops[i].color) !=
	     hb_color_get_alpha (stops[0].color);
  if (!differ)
    return false;

  /* Long color lines only get the transparent stops fixed. */
  unsigned max_segments = count <= 256 ? 16 : 1;

  if (unlikely (!out.alloc (count + count / 2)))
    return false;
  auto with_alpha = [] (hb_color_t c, unsigned a)
  { return HB_COLOR (hb_color_get_blue (c), hb_color_get_green (c), hb_color_get_red (c), a); };
  for (unsigned i = 0; i < count; i++)
  {
    const hb_color_stop_t &s = stops[i];
    unsigned a = hb_color_get_alpha (s.color);
    const hb_color_stop_t *next = i + 1 < count ? &stops[i + 1] : nullptr;
    unsigned next_a = next ? hb_color_get_alpha (next->color) : 0;

    if (!a)
    {
      unsigned prev_a = i ? hb_color_get_alpha (stops[i - 1].color) : 0;
      hb_color_stop_t t = s;
      if (prev_a)
      {
	t.color = with_alpha (stops[i - 1].color, 0);
	out.push (t);
      }
      if (next_a)
      {
	t.color = with_alpha (next->color, 0);
	out.push (t);
      }
      if (!prev_a && !next_a)
	out.push (s);
      continue;
    }

    out.push (s);
    if (!next_a || next_a == a || !(next->offset > s.offset))
      continue;

    /* Midway, the premultiplied color is off by a quarter of the
     * difference in color times the difference in alpha. */
    unsigned dc = 0;
    for (unsigned shift = 8; shift <= 24; shift += 8)
    {
      int d = (int) ((s.color >> shift) & 0xFF) - (int) ((next->color >> shift) & 0xFF);
      dc = hb_max (dc, (unsigned) (d < 0 ? -d : d));
    }
    unsigned da = a > next_a ? a - next_a : next_a - a;
    unsigned segments = hb_min (max_segments, (dc * da + 2039) / 2040);
    for (unsigned k = 1; k < segments; k++)
    {
      float u = (float) k / segments;
      float alpha = a + u * ((float) next_a - (float) a);
      auto channel = [&] (unsigned shift) -> unsigned
      {
	float c0 = (float) ((s.color >> shift) & 0xFF) * a;
	float c1 = (float) ((next->color >> shift) & 0xFF) * next_a;
	return (unsigned) hb_clamp ((c0 + u * (c1 - c0)) / alpha + .5f, 0.f, 255.f);
      };
      hb_color_stop_t t;
      t.offset = s.offset + u * (next->offset - s.offset);
      t.is_foreground = false;
      t.color = HB_COLOR (channel (24), channel (16), channel (8),
			  (unsigned) (alpha + .5f));
      out.push (t);
    }
  }
  return !out.in_error ();
}


/* Linearly interpolate between two hb_color_t values, component-wise,
 * in byte-channel space with rounding. */
static inline hb_color_t
hb_color_lerp (hb_color_t c0, hb_color_t c1, float t)
{
  auto lerp = [&] (unsigned shift) -> unsigned {
    unsigned v0 = (c0 >> shift) & 0xFF;
    unsigned v1 = (c1 >> shift) & 0xFF;
    float v = v0 + t * ((float) v1 - (float) v0) + 0.5f;
    /* t may be outside [0,1] (or NaN) for malformed gradients;
     * keep the float-to-unsigned conversion defined. */
    if (!(v > 0.f)) return 0;
    if (v > 255.f) return 255;
    return (unsigned) v;
  };
  return HB_COLOR (lerp (24), lerp (16), lerp (8), lerp (0));
}

#endif /* HB_PAINT_HH */
