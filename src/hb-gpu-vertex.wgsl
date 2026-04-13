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


/* Requires WGSL (WebGPU Shading Language). */


/* Dilate a glyph vertex by half a pixel on screen.
 *
 * position:  object-space vertex position (modified in place)
 * texcoord:  em-space sample coordinates (modified in place)
 * normal:    object-space outward normal at this vertex
 * jac:       inverse of the 2x2 linear part of the em-to-object transform,
 *            stored row-major as (j00, j01, j10, j11).  Maps object-space
 *            displacements back to em-space for texcoord adjustment.
 *            For simple scaling with y-flip (the common case):
 *              em-to-object = [[s, 0], [0, -s]]
 *              jac = (1/s, 0, 0, -1/s)
 * m:         model-view-projection matrix
 * viewport:  viewport size in pixels
 *
 * Returns (new_position, new_texcoord).
 */
fn hb_gpu_dilate (position: vec2f, texcoord: vec2f,
                  normal: vec2f, jac: vec4f,
                  m: mat4x4f, viewport: vec2f) -> array<vec2f, 2>
{
  let n = normalize (normal);

  let clipPos = m * vec4f (position, 0.0, 1.0);
  let clipN   = m * vec4f (n, 0.0, 0.0);

  let s = clipPos.w;
  let t = clipN.w;

  let u = (s * clipN.x - t * clipPos.x) * viewport.x;
  let v = (s * clipN.y - t * clipPos.y) * viewport.y;

  let s2 = s * s;
  let st = s * t;
  let uv = u * u + v * v;

  let denom = uv - st * st;
  var d: f32;
  if (abs (denom) > 1.0 / 16777216.0) {
    d = s2 * (st + sqrt (uv)) / denom;
  } else {
    d = 0.0;
  }

  let dPos = d * normal;
  return array<vec2f, 2> (
    position + dPos,
    texcoord + vec2f (dot (dPos, jac.xy), dot (dPos, jac.zw))
  );
}
