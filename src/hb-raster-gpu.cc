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

#include "hb-raster.h"
#include "hb-raster-image.hh"
#include "hb-gpu-draw.hh"

#include <math.h>


#ifndef HB_NO_RASTER_GPU

struct hb_raster_gpu_texel_t
{
  int16_t r, g, b, a;
};

struct hb_raster_gpu_blob_t
{
  const char *data = nullptr;
  unsigned texel_count = 0;

  float min_x = 0.f;
  float min_y = 0.f;
  float max_x = 0.f;
  float max_y = 0.f;

  int num_hbands = 0;
  int num_vbands = 0;
  float scale_x = 0.f;
  float scale_y = 0.f;

  float band_scale_x = 0.f;
  float band_scale_y = 0.f;
  float band_offset_x = 0.f;
  float band_offset_y = 0.f;
  unsigned band_base = 2;
};

static constexpr float HB_RASTER_GPU_INV_UNITS = 1.f / HB_GPU_UNITS_PER_EM;
static constexpr float HB_RASTER_GPU_EPSILON = 1.f / 65536.f;

static HB_ALWAYS_INLINE unsigned
hb_raster_gpu_decode_offset (int16_t v)
{
  return (unsigned) ((int) v + 32768);
}

static HB_ALWAYS_INLINE float
hb_raster_gpu_clamp01 (float v)
{
  return hb_clamp (v, 0.f, 1.f);
}

static HB_ALWAYS_INLINE float
hb_raster_gpu_smoothstep (float edge0,
			  float edge1,
			  float x)
{
  float t = hb_raster_gpu_clamp01 ((x - edge0) / (edge1 - edge0));
  return t * t * (3.f - 2.f * t);
}

static HB_ALWAYS_INLINE uint32_t
hb_raster_gpu_float_bits_to_uint (float v)
{
  uint32_t u = 0;
  hb_memcpy (&u, &v, sizeof (u));
  return u;
}

static HB_ALWAYS_INLINE uint32_t
hb_raster_gpu_calc_root_code (float y1,
			      float y2,
			      float y3)
{
  uint32_t i1 = hb_raster_gpu_float_bits_to_uint (y1) >> 31U;
  uint32_t i2 = hb_raster_gpu_float_bits_to_uint (y2) >> 30U;
  uint32_t i3 = hb_raster_gpu_float_bits_to_uint (y3) >> 29U;

  uint32_t shift = (i2 & 2U) | (i1 & ~2U);
  shift = (i3 & 4U) | (shift & ~4U);

  return (0x2E74U >> shift) & 0x0101U;
}

static HB_ALWAYS_INLINE bool
hb_raster_gpu_read_texel (const hb_raster_gpu_blob_t *blob,
			  unsigned                    offset,
			  hb_raster_gpu_texel_t      *out)
{
  if (unlikely (offset >= blob->texel_count))
    return false;

  hb_memcpy (out, blob->data + offset * sizeof (*out), sizeof (*out));
  return true;
}

static HB_ALWAYS_INLINE void
hb_raster_gpu_solve_horiz_poly (float  ax,
				float  ay,
				float  bx,
				float  by,
				float  p1x,
				float  p1y,
				float *r0,
				float *r1)
{
  float ra = 1.f / ay;
  float rb = 0.5f / by;

  float d = sqrtf (hb_max (by * by - ay * p1y, 0.f));
  float t1 = (by - d) * ra;
  float t2 = (by + d) * ra;

  if (ay == 0.f)
    t1 = t2 = p1y * rb;

  *r0 = (ax * t1 - bx * 2.f) * t1 + p1x;
  *r1 = (ax * t2 - bx * 2.f) * t2 + p1x;
}

