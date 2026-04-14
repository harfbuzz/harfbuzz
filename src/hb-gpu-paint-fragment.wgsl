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
 */


/* Paint-renderer fragment shader (WGSL).
 *
 * Assumes the shared fragment helpers (hb-gpu-fragment.wgsl) and
 * the draw-renderer fragment helpers (hb-gpu-draw-fragment.wgsl)
 * are prepended to this source.
 *
 * atlas is passed as an explicit storage-buffer pointer parameter,
 * matching WGSL's scoping rules.
 */


fn _hb_gpu_stop_color (hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>,
                       stops_base: i32, i: i32, foreground: vec4f,
                       offset: ptr<function, f32>) -> vec4f
{
  let a = hb_gpu_fetch (hb_gpu_atlas, stops_base + i * 2);
  *offset = f32 (a.r) / 32767.0;
  if ((a.g & 1) != 0) {
    return foreground;
  }
  let b = hb_gpu_fetch (hb_gpu_atlas, stops_base + i * 2 + 1);
  return vec4f (b) / 32767.0;
}

fn _hb_gpu_extend_t (t: f32, extend: i32) -> f32
{
  if (extend == 1) { return t - floor (t); }
  if (extend == 2) {
    let u = t - 2.0 * floor (t * 0.5);
    if (u > 1.0) { return 2.0 - u; }
    return u;
  }
  return clamp (t, 0.0, 1.0);
}

fn _hb_gpu_eval_stops (hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>,
                       stops_base: i32, stop_count: i32,
                       t: f32, foreground: vec4f) -> vec4f
{
  var off_prev: f32;
  var col_prev = _hb_gpu_stop_color (hb_gpu_atlas, stops_base, 0, foreground, &off_prev);
  if (t <= off_prev) { return col_prev; }
  for (var i: i32 = 1; i < stop_count; i = i + 1)
  {
    var off: f32;
    let col = _hb_gpu_stop_color (hb_gpu_atlas, stops_base, i, foreground, &off);
    if (t <= off)
    {
      let span = off - off_prev;
      var f: f32 = 0.0;
      if (span > 1e-6) { f = (t - off_prev) / span; }
      return mix (col_prev, col, f);
    }
    col_prev = col;
    off_prev = off;
  }
  return col_prev;
}

fn _hb_gpu_sample_linear (renderCoord: vec2f, grad_base: i32,
                          stop_count: i32, extend: i32, foreground: vec4f,
                          hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> vec4f
{
  let axis = hb_gpu_fetch (hb_gpu_atlas, grad_base);
  let p0 = vec2f (f32 (axis.r), f32 (axis.g));
  let p1 = vec2f (f32 (axis.b), f32 (axis.a));
  let d = p1 - p0;
  let denom = dot (d, d);
  if (denom < 1e-6) { return vec4f (0.0); }
  var t = dot (renderCoord - p0, d) / denom;
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (hb_gpu_atlas, grad_base + 1, stop_count, t, foreground);
}

fn _hb_gpu_sample_radial (renderCoord: vec2f, grad_base: i32,
                          stop_count: i32, extend: i32, foreground: vec4f,
                          hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> vec4f
{
  let c0_ = hb_gpu_fetch (hb_gpu_atlas, grad_base);
  let c1_ = hb_gpu_fetch (hb_gpu_atlas, grad_base + 1);
  let c0 = vec2f (f32 (c0_.r), f32 (c0_.g));
  let r0 = f32 (c0_.b);
  let c1 = vec2f (f32 (c1_.r), f32 (c1_.g));
  let r1 = f32 (c1_.b);

  let cd = c1 - c0;
  let dr = r1 - r0;
  let p  = renderCoord - c0;

  let A = dot (cd, cd) - dr * dr;
  let B = -2.0 * (dot (p, cd) + r0 * dr);
  let C = dot (p, p) - r0 * r0;

  var t: f32;
  if (abs (A) > 1e-6)
  {
    let disc = B * B - 4.0 * A * C;
    if (disc < 0.0) { return vec4f (0.0); }
    let sq = sqrt (disc);
    let t1 = (-B + sq) / (2.0 * A);
    let t2 = (-B - sq) / (2.0 * A);
    if (r0 + t1 * dr >= 0.0) { t = t1; } else { t = t2; }
  }
  else
  {
    if (abs (B) < 1e-6) { return vec4f (0.0); }
    t = -C / B;
  }
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (hb_gpu_atlas, grad_base + 2, stop_count, t, foreground);
}

fn hb_gpu_paint (renderCoord: vec2f, glyphLoc_: u32, foreground: vec4f,
                 hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> vec4f
{
  /* Compute pixelsPerEm once here at uniform control flow.  WGSL
   * rejects fwidth inside a loop-conditional branch, so we call
   * _hb_gpu_draw_impl (the MSAA-aware implementation that takes a
   * pre-computed pixelsPerEm) instead of the top-level
   * hb_gpu_draw() which would re-call fwidth. */
  let emsPerPixel = fwidth (renderCoord);
  let pixelsPerEm = 1.0 / emsPerPixel;

  let base    = i32 (glyphLoc_);
  let h0      = hb_gpu_fetch (hb_gpu_atlas, base);     // (num_ops, _, _, _)
  let h2      = hb_gpu_fetch (hb_gpu_atlas, base + 2); // (ops_offset, _, _, _)
  let num_ops = h0.r;
  var cursor  = base + h2.r;

  var acc = vec4f (0.0);

  for (var i: i32 = 0; i < num_ops; i = i + 1)
  {
    let op      = hb_gpu_fetch (hb_gpu_atlas, cursor);
    let op_type = op.r;
    let aux     = op.g;
    let payload = (op.b << 16) | (op.a & 0xffff);

    if (op_type == 0) {  // LAYER_SOLID
      // texel 1 holds RGBA as signed Q15.
      let ct = hb_gpu_fetch (hb_gpu_atlas, cursor + 1);
      var col: vec4f;
      if ((aux & 1) != 0) {
        col = foreground;
      } else {
        col = vec4f (ct) / 32767.0;
      }

      let cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
                                   u32 (base + payload), hb_gpu_atlas);
      let src = vec4f (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor = cursor + 2;
    } else if (op_type == 1) {  // LAYER_GRADIENT
      let op2 = hb_gpu_fetch (hb_gpu_atlas, cursor + 1);
      let grad_payload = (op2.r << 16) | (op2.g & 0xffff);
      let extend = op2.b;
      let stop_count = op2.a;

      var col = vec4f (0.0);
      if (aux == 0) {
        col = _hb_gpu_sample_linear (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground,
                                     hb_gpu_atlas);
      } else if (aux == 1) {
        col = _hb_gpu_sample_radial (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground,
                                     hb_gpu_atlas);
      }

      let cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
                                   u32 (base + payload), hb_gpu_atlas);
      let src = vec4f (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor = cursor + 2;
    } else {
      /* PUSH_GROUP, POP_GROUP not yet handled. */
      break;
    }
  }

  return acc;
}
