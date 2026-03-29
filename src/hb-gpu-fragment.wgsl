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


/* Requires WGSL (WebGPU Shading Language). */


const HB_GPU_UNITS_PER_EM: f32 = 4.0;
const HB_GPU_INV_UNITS: f32 = 1.0 / 4.0;


fn _hb_gpu_fetch (hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>,
                  offset: i32) -> vec4<i32>
{
  return (*hb_gpu_atlas)[offset];
}


fn _hb_gpu_calc_root_code (y1: f32, y2: f32, y3: f32) -> u32
{
  let i1 = bitcast<u32> (y1) >> 31u;
  let i2 = bitcast<u32> (y2) >> 30u;
  let i3 = bitcast<u32> (y3) >> 29u;

  var shift = (i2 & 2u) | (i1 & ~2u);
  shift = (i3 & 4u) | (shift & ~4u);

  return (0x2E74u >> shift) & 0x0101u;
}

fn _hb_gpu_solve_horiz_poly (a: vec2f, b: vec2f, p1: vec2f) -> vec2f
{
  let ra = 1.0 / a.y;
  let rb = 0.5 / b.y;

  let d = sqrt (max (b.y * b.y - a.y * p1.y, 0.0));
  var t1 = (b.y - d) * ra;
  var t2 = (b.y + d) * ra;

  if (a.y == 0.0) {
    t1 = p1.y * rb;
    t2 = t1;
  }

  return vec2f ((a.x * t1 - b.x * 2.0) * t1 + p1.x,
                (a.x * t2 - b.x * 2.0) * t2 + p1.x);
}

fn _hb_gpu_solve_vert_poly (a: vec2f, b: vec2f, p1: vec2f) -> vec2f
{
  let ra = 1.0 / a.x;
  let rb = 0.5 / b.x;

  let d = sqrt (max (b.x * b.x - a.x * p1.x, 0.0));
  var t1 = (b.x - d) * ra;
  var t2 = (b.x + d) * ra;

  if (a.x == 0.0) {
    t1 = p1.x * rb;
    t2 = t1;
  }

  return vec2f ((a.y * t1 - b.y * 2.0) * t1 + p1.y,
                (a.y * t2 - b.y * 2.0) * t2 + p1.y);
}

fn _hb_gpu_calc_coverage (xcov: f32, ycov: f32, xwgt: f32, ywgt: f32) -> f32
{
  let coverage = max (abs (xcov * xwgt + ycov * ywgt) /
                      max (xwgt + ywgt, 1.0 / 65536.0),
                      min (abs (xcov), abs (ycov)));

  return clamp (coverage, 0.0, 1.0);
}

/* Render a glyph and return its coverage in [0, 1].
 *
 * renderCoord:  em-space sample position (interpolated from vertex shader)
 * glyphLoc:     offset into hb_gpu_atlas for this glyph's encoded blob
 * hb_gpu_atlas: storage buffer with the glyph atlas data
 * emsPerPixel:  screen-space derivatives of renderCoord (fwidth equivalent)
 */
