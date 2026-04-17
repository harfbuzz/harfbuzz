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
  ivec4 b = hb_gpu_fetch (stops_base + i * 2 + 1);
  if ((a.g & 1) != 0)
    return vec4 (foreground.rgb, foreground.a * (float (b.a) / 32767.0));
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

/* Walk stops starting at @stops_base and return the sampled color
 * at @t.  Same logic reused by all gradient subtypes. */
vec4 _hb_gpu_eval_stops (int stops_base, int stop_count, float t, vec4 foreground)
{
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
      /* Interpolate in premultiplied space per OpenType COLR spec. */
      vec4 p0 = vec4 (col_prev.rgb * col_prev.a, col_prev.a);
      vec4 p1 = vec4 (col.rgb * col.a, col.a);
      vec4 pm = mix (p0, p1, f);
      return pm.a > 1e-6 ? vec4 (pm.rgb / pm.a, pm.a) : vec4 (0.0);
    }
    col_prev = col;
    off_prev = off;
  }
  return col_prev;
}

/* Apply the stored 2x2 M^-1 (row-major i16 Q10) to @v.  Scaling
 * renderCoord deltas back into canonical gradient space. */
vec2 _hb_gpu_apply_minv (ivec4 m, vec2 v)
{
  vec4 mf = vec4 (m) * (1.0 / 1024.0);
  return vec2 (mf.x * v.x + mf.y * v.y,
	       mf.z * v.x + mf.w * v.y);
}

/* Sample a linear gradient whose param blob starts at @grad_base:
 *   texel 0: (p0_rendered.x, p0_rendered.y, d_canonical.x, d_canonical.y)
 *   texel 1: L^-1 as i16 Q10 (row-major)
 *   texels 2..: stops (2 texels each)
 * Evaluate t in untransformed space. */
vec4 _hb_gpu_sample_linear (vec2 renderCoord, int grad_base,
			    int stop_count, int extend, vec4 foreground)
{
  ivec4 t0 = hb_gpu_fetch (grad_base);
  ivec4 m  = hb_gpu_fetch (grad_base + 1);
  vec2 p0_r = vec2 (float (t0.r), float (t0.g));
  vec2 d    = vec2 (float (t0.b), float (t0.a));
  float denom = dot (d, d);
  if (denom < 1e-6) return vec4 (0.0);
  vec2 p = _hb_gpu_apply_minv (m, renderCoord - p0_r);
  float t = dot (p, d) / denom;
  t = _hb_gpu_extend_t (t, extend);

  return _hb_gpu_eval_stops (grad_base + 2, stop_count, t, foreground);
}

/* Sample a two-circle radial gradient whose param blob starts at
 * @grad_base:
 *   texel 0: (c0_rendered.x, c0_rendered.y, d_canonical.x, d_canonical.y)
 *     d = c1 - c0 in untransformed space
 *   texel 1: (r0, r1, _, _) in untransformed font units
 *   texel 2: L^-1 as i16 Q10 (row-major)
 *   texels 3..: stops (2 texels each)
 * Solves |p - t*cd|^2 = (r0 + t*(r1-r0))^2 with p in untransformed
 * space, so non-uniform scale / shear on the transform becomes a
 * proper ellipse-in-rendered-space instead of a scalar-fudge. */
