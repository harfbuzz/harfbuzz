/*
 * Copyright (C) 2026  Behdad Esfahbod
 * Copyright (C) 2017  Eric Lengyel
 *
 * Based on the Slug algorithm by Eric Lengyel:
 * https://github.com/EricLengyel/Slug
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


/* Requires GLSL 3.30 or GLSL ES 3.00.
 *
 * For GLSL ES 3.00 / WebGL2, define HB_GPU_ATLAS_2D before
 * including this source.  The atlas is then a 2D isampler2D
 * texture of known width (set via hb_gpu_atlas_width uniform)
 * instead of an isamplerBuffer.
 */


#ifndef HB_GPU_UNITS_PER_EM
#define HB_GPU_UNITS_PER_EM 4
#endif

#define HB_GPU_INV_UNITS float(1.0 / float(HB_GPU_UNITS_PER_EM))


#ifdef HB_GPU_ATLAS_2D
uniform highp isampler2D hb_gpu_atlas;
uniform int hb_gpu_atlas_width;
ivec4 _hb_gpu_fetch (int offset)
{
  return texelFetch (hb_gpu_atlas,
		     ivec2 (offset % hb_gpu_atlas_width,
			    offset / hb_gpu_atlas_width), 0);
}
#else
uniform isamplerBuffer hb_gpu_atlas;
ivec4 _hb_gpu_fetch (int offset)
{
  return texelFetch (hb_gpu_atlas, offset);
}
#endif


uint _hb_gpu_calc_root_code (float y1, float y2, float y3)
{
  uint i1 = floatBitsToUint (y1) >> 31U;
  uint i2 = floatBitsToUint (y2) >> 30U;
  uint i3 = floatBitsToUint (y3) >> 29U;

  uint shift = (i2 & 2U) | (i1 & ~2U);
  shift = (i3 & 4U) | (shift & ~4U);

  return (0x2E74U >> shift) & 0x0101U;
}

vec2 _hb_gpu_solve_horiz_poly (vec4 p12, vec2 p3)
{
  vec2 a = p12.xy - p12.zw * 2.0 + p3;
  vec2 b = p12.xy - p12.zw;
  float ra = 1.0 / a.y;
  float rb = 0.5 / b.y;

  float d = sqrt (max (b.y * b.y - a.y * p12.y, 0.0));
  float t1 = (b.y - d) * ra;
  float t2 = (b.y + d) * ra;

  if (abs (a.y) < 1.0 / 65536.0)
    t1 = t2 = p12.y * rb;

  return vec2 ((a.x * t1 - b.x * 2.0) * t1 + p12.x,
	       (a.x * t2 - b.x * 2.0) * t2 + p12.x);
}

vec2 _hb_gpu_solve_vert_poly (vec4 p12, vec2 p3)
{
  vec2 a = p12.xy - p12.zw * 2.0 + p3;
  vec2 b = p12.xy - p12.zw;
  float ra = 1.0 / a.x;
  float rb = 0.5 / b.x;

  float d = sqrt (max (b.x * b.x - a.x * p12.x, 0.0));
  float t1 = (b.x - d) * ra;
  float t2 = (b.x + d) * ra;

  if (abs (a.x) < 1.0 / 65536.0)
    t1 = t2 = p12.x * rb;

  return vec2 ((a.y * t1 - b.y * 2.0) * t1 + p12.y,
	       (a.y * t2 - b.y * 2.0) * t2 + p12.y);
}

float _hb_gpu_calc_coverage (float xcov, float ycov, float xwgt, float ywgt)
{
  float coverage = max (abs (xcov * xwgt + ycov * ywgt) /
			max (xwgt + ywgt, 1.0 / 65536.0),
			min (abs (xcov), abs (ycov)));

  return clamp (coverage, 0.0, 1.0);
}

/* Render a glyph and return its coverage in [0, 1].
 *
 * Requires the hb_gpu_atlas uniform to be bound to the glyph atlas buffer.
 *
 * renderCoord:  em-space sample position (interpolated from vertex shader)
 * glyphLoc:     offset into hb_gpu_atlas for this glyph's encoded blob
 */
