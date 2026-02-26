/*
 * Copyright (C) 2026  Behdad Esfahbod
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
 *
 * Author(s): Behdad Esfahbod
 */

#ifndef HB_VECTOR_H
#define HB_VECTOR_H

#include "hb.h"

HB_BEGIN_DECLS

/**
 * hb_vector_format_t:
 * @HB_VECTOR_FORMAT_INVALID: Invalid format.
 * @HB_VECTOR_FORMAT_SVG: SVG output.
 *
 * Output format for vector conversion.
 *
 * XSince: REPLACEME
 */
typedef enum {
  HB_VECTOR_FORMAT_INVALID = HB_TAG_NONE,
  HB_VECTOR_FORMAT_SVG = HB_TAG ('s','v','g',' '),
} hb_vector_format_t;

/**
 * hb_vector_extents_t:
 * @x: Left edge of the output coordinate system.
 * @y: Top edge of the output coordinate system.
 * @width: Width of the output coordinate system.
 * @height: Height of the output coordinate system.
 *
 * Vector output extents, mapped to SVG viewBox.
 *
 * XSince: REPLACEME
 */
typedef struct hb_vector_extents_t {
  float x, y;
  float width, height;
} hb_vector_extents_t;

/**
 * hb_vector_extents_mode_t:
 * @HB_VECTOR_EXTENTS_MODE_NONE: Do not update extents.
 * @HB_VECTOR_EXTENTS_MODE_EXPAND: Union glyph ink extents into current extents.
 *
 * Controls whether convenience glyph APIs update context extents.
 *
 * XSince: REPLACEME
 */
typedef enum {
  HB_VECTOR_EXTENTS_MODE_NONE = 0,
  HB_VECTOR_EXTENTS_MODE_EXPAND = 1,
} hb_vector_extents_mode_t;

/**
 * hb_vector_draw_t:
 *
 * Opaque draw context for vector outline conversion.
 *
 * XSince: REPLACEME
 */
typedef struct hb_vector_draw_t hb_vector_draw_t;

/**
 * hb_vector_paint_t:
 *
 * Opaque paint context for vector color-glyph conversion.
 *
 * XSince: REPLACEME
 */
typedef struct hb_vector_paint_t hb_vector_paint_t;

/* hb_vector_draw_t */

