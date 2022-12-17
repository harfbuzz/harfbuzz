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
#include "hb-font.h"

HB_BEGIN_DECLS

/**
 * hb_paint_funcs_t:
 *
 * Glyph paint callbacks.
 *
 * All the callbacks are necessary.
 *
 * The callbacks assume that the caller maintains a stack
 * of current transforms, clips and intermediate surfaces,
 * as evidenced by the pairs of push/pop callbacks. The
 * push/pop calls will be properly nested, so it is fine
 * to store the different kinds of object on a single stack.
 *
 * The callbacks also assume that the caller uses
 * hb_ot_color_palette_get_colors() to obtain colors
 * from one of the fonts color palettes. If the font does
 * not have color palettes, the color index will always
 * be 0xFFFF, indicating the use of the foreground color.
 *
 * Not all callbacks are required for all kinds of glyphs.
 * For rendering COLRv0 or non-color outline glyphs, the
 * gradient and composite callbacks are not needed.
 *
 * The paint-image callback is only needed for glyphs
 * with blobs in the CBDT, sbix or SVG tables.
 *
 * Since: REPLACEME
 **/
typedef struct hb_paint_funcs_t hb_paint_funcs_t;

HB_EXTERN hb_paint_funcs_t *
hb_paint_funcs_create (void);

HB_EXTERN hb_paint_funcs_t *
hb_paint_funcs_reference (hb_paint_funcs_t *funcs);

HB_EXTERN void
hb_paint_funcs_destroy (hb_paint_funcs_t *funcs);

HB_EXTERN void
hb_paint_funcs_make_immutable (hb_paint_funcs_t *funcs);

HB_EXTERN hb_bool_t
hb_paint_funcs_is_immutable (hb_paint_funcs_t *funcs);

/**
 * hb_paint_push_transform_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @xx: xx component of the transform matrix
 * @yx: yx component of the transform matrix
 * @xy: xy component of the transform matrix
 * @yy: yy component of the transform matrix
 * @dx: dx component of the transform matrix
 * @dy: dy component of the transform matrix
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to apply
 * a transform to subsequent paint calls.
 *
 * This transform is applied after the current transform,
 * and remains in effect until a matching call to
 * the #hb_paint_funcs_pop_transform_func_t vfunc.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_push_transform_func_t) (hb_paint_funcs_t *funcs,
                                                void *paint_data,
                                                float xx, float yx,
                                                float xy, float yy,
                                                float dx, float dy,
                                                void *user_data);

/**
 * hb_paint_pop_transform_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to undo
 * the effect of a prior call to the #hb_paint_funcs_push_transform_func_t
 * vfunc.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_pop_transform_func_t) (hb_paint_funcs_t *funcs,
                                               void *paint_data,
                                               void *user_data);

/**
 * hb_paint_push_clip_glyph_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @glyph: the glyph ID
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to clip
 * subsequent paint calls to the outline of a glyph.
 *
 * The coordinates of the glyph outline are interpreted according
 * to the current transform.
 *
 * Note that hb_font_paint_glyph() applies the scale and slant of
 * the font as a transform, so you need to pass an unscaled, unslanted
 * copy of the font to hb_font_get_glyph_shape() when obtaining outlines,
 * to avoid double scaling.
 *
 * This clip is applied in addition to the current clip,
 * and remains in effect until a matching call to
 * the #hb_paint_funcs_pop_clip_func_t vfunc.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_push_clip_glyph_func_t) (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 hb_codepoint_t glyph,
                                                 void *user_data);

/**
 * hb_paint_push_clip_rectangle_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @xmin: min X for the rectangle
 * @ymin: min Y for the rectangle
 * @xmax: max X for the rectangle
 * @ymax: max Y for the rectangle
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to clip
 * subsequent paint calls to a rectangle.
 *
 * The coordinates of the rectangle are interpreted according
 * to the current transform.
 *
 * This clip is applied in addition to the current clip,
 * and remains in effect until a matching call to
 * the #hb_paint_funcs_pop_clip_func_t vfunc.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_push_clip_rectangle_func_t) (hb_paint_funcs_t *funcs,
                                                     void *paint_data,
                                                     float xmin, float ymin,
                                                     float xmax, float ymax,
                                                     void *user_data);

/**
 * hb_paint_pop_clip_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to undo
 * the effect of a prior call to the #hb_paint_funcs_push_clip_glyph_func_t
 * or #hb_paint_funcs_push_clip_rectangle_func_t vfuncs.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_pop_clip_func_t) (hb_paint_funcs_t *funcs,
                                          void *paint_data,
                                          void *user_data);

/**
 * hb_paint_color_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @color_index: Index of a color in the fonts selected color palette
 * @alpha: alpha to apply in addition
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to paint a
 * color everywhere within the current clip.
 *
 * The @color_index can be either an index into one of the fonts
 * color palettes, or the special value 0xFFFF, which indicates that
 * the foreground color should be used.
 *
 * In either case, the @alpha value should be applied in addition
 * (i.e. multiplied with) the alpha value found in the color.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_color_func_t) (hb_paint_funcs_t *funcs,
                                       void *paint_data,
                                       unsigned int color_index,
                                       float alpha,
                                       void *user_data);

/**
 * hb_paint_image_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @image: the image data
 * @mimetype: the mime type for the image data
 * @extents: (nullable): glyph extents
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to paint the
 * glyph image.
 *
 * This method is intended for glyphs with image blobs in the CBDT,
 * sbix or SVG tables. The @mimetype identifies the kind of data
 * that is contained in @image. Possible values include "image/png"
 * and "image/svg+xml". The glyph extents are provided if available,
 * and should be used to position the image.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_image_func_t) (hb_paint_funcs_t *funcs,
                                       void *paint_data,
                                       hb_blob_t *image,
                                       const char *mimetype,
                                       hb_glyph_extents_t *extents,
                                       void *user_data);

/**
 * hb_color_line_t:
 *
 * An opaque struct containing color information for a gradient.
 *
 * Since: REPLACEME
 */