static HB_ALWAYS_INLINE void
hb_raster_gpu_solve_vert_poly (float  ax,
			       float  ay,
			       float  bx,
			       float  by,
			       float  p1x,
			       float  p1y,
			       float *r0,
			       float *r1)
{
  float ra = 1.f / ax;
  float rb = 0.5f / bx;

  float d = sqrtf (hb_max (bx * bx - ax * p1x, 0.f));
  float t1 = (bx - d) * ra;
  float t2 = (bx + d) * ra;

  if (ax == 0.f)
    t1 = t2 = p1x * rb;

  *r0 = (ay * t1 - by * 2.f) * t1 + p1y;
  *r1 = (ay * t2 - by * 2.f) * t2 + p1y;
}

static HB_ALWAYS_INLINE float
hb_raster_gpu_calc_coverage (float xcov,
			     float ycov,
			     float xwgt,
			     float ywgt)
{
  float coverage = hb_max (fabsf (xcov * xwgt + ycov * ywgt) /
			   hb_max (xwgt + ywgt, HB_RASTER_GPU_EPSILON),
			   hb_min (fabsf (xcov), fabsf (ycov)));

  return hb_raster_gpu_clamp01 (coverage);
}

static bool
hb_raster_gpu_decode_blob (const char           *data,
			   unsigned              texel_count,
			   hb_raster_gpu_blob_t *blob)
{
  if (unlikely (texel_count < 2))
    return false;

  blob->data = data;
  blob->texel_count = texel_count;

  hb_raster_gpu_texel_t header0;
  hb_raster_gpu_texel_t header1;
  if (unlikely (!hb_raster_gpu_read_texel (blob, 0, &header0) ||
		!hb_raster_gpu_read_texel (blob, 1, &header1)))
    return false;

  blob->min_x = header0.r * HB_RASTER_GPU_INV_UNITS;
  blob->min_y = header0.g * HB_RASTER_GPU_INV_UNITS;
  blob->max_x = header0.b * HB_RASTER_GPU_INV_UNITS;
  blob->max_y = header0.a * HB_RASTER_GPU_INV_UNITS;
  blob->num_hbands = header1.r;
  blob->num_vbands = header1.g;
  blob->scale_x = (float) header1.b;
  blob->scale_y = (float) header1.a;

  if (unlikely (blob->num_hbands <= 0 || blob->num_vbands <= 0 ||
		blob->max_x < blob->min_x ||
		blob->max_y < blob->min_y))
    return false;

  unsigned band_headers = (unsigned) blob->num_hbands + (unsigned) blob->num_vbands;
  if (unlikely (blob->band_base + band_headers > texel_count))
    return false;

  float ext_size_x = blob->max_x - blob->min_x;
  float ext_size_y = blob->max_y - blob->min_y;

  blob->band_scale_x = (float) blob->num_vbands /
		       hb_max (ext_size_x, HB_RASTER_GPU_EPSILON);
  blob->band_scale_y = (float) blob->num_hbands /
		       hb_max (ext_size_y, HB_RASTER_GPU_EPSILON);
  blob->band_offset_x = -blob->min_x * blob->band_scale_x;
  blob->band_offset_y = -blob->min_y * blob->band_scale_y;
  return true;
}

