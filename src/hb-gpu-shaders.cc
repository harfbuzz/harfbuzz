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

#include "hb.hh"

#include "hb-gpu.h"

#include "hb-gpu-draw-fragment-glsl.hh"
#include "hb-gpu-draw-vertex-glsl.hh"
#include "hb-gpu-draw-fragment-msl.hh"
#include "hb-gpu-draw-vertex-msl.hh"
#include "hb-gpu-draw-fragment-wgsl.hh"
#include "hb-gpu-draw-vertex-wgsl.hh"
#include "hb-gpu-draw-fragment-hlsl.hh"
#include "hb-gpu-draw-vertex-hlsl.hh"


/**
 * hb_gpu_draw_shader_source:
 * @stage: pipeline stage (vertex or fragment)
 * @lang: shader language variant
 *
 * Returns the shader source for the hb_gpu_draw_t renderer for the
 * specified stage and language.  The returned string is static and
 * must not be freed.
 *
 * For GLSL, the caller should prepend a `#version` directive and
 * append their own `main()` function, then pass the combined sources
 * to `glShaderSource()`.
 *
 * Return value: (transfer none):
 * A shader source string, or `NULL` if @stage or @lang is
 * unsupported.
 *
 * XSince: REPLACEME
 **/
const char *
hb_gpu_draw_shader_source (hb_gpu_shader_stage_t stage,
			   hb_gpu_shader_lang_t  lang)
{
  switch (stage) {
  case HB_GPU_SHADER_STAGE_FRAGMENT:
    switch (lang) {
    case HB_GPU_SHADER_LANG_GLSL: return hb_gpu_draw_fragment_glsl;
    case HB_GPU_SHADER_LANG_MSL:  return hb_gpu_draw_fragment_msl;
    case HB_GPU_SHADER_LANG_WGSL: return hb_gpu_draw_fragment_wgsl;
    case HB_GPU_SHADER_LANG_HLSL: return hb_gpu_draw_fragment_hlsl;
    default: return nullptr;
    }
  case HB_GPU_SHADER_STAGE_VERTEX:
    switch (lang) {
    case HB_GPU_SHADER_LANG_GLSL: return hb_gpu_draw_vertex_glsl;
    case HB_GPU_SHADER_LANG_MSL:  return hb_gpu_draw_vertex_msl;
    case HB_GPU_SHADER_LANG_WGSL: return hb_gpu_draw_vertex_wgsl;
    case HB_GPU_SHADER_LANG_HLSL: return hb_gpu_draw_vertex_hlsl;
    default: return nullptr;
    }
  default:
    return nullptr;
  }
}
