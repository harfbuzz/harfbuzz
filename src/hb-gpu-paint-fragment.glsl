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


/* Paint-renderer fragment shader.
 *
 * Assumes the shared fragment helpers (hb-gpu-fragment.glsl) and
 * the draw-renderer fragment helpers (hb-gpu-draw-fragment.glsl)
 * are prepended to this source.  The draw helper provides
 * hb_gpu_draw() which this interpreter calls to compute clip-glyph
 * coverage.
 */


/* Fetch the i'th stop of a gradient color line starting at @stops_base
 * (2 texels per stop).  Resolves is_foreground to @foreground. */
vec4 _hb_gpu_stop_color (int stops_base, int i, vec4 foreground, out float offset)
{
  ivec4 a = hb_gpu_fetch (stops_base + i * 2);
  offset = float (a.r) / 32767.0;
  if ((a.g & 1) != 0)
    return foreground;
  ivec4 b = hb_gpu_fetch (stops_base + i * 2 + 1);
  return vec4 (b) / 32767.0;
}

/* Apply the color-line extend mode to a projected `t` value. */
float _hb_gpu_extend_t (float t, int extend)
{
  if (extend == 1) {           /* HB_PAINT_EXTEND_REPEAT */
    return t - floor (t);
  } else if (extend == 2) {    /* HB_PAINT_EXTEND_REFLECT */
    float u = t - 2.0 * floor (t * 0.5);
    return u > 1.0 ? 2.0 - u : u;
  }
  return clamp (t, 0.0, 1.0);  /* PAD (default) */
}

/* Sample a linear gradient whose param blob starts at @grad_base:
 *   texel 0: (x0, y0, x1, y1) in font units (i16)
 *   texels 1..: stops (2 texels each) */
vec4 _hb_gpu_sample_linear (vec2 renderCoord, int grad_base,
			    int stop_count, int extend, vec4 foreground)
{
  ivec4 axis = hb_gpu_fetch (grad_base);
  vec2 p0 = vec2 (float (axis.r), float (axis.g));
  vec2 p1 = vec2 (float (axis.b), float (axis.a));
  vec2 d = p1 - p0;
  float denom = dot (d, d);
  if (denom < 1e-6) return vec4 (0.0);
  float t = dot (renderCoord - p0, d) / denom;
  t = _hb_gpu_extend_t (t, extend);

  int stops_base = grad_base + 1;
  float off_prev;
  vec4 col_prev = _hb_gpu_stop_color (stops_base, 0, foreground, off_prev);
  if (t <= off_prev)
    return col_prev;
  for (int i = 1; i < stop_count; i++)
  {
    float off;
    vec4 col = _hb_gpu_stop_color (stops_base, i, foreground, off);
    if (t <= off)
    {
      float span = off - off_prev;
      float f = span > 1e-6 ? (t - off_prev) / span : 0.0;
      return mix (col_prev, col, f);
    }
    col_prev = col;
    off_prev = off;
  }
  return col_prev;
}

/* Walks the paint blob's flat op stream and returns a
 * premultiplied RGBA coverage value for the current fragment.
 *
 * glyphLoc: atlas texel offset of the paint-blob header.
 * foreground: caller-supplied foreground color, used when an op
 *             sets the is_foreground flag.
 */
vec4 hb_gpu_paint (vec2 renderCoord, uint glyphLoc, vec4 foreground)
{
  /* fwidth once, at uniform control flow: every per-layer
   * coverage sample below uses this pre-computed pixelsPerEm via
   * _hb_gpu_draw_impl. */
  vec2 pixelsPerEm = 1.0 / fwidth (renderCoord);

  int base    = int (glyphLoc);
  ivec4 h0    = hb_gpu_fetch (base);      /* (num_ops, _, _, _) */
  ivec4 h2    = hb_gpu_fetch (base + 2);  /* (ops_offset, _, _, _) */
  int num_ops = h0.r;
  int cursor  = base + h2.r;

  vec4 acc = vec4 (0.0);

  for (int i = 0; i < num_ops; i++)
  {
    ivec4 op    = hb_gpu_fetch (cursor);
    int op_type = op.r;
    int aux     = op.g;
    int payload = (op.b << 16) | (op.a & 0xffff);

    if (op_type == 0)  /* LAYER_SOLID */
    {
      /* texel 1 holds RGBA as signed Q15. */
      ivec4 ct = hb_gpu_fetch (cursor + 1);
      vec4 col = ((aux & 1) != 0)
	       ? foreground
	       : vec4 (ct) / 32767.0;

      float cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
				     uint (base + payload));
      vec4 src = vec4 (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor += 2;
    }
    else if (op_type == 1)  /* LAYER_GRADIENT */
    {
      /* texel 1: (grad_payload_hi, grad_payload_lo, extend, stop_count) */
      ivec4 op2 = hb_gpu_fetch (cursor + 1);
      int grad_payload = (op2.r << 16) | (op2.g & 0xffff);
      int extend       = op2.b;
      int stop_count   = op2.a;

      vec4 col = vec4 (0.0);
      if (aux == 0)  /* linear */
        col = _hb_gpu_sample_linear (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground);
      /* Radial / sweep fall through as transparent for now. */

      float cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
				     uint (base + payload));
      vec4 src = vec4 (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor += 2;
    }
    else
    {
      /* PUSH_GROUP, POP_GROUP not yet handled. */
      break;
    }
  }

  return acc;
}
