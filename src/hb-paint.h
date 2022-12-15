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

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_PAINT_H
#define HB_PAINT_H

#include "hb.h"

HB_BEGIN_DECLS

typedef struct hb_paint_funcs_t hb_paint_funcs_t;

HB_EXTERN hb_paint_funcs_t *
hb_paint_funcs_create ();

HB_EXTERN hb_paint_funcs_t *
hb_paint_funcs_reference (hb_paint_funcs_t *funcs);

HB_EXTERN void
hb_paint_funcs_destroy (hb_paint_funcs_t *funcs);

HB_EXTERN void
hb_paint_funcs_make_immutable (hb_paint_funcs_t *funcs);

HB_EXTERN hb_bool_t
hb_paint_funcs_is_immutable (hb_paint_funcs_t *funcs);

typedef void (*hb_paint_push_transform_func_t) (hb_paint_funcs_t *funcs,
                                                void *paint_data,
                                                float xx, float xy,
                                                float yx, float yy,
                                                float x0, float y0,
                                                void *user_data);

typedef void (*hb_paint_pop_transform_func_t)  (hb_paint_funcs_t *funcs,
                                                void *paint_data,
                                                void *user_data);

typedef void (*hb_paint_push_clip_glyph_func_t) (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 hb_codepoint_t glyph,
                                                 void *user_data);

typedef void (*hb_paint_push_clip_rect_func_t) (hb_paint_funcs_t *funcs,
                                                void *paint_data,
                                                float xmin, float ymin,
                                                float xmax, float ymax,
                                                void *user_data);

typedef void (*hb_paint_pop_clip_func_t)       (hb_paint_funcs_t *funcs,
                                                void *paint_data,
                                                void *user_data);

typedef void (*hb_paint_solid_func_t)          (hb_paint_funcs_t *funcs,
                                                void *paint_data,
                                                unsigned int color_index,
                                                float alpha,
                                                void *user_data);


typedef struct hb_color_line_t hb_color_line_t;

typedef struct {
  float offset;
  unsigned int color_index;
} hb_color_stop_t;

HB_EXTERN unsigned int
hb_color_line_get_color_stops (hb_color_line_t color_line,
                               unsigned int start,
                               unsigned int count,
                               hb_color_stop_t *color_stops);

typedef enum {
  HB_PAINT_EXTEND_PAD,
  HB_PAINT_EXTEND_REPEAT,
  HB_PAINT_EXTEND_REFLECT
} hb_paint_extend_t;

HB_EXTERN hb_paint_extend_t
hb_color_line_get_extend (hb_color_line_t *color_line);

typedef void (*hb_paint_linear_gradient_func_t) (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 hb_color_line_t *color_line,
                                                 float x0, float y0,
                                                 float x1, float y1,
                                                 float x2, float y2,
                                                 void *user_data);

typedef void (*hb_paint_radial_gradient_func_t) (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 hb_color_line_t *color_line,
                                                 float x0, float y0, float r0,
                                                 float x1, float y1, float r1,
                                                 void *user_data);

typedef void (*hb_paint_sweep_gradient_func_t)  (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 hb_color_line_t *color_line,
                                                 float x0, float y0,
                                                 float start_angle,
                                                 float end_angle,
                                                 void *user_data);

typedef enum {
  HB_PAINT_COMPOSITE_MODE_CLEAR,
  HB_PAINT_COMPOSITE_MODE_SRC,
  HB_PAINT_COMPOSITE_MODE_DEST,
  HB_PAINT_COMPOSITE_MODE_SRC_OVER,
  HB_PAINT_COMPOSITE_MODE_DEST_OVER,
  HB_PAINT_COMPOSITE_MODE_SRC_IN,
  HB_PAINT_COMPOSITE_MODE_DEST_IN,
  HB_PAINT_COMPOSITE_MODE_SRC_OUT,
  HB_PAINT_COMPOSITE_MODE_DEST_OUT,
  HB_PAINT_COMPOSITE_MODE_SRC_ATOP,
  HB_PAINT_COMPOSITE_MODE_DEST_ATOP,
  HB_PAINT_COMPOSITE_MODE_XOR,
  HB_PAINT_COMPOSITE_MODE_PLUS,
  HB_PAINT_COMPOSITE_MODE_SCREEN,
  HB_PAINT_COMPOSITE_MODE_OVERLAY,
  HB_PAINT_COMPOSITE_MODE_DARKEN,
  HB_PAINT_COMPOSITE_MODE_LIGHTEN,
  HB_PAINT_COMPOSITE_MODE_COLOR_DODGE,
  HB_PAINT_COMPOSITE_MODE_COLOR_BURN,
  HB_PAINT_COMPOSITE_MODE_HARD_LIGHT,
  HB_PAINT_COMPOSITE_MODE_SOFT_LIGHT,
  HB_PAINT_COMPOSITE_MODE_DIFFERENCE,
  HB_PAINT_COMPOSITE_MODE_EXCLUSION,
  HB_PAINT_COMPOSITE_MODE_MULTIPLY,
  HB_PAINT_COMPOSITE_MODE_HSL_HUE,
  HB_PAINT_COMPOSITE_MODE_HSL_SATURATION,
  HB_PAINT_COMPOSITE_MODE_HSL_COLOR,
  HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY,
} hb_paint_composite_mode_t;

