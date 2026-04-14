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
    let flags   = op.g;
    let payload = (op.b << 16) | (op.a & 0xffff);

    if (op_type == 0) {  // LAYER_SOLID
      // texel 1 holds RGBA as signed Q15.
      let ct = hb_gpu_fetch (hb_gpu_atlas, cursor + 1);
      var col: vec4f;
      if ((flags & 1) != 0) {
        col = foreground;
      } else {
        col = vec4f (ct) / 32767.0;
      }

      let cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
                                   u32 (base + payload), hb_gpu_atlas);
      let src = col * cov;
      /* SRC_OVER with premultiplied source. */
      acc = src + acc * (1.0 - src.a);

      cursor = cursor + 2;
    } else {
      /* LAYER_GRADIENT, PUSH_GROUP, POP_GROUP not yet handled. */
      break;
    }
  }

  return acc;
}