typedef struct hb_color_line_t hb_color_line_t;

/**
 * hb_color_stop_t:
 * @offset: the offset of the color stop
 * @color_index: either a color palette index or the special value 0xFFFF
 * @alpha: alpha to apply
 *
 * Information about a color stop on a color line.
 *
 * Color lines typically have offsets ranging between 0 and 1,
 * but that is not required.
 *
 * Since: REPLACEME
 */
typedef struct {
  float offset;
  unsigned int color_index;
  float alpha;
} hb_color_stop_t;

HB_EXTERN unsigned int
hb_color_line_get_color_stops (hb_color_line_t *color_line,
                               unsigned int start,
                               unsigned int *count,
                               hb_color_stop_t *color_stops);

typedef enum {
  HB_PAINT_EXTEND_PAD,
  HB_PAINT_EXTEND_REPEAT,
  HB_PAINT_EXTEND_REFLECT
} hb_paint_extend_t;

HB_EXTERN hb_paint_extend_t
hb_color_line_get_extend (hb_color_line_t *color_line);

/**
 * hb_paint_linear_gradient_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @color_line: Color information for the gradient
 * @x0: X coordinate of the first point
 * @y0: Y coordinate of the first point
 * @x1: X coordinate of the second point
 * @y1: Y coordinate of the second point
 * @x2: X coordinate of the third point
 * @y2: Y coordinate of the third point
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to paint a linear
 * gradient everywhere within the current clip.
 *
 * The coordinates of the points are interpreted according
 * to the current transform.
 *
 * See the OpenType spec COLR section (https://learn.microsoft.com/en-us/typography/opentype/spec/colr)
 * for details on how the points define the direction of the
 * gradient, and how to interpret the @color_line.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_linear_gradient_func_t) (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 hb_color_line_t *color_line,
                                                 float x0, float y0,
                                                 float x1, float y1,
                                                 float x2, float y2,
                                                 void *user_data);

/**
 * hb_paint_radial_gradient_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @color_line: Color information for the gradient
 * @x0: X coordinate of the first circle's center
 * @y0: Y coordinate of the first circle's center
 * @r0: radius of the first circle
 * @x1: X coordinate of the second circle's center
 * @y1: Y coordinate of the second circle's center
 * @r1: radius of the second circle
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to paint a radial
 * gradient everywhere within the current clip.
 *
 * The coordinates of the points are interpreted according
 * to the current transform.
 *
 * See the OpenType spec COLR section (https://learn.microsoft.com/en-us/typography/opentype/spec/colr)
 * for details on how the points define the direction of the
 * gradient, and how to interpret the @color_line.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_radial_gradient_func_t) (hb_paint_funcs_t *funcs,
                                                 void *paint_data,
                                                 hb_color_line_t *color_line,
                                                 float x0, float y0, float r0,
                                                 float x1, float y1, float r1,
                                                 void *user_data);

/**
 * hb_paint_sweep_gradient_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @color_line: Color information for the gradient
 * @x0: X coordinate of the circle's center
 * @y0: Y coordinate of the circle's center
 * @start_angle: the start angle
 * @end_angle: the end angle
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to paint a sweep
 * gradient everywhere within the current clip.
 *
 * The coordinates of the points are interpreted according
 * to the current transform.
 *
 * See the OpenType spec COLR section (https://learn.microsoft.com/en-us/typography/opentype/spec/colr)
 * for details on how the points define the direction of the
 * gradient, and how to interpret the @color_line.
 *
 * Since: REPLACEME
 */
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