typedef void (*hb_paint_push_group_func_t)      (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 void *user_data);

typedef void (*hb_paint_pop_group_and_composite_func_t) (hb_paint_funcs_t *funcs,
                                                         void *paint_data,
                                                         hb_paint_composite_mode_t mode,
                                                         void *user_data);

/**
 * hb_paint_funcs_set_push_transform_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_push_transform_func (hb_paint_funcs_t               *funcs,
                                        hb_paint_push_transform_func_t  func,
                                        void                           *user_data,
                                        hb_destroy_func_t               destroy);

/**
 * hb_paint_funcs_set_pop_transform_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_pop_transform_func (hb_paint_funcs_t              *funcs,
                                       hb_paint_pop_transform_func_t  func,
                                       void                          *user_data,
                                       hb_destroy_func_t              destroy);

/**
 * hb_paint_funcs_set_push_clip_glyph_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_push_clip_glyph_func (hb_paint_funcs_t                *funcs,
                                         hb_paint_push_clip_glyph_func_t  func,
                                         void                            *user_data,
                                         hb_destroy_func_t                destroy);

/**
 * hb_paint_funcs_set_push_clip_rect_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_push_clip_rect_func (hb_paint_funcs_t               *funcs,
                                        hb_paint_push_clip_rect_func_t  func,
                                        void                           *user_data,
                                        hb_destroy_func_t               destroy);

/**
 * hb_paint_funcs_set_pop_clip_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_pop_clip_func (hb_paint_funcs_t         *funcs,
                                  hb_paint_pop_clip_func_t  func,
                                  void                     *user_data,
                                  hb_destroy_func_t         destroy);

/**
 * hb_paint_funcs_set_solid_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_solid_func (hb_paint_funcs_t      *funcs,
                               hb_paint_solid_func_t  func,
                               void                  *user_data,
                               hb_destroy_func_t      destroy);

/**
 * hb_paint_funcs_set_linear_gradient_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_linear_gradient_func (hb_paint_funcs_t                *funcs,
                                         hb_paint_linear_gradient_func_t  func,
                                         void                            *user_data,
                                         hb_destroy_func_t                destroy);

/**
 * hb_paint_funcs_set_radial_gradient_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_radial_gradient_func (hb_paint_funcs_t                *funcs,
                                         hb_paint_radial_gradient_func_t  func,
                                         void                            *user_data,
                                         hb_destroy_func_t                destroy);

/**
 * hb_paint_funcs_set_sweep_gradient_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_sweep_gradient_func (hb_paint_funcs_t               *funcs,
                                        hb_paint_sweep_gradient_func_t  func,
                                        void                           *user_data,
                                        hb_destroy_func_t               destroy);

/**
 * hb_paint_funcs_set_push_group_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_push_group_func (hb_paint_funcs_t           *funcs,
                                    hb_paint_push_group_func_t  func,
                                    void                       *user_data,
                                    hb_destroy_func_t           destroy);

/**
 * hb_paint_funcs_set_pop_group_and_composite_func:
 * @funcs:
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy: (nullable)
 */
HB_EXTERN void
hb_paint_funcs_set_pop_group_and_composite_func (hb_paint_funcs_t                        *funcs,
                                                 hb_paint_pop_group_and_composite_func_t  func,
                                                 void                       *user_data,
                                                 hb_destroy_func_t           destroy);

HB_EXTERN void
hb_paint_push_transform (hb_paint_funcs_t *funcs, void *paint_data,
                         float xx, float xy,
                         float yx, float yy,
                         float x0, float y0);

HB_EXTERN void
hb_paint_pop_transform (hb_paint_funcs_t *funcs, void *paint_data);

HB_EXTERN void
hb_paint_push_clip_glyph (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_codepoint_t glyph);

HB_EXTERN void
hb_paint_push_clip_rect (hb_paint_funcs_t *funcs, void *paint_data,
                         float xmin, float ymin, float xmax, float ymax);

HB_EXTERN void
hb_paint_pop_clip (hb_paint_funcs_t *funcs, void *paint_data);

HB_EXTERN void
hb_paint_solid (hb_paint_funcs_t *funcs, void *paint_data,
                unsigned int color_index,
                float alpha);

HB_EXTERN void
hb_paint_linear_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_color_line_t *color_line,
                          float x0, float y0,
                          float x1, float y1,
                          float x2, float y2);

HB_EXTERN void
hb_paint_radial_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_color_line_t *color_line,
                          float x0, float y0, float r0,
                          float x1, float y1, float r1);

HB_EXTERN void
hb_paint_sweep_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                         hb_color_line_t *color_line,
                         float x0, float y0,
                         float start_angle, float end_angle);

HB_EXTERN void
hb_paint_push_group (hb_paint_funcs_t *funcs, void *paint_data);

HB_EXTERN void
hb_paint_pop_group_and_composite (hb_paint_funcs_t *funcs, void *paint_data,
                                  hb_paint_composite_mode_t mode);

HB_END_DECLS

#endif  /* HB_PAINT_H */