vec4 _hb_gpu_sample_radial (vec2 renderCoord, int grad_base,
			    int stop_count, int extend, vec4 foreground)
{
  ivec4 t0 = hb_gpu_fetch (grad_base);
  ivec4 t1 = hb_gpu_fetch (grad_base + 1);
  ivec4 m  = hb_gpu_fetch (grad_base + 2);
  vec2 c0_r = vec2 (float (t0.r), float (t0.g));
  vec2 cd   = vec2 (float (t0.b), float (t0.a));
  float r0 = float (t1.r);
  float r1 = float (t1.g);

  float dr = r1 - r0;
  vec2 p  = _hb_gpu_apply_minv (m, renderCoord - c0_r);

  float A = dot (cd, cd) - dr * dr;
  float B = -2.0 * (dot (p, cd) + r0 * dr);
  float C = dot (p, p) - r0 * r0;

  float t;
  if (abs (A) > 1e-6)
  {
    float disc = B * B - 4.0 * A * C;
    if (disc < 0.0) return vec4 (0.0);
    float sq = sqrt (disc);
    /* Prefer the larger root; fall back to the smaller if the
     * larger gives a negative interpolated radius. */
    float t1 = (-B + sq) / (2.0 * A);
    float t2 = (-B - sq) / (2.0 * A);
    t = (r0 + t1 * dr >= 0.0) ? t1 : t2;
  }
  else
  {
    if (abs (B) < 1e-6) return vec4 (0.0);
    t = -C / B;
  }

  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (grad_base + 3, stop_count, t, foreground);
}

/* Sample a sweep gradient whose param blob starts at @grad_base:
 *   texel 0: (center_rendered.x, center_rendered.y, start_q14, end_q14)
 *            start/end are Q14 fractions of pi in untransformed space
 *   texel 1: L^-1 as i16 Q10 (row-major)
 *   texels 2..: stops (2 texels each) */
vec4 _hb_gpu_sample_sweep (vec2 renderCoord, int grad_base,
			   int stop_count, int extend, vec4 foreground)
{
  ivec4 t0 = hb_gpu_fetch (grad_base);
  ivec4 m  = hb_gpu_fetch (grad_base + 1);
  vec2 c_r = vec2 (float (t0.r), float (t0.g));
  float a0 = float (t0.b) / 16384.0;  /* fraction of pi */
  float a1 = float (t0.a) / 16384.0;
  float span = a1 - a0;
  if (abs (span) < 1e-6) return vec4 (0.0);

  vec2 p = _hb_gpu_apply_minv (m, renderCoord - c_r);
  /* atan2 returns (-pi, pi]; normalize to [0, 2) fractions of pi. */
  float ang = atan (p.y, p.x) / 3.14159265358979;
  if (ang < 0.0) ang += 2.0;
  float t = (ang - a0) / span;
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (grad_base + 2, stop_count, t, foreground);
}

/* Composite two premultiplied RGBA layers using one of the COLRv1
 * compositing modes.  Unsupported modes fall back to SRC_OVER.
 * Values match hb_paint_composite_mode_t. */
vec4 _hb_gpu_composite (vec4 src, vec4 dst, int mode)
{
  vec4 r = src + dst * (1.0 - src.a);  /* SRC_OVER default */

  /* Approximate unsupported COLRv1 modes with the nearest Porter-Duff
   * mode we do implement.  Better a recognizable rendering than a
   * silent SRC_OVER fallback.  DIFFERENCE / EXCLUSION / HSL_* are
   * not similar enough to anything we have, so they still fall
   * through to SRC_OVER below. */
  if      (mode == 14 || mode == 18 || mode == 19) mode = 23; /* OVERLAY / COLOR_BURN / HARD_LIGHT -> MULTIPLY */
  else if (mode == 17 || mode == 20)               mode = 13; /* COLOR_DODGE / SOFT_LIGHT -> SCREEN */

  if      (mode == 0)  r = vec4 (0.0);                       /* CLEAR */
  else if (mode == 1)  r = src;                              /* SRC */
  else if (mode == 2)  r = dst;                              /* DST */
  else if (mode == 4)  r = dst + src * (1.0 - dst.a);        /* DST_OVER */
  else if (mode == 5)  r = src * dst.a;                      /* SRC_IN */
  else if (mode == 6)  r = dst * src.a;                      /* DST_IN */
  else if (mode == 7)  r = src * (1.0 - dst.a);              /* SRC_OUT */
  else if (mode == 8)  r = dst * (1.0 - src.a);              /* DST_OUT */
  else if (mode == 9)                                        /* SRC_ATOP */
    r = src * dst.a + dst * (1.0 - src.a);
  else if (mode == 10)                                       /* DST_ATOP */
    r = dst * src.a + src * (1.0 - dst.a);
  else if (mode == 11)                                       /* XOR */
    r = src * (1.0 - dst.a) + dst * (1.0 - src.a);
  else if (mode == 12)                                       /* PLUS */
    r = min (src + dst, vec4 (1.0));
  else if (mode == 13) {                                     /* SCREEN (premul) */
    r.rgb = src.rgb + dst.rgb - src.rgb * dst.rgb;
    r.a = src.a + dst.a - src.a * dst.a;
  }
  else if (mode == 15) {                                     /* DARKEN */
    r.rgb = min (src.rgb * dst.a, dst.rgb * src.a)
          + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
    r.a = src.a + dst.a - src.a * dst.a;
  }
  else if (mode == 16) {                                     /* LIGHTEN */
    r.rgb = max (src.rgb * dst.a, dst.rgb * src.a)
          + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
    r.a = src.a + dst.a - src.a * dst.a;
  }
  else if (mode == 23) {                                     /* MULTIPLY (premul) */
    r.rgb = src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a)
          + src.rgb * dst.rgb;
    r.a = src.a + dst.a - src.a * dst.a;
  }
  /* SRC_OVER (3) and DIFFERENCE / EXCLUSION / HSL_* (21, 22, 24-27)
   * fall through to the SRC_OVER default. */

  return r;
}

