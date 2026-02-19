/*
 * Copyright © 2026  Behdad Esfahbod
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

#ifndef HB_RASTER_H
#define HB_RASTER_H

#include "hb.h"

HB_BEGIN_DECLS


/* ── Shared types ─────────────────────────────────────────────────── */

/**
 * hb_raster_format_t:
 * @HB_RASTER_FORMAT_A8: 8-bit alpha-only coverage
 * @HB_RASTER_FORMAT_BGRA32: 32-bit BGRA color
 *
 * Pixel format for raster images.
 *
 * Since: REPLACEME
 */
typedef enum {
  HB_RASTER_FORMAT_A8     = 0,
  HB_RASTER_FORMAT_BGRA32 = 1,
} hb_raster_format_t;

/**
 * hb_raster_extents_t:
 * @x_origin: X coordinate of the left edge of the image in glyph space
 * @y_origin: Y coordinate of the bottom edge of the image in glyph space
 * @width: Width in pixels
 * @height: Height in pixels
 * @stride: Bytes per row; 0 means auto-calculate on input, filled on output
 *
 * Pixel-buffer extents for raster operations.
 *
 * XSince: REPLACEME
 */
typedef struct hb_raster_extents_t {
  int      x_origin, y_origin;
  unsigned width, height;
  unsigned stride;
} hb_raster_extents_t;


/* ── hb_raster_image_t ────────────────────────────────────────────── */

typedef struct hb_raster_image_t hb_raster_image_t;

HB_EXTERN hb_raster_image_t *
hb_raster_image_reference (hb_raster_image_t *image);

HB_EXTERN void
hb_raster_image_destroy (hb_raster_image_t *image);

HB_EXTERN hb_bool_t
hb_raster_image_set_user_data (hb_raster_image_t  *image,
			       hb_user_data_key_t *key,
			       void               *data,
			       hb_destroy_func_t   destroy,
			       hb_bool_t           replace);

HB_EXTERN void *
hb_raster_image_get_user_data (hb_raster_image_t  *image,
			       hb_user_data_key_t *key);

HB_EXTERN const uint8_t *
hb_raster_image_get_buffer (hb_raster_image_t *image);

HB_EXTERN void
hb_raster_image_get_extents (hb_raster_image_t   *image,
			     hb_raster_extents_t *extents);

HB_EXTERN hb_raster_format_t
hb_raster_image_get_format (hb_raster_image_t *image);


/* ── hb_raster_draw_t ─────────────────────────────────────────────── */

typedef struct hb_raster_draw_t hb_raster_draw_t;

HB_EXTERN hb_raster_draw_t *
hb_raster_draw_create (void);

HB_EXTERN hb_raster_draw_t *
hb_raster_draw_reference (hb_raster_draw_t *draw);

HB_EXTERN void
hb_raster_draw_destroy (hb_raster_draw_t *draw);

HB_EXTERN hb_bool_t
hb_raster_draw_set_user_data (hb_raster_draw_t   *draw,
			      hb_user_data_key_t *key,
			      void               *data,
			      hb_destroy_func_t   destroy,
			      hb_bool_t           replace);

HB_EXTERN void *
hb_raster_draw_get_user_data (hb_raster_draw_t   *draw,
			      hb_user_data_key_t *key);

HB_EXTERN void
hb_raster_draw_set_format (hb_raster_draw_t  *draw,
			   hb_raster_format_t format);

HB_EXTERN hb_raster_format_t
hb_raster_draw_get_format (hb_raster_draw_t *draw);

HB_EXTERN void
hb_raster_draw_set_transform (hb_raster_draw_t *draw,
			      float xx, float yx,
			      float xy, float yy,
			      float dx, float dy);

HB_EXTERN void
hb_raster_draw_get_transform (hb_raster_draw_t *draw,
			      float *xx, float *yx,
			      float *xy, float *yy,
			      float *dx, float *dy);

HB_EXTERN void
hb_raster_draw_set_extents (hb_raster_draw_t          *draw,
			    const hb_raster_extents_t *extents);

HB_EXTERN hb_draw_funcs_t *
hb_raster_draw_get_funcs (void);

HB_EXTERN hb_raster_image_t *
hb_raster_draw_render (hb_raster_draw_t *draw);

HB_EXTERN void
hb_raster_draw_reset (hb_raster_draw_t *draw);


HB_END_DECLS

#endif /* HB_RASTER_H */
