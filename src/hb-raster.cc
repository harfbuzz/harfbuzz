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

#include "hb.hh"

#include "hb-raster.hh"


/**
 * SECTION:hb-raster
 * @title: hb-raster
 * @short_description: Glyph rasterization
 * @include: hb-raster.h
 *
 * Functions for rasterizing glyph outlines into pixel buffers.
 *
 * The #hb_raster_draw_t object accumulates glyph outlines via
 * #hb_draw_funcs_t callbacks obtained from hb_raster_draw_get_funcs(),
 * then produces an #hb_raster_image_t with hb_raster_draw_render().
 **/


/* hb_raster_image_t */

/**
 * hb_raster_image_reference: (skip)
 * @image: a raster image
 *
 * Increases the reference count on @image by one.
 *
 * This prevents @image from being destroyed until a matching
 * call to hb_raster_image_destroy() is made.
 *
 * Return value: (transfer full):
 * The referenced #hb_raster_image_t.
 *
 * XSince: REPLACEME
 **/
hb_raster_image_t *
hb_raster_image_reference (hb_raster_image_t *image)
{
  return hb_object_reference (image);
}

/**
 * hb_raster_image_destroy: (skip)
 * @image: a raster image
 *
 * Decreases the reference count on @image by one. When the
 * reference count reaches zero, the image and its pixel buffer
 * are freed.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_image_destroy (hb_raster_image_t *image)
{
  if (!hb_object_destroy (image)) return;
  hb_free (image);
}

/**
 * hb_raster_image_set_user_data: (skip)
 * @image: a raster image
 * @key: the user-data key
 * @data: a pointer to the user data
 * @destroy: (nullable): a callback to call when @data is not needed anymore
 * @replace: whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified raster image.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_raster_image_set_user_data (hb_raster_image_t  *image,
			       hb_user_data_key_t *key,
			       void               *data,
			       hb_destroy_func_t   destroy,
			       hb_bool_t           replace)
{
  return hb_object_set_user_data (image, key, data, destroy, replace);
}

/**
 * hb_raster_image_get_user_data: (skip)
 * @image: a raster image
 * @key: the user-data key
 *
 * Fetches the user-data associated with the specified key,
 * attached to the specified raster image.
 *
 * Return value: (transfer none):
 * A pointer to the user data
 *
 * XSince: REPLACEME
 **/
void *
hb_raster_image_get_user_data (hb_raster_image_t  *image,
			       hb_user_data_key_t *key)
{
  return hb_object_get_user_data (image, key);
}

/**
 * hb_raster_image_get_buffer:
 * @image: a raster image
 *
 * Fetches the raw pixel buffer of @image.  The buffer layout is
 * described by the extents obtained from hb_raster_image_get_extents()
 * and the format from hb_raster_image_get_format().
 *
 * Return value: (transfer none) (array):
 * The pixel buffer, or `NULL`
 *
 * XSince: REPLACEME
 **/
const uint8_t *
hb_raster_image_get_buffer (hb_raster_image_t *image)
{
  if (unlikely (!image)) return nullptr;
  return image->buffer.arrayZ;
}

/**
 * hb_raster_image_get_extents:
 * @image: a raster image
 * @extents: (out): the image extents
 *
 * Fetches the pixel-buffer extents of @image.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_image_get_extents (hb_raster_image_t   *image,
			     hb_raster_extents_t *extents)
{
  if (unlikely (!image || !extents)) return;
  *extents = image->extents;
}

/**
 * hb_raster_image_get_format:
 * @image: a raster image
 *
 * Fetches the pixel format of @image.
 *
 * Return value:
 * The #hb_raster_format_t of the image
 *
 * XSince: REPLACEME
 **/
hb_raster_format_t
hb_raster_image_get_format (hb_raster_image_t *image)
{
  if (unlikely (!image)) return HB_RASTER_FORMAT_A8;
  return image->format;
}
