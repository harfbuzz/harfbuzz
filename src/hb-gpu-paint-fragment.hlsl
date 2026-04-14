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


/* Paint-renderer fragment shader (HLSL).
 *
 * Assumes the shared fragment helpers (hb-gpu-fragment.hlsl) and
 * the draw-renderer fragment helpers (hb-gpu-draw-fragment.hlsl)
 * are prepended to this source.
 */


float4 _hb_gpu_stop_color (int stops_base, int i, float4 foreground,
			   out float offset)
{
  int4 a = hb_gpu_fetch (stops_base + i * 2);
  offset = (float) a.r / 32767.0;
  if ((a.g & 1) != 0)
    return foreground;
  int4 b = hb_gpu_fetch (stops_base + i * 2 + 1);
  return (float4) b / 32767.0;
}

float _hb_gpu_extend_t (float t, int extend)
{
  if (extend == 1) return t - floor (t);
  if (extend == 2) {
    float u = t - 2.0 * floor (t * 0.5);
    return u > 1.0 ? 2.0 - u : u;
  }
  return clamp (t, 0.0, 1.0);
}

float4 _hb_gpu_eval_stops (int stops_base, int stop_count, float t, float4 foreground)
{
  float off_prev;
  float4 col_prev = _hb_gpu_stop_color (stops_base, 0, foreground, off_prev);
  if (t <= off_prev) return col_prev;
  for (int i = 1; i < stop_count; i++)
  {
    float off;
    float4 col = _hb_gpu_stop_color (stops_base, i, foreground, off);
    if (t <= off)
    {
      float span = off - off_prev;
      float f = span > 1e-6 ? (t - off_prev) / span : 0.0;
      return lerp (col_prev, col, f);
    }
    col_prev = col;
    off_prev = off;
  }
  return col_prev;
}

float4 _hb_gpu_sample_linear (float2 renderCoord, int grad_base,
			      int stop_count, int extend, float4 foreground)
{
  int4 axis = hb_gpu_fetch (grad_base);
  float2 p0 = float2 ((float) axis.r, (float) axis.g);
  float2 p1 = float2 ((float) axis.b, (float) axis.a);
  float2 d = p1 - p0;
  float denom = dot (d, d);
  if (denom < 1e-6) return float4 (0.0, 0.0, 0.0, 0.0);
  float t = dot (renderCoord - p0, d) / denom;
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (grad_base + 1, stop_count, t, foreground);
}

float4 _hb_gpu_sample_radial (float2 renderCoord, int grad_base,
			      int stop_count, int extend, float4 foreground)
{
  int4 c0_ = hb_gpu_fetch (grad_base);
  int4 c1_ = hb_gpu_fetch (grad_base + 1);
  float2 c0 = float2 ((float) c0_.r, (float) c0_.g);
  float r0 = (float) c0_.b;
  float2 c1 = float2 ((float) c1_.r, (float) c1_.g);
  float r1 = (float) c1_.b;

  float2 cd = c1 - c0;
  float dr = r1 - r0;
  float2 p  = renderCoord - c0;

  float A = dot (cd, cd) - dr * dr;
  float B = -2.0 * (dot (p, cd) + r0 * dr);
  float C = dot (p, p) - r0 * r0;

  float t;
  if (abs (A) > 1e-6)
  {
    float disc = B * B - 4.0 * A * C;
    if (disc < 0.0) return float4 (0.0, 0.0, 0.0, 0.0);
    float sq = sqrt (disc);
    float t1 = (-B + sq) / (2.0 * A);
    float t2 = (-B - sq) / (2.0 * A);
    t = (r0 + t1 * dr >= 0.0) ? t1 : t2;
  }
  else
  {
    if (abs (B) < 1e-6) return float4 (0.0, 0.0, 0.0, 0.0);
    t = -C / B;
  }
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (grad_base + 2, stop_count, t, foreground);
}

float4 _hb_gpu_sample_sweep (float2 renderCoord, int grad_base,
			     int stop_count, int extend, float4 foreground)
{
  int4 t0 = hb_gpu_fetch (grad_base);
  float2 c = float2 ((float) t0.r, (float) t0.g);
  float a0 = (float) t0.b / 16384.0;
  float a1 = (float) t0.a / 16384.0;
  float span = a1 - a0;
  if (abs (span) < 1e-6) return float4 (0.0, 0.0, 0.0, 0.0);

  float2 p = renderCoord - c;
  float ang = atan2 (p.y, p.x) / 3.14159265358979;
  if (ang < 0.0) ang += 2.0;
  float t = (ang - a0) / span;
  t = _hb_gpu_extend_t (t, extend);
  return _hb_gpu_eval_stops (grad_base + 1, stop_count, t, foreground);
}

