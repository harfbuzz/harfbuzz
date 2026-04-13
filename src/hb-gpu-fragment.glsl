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


/* Shared fragment-shader helpers for the hb-gpu renderers.
 *
 * Requires GLSL 3.30 or GLSL ES 3.00.
 *
 * For GLSL ES 3.00 / WebGL2, define HB_GPU_ATLAS_2D before
 * including this source.  The atlas is then a 2D isampler2D
 * texture of known width (set via hb_gpu_atlas_width uniform)
 * instead of an isamplerBuffer.
 */


#ifndef HB_GPU_UNITS_PER_EM
#define HB_GPU_UNITS_PER_EM 4
#endif

#define HB_GPU_INV_UNITS float(1.0 / float(HB_GPU_UNITS_PER_EM))


#ifdef HB_GPU_ATLAS_2D
uniform highp isampler2D hb_gpu_atlas;
uniform int hb_gpu_atlas_width;
ivec4 hb_gpu_fetch (int offset)
{
  return texelFetch (hb_gpu_atlas,
		     ivec2 (offset % hb_gpu_atlas_width,
			    offset / hb_gpu_atlas_width), 0);
}
#else
uniform isamplerBuffer hb_gpu_atlas;
ivec4 hb_gpu_fetch (int offset)
{
  return texelFetch (hb_gpu_atlas, offset);
}
#endif