/**
 * hb_paint_push_group_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to use
 * an intermediate surface for subsequent paint calls.
 *
 * The drawing will be redirected to an intermediate surface
 * until a matching call to the #hb_paint_funcs_pop_group_func_t
 * vfunc.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_push_group_func_t) (hb_paint_funcs_t *funcs,
                                            void *paint_data,
                                            void *user_data);

/**
 * hb_paint_pop_group_func_t:
 * @funcs: paint functions object
 * @paint_data: The data accompanying the paint functions
 * @mode: the compositing mode to use
 * @user_data: user data passed to the hb_font_paint_glyph() call
 *
 * A virtual method for the #hb_paint_funcs_t to undo
 * the effect of a prior call to the #hb_paint_funcs_push_group_func_t
 * vfunc.
 *
 * This call stops the redirection to the intermediate surface,
 * and then composites it on the previous surface, using the
 * compositing mode passed to this call.
 *
 * Since: REPLACEME
 */
typedef void (*hb_paint_pop_group_func_t) (hb_paint_funcs_t *funcs,
                                           void *paint_data,
                                           hb_paint_composite_mode_t mode,
                                           void *user_data);

/**
 * hb_paint_funcs_set_push_transform_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The push-transform callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the push-transform callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_push_transform_func (hb_paint_funcs_t               *funcs,
                                        hb_paint_push_transform_func_t  func,
                                        void                           *user_data,
                                        hb_destroy_func_t               destroy);

/**
 * hb_paint_funcs_set_pop_transform_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The pop-transform callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the pop-transform callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_pop_transform_func (hb_paint_funcs_t              *funcs,
                                       hb_paint_pop_transform_func_t  func,
                                       void                          *user_data,
                                       hb_destroy_func_t              destroy);

/**
 * hb_paint_funcs_set_push_clip_glyph_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The push-clip-glyph callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the push-clip-glyph callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_push_clip_glyph_func (hb_paint_funcs_t                *funcs,
                                         hb_paint_push_clip_glyph_func_t  func,
                                         void                            *user_data,
                                         hb_destroy_func_t                destroy);

/**
 * hb_paint_funcs_set_push_clip_rectangle_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The push-clip-rectangle callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the push-clip-rect callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_push_clip_rectangle_func (hb_paint_funcs_t                    *funcs,
                                             hb_paint_push_clip_rectangle_func_t  func,
                                             void                                *user_data,
                                             hb_destroy_func_t                    destroy);

/**
 * hb_paint_funcs_set_pop_clip_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The pop-clip callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the pop-clip callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_pop_clip_func (hb_paint_funcs_t         *funcs,
                                  hb_paint_pop_clip_func_t  func,
                                  void                     *user_data,
                                  hb_destroy_func_t         destroy);

/**
 * hb_paint_funcs_set_color_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The paint-color callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the paint-color callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_color_func (hb_paint_funcs_t      *funcs,
                               hb_paint_color_func_t  func,
                               void                  *user_data,
                               hb_destroy_func_t      destroy);

/**
 * hb_paint_funcs_set_image_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The paint-image callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the paint-image callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_image_func (hb_paint_funcs_t      *funcs,
                               hb_paint_image_func_t  func,
                               void                  *user_data,
                               hb_destroy_func_t      destroy);

/**
 * hb_paint_funcs_set_linear_gradient_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The linear-gradient callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the linear-gradient callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_linear_gradient_func (hb_paint_funcs_t                *funcs,
                                         hb_paint_linear_gradient_func_t  func,
                                         void                            *user_data,
                                         hb_destroy_func_t                destroy);

/**
 * hb_paint_funcs_set_radial_gradient_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The radial-gradient callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the radial-gradient callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_radial_gradient_func (hb_paint_funcs_t                *funcs,
                                         hb_paint_radial_gradient_func_t  func,
                                         void                            *user_data,
                                         hb_destroy_func_t                destroy);

/**
 * hb_paint_funcs_set_sweep_gradient_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The sweep-gradient callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the sweep-gradient callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_sweep_gradient_func (hb_paint_funcs_t               *funcs,
                                        hb_paint_sweep_gradient_func_t  func,
                                        void                           *user_data,
                                        hb_destroy_func_t               destroy);

/**
 * hb_paint_funcs_set_push_group_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The push-group callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the push-group callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_push_group_func (hb_paint_funcs_t           *funcs,
                                    hb_paint_push_group_func_t  func,
                                    void                       *user_data,
                                    hb_destroy_func_t           destroy);

/**
 * hb_paint_funcs_set_pop_group_func:
 * @funcs: A paint functions struct
 * @func: (closure user_data) (destroy destroy) (scope notified): The pop-group callback
 * @user_data: Data to pass to @func
 * @destroy: (nullable): Function to call when @user_data is no longer needed
 *
 * Sets the pop-group callback on the paint functions struct.
 *
 * Since: REPLACEME
 */
