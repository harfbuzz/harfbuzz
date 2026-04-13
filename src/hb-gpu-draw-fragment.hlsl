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


/* Requires Shader Model 5.0+.
 *
 * The caller must declare:
 *   StructuredBuffer<int4> hb_gpu_atlas : register(t0);
 */


#ifndef HB_GPU_UNITS_PER_EM
#define HB_GPU_UNITS_PER_EM 4
#endif

#define HB_GPU_INV_UNITS (1.0 / (float) HB_GPU_UNITS_PER_EM)


int4 hb_gpu_fetch (int offset)
{
  return hb_gpu_atlas[offset];
}


uint _hb_gpu_calc_root_code (float y1, float y2, float y3)
{
  uint i1 = asuint (y1) >> 31u;
  uint i2 = asuint (y2) >> 30u;
  uint i3 = asuint (y3) >> 29u;

  uint shift = (i2 & 2u) | (i1 & ~2u);
  shift = (i3 & 4u) | (shift & ~4u);

  return (0x2E74u >> shift) & 0x0101u;
}

float2 _hb_gpu_solve_horiz_poly (float2 a, float2 b, float2 p1)
{
  float ra = 1.0 / a.y;
  float rb = 0.5 / b.y;

  float d = sqrt (max (b.y * b.y - a.y * p1.y, 0.0));
  float t1 = (b.y - d) * ra;
  float t2 = (b.y + d) * ra;

  if (a.y == 0.0)
    t1 = t2 = p1.y * rb;

  return float2 ((a.x * t1 - b.x * 2.0) * t1 + p1.x,
                 (a.x * t2 - b.x * 2.0) * t2 + p1.x);
}

float2 _hb_gpu_solve_vert_poly (float2 a, float2 b, float2 p1)
{
  float ra = 1.0 / a.x;
  float rb = 0.5 / b.x;

  float d = sqrt (max (b.x * b.x - a.x * p1.x, 0.0));
  float t1 = (b.x - d) * ra;
  float t2 = (b.x + d) * ra;

  if (a.x == 0.0)
    t1 = t2 = p1.x * rb;

  return float2 ((a.y * t1 - b.y * 2.0) * t1 + p1.y,
                 (a.y * t2 - b.y * 2.0) * t2 + p1.y);
}

float _hb_gpu_calc_coverage (float xcov, float ycov, float xwgt, float ywgt)
{
  float coverage = max (abs (xcov * xwgt + ycov * ywgt) /
                        max (xwgt + ywgt, 1.0 / 65536.0),
                        min (abs (xcov), abs (ycov)));

  return clamp (coverage, 0.0, 1.0);
}


struct _hb_gpu_glyph_info
{
  int glyphLoc;
  int bandBase;
  int2 bandIndex;
  int numHBands;
  int numVBands;
  float2 scale;
};

_hb_gpu_glyph_info _hb_gpu_decode_glyph (float2 renderCoord, uint glyphLoc_)
{
  _hb_gpu_glyph_info gi;
  gi.glyphLoc = (int) glyphLoc_;

  int4 header0 = hb_gpu_fetch (gi.glyphLoc);
  int4 header1 = hb_gpu_fetch (gi.glyphLoc + 1);
  float4 ext = (float4) header0 * HB_GPU_INV_UNITS;
  gi.numHBands = header1.r;
  gi.numVBands = header1.g;
  gi.scale = float2 ((float) header1.b, (float) header1.a);

  float2 extSize = ext.zw - ext.xy;
  float2 bandScale = float2 ((float) gi.numVBands, (float) gi.numHBands) / max (extSize, float2 (1.0 / 65536.0, 1.0 / 65536.0));
  float2 bandOffset = -ext.xy * bandScale;

  gi.bandIndex = clamp ((int2) (renderCoord * bandScale + bandOffset),
                        int2 (0, 0),
                        int2 (gi.numVBands - 1, gi.numHBands - 1));

  gi.bandBase = gi.glyphLoc + 2;
  return gi;
}

