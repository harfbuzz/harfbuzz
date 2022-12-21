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

#define HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS \
  HB_PAINT_FUNC_IMPLEMENT (push_transform) \
  HB_PAINT_FUNC_IMPLEMENT (pop_transform) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_glyph) \
  HB_PAINT_FUNC_IMPLEMENT (push_clip_rectangle) \
  HB_PAINT_FUNC_IMPLEMENT (pop_clip) \
  HB_PAINT_FUNC_IMPLEMENT (color) \
  HB_PAINT_FUNC_IMPLEMENT (image) \
  HB_PAINT_FUNC_IMPLEMENT (linear_gradient) \
  HB_PAINT_FUNC_IMPLEMENT (radial_gradient) \
  HB_PAINT_FUNC_IMPLEMENT (sweep_gradient) \
  HB_PAINT_FUNC_IMPLEMENT (push_group) \
  HB_PAINT_FUNC_IMPLEMENT (pop_group) \
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
                       float dx, float dy,
                       hb_font_t *font)
  { func.push_transform (this, paint_data,
                         xx, yx, xy, yy, dx, dy,
                         font,
                         !user_data ? nullptr : user_data->push_transform); }
  void pop_transform (void *paint_data,
                      hb_font_t *font)
  { func.pop_transform (this, paint_data, font,
                        !user_data ? nullptr : user_data->pop_transform); }
  void push_clip_glyph (void *paint_data,
                        hb_codepoint_t glyph,
                        hb_font_t *font)
  { func.push_clip_glyph (this, paint_data,
                          glyph,
                          font,
                          !user_data ? nullptr : user_data->push_clip_glyph); }
  void push_clip_rectangle (void *paint_data,
                            float xmin, float ymin, float xmax, float ymax,
                            hb_font_t *font)
  { func.push_clip_rectangle (this, paint_data,
                              xmin, ymin, xmax, ymax,
                              font,
                              !user_data ? nullptr : user_data->push_clip_rectangle); }
  void pop_clip (void *paint_data,
                 hb_font_t *font)
  { func.pop_clip (this, paint_data, font,
                   !user_data ? nullptr : user_data->pop_clip); }
  void color (void *paint_data,
              unsigned int color_index,
              float alpha,
              hb_font_t *font)
  { func.color (this, paint_data,
                color_index, alpha,
                font,
                !user_data ? nullptr : user_data->color); }
  void image (void *paint_data,
              hb_blob_t *image,
              hb_tag_t format,
              hb_glyph_extents_t *extents,
              hb_font_t *font)
  { func.image (this, paint_data,
                image, format, extents,
                font,
                !user_data ? nullptr : user_data->image); }
  void linear_gradient (void *paint_data,
                        hb_color_line_t *color_line,
                        float x0, float y0,
                        float x1, float y1,
                        float x2, float y2,
                        hb_font_t *font)
  { func.linear_gradient (this, paint_data,
                          color_line, x0, y0, x1, y1, x2, y2,
                          font,
                          !user_data ? nullptr : user_data->linear_gradient); }
  void radial_gradient (void *paint_data,
                        hb_color_line_t *color_line,
                        float x0, float y0, float r0,
                        float x1, float y1, float r1,
                        hb_font_t *font)
  { func.radial_gradient (this, paint_data,
                          color_line, x0, y0, r0, x1, y1, r1,
                          font,
                          !user_data ? nullptr : user_data->radial_gradient); }
  void sweep_gradient (void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0,
                       float start_angle,
                       float end_angle,
                       hb_font_t *font)
  { func.sweep_gradient (this, paint_data,
                         color_line, x0, y0, start_angle, end_angle,
                         font,
                         !user_data ? nullptr : user_data->sweep_gradient); }
  void push_group (void *paint_data,
                   hb_font_t *font)
  { func.push_group (this, paint_data, font,
                     !user_data ? nullptr : user_data->push_group); }
  void pop_group (void *paint_data,
                  hb_paint_composite_mode_t mode,
                  hb_font_t *font)
  { func.pop_group (this, paint_data,
                    mode,
                    font,
                    !user_data ? nullptr : user_data->pop_group); }

  void push_root_transform (void *paint_data,
                            hb_font_t *font)
  {
    int xscale = font->x_scale, yscale = font->y_scale;
    float upem = font->face->get_upem ();
    float slant = font->slant_xy;

    func.push_transform (this, paint_data,
                         xscale/upem, 0, slant * yscale/upem, yscale/upem, 0, 0,
                         font,
                         !user_data ? nullptr : user_data->push_transform);
  }
  void pop_root_transform (void *paint_data,
                           hb_font_t *font)
  {
    func.pop_transform (this, paint_data, font,
                        !user_data ? nullptr : user_data->pop_transform);
  }

  void push_inverse_root_transform (void *paint_data,
                                    hb_font_t *font)
  {
    int xscale = font->x_scale, yscale = font->y_scale;
    float upem = font->face->get_upem ();
    float slant = font->slant_xy;

    func.push_transform (this, paint_data,
                         upem/xscale, 0, -slant * upem/xscale, upem/yscale, 0, 0,
                         font,
                         !user_data ? nullptr : user_data->push_transform);
  }
  void pop_inverse_root_transform (void *paint_data,
                                   hb_font_t *font)
  {
    func.pop_transform (this, paint_data, font,
                        !user_data ? nullptr : user_data->pop_transform);
  }
};
DECLARE_NULL_INSTANCE (hb_paint_funcs_t);

#endif /* HB_PAINT_HH */
