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
 * are prepended to this source.  The caller must also declare the
 * palette buffer before this source, e.g.:
 *
 *   StructuredBuffer<float4> hb_gpu_palette : register(t1);
 */


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

  for (int i = 0; i < num_ops; i++)
  {
    int4 op     = hb_gpu_fetch (cursor);
    int op_type = op.r;
    int payload = (op.b << 16) | (op.a & 0xffff);

    if (op_type == 0)  /* LAYER_SOLID */
    {
      int4 ct = hb_gpu_fetch (cursor + 1);
      int palette_index = ct.r;
      int flags         = ct.g;
      float alpha       = (float) ct.b / 32767.0;

      float4 col = ((flags & 1) != 0)
		 ? foreground
		 : hb_gpu_palette[palette_index];
      col.a *= alpha;

      float cov = _hb_gpu_draw_impl (renderCoord, pixelsPerEm,
				     (uint) (base + payload));
      float4 src = col * cov;
      /* SRC_OVER with premultiplied source. */
      acc = src + acc * (1.0 - src.a);

      cursor += 2;
    }
    else
    {
      /* LAYER_GRADIENT, PUSH_GROUP, POP_GROUP not yet handled. */
      break;
    }
  }

  return acc;
}