/**
 * hb_vector_draw_create_or_fail:
 * @format: output format.
 *
 * Creates a new draw context for vector output.
 *
 * Return value: (nullable): a newly allocated #hb_vector_draw_t, or `NULL` on failure.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_vector_draw_t *
hb_vector_draw_create_or_fail (hb_vector_format_t format);

/**
 * hb_vector_draw_reference:
 * @draw: a draw context.
 *
 * Increases the reference count of @draw.
 *
 * Return value: (transfer full): referenced @draw.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_vector_draw_t *
hb_vector_draw_reference (hb_vector_draw_t *draw);

/**
 * hb_vector_draw_destroy:
 * @draw: a draw context.
 *
 * Decreases the reference count of @draw and destroys it when it reaches zero.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_draw_destroy (hb_vector_draw_t *draw);

/**
 * hb_vector_draw_set_user_data:
 * @draw: a draw context.
 * @key: user-data key.
 * @data: user-data value.
 * @destroy: (nullable): destroy callback for @data.
 * @replace: whether to replace an existing value for @key.
 *
 * Attaches user data to @draw.
 *
 * Return value: `true` on success, `false` otherwise.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_bool_t
hb_vector_draw_set_user_data (hb_vector_draw_t   *draw,
                              hb_user_data_key_t *key,
                              void               *data,
                              hb_destroy_func_t   destroy,
                              hb_bool_t           replace);

/**
 * hb_vector_draw_get_user_data:
 * @draw: a draw context.
 * @key: user-data key.
 *
 * Gets previously attached user data from @draw.
 *
 * Return value: (nullable): user-data value associated with @key.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void *
hb_vector_draw_get_user_data (hb_vector_draw_t   *draw,
                              hb_user_data_key_t *key);

/**
 * hb_vector_draw_set_transform:
 * @draw: a draw context.
 * @xx: transform xx component.
 * @yx: transform yx component.
 * @xy: transform xy component.
 * @yy: transform yy component.
 * @dx: transform x translation.
 * @dy: transform y translation.
 *
 * Sets the affine transform used when drawing glyphs.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_draw_set_transform (hb_vector_draw_t *draw,
                              float xx, float yx,
                              float xy, float yy,
                              float dx, float dy);

HB_EXTERN void
hb_vector_draw_get_transform (hb_vector_draw_t *draw,
                              float *xx, float *yx,
                              float *xy, float *yy,
                              float *dx, float *dy);

/**
 * hb_vector_draw_set_scale_factor:
 * @draw: a draw context.
 * @x_scale_factor: x scale factor.
 * @y_scale_factor: y scale factor.
 *
 * Sets additional output scaling factors.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_draw_set_scale_factor (hb_vector_draw_t *draw,
                                 float x_scale_factor,
                                 float y_scale_factor);

HB_EXTERN void
hb_vector_draw_get_scale_factor (hb_vector_draw_t *draw,
                                 float *x_scale_factor,
                                 float *y_scale_factor);

/**
 * hb_vector_draw_set_extents:
 * @draw: a draw context.
 * @extents: (nullable): output extents to set or expand.
 *
 * Sets or expands output extents on @draw. Passing `NULL` clears extents.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_draw_set_extents (hb_vector_draw_t *draw,
                            const hb_vector_extents_t *extents);

HB_EXTERN hb_bool_t
hb_vector_draw_get_extents (hb_vector_draw_t *draw,
                            hb_vector_extents_t *extents);

/**
 * hb_vector_draw_set_glyph_extents:
 * @draw: a draw context.
 * @glyph_extents: glyph extents in font units.
 *
 * Expands @draw extents using @glyph_extents under the current transform.
 *
 * Return value: `true` on success, `false` otherwise.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_bool_t
hb_vector_draw_set_glyph_extents (hb_vector_draw_t *draw,
                                  const hb_glyph_extents_t *glyph_extents);

/**
 * hb_vector_draw_get_funcs:
 *
 * Gets draw callbacks implemented by the vector draw backend.
 *
 * Return value: (transfer none): immutable #hb_draw_funcs_t singleton.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_draw_funcs_t *
hb_vector_draw_get_funcs (void);

/**
 * hb_vector_draw_glyph:
 * @draw: a draw context.
 * @font: font object.
 * @glyph: glyph ID.
 * @pen_x: glyph x origin before context transform.
 * @pen_y: glyph y origin before context transform.
 * @extents_mode: extents update mode.
 *
 * Draws one glyph into @draw.
 *
 * Return value: `true` if glyph data was emitted, `false` otherwise.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_bool_t
hb_vector_draw_glyph (hb_vector_draw_t *draw,
                      hb_font_t *font,
                      hb_codepoint_t glyph,
                      float pen_x,
                      float pen_y,
                      hb_vector_extents_mode_t extents_mode);

/**
 * hb_vector_svg_set_flat:
 * @draw: a draw context.
 * @flat: whether to flatten geometry and disable reuse.
 *
 * Enables or disables SVG draw flattening.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_svg_set_flat (hb_vector_draw_t *draw,
                        hb_bool_t flat);

/**
 * hb_vector_svg_set_precision:
 * @draw: a draw context.
 * @precision: decimal precision.
 *
 * Sets numeric output precision for SVG draw output.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_svg_set_precision (hb_vector_draw_t *draw,
                             unsigned precision);

/**
 * hb_vector_draw_render:
 * @draw: a draw context.
 *
 * Renders accumulated draw content to an SVG blob.
 *
 * Return value: (transfer full) (nullable): output blob, or `NULL` if rendering cannot proceed.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_blob_t *
hb_vector_draw_render (hb_vector_draw_t *draw);

/**
 * hb_vector_draw_reset:
 * @draw: a draw context.
 *
 * Resets @draw state and clears accumulated content.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_draw_reset (hb_vector_draw_t *draw);

/**
 * hb_vector_draw_recycle_blob:
 * @draw: a draw context.
 * @blob: (nullable): previously rendered blob to recycle.
 *
 * Provides a blob for internal buffer reuse by later render calls.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_draw_recycle_blob (hb_vector_draw_t *draw,
                             hb_blob_t *blob);


/* hb_vector_paint_t */

