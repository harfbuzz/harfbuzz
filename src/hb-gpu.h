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
 *
 * Author(s): Behdad Esfahbod
 */

#ifndef HB_GPU_H
#define HB_GPU_H

#include "hb.h"

HB_BEGIN_DECLS


/**
 * hb_gpu_shader_lang_t:
 * @HB_GPU_SHADER_LANG_GLSL: GLSL (OpenGL 3.3 / OpenGL ES 3.0 / WebGL 2.0)
 * @HB_GPU_SHADER_LANG_WGSL: WGSL (WebGPU)
 * @HB_GPU_SHADER_LANG_MSL: MSL (Metal)
 * @HB_GPU_SHADER_LANG_HLSL: HLSL (Direct3D)
 *
 * Shader language variant.
 *
 * Since: 14.0.0
 */
typedef enum {
  HB_GPU_SHADER_LANG_GLSL,
  HB_GPU_SHADER_LANG_WGSL,
  HB_GPU_SHADER_LANG_MSL,
  HB_GPU_SHADER_LANG_HLSL,
} hb_gpu_shader_lang_t;

HB_EXTERN const char *
hb_gpu_shader_fragment_source (hb_gpu_shader_lang_t lang);

HB_EXTERN const char *
hb_gpu_shader_vertex_source (hb_gpu_shader_lang_t lang);


/**
 * hb_gpu_draw_t:
 *
 * An opaque GPU shape encoder.  Accumulates outlines via draw
 * callbacks, then encodes them into a compact blob for GPU
 * rendering.
 *
 * Since: 14.0.0
 */
typedef struct hb_gpu_draw_t hb_gpu_draw_t;

HB_EXTERN hb_gpu_draw_t *
hb_gpu_draw_create_or_fail (void);

HB_EXTERN hb_gpu_draw_t *
hb_gpu_draw_reference (hb_gpu_draw_t *draw);

HB_EXTERN void
hb_gpu_draw_destroy (hb_gpu_draw_t *draw);

HB_EXTERN hb_bool_t
hb_gpu_draw_set_user_data (hb_gpu_draw_t     *draw,
			     hb_user_data_key_t *key,
			     void               *data,
			     hb_destroy_func_t   destroy,
			     hb_bool_t           replace);

HB_EXTERN void *
hb_gpu_draw_get_user_data (const hb_gpu_draw_t     *draw,
			     hb_user_data_key_t *key);


/* Scale */

HB_EXTERN void
hb_gpu_draw_set_scale (hb_gpu_draw_t *draw,
		       int            x_scale,
		       int            y_scale);

HB_EXTERN void
hb_gpu_draw_get_scale (const hb_gpu_draw_t *draw,
		       int                 *x_scale,
		       int                 *y_scale);

/* Draw */

HB_EXTERN hb_draw_funcs_t *
hb_gpu_draw_get_funcs (void);

HB_EXTERN hb_bool_t
hb_gpu_draw_glyph (hb_gpu_draw_t  *draw,
		   hb_font_t      *font,
		   hb_codepoint_t  glyph);


/* Encode */

HB_EXTERN hb_blob_t *
hb_gpu_draw_encode (hb_gpu_draw_t      *draw,
                    hb_glyph_extents_t *extents);

HB_EXTERN void
hb_gpu_draw_clear (hb_gpu_draw_t *draw);

HB_EXTERN void
hb_gpu_draw_reset (hb_gpu_draw_t *draw);

HB_EXTERN void
hb_gpu_draw_recycle_blob (hb_gpu_draw_t *draw,
			    hb_blob_t      *blob);


HB_END_DECLS


#if defined(__cplusplus) && defined(HB_CPLUSPLUS_HH)
namespace hb {
HB_DEFINE_VTABLE (gpu_draw, nullptr);
} // namespace hb
#endif

#endif /* HB_GPU_H */