/* Return pixels per em at this fragment.
 *
 * renderCoord:  em-space sample position
 * glyphLoc:     texel offset of glyph blob in atlas
 */
float hb_gpu_ppem (float2 renderCoord, uint glyphLoc_)
{
  _hb_gpu_glyph_info gi = _hb_gpu_decode_glyph (renderCoord, glyphLoc_);
  float2 emsPerPixel = fwidth (renderCoord);
  return min (gi.scale.x, gi.scale.y) /
	 max (emsPerPixel.x, emsPerPixel.y);
}

int2 _hb_gpu_curve_counts (float2 renderCoord, uint glyphLoc_)
{
  _hb_gpu_glyph_info gi = _hb_gpu_decode_glyph (renderCoord, glyphLoc_);
  int hCount = hb_gpu_fetch (gi.bandBase + gi.bandIndex.y).r;
  int vCount = hb_gpu_fetch (gi.bandBase + gi.numHBands + gi.bandIndex.x).r;
  return int2 (hCount, vCount);
}


/* Single-sample coverage in [0, 1]. */
float _hb_gpu_draw_single (float2 renderCoord, float2 pixelsPerEm, uint glyphLoc_)
{

  _hb_gpu_glyph_info gi = _hb_gpu_decode_glyph (renderCoord, glyphLoc_);
  int glyphLoc = gi.glyphLoc;
  int bandBase = gi.bandBase;
  int numHBands = gi.numHBands;

  float xcov = 0.0;
  float xwgt = 0.0;

  int4 hbandData = hb_gpu_fetch (bandBase + gi.bandIndex.y);
  int hCurveCount = hbandData.r;
  float hSplit = (float) hbandData.a * HB_GPU_INV_UNITS;
  bool hLeftRay = (renderCoord.x < hSplit);
  int hDataOffset = (hLeftRay ? hbandData.b : hbandData.g) + 32768;

  for (int ci = 0; ci < hCurveCount; ci++)
  {
    int curveOffset = hb_gpu_fetch (glyphLoc + hDataOffset + ci).r + 32768;

    int4 raw12 = hb_gpu_fetch (glyphLoc + curveOffset);
    int4 raw3 = hb_gpu_fetch (glyphLoc + curveOffset + 1);

    float4 q12 = (float4) raw12 * HB_GPU_INV_UNITS;
    float2 q3 = (float2) raw3.rg * HB_GPU_INV_UNITS;

    float4 p12 = q12 - float4 (renderCoord, renderCoord);
    float2 p3 = q3 - renderCoord;

    if (hLeftRay) {
      if (min (min (p12.x, p12.z), p3.x) * pixelsPerEm.x > 0.5) break;
    } else {
      if (max (max (p12.x, p12.z), p3.x) * pixelsPerEm.x < -0.5) break;
    }

    uint code = _hb_gpu_calc_root_code (p12.y, p12.w, p3.y);
    if (code != 0u)
    {
      float2 a = q12.xy - q12.zw * 2.0 + q3;
      float2 b = q12.xy - q12.zw;
      float2 r = _hb_gpu_solve_horiz_poly (a, b, p12.xy) * pixelsPerEm.x;
      float2 cov = hLeftRay ? clamp (float2 (0.5, 0.5) - r, 0.0, 1.0)
                            : clamp (r + float2 (0.5, 0.5), 0.0, 1.0);

      if ((code & 1u) != 0u)
      {
        xcov += cov.x;
        xwgt = max (xwgt, clamp (1.0 - abs (r.x) * 2.0, 0.0, 1.0));
      }

      if (code > 1u)
      {
        xcov -= cov.y;
        xwgt = max (xwgt, clamp (1.0 - abs (r.y) * 2.0, 0.0, 1.0));
      }
    }
  }

  float ycov = 0.0;
  float ywgt = 0.0;

  int4 vbandData = hb_gpu_fetch (bandBase + numHBands + gi.bandIndex.x);
  int vCurveCount = vbandData.r;
  float vSplit = (float) vbandData.a * HB_GPU_INV_UNITS;
  bool vLeftRay = (renderCoord.y < vSplit);
  int vDataOffset = (vLeftRay ? vbandData.b : vbandData.g) + 32768;

  for (int ci = 0; ci < vCurveCount; ci++)
  {
    int curveOffset = hb_gpu_fetch (glyphLoc + vDataOffset + ci).r + 32768;

    int4 raw12 = hb_gpu_fetch (glyphLoc + curveOffset);
    int4 raw3 = hb_gpu_fetch (glyphLoc + curveOffset + 1);

    float4 q12 = (float4) raw12 * HB_GPU_INV_UNITS;
    float2 q3 = (float2) raw3.rg * HB_GPU_INV_UNITS;

    float4 p12 = q12 - float4 (renderCoord, renderCoord);
    float2 p3 = q3 - renderCoord;

    if (vLeftRay) {
      if (min (min (p12.y, p12.w), p3.y) * pixelsPerEm.y > 0.5) break;
    } else {
      if (max (max (p12.y, p12.w), p3.y) * pixelsPerEm.y < -0.5) break;
    }

    uint code = _hb_gpu_calc_root_code (p12.x, p12.z, p3.x);
    if (code != 0u)
    {
      float2 a = q12.xy - q12.zw * 2.0 + q3;
      float2 b = q12.xy - q12.zw;
      float2 r = _hb_gpu_solve_vert_poly (a, b, p12.xy) * pixelsPerEm.y;
      float2 cov = vLeftRay ? clamp (float2 (0.5, 0.5) - r, 0.0, 1.0)
                            : clamp (r + float2 (0.5, 0.5), 0.0, 1.0);

      if ((code & 1u) != 0u)
      {
        ycov -= cov.x;
        ywgt = max (ywgt, clamp (1.0 - abs (r.x) * 2.0, 0.0, 1.0));
      }

      if (code > 1u)
      {
        ycov += cov.y;
        ywgt = max (ywgt, clamp (1.0 - abs (r.y) * 2.0, 0.0, 1.0));
      }
    }
  }

  return _hb_gpu_calc_coverage (xcov, ycov, xwgt, ywgt);
}