/* Wrap _hb_gpu_slug with a sub-glyph extents bail-out.  Many
 * paint layers cover a small region of the outer glyph quad; for
 * fragments outside the layer's bbox (with an AA + MSAA-spread
 * margin) the slug coverage is exactly 0, so we can skip the
 * band/curve walk entirely. */
float _hb_gpu_slug_clipped (vec2 renderCoord, vec2 pixelsPerEm, uint glyphLoc_)
{
  ivec4 header0 = hb_gpu_fetch (int (glyphLoc_));
  vec4 ext = vec4 (header0) * HB_GPU_INV_UNITS;
  vec2 margin = 2.0 / pixelsPerEm;
  if (any (lessThan    (renderCoord, ext.xy - margin)) ||
      any (greaterThan (renderCoord, ext.zw + margin)))
    return 0.0;
  return _hb_gpu_slug (renderCoord, pixelsPerEm, glyphLoc_);
}

/* Combine slug coverages from all clip outlines on the current
 * layer.  Factored out of LAYER_SOLID and LAYER_GRADIENT so the
 * shader has one set of inlined slug walks instead of two.  flags
 * bits: 0x100 = HAS_CLIP2; 0x200 = HAS_CLIP3 (HAS_CLIP3 implies
 * HAS_CLIP2). */
float _hb_gpu_layer_coverage (vec2 renderCoord, vec2 pixelsPerEm,
			      int base, int flags,
			      int clip1_payload, int clip2_payload, int clip3_payload)
{
  float cov = _hb_gpu_slug_clipped (renderCoord, pixelsPerEm,
				    uint (base + clip1_payload));
  if ((flags & 0x100) != 0)
  {
    cov *= _hb_gpu_slug_clipped (renderCoord, pixelsPerEm,
				 uint (base + clip2_payload));
    if ((flags & 0x200) != 0)
      cov *= _hb_gpu_slug_clipped (renderCoord, pixelsPerEm,
				   uint (base + clip3_payload));
  }
  return cov;
}

/* Walks the paint blob's flat op stream and returns a
 * premultiplied RGBA coverage value for the current fragment.
 *
 * glyphLoc: atlas texel offset of the paint-blob header.
 * foreground: caller-supplied foreground color, used when an op
 *             sets the is_foreground flag.
 */
#define HB_GPU_PAINT_GROUP_DEPTH 4