/**
 * hb_vector_paint_create_or_fail:
 * @format: output format.
 *
 * Creates a new paint context for vector output.
 *
 * Return value: (nullable): a newly allocated #hb_vector_paint_t, or `NULL` on failure.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_vector_paint_t *
hb_vector_paint_create_or_fail (hb_vector_format_t format);

/**
 * hb_vector_paint_reference:
 * @paint: a paint context.
 *
 * Increases the reference count of @paint.
 *
 * Return value: (transfer full): referenced @paint.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_vector_paint_t *
hb_vector_paint_reference (hb_vector_paint_t *paint);

/**
 * hb_vector_paint_destroy:
 * @paint: a paint context.
 *
 * Decreases the reference count of @paint and destroys it when it reaches zero.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_destroy (hb_vector_paint_t *paint);

/**
 * hb_vector_paint_set_user_data:
 * @paint: a paint context.
 * @key: user-data key.
 * @data: user-data value.
 * @destroy: (nullable): destroy callback for @data.
 * @replace: whether to replace an existing value for @key.
 *
 * Attaches user data to @paint.
 *
 * Return value: `true` on success, `false` otherwise.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_bool_t
hb_vector_paint_set_user_data (hb_vector_paint_t  *paint,
                               hb_user_data_key_t *key,
                               void               *data,
                               hb_destroy_func_t   destroy,
                               hb_bool_t           replace);

/**
 * hb_vector_paint_get_user_data:
 * @paint: a paint context.
 * @key: user-data key.
 *
 * Gets previously attached user data from @paint.
 *
 * Return value: (nullable): user-data value associated with @key.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void *
hb_vector_paint_get_user_data (hb_vector_paint_t  *paint,
                               hb_user_data_key_t *key);

/**
 * hb_vector_paint_set_transform:
 * @paint: a paint context.
 * @xx: transform xx component.
 * @yx: transform yx component.
 * @xy: transform xy component.
 * @yy: transform yy component.
 * @dx: transform x translation.
 * @dy: transform y translation.
 *
 * Sets the affine transform used when painting glyphs.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_set_transform (hb_vector_paint_t *paint,
                               float xx, float yx,
                               float xy, float yy,
                               float dx, float dy);

HB_EXTERN void
hb_vector_paint_get_transform (hb_vector_paint_t *paint,
                               float *xx, float *yx,
                               float *xy, float *yy,
                               float *dx, float *dy);

/**
 * hb_vector_paint_set_scale_factor:
 * @paint: a paint context.
 * @x_scale_factor: x scale factor.
 * @y_scale_factor: y scale factor.
 *
 * Sets additional output scaling factors.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_set_scale_factor (hb_vector_paint_t *paint,
                                  float x_scale_factor,
                                  float y_scale_factor);

HB_EXTERN void
hb_vector_paint_get_scale_factor (hb_vector_paint_t *paint,
                                  float *x_scale_factor,
                                  float *y_scale_factor);

/**
 * hb_vector_paint_set_extents:
 * @paint: a paint context.
 * @extents: (nullable): output extents to set or expand.
 *
 * Sets or expands output extents on @paint. Passing `NULL` clears extents.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_set_extents (hb_vector_paint_t *paint,
                             const hb_vector_extents_t *extents);

HB_EXTERN hb_bool_t
hb_vector_paint_get_extents (hb_vector_paint_t *paint,
                             hb_vector_extents_t *extents);

/**
 * hb_vector_paint_set_glyph_extents:
 * @paint: a paint context.
 * @glyph_extents: glyph extents in font units.
 *
 * Expands @paint extents using @glyph_extents under the current transform.
 *
 * Return value: `true` on success, `false` otherwise.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_bool_t
hb_vector_paint_set_glyph_extents (hb_vector_paint_t *paint,
                                   const hb_glyph_extents_t *glyph_extents);

/**
 * hb_vector_paint_set_foreground:
 * @paint: a paint context.
 * @foreground: foreground color used for COLR foreground paints.
 *
 * Sets fallback foreground color used by paint operations.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_set_foreground (hb_vector_paint_t *paint,
                                hb_color_t foreground);

/**
 * hb_vector_paint_set_palette:
 * @paint: a paint context.
 * @palette: palette index for color glyph painting.
 *
 * Sets the color palette index used by paint operations.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_set_palette (hb_vector_paint_t *paint,
                             int palette);

/**
 * hb_vector_paint_set_custom_palette_color:
 * @paint: a paint context.
 * @color_index: color index to override.
 * @color: replacement color.
 *
 * Overrides one font palette color entry for subsequent paint operations.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_set_custom_palette_color (hb_vector_paint_t *paint,
                                          unsigned color_index,
                                          hb_color_t color);

/**
 * hb_vector_paint_clear_custom_palette_colors:
 * @paint: a paint context.
 *
 * Clears all custom palette color overrides.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_clear_custom_palette_colors (hb_vector_paint_t *paint);

/**
 * hb_vector_paint_get_funcs:
 *
 * Gets paint callbacks implemented by the vector paint backend.
 *
 * Return value: (transfer none): immutable #hb_paint_funcs_t singleton.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_paint_funcs_t *
hb_vector_paint_get_funcs (void);

/**
 * hb_vector_paint_glyph:
 * @paint: a paint context.
 * @font: font object.
 * @glyph: glyph ID.
 * @pen_x: glyph x origin before context transform.
 * @pen_y: glyph y origin before context transform.
 * @extents_mode: extents update mode.
 *
 * Paints one color glyph into @paint.
 *
 * Return value: `true` if glyph paint data was emitted, `false` otherwise.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_bool_t
hb_vector_paint_glyph (hb_vector_paint_t *paint,
		       hb_font_t         *font,
		       hb_codepoint_t     glyph,
		       float              pen_x,
		       float              pen_y,
		       hb_vector_extents_mode_t extents_mode);

/**
 * hb_vector_svg_paint_set_flat:
 * @paint: a paint context.
 * @flat: whether to flatten paint output and disable glyph-group reuse.
 *
 * Enables or disables SVG paint flattening.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_svg_paint_set_flat (hb_vector_paint_t *paint,
                              hb_bool_t flat);

/**
 * hb_vector_svg_paint_set_precision:
 * @paint: a paint context.
 * @precision: decimal precision.
 *
 * Sets numeric output precision for SVG paint output.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_svg_paint_set_precision (hb_vector_paint_t *paint,
                                   unsigned precision);

/**
 * hb_vector_paint_render:
 * @paint: a paint context.
 *
 * Renders accumulated paint content to an SVG blob.
 *
 * Return value: (transfer full) (nullable): output blob, or `NULL` if rendering cannot proceed.
 *
 * XSince: REPLACEME
 */
HB_EXTERN hb_blob_t *
hb_vector_paint_render (hb_vector_paint_t *paint);

/**
 * hb_vector_paint_reset:
 * @paint: a paint context.
 *
 * Resets @paint state and clears accumulated content.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_reset (hb_vector_paint_t *paint);

/**
 * hb_vector_paint_recycle_blob:
 * @paint: a paint context.
 * @blob: (nullable): previously rendered blob to recycle.
 *
 * Provides a blob for internal buffer reuse by later render calls.
 *
 * XSince: REPLACEME
 */
HB_EXTERN void
hb_vector_paint_recycle_blob (hb_vector_paint_t *paint,
                              hb_blob_t *blob);

HB_END_DECLS

#endif /* HB_VECTOR_H */