fn hb_gpu_render (renderCoord: vec2f, glyphLoc_: u32,
                  hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>,
                  emsPerPixel: vec2f) -> f32
{
  let pixelsPerEm = 1.0 / emsPerPixel;

  let glyphLoc = i32 (glyphLoc_);

  /* Read blob header */
  let header0 = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc);
  let header1 = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc + 1);
  let ext = vec4f (header0) * HB_GPU_INV_UNITS; /* min_x, min_y, max_x, max_y */
  let numHBands = header1.r;
  let numVBands = header1.g;

  /* Compute band transform from extents */
  let extSize = ext.zw - ext.xy; /* (width, height) */
  let bandScale = vec2f (f32 (numVBands), f32 (numHBands)) / max (extSize, vec2f (1.0 / 65536.0));
  let bandOffset = -ext.xy * bandScale;

  let bandIndex = clamp (vec2<i32> (renderCoord * bandScale + bandOffset),
                         vec2<i32> (0, 0),
                         vec2<i32> (numVBands - 1, numHBands - 1));

  /* Skip past header (2 texels) */
  let bandBase = glyphLoc + 2;

  var xcov: f32 = 0.0;
  var xwgt: f32 = 0.0;

  let hbandData = _hb_gpu_fetch (hb_gpu_atlas, bandBase + bandIndex.y);
  let hCurveCount = hbandData.r;
  /* Symmetric: choose rightward (desc) or leftward (asc) sort */
  let hSplit = f32 (hbandData.a) * HB_GPU_INV_UNITS;
  let hLeftRay = (renderCoord.x < hSplit);
  var hDataOffset: i32;
  if (hLeftRay) { hDataOffset = hbandData.b + 32768; }
  else          { hDataOffset = hbandData.g + 32768; }

  for (var ci: i32 = 0; ci < hCurveCount; ci++)
  {
    let curveOffset = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc + hDataOffset + ci).r + 32768;

    let raw12 = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc + curveOffset);
    let raw3 = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc + curveOffset + 1);

    let q12 = vec4f (raw12) * HB_GPU_INV_UNITS;
    let q3 = vec2f (vec2<i32> (raw3.r, raw3.g)) * HB_GPU_INV_UNITS;

    let p12 = q12 - vec4f (renderCoord, renderCoord);
    let p3 = q3 - renderCoord;

    if (hLeftRay) {
      if (min (min (p12.x, p12.z), p3.x) * pixelsPerEm.x > 0.5) { break; }
    } else {
      if (max (max (p12.x, p12.z), p3.x) * pixelsPerEm.x < -0.5) { break; }
    }

    let code = _hb_gpu_calc_root_code (p12.y, p12.w, p3.y);
    if (code != 0u)
    {
      let a = q12.xy - q12.zw * 2.0 + q3;
      let b = q12.xy - q12.zw;
      let r = _hb_gpu_solve_horiz_poly (a, b, p12.xy) * pixelsPerEm.x;
      /* For leftward ray: saturate(0.5 - r) counts coverage from the left */
      var cov: vec2f;
      if (hLeftRay) { cov = clamp (vec2f (0.5) - r, vec2f (0.0), vec2f (1.0)); }
      else          { cov = clamp (r + vec2f (0.5), vec2f (0.0), vec2f (1.0)); }

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

  var ycov: f32 = 0.0;
  var ywgt: f32 = 0.0;

  let vbandData = _hb_gpu_fetch (hb_gpu_atlas, bandBase + numHBands + bandIndex.x);
  let vCurveCount = vbandData.r;
  let vSplit = f32 (vbandData.a) * HB_GPU_INV_UNITS;
  let vLeftRay = (renderCoord.y < vSplit);
  var vDataOffset: i32;
  if (vLeftRay) { vDataOffset = vbandData.b + 32768; }
  else          { vDataOffset = vbandData.g + 32768; }

  for (var ci: i32 = 0; ci < vCurveCount; ci++)
  {
    let curveOffset = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc + vDataOffset + ci).r + 32768;

    let raw12 = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc + curveOffset);
    let raw3 = _hb_gpu_fetch (hb_gpu_atlas, glyphLoc + curveOffset + 1);

    let q12 = vec4f (raw12) * HB_GPU_INV_UNITS;
    let q3 = vec2f (vec2<i32> (raw3.r, raw3.g)) * HB_GPU_INV_UNITS;

    let p12 = q12 - vec4f (renderCoord, renderCoord);
    let p3 = q3 - renderCoord;

    if (vLeftRay) {
      if (min (min (p12.y, p12.w), p3.y) * pixelsPerEm.y > 0.5) { break; }
    } else {
      if (max (max (p12.y, p12.w), p3.y) * pixelsPerEm.y < -0.5) { break; }
    }

    let code = _hb_gpu_calc_root_code (p12.x, p12.z, p3.x);
    if (code != 0u)
    {
      let a = q12.xy - q12.zw * 2.0 + q3;
      let b = q12.xy - q12.zw;
      let r = _hb_gpu_solve_vert_poly (a, b, p12.xy) * pixelsPerEm.y;
      var cov: vec2f;
      if (vLeftRay) { cov = clamp (vec2f (0.5) - r, vec2f (0.0), vec2f (1.0)); }
      else          { cov = clamp (r + vec2f (0.5), vec2f (0.0), vec2f (1.0)); }

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
