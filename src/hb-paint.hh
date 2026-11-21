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


static inline hb_color_t
hb_color_lerp (hb_color_t c0, hb_color_t c1, float t);

/* Whether the stops of a color line all share one offset, which leaves
 * it without a length. */
static inline bool
hb_color_stops_share_offset (const hb_color_stop_t *stops, unsigned len)
{
  for (unsigned i = 1; i < len; i++)
    if (stops[i].offset != stops[0].offset)
      return false;
  return true;
}

/* A color line whose stops all share one offset has no length.  If it
 * pads, it still has two colors: that of the first stop before the
 * offset, and that of the last from it on.  If it repeats or reflects
 * it is ill-formed, and nothing is painted; that is how
 * hb_paint_sweep_gradient_tiles() has it too.
 *
 * Takes the @stops of a color line as the font has them, before they
 * are sorted, in an array with room for four.  Returns false if
 * nothing is to be painted.  A color line without a length is given
 * one of 1, around its offset, with its two colors meeting halfway
 * along.  That is a color line like any other, so that what paints
 * the gradient needs to know of nothing new; and its gradient is as
 * long as the font's and runs the same way, which a sliver of one
 * would not once its ends are written. */
static inline bool
hb_paint_color_line_give_length (hb_color_stop_t *stops, unsigned *len,
				 hb_paint_extend_t extend)
{
  unsigned count = *len;
  if (likely (!hb_color_stops_share_offset (stops, count)))
    return true;
  if (unlikely (!count) || extend != HB_PAINT_EXTEND_PAD)
    return false;

  hb_color_stop_t first = stops[0], last = stops[count - 1];
  float offset = first.offset;
  unsigned n = 0;
  stops[n] = first;
  stops[n++].offset = offset - 0.5f;
  if (count > 1)
  {
    stops[n] = first;
    stops[n++].offset = offset;
    stops[n] = last;
    stops[n++].offset = offset;
  }
  stops[n] = last;
  stops[n++].offset = offset + 0.5f;
  *len = n;
  return true;
}

/* The same for stops in a vector, which makes the room. */
static inline bool
hb_paint_color_line_give_length (hb_vector_t<hb_color_stop_t> &stops,
				 hb_paint_extend_t extend)
{
  unsigned len = stops.length;
  if (likely (!hb_color_stops_share_offset (stops.arrayZ, len)))
    return true;
  if (unlikely (!stops.resize (hb_max (len, 4u))))
    return false;
  if (!hb_paint_color_line_give_length (stops.arrayZ, &len, extend))
    return false;
  stops.shrink (len);
  return true;
}

/* Cuts a color line whose stops are sorted, and which goes from 0 to 1,
 * down to the stretch from @from to @to, in place; it goes from 0 to 1
 * again, in no more stops.  An end that is not cut keeps its stops, and
 * with them the color that goes on from it.  Returns false if there is
 * no such stretch. */
static inline bool
hb_color_stops_cut (hb_color_stop_t *stops, unsigned *len,
		    float from, float to)
{
  unsigned count = *len;
  if (unlikely (!count || !(from < to) ||
		!(from < stops[count - 1].offset) || !(to > stops[0].offset)))
    return false;

  /* The color at @offset between two stops.  Colors are interpolated
   * with alpha multiplied into them: the more transparent a stop is,
   * the less of its color there is to give. */
  auto color_at = [] (const hb_color_stop_t &s0, const hb_color_stop_t &s1,
		      float offset) -> hb_color_t
  {
    float t = (offset - s0.offset) / (s1.offset - s0.offset);
    hb_color_t c0 = s0.color, c1 = s1.color;
    float a0 = hb_color_get_alpha (c0), a1 = hb_color_get_alpha (c1);
    if (a0 == a1)
      return hb_color_lerp (c0, c1, t);

    float w0 = (1.f - t) * a0, w1 = t * a1;
    float a = w0 + w1;
    if (!(a > 0.f))
      /* Transparent: the color that the other one fades out in. */
      return (a0 > a1 ? c0 : c1) & ~(hb_color_t) 0xFF;

    auto channel = [&] (unsigned shift) -> unsigned
    {
      float v = (w0 * ((c0 >> shift) & 0xFF) + w1 * ((c1 >> shift) & 0xFF)) / a + 0.5f;
      if (!(v > 0.f)) return 0;
      if (v > 255.f) return 255;
      return (unsigned) v;
    };
    return HB_COLOR (channel (24), channel (16), channel (8),
		     a < 255.f ? (unsigned) (a + 0.5f) : 255);
  };

  if (to < stops[count - 1].offset)
  {
    /* The first stop that is not before @to ends the line: where it
     * is, if that is at @to, as it is the one that says what the color
     * is up to there; and with the color at @to, if it is after it. */
    unsigned j = 1;
    while (stops[j].offset < to)
      j++;
    if (stops[j].offset > to)
    {
      stops[j].color = color_at (stops[j - 1], stops[j], to);
      stops[j].is_foreground = false;
      stops[j].offset = to;
    }
    count = j + 1;
  }

  unsigned first = 0;
  if (from > stops[0].offset)
  {
    /* The last stop that is not after @from starts it, likewise. */
    first = count - 2;
    while (stops[first].offset > from)
      first--;
    if (stops[first].offset < from)
    {
      stops[first].color = color_at (stops[first], stops[first + 1], from);
      stops[first].is_foreground = false;
      stops[first].offset = from;
    }
  }

  for (unsigned i = 0; first + i < count; i++)
  {
    stops[i] = stops[first + i];
    stops[i].offset = (stops[i].offset - from) / (to - from);
  }
  *len = count - first;
  return true;
}