float4 _hb_gpu_composite (float4 src, float4 dst, int mode)
{
  float4 r = src + dst * (1.0 - src.a);

  if      (mode == 0)  r = float4 (0.0, 0.0, 0.0, 0.0);
  else if (mode == 1)  r = src;
  else if (mode == 2)  r = dst;
  else if (mode == 4)  r = dst + src * (1.0 - dst.a);
  else if (mode == 5)  r = src * dst.a;
  else if (mode == 6)  r = dst * src.a;
  else if (mode == 7)  r = src * (1.0 - dst.a);
  else if (mode == 8)  r = dst * (1.0 - src.a);
  else if (mode == 9)  r = src * dst.a + dst * (1.0 - src.a);
  else if (mode == 10) r = dst * src.a + src * (1.0 - dst.a);
  else if (mode == 11) r = src * (1.0 - dst.a) + dst * (1.0 - src.a);
  else if (mode == 12) r = min (src + dst, float4 (1.0, 1.0, 1.0, 1.0));
  else if (mode == 13) {
    r.rgb = src.rgb + dst.rgb - src.rgb * dst.rgb;
    r.a = src.a + dst.a - src.a * dst.a;
  }
  else if (mode == 15) {
    r.rgb = min (src.rgb * dst.a, dst.rgb * src.a)
          + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
    r.a = src.a + dst.a - src.a * dst.a;
  }
  else if (mode == 16) {
    r.rgb = max (src.rgb * dst.a, dst.rgb * src.a)
          + src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
    r.a = src.a + dst.a - src.a * dst.a;
  }
  else if (mode == 23) {
    r.rgb = src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a)
          + src.rgb * dst.rgb;
    r.a = src.a + dst.a - src.a * dst.a;
  }
  return r;
}

#define HB_GPU_PAINT_GROUP_DEPTH 4

float4 hb_gpu_paint (float2 renderCoord, uint glyphLoc_, float4 foreground)
{
  /* fwidth once, at uniform control flow. */
  float2 pixelsPerEm = 1.0 / fwidth (renderCoord);

  int base    = (int) glyphLoc_;
  int4 h0     = hb_gpu_fetch (base);       /* (num_ops, _, _, _) */
  int4 h2     = hb_gpu_fetch (base + 2);   /* (ops_offset, _, _, _) */
  int num_ops = h0.r;
  int cursor  = base + h2.r;

  float4 acc = float4 (0.0, 0.0, 0.0, 0.0);
  float4 group_stack[HB_GPU_PAINT_GROUP_DEPTH];
  int sp = 0;

  for (int i = 0; i < num_ops; i++)
  {
    int4 op     = hb_gpu_fetch (cursor);
    int op_type = op.r;
    int aux     = op.g;
    int payload = (op.b << 16) | (op.a & 0xffff);

    if (op_type == 0)  /* LAYER_SOLID */
    {
      int4 ct = hb_gpu_fetch (cursor + 1);
      float4 col = ((aux & 1) != 0)
		 ? foreground
		 : (float4) ct / 32767.0;

      float cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
				     (uint) (base + payload));
      float4 src = float4 (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor += 2;
    }
    else if (op_type == 1)  /* LAYER_GRADIENT */
    {
      int4 op2 = hb_gpu_fetch (cursor + 1);
      int grad_payload = (op2.r << 16) | (op2.g & 0xffff);
      int extend       = op2.b;
      int stop_count   = op2.a;

      float4 col = float4 (0.0, 0.0, 0.0, 0.0);
      if (aux == 0)
        col = _hb_gpu_sample_linear (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground);
      else if (aux == 1)
        col = _hb_gpu_sample_radial (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground);
      else if (aux == 2)
        col = _hb_gpu_sample_sweep  (renderCoord,
                                     base + grad_payload,
                                     stop_count, extend, foreground);

      float cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
				     (uint) (base + payload));
      float4 src = float4 (col.rgb * col.a, col.a) * cov;
      acc = src + acc * (1.0 - src.a);

      cursor += 2;
    }
    else if (op_type == 2)
    {
      if (sp < HB_GPU_PAINT_GROUP_DEPTH) {
        group_stack[sp] = acc;
        sp++;
      }
      acc = float4 (0.0, 0.0, 0.0, 0.0);
      cursor += 1;
    }
    else if (op_type == 3)
    {
      if (sp > 0) {
        sp--;
        float4 src = acc;
        float4 dst = group_stack[sp];
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