static bool
hb_raster_gpu_render_single (const hb_raster_gpu_blob_t *blob,
			     float                       render_x,
			     float                       render_y,
			     float                       pixels_per_em_x,
			     float                       pixels_per_em_y,
			     float                      *coverage)
{
  int band_index_x = hb_clamp ((int) (render_x * blob->band_scale_x + blob->band_offset_x),
			       0, blob->num_vbands - 1);
  int band_index_y = hb_clamp ((int) (render_y * blob->band_scale_y + blob->band_offset_y),
			       0, blob->num_hbands - 1);

  float xcov = 0.f;
  float xwgt = 0.f;

  hb_raster_gpu_texel_t hband_data;
  if (unlikely (!hb_raster_gpu_read_texel (blob, blob->band_base + (unsigned) band_index_y, &hband_data) ||
		hband_data.r < 0))
    return false;

  int h_curve_count = hband_data.r;
  float h_split = hband_data.a * HB_RASTER_GPU_INV_UNITS;
  bool h_left_ray = render_x < h_split;
  unsigned h_data_offset = hb_raster_gpu_decode_offset (h_left_ray ? hband_data.b : hband_data.g);

  for (int ci = 0; ci < h_curve_count; ci++)
  {
    hb_raster_gpu_texel_t curve_index;
    if (unlikely (!hb_raster_gpu_read_texel (blob, h_data_offset + (unsigned) ci, &curve_index)))
      return false;

    unsigned curve_offset = hb_raster_gpu_decode_offset (curve_index.r);
    hb_raster_gpu_texel_t raw12;
    hb_raster_gpu_texel_t raw3;
    if (unlikely (!hb_raster_gpu_read_texel (blob, curve_offset, &raw12) ||
		  !hb_raster_gpu_read_texel (blob, curve_offset + 1u, &raw3)))
      return false;

    float q12x = raw12.r * HB_RASTER_GPU_INV_UNITS;
    float q12y = raw12.g * HB_RASTER_GPU_INV_UNITS;
    float q12z = raw12.b * HB_RASTER_GPU_INV_UNITS;
    float q12w = raw12.a * HB_RASTER_GPU_INV_UNITS;
    float q3x = raw3.r * HB_RASTER_GPU_INV_UNITS;
    float q3y = raw3.g * HB_RASTER_GPU_INV_UNITS;

    float p12x = q12x - render_x;
    float p12y = q12y - render_y;
    float p12z = q12z - render_x;
    float p12w = q12w - render_y;
    float p3x = q3x - render_x;
    float p3y = q3y - render_y;

    if (h_left_ray)
    {
      if (hb_min (hb_min (p12x, p12z), p3x) * pixels_per_em_x > 0.5f)
	break;
    }
    else
    {
      if (hb_max (hb_max (p12x, p12z), p3x) * pixels_per_em_x < -0.5f)
	break;
    }

    uint32_t code = hb_raster_gpu_calc_root_code (p12y, p12w, p3y);
    if (!code)
      continue;

    float ax = q12x - q12z * 2.f + q3x;
    float ay = q12y - q12w * 2.f + q3y;
    float bx = q12x - q12z;
    float by = q12y - q12w;
    float r0, r1;
    hb_raster_gpu_solve_horiz_poly (ax, ay, bx, by, p12x, p12y, &r0, &r1);
    r0 *= pixels_per_em_x;
    r1 *= pixels_per_em_x;

    float cov0 = h_left_ray ? hb_raster_gpu_clamp01 (0.5f - r0)
			    : hb_raster_gpu_clamp01 (r0 + 0.5f);
    float cov1 = h_left_ray ? hb_raster_gpu_clamp01 (0.5f - r1)
			    : hb_raster_gpu_clamp01 (r1 + 0.5f);

    if (code & 1U)
    {
      xcov += cov0;
      xwgt = hb_max (xwgt, hb_raster_gpu_clamp01 (1.f - fabsf (r0) * 2.f));
    }

    if (code > 1U)
    {
      xcov -= cov1;
      xwgt = hb_max (xwgt, hb_raster_gpu_clamp01 (1.f - fabsf (r1) * 2.f));
    }
  }

  float ycov = 0.f;
  float ywgt = 0.f;

  hb_raster_gpu_texel_t vband_data;
  if (unlikely (!hb_raster_gpu_read_texel (blob,
					   blob->band_base + (unsigned) blob->num_hbands + (unsigned) band_index_x,
					   &vband_data) ||
		vband_data.r < 0))
    return false;

  int v_curve_count = vband_data.r;
  float v_split = vband_data.a * HB_RASTER_GPU_INV_UNITS;
  bool v_left_ray = render_y < v_split;
  unsigned v_data_offset = hb_raster_gpu_decode_offset (v_left_ray ? vband_data.b : vband_data.g);

  for (int ci = 0; ci < v_curve_count; ci++)
  {
    hb_raster_gpu_texel_t curve_index;
    if (unlikely (!hb_raster_gpu_read_texel (blob, v_data_offset + (unsigned) ci, &curve_index)))
      return false;

    unsigned curve_offset = hb_raster_gpu_decode_offset (curve_index.r);
    hb_raster_gpu_texel_t raw12;
    hb_raster_gpu_texel_t raw3;
    if (unlikely (!hb_raster_gpu_read_texel (blob, curve_offset, &raw12) ||
		  !hb_raster_gpu_read_texel (blob, curve_offset + 1u, &raw3)))
      return false;

    float q12x = raw12.r * HB_RASTER_GPU_INV_UNITS;
    float q12y = raw12.g * HB_RASTER_GPU_INV_UNITS;
    float q12z = raw12.b * HB_RASTER_GPU_INV_UNITS;
    float q12w = raw12.a * HB_RASTER_GPU_INV_UNITS;
    float q3x = raw3.r * HB_RASTER_GPU_INV_UNITS;
    float q3y = raw3.g * HB_RASTER_GPU_INV_UNITS;

    float p12x = q12x - render_x;
    float p12y = q12y - render_y;
    float p12z = q12z - render_x;
    float p12w = q12w - render_y;
    float p3x = q3x - render_x;
    float p3y = q3y - render_y;

    if (v_left_ray)
    {
      if (hb_min (hb_min (p12y, p12w), p3y) * pixels_per_em_y > 0.5f)
	break;
    }
    else
    {
      if (hb_max (hb_max (p12y, p12w), p3y) * pixels_per_em_y < -0.5f)
	break;
    }

    uint32_t code = hb_raster_gpu_calc_root_code (p12x, p12z, p3x);
    if (!code)
      continue;

    float ax = q12x - q12z * 2.f + q3x;
    float ay = q12y - q12w * 2.f + q3y;
    float bx = q12x - q12z;
    float by = q12y - q12w;
    float r0, r1;
    hb_raster_gpu_solve_vert_poly (ax, ay, bx, by, p12x, p12y, &r0, &r1);
    r0 *= pixels_per_em_y;
    r1 *= pixels_per_em_y;

    float cov0 = v_left_ray ? hb_raster_gpu_clamp01 (0.5f - r0)
			    : hb_raster_gpu_clamp01 (r0 + 0.5f);
    float cov1 = v_left_ray ? hb_raster_gpu_clamp01 (0.5f - r1)
			    : hb_raster_gpu_clamp01 (r1 + 0.5f);

    if (code & 1U)
    {
      ycov -= cov0;
      ywgt = hb_max (ywgt, hb_raster_gpu_clamp01 (1.f - fabsf (r0) * 2.f));
    }

    if (code > 1U)
    {
      ycov += cov1;
      ywgt = hb_max (ywgt, hb_raster_gpu_clamp01 (1.f - fabsf (r1) * 2.f));
    }
  }

  *coverage = hb_raster_gpu_calc_coverage (xcov, ycov, xwgt, ywgt);
  return true;
}

