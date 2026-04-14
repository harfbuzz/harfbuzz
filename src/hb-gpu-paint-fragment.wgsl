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

/* Apply the stored 2x2 M^-1 (row-major i16 Q10) to a vector. */
fn _hb_gpu_apply_minv (m: vec4<i32>, v: vec2f) -> vec2f
{
  let mf = vec4f (m) * (1.0 / 1024.0);
  return vec2f (mf.x * v.x + mf.y * v.y,
                mf.z * v.x + mf.w * v.y);
}

fn _hb_gpu_sample_linear (renderCoord: vec2f, grad_base: i32,
                          stop_count: i32, extend: i32, foreground: vec4f,
                          hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> vec4f
{
  let t0 = hb_gpu_fetch (hb_gpu_atlas, grad_base);
  let m  = hb_gpu_fetch (hb_gpu_atlas, grad_base + 1);
  let p0_r = vec2f (f32 (t0.r), f32 (t0.g));
  let d    = vec2f (f32 (t0.b), f32 (t0.a));
  let denom = dot (d, d);
  if (denom < 1e-6) { return vec4f (0.0); }
  let p = _hb_gpu_apply_minv (m, renderCoord - p0_r);
  var t = dot (p, d) / denom;
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (hb_gpu_atlas, grad_base + 2, stop_count, t, foreground);
}

fn _hb_gpu_sample_radial (renderCoord: vec2f, grad_base: i32,
                          stop_count: i32, extend: i32, foreground: vec4f,
                          hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> vec4f
{
  let t0 = hb_gpu_fetch (hb_gpu_atlas, grad_base);
  let t1 = hb_gpu_fetch (hb_gpu_atlas, grad_base + 1);
  let m  = hb_gpu_fetch (hb_gpu_atlas, grad_base + 2);
  let c0_r = vec2f (f32 (t0.r), f32 (t0.g));
  let cd   = vec2f (f32 (t0.b), f32 (t0.a));
  let r0 = f32 (t1.r);
  let r1 = f32 (t1.g);

  let dr = r1 - r0;
  let p  = _hb_gpu_apply_minv (m, renderCoord - c0_r);

  let A = dot (cd, cd) - dr * dr;
  let B = -2.0 * (dot (p, cd) + r0 * dr);
  let C = dot (p, p) - r0 * r0;

  var t: f32;
  if (abs (A) > 1e-6)
  {
    let disc = B * B - 4.0 * A * C;
    if (disc < 0.0) { return vec4f (0.0); }
    let sq = sqrt (disc);
    let t1r = (-B + sq) / (2.0 * A);
    let t2r = (-B - sq) / (2.0 * A);
    if (r0 + t1r * dr >= 0.0) { t = t1r; } else { t = t2r; }
  }
  else
  {
    if (abs (B) < 1e-6) { return vec4f (0.0); }
    t = -C / B;
  }
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (hb_gpu_atlas, grad_base + 3, stop_count, t, foreground);
}

fn _hb_gpu_sample_sweep (renderCoord: vec2f, grad_base: i32,
                         stop_count: i32, extend: i32, foreground: vec4f,
                         hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> vec4f
{
  let t0 = hb_gpu_fetch (hb_gpu_atlas, grad_base);
  let m  = hb_gpu_fetch (hb_gpu_atlas, grad_base + 1);
  let c_r = vec2f (f32 (t0.r), f32 (t0.g));
  let a0 = f32 (t0.b) / 16384.0;
  let a1 = f32 (t0.a) / 16384.0;
  let span = a1 - a0;
  if (abs (span) < 1e-6) { return vec4f (0.0); }

  let p = _hb_gpu_apply_minv (m, renderCoord - c_r);
  var ang = atan2 (p.y, p.x) / 3.14159265358979;
  if (ang < 0.0) { ang = ang + 2.0; }
  var t = (ang - a0) / span;
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (hb_gpu_atlas, grad_base + 2, stop_count, t, foreground);
}

fn _hb_gpu_composite (src: vec4f, dst: vec4f, mode: i32) -> vec4f
{
  var r = src + dst * (1.0 - src.a);

  if      (mode == 0)  { r = vec4f (0.0); }
  else if (mode == 1)  { r = src; }
  else if (mode == 2)  { r = dst; }
  else if (mode == 4)  { r = dst + src * (1.0 - dst.a); }
  else if (mode == 5)  { r = src * dst.a; }
  else if (mode == 6)  { r = dst * src.a; }
  else if (mode == 7)  { r = src * (1.0 - dst.a); }
  else if (mode == 8)  { r = dst * (1.0 - src.a); }
  else if (mode == 9)  { r = src * dst.a + dst * (1.0 - src.a); }
  else if (mode == 10) { r = dst * src.a + src * (1.0 - dst.a); }
  else if (mode == 11) { r = src * (1.0 - dst.a) + dst * (1.0 - src.a); }
  else if (mode == 12) { r = min (src + dst, vec4f (1.0)); }
  else if (mode == 13) {
    r = vec4f (src.rgb + dst.rgb - src.rgb * dst.rgb,
               src.a + dst.a - src.a * dst.a);
  }
  else if (mode == 15) {
    r = vec4f (min (src.rgb * dst.a, dst.rgb * src.a)
             + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a),
               src.a + dst.a - src.a * dst.a);
  }
  else if (mode == 16) {
    r = vec4f (max (src.rgb * dst.a, dst.rgb * src.a)
             + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a),
               src.a + dst.a - src.a * dst.a);
  }
  else if (mode == 23) {
    r = vec4f (src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a)
             + src.rgb * dst.rgb,
               src.a + dst.a - src.a * dst.a);
  }
  return r;
}