/* Return coverage in [0, 1].
 *
 * Caller must declare: StructuredBuffer<int4> hb_gpu_atlas
 *
 * renderCoord:  em-space sample position
 * glyphLoc:     texel offset of glyph blob in atlas
 */
float hb_gpu_draw (float2 renderCoord, uint glyphLoc_)
{
  float2 emsPerPixel = fwidth (renderCoord);
  float2 pixelsPerEm = 1.0 / emsPerPixel;

  float c = _hb_gpu_draw_single (renderCoord, pixelsPerEm, glyphLoc_);

#ifndef HB_GPU_NO_MSAA
  float ppem = hb_gpu_ppem (renderCoord, glyphLoc_);

  if (ppem < 16.0)
  {
    float2 d = emsPerPixel * (1.0 / 3.0);
    float msaa = 0.25 *
      (_hb_gpu_draw_single (renderCoord + float2 (-d.x, -d.y), pixelsPerEm, glyphLoc_) +
       _hb_gpu_draw_single (renderCoord + float2 ( d.x, -d.y), pixelsPerEm, glyphLoc_) +
       _hb_gpu_draw_single (renderCoord + float2 (-d.x,  d.y), pixelsPerEm, glyphLoc_) +
       _hb_gpu_draw_single (renderCoord + float2 ( d.x,  d.y), pixelsPerEm, glyphLoc_));

    c = lerp (c, msaa, smoothstep (16.0, 8.0, ppem));
  }
#endif

  return c;
}

/* Stem darkening for small sizes.
 *
 * coverage:    output of hb_gpu_draw
 * brightness:  foreground brightness in [0, 1]
 * ppem:        pixels per em at this fragment
 */
float hb_gpu_stem_darken (float coverage, float brightness, float ppem)
{
  return pow (coverage,
	      lerp (pow (2.0, brightness - 0.5), 1.0,
		    smoothstep (8.0, 48.0, ppem)));
}