static bool
hb_raster_gpu_render_coverage (const hb_raster_gpu_blob_t *blob,
			       float                       render_x,
			       float                       render_y,
			       float                      *coverage)
{
  float c = 0.f;
  if (unlikely (!hb_raster_gpu_render_single (blob, render_x, render_y,
					      1.f, 1.f, &c)))
    return false;

#ifndef HB_GPU_NO_MSAA
  float ppem = hb_min (blob->scale_x, blob->scale_y);
  if (ppem < 16.f)
  {
    float c0, c1, c2, c3;
    if (unlikely (!hb_raster_gpu_render_single (blob, render_x - 1.f / 3.f, render_y - 1.f / 3.f, 1.f, 1.f, &c0) ||
		  !hb_raster_gpu_render_single (blob, render_x + 1.f / 3.f, render_y - 1.f / 3.f, 1.f, 1.f, &c1) ||
		  !hb_raster_gpu_render_single (blob, render_x - 1.f / 3.f, render_y + 1.f / 3.f, 1.f, 1.f, &c2) ||
		  !hb_raster_gpu_render_single (blob, render_x + 1.f / 3.f, render_y + 1.f / 3.f, 1.f, 1.f, &c3)))
      return false;

    float msaa = 0.25f * (c0 + c1 + c2 + c3);
    c = c + (msaa - c) * hb_raster_gpu_smoothstep (16.f, 8.f, ppem);
  }
#endif

  *coverage = c;
  return true;
}