/* Wrap _hb_gpu_slug with a sub-glyph extents bail-out.  Many
 * paint layers cover a small region of the outer glyph quad; for
 * fragments outside the layer's bbox (with an AA + MSAA-spread
 * margin) the slug coverage is exactly 0, so we can skip the
 * band/curve walk entirely. */
fn _hb_gpu_slug_clipped (renderCoord: vec2f, pixelsPerEm: vec2f, glyphLoc_: u32,
                         hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> f32
{
  let header0 = hb_gpu_fetch (hb_gpu_atlas, i32 (glyphLoc_));
  let ext = vec4f (header0) * HB_GPU_INV_UNITS;
  let margin = 2.0 / pixelsPerEm;
  if (any (renderCoord < ext.xy - margin) ||
      any (renderCoord > ext.zw + margin)) {
    return 0.0;
  }
  return _hb_gpu_slug (renderCoord, pixelsPerEm, glyphLoc_, hb_gpu_atlas);
}

const HB_GPU_PAINT_GROUP_DEPTH: i32 = 4;

fn hb_gpu_paint (renderCoord: vec2f, glyphLoc_: u32, foreground: vec4f,
                 hb_gpu_atlas: ptr<storage, array<vec4<i32>>, read>) -> vec4f
{
  /* Compute pixelsPerEm once here at uniform control flow.  WGSL
   * rejects fwidth inside a loop-conditional branch, so we call
   * _hb_gpu_slug (the MSAA-aware implementation that takes a
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
  var group_stack: array<vec4f, HB_GPU_PAINT_GROUP_DEPTH>;
  var sp: i32 = 0;

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

      let cov = _hb_gpu_slug_clipped (renderCoord, pixelsPerEm,
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
      } else if (aux == 2) {
        col = _hb_gpu_sample_sweep  (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground,
                                     hb_gpu_atlas);
      }

      let cov = _hb_gpu_slug_clipped (renderCoord, pixelsPerEm,
                                           u32 (base + payload), hb_gpu_atlas);
      let src = vec4f (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor = cursor + 2;
    } else if (op_type == 2) {  // PUSH_GROUP
      if (sp < HB_GPU_PAINT_GROUP_DEPTH) {
        group_stack[sp] = acc;
        sp = sp + 1;
      }
      acc = vec4f (0.0);
      cursor = cursor + 1;
    } else if (op_type == 3) {  // POP_GROUP
      if (sp > 0) {
        sp = sp - 1;
        let src = acc;
        let dst = group_stack[sp];
        acc = _hb_gpu_composite (src, dst, aux);
      }
      cursor = cursor + 1;
    } else {
      break;
    }
  }

  return acc;
}