/* A radial gradient is given by the two circles where its color line
 * starts and ends, and paints the circles between and beyond them that
 * have a radius.  One of the two may not: its radius is negative if a
 * color stop lies past the tip of the cone.  Such a circle cannot be
 * given to what takes two circles to paint a gradient.
 *
 * Gives the gradient whose color line is @stops, sorted and going from
 * 0 to 1, two circles that can be.  A color line that repeats is taken
 * a whole number of periods further along, an even number if it
 * reflects, which paints the same.  One that does not is cut where the
 * radius is zero, in place.  Returns false if nothing is to be
 * painted. */
static inline bool
hb_paint_radial_gradient_circles (hb_color_stop_t *stops, unsigned *len,
				  hb_paint_extend_t extend,
				  float *x0, float *y0, float *r0,
				  float *x1, float *y1, float *r1)
{
  if (likely (*r0 >= 0.f && *r1 >= 0.f))
    return true;

  float dx = *x1 - *x0, dy = *y1 - *y0, dr = *r1 - *r0;
  /* No circle has a radius, or the radii are not numbers. */
  if ((!(dr > 0.f) && !(dr < 0.f)) || unlikely (!*len))
    return false;

  /* Where the radius is zero.  The circles that have one are those
   * past it if they grow, and those before it if they shrink. */
  float tip = -*r0 / dr;
  if (unlikely (!std::isfinite (tip)))
    return false;

  float from = 0.f, to = 1.f;
  if (extend == HB_PAINT_EXTEND_REPEAT || extend == HB_PAINT_EXTEND_REFLECT)
  {
    float periods = dr > 0.f ? ceilf (tip) : floorf (tip) - 1.f;
    if (extend == HB_PAINT_EXTEND_REFLECT && fmodf (periods, 2.f) != 0.f)
      periods += dr > 0.f ? 1.f : -1.f;
    from += periods;
    to += periods;
  }
  else if (tip > 0.f && tip < 1.f)
  {
    /* The tip is on the color line. */
    if (dr > 0.f) from = tip; else to = tip;
    if (unlikely (!hb_color_stops_cut (stops, len, from, to)))
      return false;
  }
  else
  {
    /* All of the color line is past the tip.  The color at its nearer
     * end goes on from there: one color, from the tip on. */
    if (unlikely (*len < 2))
      return false;
    hb_color_t color = stops[dr > 0.f ? *len - 1 : 0].color;
    for (unsigned i = 0; i < 2; i++)
    {
      hb_color_stop_t stop = {(float) i, false, color};
      stops[i] = stop;
    }
    *len = 2;
    if (dr > 0.f) { from = tip; to = tip + 1.f; }
    else { from = tip - 1.f; to = tip; }
  }

  float x = *x0, y = *y0, r = *r0;
  *x0 = x + from * dx;
  *y0 = y + from * dy;
  *r0 = hb_max (r + from * dr, 0.f);
  *x1 = x + to * dx;
  *y1 = y + to * dy;
  *r1 = hb_max (r + to * dr, 0.f);
  return true;
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