static hb_raster_image_t *
hb_raster_gpu_render_from_blob_data_or_fail (const char *data,
					     unsigned    texel_count)
{
  hb_raster_gpu_blob_t blob;
  if (unlikely (!hb_raster_gpu_decode_blob (data, texel_count, &blob)))
    return nullptr;

  constexpr float pad = 0.5f;
  hb_raster_extents_t ext;
  ext.x_origin = (int) floorf (blob.min_x - pad);
  ext.y_origin = (int) floorf (blob.min_y - pad);
  ext.width = (unsigned) hb_max (0, (int) ceilf (blob.max_x + pad) - ext.x_origin);
  ext.height = (unsigned) hb_max (0, (int) ceilf (blob.max_y + pad) - ext.y_origin);
  ext.stride = (ext.width + 3u) & ~3u;

  hb_raster_image_t *image = hb_raster_image_create_or_fail ();
  if (unlikely (!image))
    return nullptr;

  if (unlikely (!image->configure (HB_RASTER_FORMAT_A8, ext)))
    goto fail;
  image->clear ();

  for (unsigned y = 0; y < ext.height; y++)
    for (unsigned x = 0; x < ext.width; x++)
    {
      float render_x = (float) ext.x_origin + (float) x + 0.5f;
      float render_y = (float) ext.y_origin + (float) y + 0.5f;
      float coverage;
      if (unlikely (!hb_raster_gpu_render_coverage (&blob,
						    render_x,
						    render_y,
						    &coverage)))
	goto fail;

      image->buffer.arrayZ[y * ext.stride + x] =
	(uint8_t) (hb_raster_gpu_clamp01 (coverage) * 255.f + 0.5f);
    }

  return image;

fail:
  hb_raster_image_destroy (image);
  return nullptr;
}

#endif /* !HB_NO_RASTER_GPU */


/**
 * hb_raster_gpu_render_from_blob_or_fail:
 * @blob: a blob produced by hb_gpu_draw_encode()
 *
 * Decodes a glyph blob produced by hb_gpu_draw_encode() and rasterizes
 * it into a new @HB_RASTER_FORMAT_A8 image using the same coverage
 * algorithm as hb_gpu_render().
 *
 * Return value: (transfer full):
 * A rendered #hb_raster_image_t, or `NULL` on malformed input or
 * allocation failure. An empty blob returns an empty image.
 *
 * XSince: REPLACEME
 **/
hb_raster_image_t *
hb_raster_gpu_render_from_blob_or_fail (hb_blob_t *blob)
{
#ifdef HB_NO_RASTER_GPU
  return nullptr;
#else
  if (unlikely (!blob))
    return nullptr;

  unsigned length = hb_blob_get_length (blob);
  if (!length)
    return hb_raster_image_create_or_fail ();

  if (unlikely (length % sizeof (hb_raster_gpu_texel_t)))
    return nullptr;

  const char *data = hb_blob_get_data (blob, nullptr);
  if (unlikely (!data))
    return nullptr;

  return hb_raster_gpu_render_from_blob_data_or_fail (data,
						      length / sizeof (hb_raster_gpu_texel_t));
#endif
}
