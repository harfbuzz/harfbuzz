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

#ifndef HB_RASTER_PNG_HH
#define HB_RASTER_PNG_HH

#include "hb.hh"

#ifdef HAVE_PNG
#include <png.h>

struct hb_raster_png_read_blob_t
{
  const uint8_t *data = nullptr;
  size_t size = 0;
  size_t offset = 0;
};

static void
hb_raster_png_read_blob (png_structp png, png_bytep out, png_size_t length)
{
  hb_raster_png_read_blob_t *r = (hb_raster_png_read_blob_t *) png_get_io_ptr (png);

  if (!r || !r->data || length > r->size - r->offset)
    png_error (png, "read error");

  hb_memcpy (out, r->data + r->offset, length);
  r->offset += length;
}

static bool
hb_raster_paint_decode_png (hb_blob_t *blob,
			    unsigned *out_width,
			    unsigned *out_height,
			    hb_vector_t<uint32_t> &out_pixels)
{
  if (!blob || !out_width || !out_height)
    return false;

  unsigned blob_len = 0;
  const uint8_t *blob_data = (const uint8_t *) hb_blob_get_data (blob, &blob_len);
  if (!blob_data || !blob_len)
    return false;

  png_structp png = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png)
    return false;

  png_infop info = png_create_info_struct (png);
  if (!info)
  {
    png_destroy_read_struct (&png, nullptr, nullptr);
    return false;
  }

  hb_raster_png_read_blob_t reader;
  reader.data = blob_data;
  reader.size = (size_t) blob_len;
  reader.offset = 0;
  if (setjmp (png_jmpbuf (png)))
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  png_set_read_fn (png, &reader, hb_raster_png_read_blob);
  png_read_info (png, info);

  png_uint_32 w = 0, h = 0;
  int bit_depth = 0, color_type = 0;
  int interlace_type = 0, compression_type = 0, filter_method = 0;
  png_get_IHDR (png, info, &w, &h, &bit_depth, &color_type,
		&interlace_type, &compression_type, &filter_method);

  if (!w || !h || w > (png_uint_32) INT_MAX || h > (png_uint_32) INT_MAX)
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  bool has_trns = png_get_valid (png, info, PNG_INFO_tRNS);

  if (bit_depth == 16)
    png_set_strip_16 (png);
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb (png);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8 (png);
  if (has_trns)
    png_set_tRNS_to_alpha (png);
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb (png);
  if (!(color_type & PNG_COLOR_MASK_ALPHA) && !has_trns)
    png_set_add_alpha (png, 0xff, PNG_FILLER_AFTER);
  if (interlace_type != PNG_INTERLACE_NONE)
    png_set_interlace_handling (png);

  png_read_update_info (png, info);

  if (png_get_bit_depth (png, info) != 8 || png_get_channels (png, info) != 4)
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  png_size_t rowbytes = png_get_rowbytes (png, info);
  if (rowbytes < (png_size_t) w * 4u)
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  size_t pixel_count = (size_t) w * (size_t) h;
  if (pixel_count / (size_t) w != (size_t) h)
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  if (!out_pixels.resize (pixel_count))
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  size_t rgba_size = (size_t) rowbytes * (size_t) h;
  if (h && rgba_size / (size_t) h != (size_t) rowbytes)
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  hb_vector_t<uint8_t> rgba;
  if (!rgba.resize (rgba_size))
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  hb_vector_t<png_bytep> rows;
  if (!rows.resize ((unsigned) h))
  {
    png_destroy_read_struct (&png, &info, nullptr);
    return false;
  }

  for (unsigned y = 0; y < (unsigned) h; y++)
    rows[y] = rgba.arrayZ + (size_t) y * (size_t) rowbytes;

  png_read_image (png, rows.arrayZ);
  png_read_end (png, nullptr);

  for (unsigned y = 0; y < (unsigned) h; y++)
  {
    uint32_t *dst = out_pixels.arrayZ + (size_t) y * (size_t) w;
    const uint8_t *src = rows[y];
    for (unsigned x = 0; x < (unsigned) w; x++)
    {
      uint8_t r = src[4 * x + 0];
      uint8_t g = src[4 * x + 1];
      uint8_t b = src[4 * x + 2];
      uint8_t a = src[4 * x + 3];
      dst[x] = (uint32_t) hb_raster_div255 (b * a)
	     | ((uint32_t) hb_raster_div255 (g * a) << 8)
	     | ((uint32_t) hb_raster_div255 (r * a) << 16)
	     | ((uint32_t) a << 24);
    }
  }

  png_destroy_read_struct (&png, &info, nullptr);

  *out_width = (unsigned) w;
  *out_height = (unsigned) h;
  return true;
}

#endif /* HAVE_PNG */

#endif /* HB_RASTER_PNG_HH */