HB_EXTERN void
hb_paint_funcs_set_pop_group_func (hb_paint_funcs_t          *funcs,
                                   hb_paint_pop_group_func_t  func,
                                   void                       *user_data,
                                   hb_destroy_func_t           destroy);

HB_EXTERN void
hb_paint_push_transform (hb_paint_funcs_t *funcs, void *paint_data,
                         float xx, float yx,
                         float xy, float yy,
                         float dx, float dy);

HB_EXTERN void
hb_paint_pop_transform (hb_paint_funcs_t *funcs, void *paint_data);

HB_EXTERN void
hb_paint_push_clip_glyph (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_codepoint_t glyph);

HB_EXTERN void
hb_paint_push_clip_rectangle (hb_paint_funcs_t *funcs, void *paint_data,
                              float xmin, float ymin,
                              float xmax, float ymax);

HB_EXTERN void
hb_paint_pop_clip (hb_paint_funcs_t *funcs, void *paint_data);

HB_EXTERN void
hb_paint_color (hb_paint_funcs_t *funcs, void *paint_data,
                unsigned int color_index,
                float alpha);

HB_EXTERN void
hb_paint_image (hb_paint_funcs_t *funcs, void *paint_data,
                hb_blob_t *image,
                const char *mimetype,
                hb_glyph_extents_t *extents);

HB_EXTERN void
hb_paint_linear_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_color_line_t *color_line,
                          float x0, float y0,
                          float x1, float y1,
                          float x2, float y2);

HB_EXTERN void
hb_paint_radial_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_color_line_t *color_line,
                          float x0, float y0,
                          float r0,
                          float x1, float y1,
                          float r1);

HB_EXTERN void
hb_paint_sweep_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                         hb_color_line_t *color_line,
                         float x0, float y0,
                         float start_angle, float end_angle);

HB_EXTERN void
hb_paint_push_group (hb_paint_funcs_t *funcs, void *paint_data);

HB_EXTERN void
hb_paint_pop_group (hb_paint_funcs_t *funcs, void *paint_data,
                    hb_paint_composite_mode_t mode);

HB_END_DECLS

#endif  /* HB_PAINT_H */
