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

#include "hb-gpu-fragment-glsl.hh"
#include "hb-gpu-vertex-glsl.hh"


/**
 * hb_gpu_shader_fragment_source:
 * @lang: shader language variant
 *
 * Returns the fragment shader source for the specified shader
 * language.  The returned string is static and must not be freed.
 *
 * The caller should prepend a `#version` directive and append
 * their own `main()` function, then pass the combined sources
 * to `glShaderSource()`.
 *
 * Return value: (transfer none):
 * A shader source string, or `NULL` if @lang is unsupported.
 *
 * XSince: REPLACEME
 **/
const char *
hb_gpu_shader_fragment_source (hb_gpu_shader_lang_t lang)
{
  switch (lang) {
  case HB_GPU_SHADER_GLSL_330:
    return hb_gpu_fragment_glsl;
  default:
    return nullptr;
  }
}

/**
 * hb_gpu_shader_vertex_source:
 * @lang: shader language variant
 *
 * Returns the vertex shader source for the specified shader
 * language.  The returned string is static and must not be freed.
 *
 * Return value: (transfer none):
 * A shader source string, or `NULL` if @lang is unsupported.
 *
 * XSince: REPLACEME
 **/
const char *
hb_gpu_shader_vertex_source (hb_gpu_shader_lang_t lang)
{
  switch (lang) {
  case HB_GPU_SHADER_GLSL_330:
    return hb_gpu_vertex_glsl;
  default:
    return nullptr;
  }
}