float hb_gpu_render (vec2 renderCoord, uint glyphLoc_)
{
  vec2 emsPerPixel = fwidth (renderCoord);
  vec2 pixelsPerEm = 1.0 / emsPerPixel;

  int glyphLoc = int (glyphLoc_);

  /* Read blob header */
  ivec4 header0 = _hb_gpu_fetch (glyphLoc);
  ivec4 header1 = _hb_gpu_fetch (glyphLoc + 1);
  vec4 ext = vec4 (header0) * HB_GPU_INV_UNITS; /* min_x, min_y, max_x, max_y */
  int numHBands = header1.r;
  int numVBands = header1.g;

  /* Compute band transform from extents */
  vec2 extSize = ext.zw - ext.xy; /* (width, height) */
  vec2 bandScale = vec2 (float (numVBands), float (numHBands)) / max (extSize, vec2 (1.0 / 65536.0));
  vec2 bandOffset = -ext.xy * bandScale;

  ivec2 bandIndex = clamp (ivec2 (renderCoord * bandScale + bandOffset),
			   ivec2 (0, 0),
			   ivec2 (numVBands - 1, numHBands - 1));

  /* Skip past header (2 texels) */
  int bandBase = glyphLoc + 2;

  float xcov = 0.0;
  float xwgt = 0.0;

  ivec4 hbandData = _hb_gpu_fetch (bandBase + bandIndex.y);
  int hCurveCount = hbandData.r;
  /* Symmetric: choose rightward (desc) or leftward (asc) sort */
  float hSplit = float (hbandData.a) * HB_GPU_INV_UNITS;
  bool hLeftRay = (renderCoord.x < hSplit);
  int hDataOffset = (hLeftRay ? hbandData.b : hbandData.g) + 32768;

  for (int ci = 0; ci < hCurveCount; ci++)
  {
    int curveOffset = _hb_gpu_fetch (glyphLoc + hDataOffset + ci).r + 32768;

    ivec4 raw12 = _hb_gpu_fetch (glyphLoc + curveOffset);
    ivec4 raw3 = _hb_gpu_fetch (glyphLoc + curveOffset + 1);

    vec4 p12 = vec4 (raw12) * HB_GPU_INV_UNITS - vec4 (renderCoord, renderCoord);
    vec2 p3 = vec2 (raw3.rg) * HB_GPU_INV_UNITS - renderCoord;

    if (hLeftRay) {
      if (min (min (p12.x, p12.z), p3.x) * pixelsPerEm.x > 0.5) break;
    } else {
      if (max (max (p12.x, p12.z), p3.x) * pixelsPerEm.x < -0.5) break;
    }

    uint code = _hb_gpu_calc_root_code (p12.y, p12.w, p3.y);
    if (code != 0U)
    {
      vec2 r = _hb_gpu_solve_horiz_poly (p12, p3) * pixelsPerEm.x;
      /* For leftward ray: saturate(0.5 - r) counts coverage from the left */
      vec2 cov = hLeftRay ? clamp (vec2 (0.5) - r, 0.0, 1.0)
			  : clamp (r + vec2 (0.5), 0.0, 1.0);

      if ((code & 1U) != 0U)
      {
	xcov += cov.x;
	xwgt = max (xwgt, clamp (1.0 - abs (r.x) * 2.0, 0.0, 1.0));
      }

      if (code > 1U)
      {
	xcov -= cov.y;
	xwgt = max (xwgt, clamp (1.0 - abs (r.y) * 2.0, 0.0, 1.0));
      }
    }
  }

  float ycov = 0.0;
  float ywgt = 0.0;

  ivec4 vbandData = _hb_gpu_fetch (bandBase + numHBands + bandIndex.x);
  int vCurveCount = vbandData.r;
  float vSplit = float (vbandData.a) * HB_GPU_INV_UNITS;
  bool vLeftRay = (renderCoord.y < vSplit);
  int vDataOffset = (vLeftRay ? vbandData.b : vbandData.g) + 32768;

  for (int ci = 0; ci < vCurveCount; ci++)
  {
    int curveOffset = _hb_gpu_fetch (glyphLoc + vDataOffset + ci).r + 32768;

    ivec4 raw12 = _hb_gpu_fetch (glyphLoc + curveOffset);
    ivec4 raw3 = _hb_gpu_fetch (glyphLoc + curveOffset + 1);

    vec4 p12 = vec4 (raw12) * HB_GPU_INV_UNITS - vec4 (renderCoord, renderCoord);
    vec2 p3 = vec2 (raw3.rg) * HB_GPU_INV_UNITS - renderCoord;

    if (vLeftRay) {
      if (min (min (p12.y, p12.w), p3.y) * pixelsPerEm.y > 0.5) break;
    } else {
      if (max (max (p12.y, p12.w), p3.y) * pixelsPerEm.y < -0.5) break;
    }

    uint code = _hb_gpu_calc_root_code (p12.x, p12.z, p3.x);
    if (code != 0U)
    {
      vec2 r = _hb_gpu_solve_vert_poly (p12, p3) * pixelsPerEm.y;
      vec2 cov = vLeftRay ? clamp (vec2 (0.5) - r, 0.0, 1.0)
			  : clamp (r + vec2 (0.5), 0.0, 1.0);

      if ((code & 1U) != 0U)
      {
	ycov -= cov.x;
	ywgt = max (ywgt, clamp (1.0 - abs (r.x) * 2.0, 0.0, 1.0));
      }

      if (code > 1U)
      {
	ycov += cov.y;
	ywgt = max (ywgt, clamp (1.0 - abs (r.y) * 2.0, 0.0, 1.0));
      }
    }
  }

  return _hb_gpu_calc_coverage (xcov, ycov, xwgt, ywgt);
}