vec4 hb_gpu_paint (vec2 renderCoord, uint glyphLoc, vec4 foreground)
{
  /* fwidth once, at uniform control flow: every per-layer
   * coverage sample below uses this pre-computed pixelsPerEm via
   * _hb_gpu_slug. */
  vec2 pixelsPerEm = 1.0 / fwidth (renderCoord);

  int base    = int (glyphLoc);
  ivec4 h0    = hb_gpu_fetch (base);      /* (num_ops, _, _, _) */
  ivec4 h2    = hb_gpu_fetch (base + 2);  /* (ops_offset, _, _, _) */
  int num_ops = h0.r;
  int cursor  = base + h2.r;

  vec4 acc = vec4 (0.0);
  vec4 group_stack[HB_GPU_PAINT_GROUP_DEPTH];
  int sp = 0;

  for (int i = 0; i < num_ops; i++)
  {
    ivec4 op    = hb_gpu_fetch (cursor);
    int op_type = op.r;
    int aux     = op.g;
    int payload = (op.b << 16) | (op.a & 0xffff);

    if (op_type == 0)  /* LAYER_SOLID */
    {
      /* texel 1: (clip2_hi, clip2_lo, clip3_hi, clip3_lo) -- valid
       *           per HAS_CLIP2 / HAS_CLIP3 flag bits.
       * texel 2: RGBA as signed Q15. */
      ivec4 op2 = hb_gpu_fetch (cursor + 1);
      int clip2_payload = (op2.r << 16) | (op2.g & 0xffff);
      int clip3_payload = (op2.b << 16) | (op2.a & 0xffff);
      ivec4 ct = hb_gpu_fetch (cursor + 2);
      vec4 col = ((aux & 1) != 0)
	       ? vec4 (foreground.rgb, foreground.a * (float (ct.a) / 32767.0))
	       : vec4 (ct) / 32767.0;

      float cov = _hb_gpu_layer_coverage (renderCoord, pixelsPerEm,
					  base, aux,
					  payload, clip2_payload, clip3_payload);
      vec4 src = vec4 (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor += 3;
    }
    else if (op_type == 1)  /* LAYER_GRADIENT */
    {
      /* texel 1: (clip2_hi, clip2_lo, clip3_hi, clip3_lo) -- valid
       *           per HAS_CLIP2 / HAS_CLIP3 flag bits.
       * texel 2: (grad_payload_hi, grad_payload_lo, extend, stop_count) */
      ivec4 op2 = hb_gpu_fetch (cursor + 1);
      int clip2_payload = (op2.r << 16) | (op2.g & 0xffff);
      int clip3_payload = (op2.b << 16) | (op2.a & 0xffff);
      ivec4 op3 = hb_gpu_fetch (cursor + 2);
      int grad_payload = (op3.r << 16) | (op3.g & 0xffff);
      int extend       = op3.b;
      int stop_count   = op3.a;
      int subtype      = aux & 0xff;

      vec4 col = vec4 (0.0);
      if (subtype == 0)       /* linear */
        col = _hb_gpu_sample_linear (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground);
      else if (subtype == 1)  /* radial */
        col = _hb_gpu_sample_radial (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground);
      else if (subtype == 2)  /* sweep */
        col = _hb_gpu_sample_sweep  (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground);

      float cov = _hb_gpu_layer_coverage (renderCoord, pixelsPerEm,
					  base, aux,
					  payload, clip2_payload, clip3_payload);
      vec4 src = vec4 (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor += 3;
    }
    else if (op_type == 2)  /* PUSH_GROUP */
    {
      if (sp < HB_GPU_PAINT_GROUP_DEPTH) {
        group_stack[sp] = acc;
        sp++;
      }
      acc = vec4 (0.0);
      cursor += 1;
    }
    else if (op_type == 3)  /* POP_GROUP */
    {
      if (sp > 0) {
        sp--;
        vec4 src = acc;
        vec4 dst = group_stack[sp];
        acc = _hb_gpu_composite (src, dst, aux);
      }
      cursor += 1;
    }
    else
    {
      break;
    }
  }

  return acc;
}
