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

#include "hb-gpu-fragment-glsl.h"
#include "hb-gpu-vertex-glsl.h"


static const char * const fragment_sources_330[] = { hb_gpu_fragment_glsl };
static const char * const vertex_sources_330[]   = { hb_gpu_vertex_glsl };

/**
 * hb_gpu_shader_fragment_sources:
 * @lang: shader language variant
 * @count: (out): number of source strings returned
 *
 * Returns the fragment shader source strings for the specified
 * shader language.  The returned array and strings are static and
 * must not be freed.
 *
 * The caller should pass all @count strings to `glShaderSource()`
 * (or the equivalent) when compiling the fragment shader.
 *
 * Return value: (transfer none) (array length=count):
 * An array of source strings, or `NULL` if @lang is unsupported.
 *
 * XSince: REPLACEME
 **/
const char * const *
hb_gpu_shader_fragment_sources (hb_gpu_shader_lang_t  lang,
				unsigned int         *count)
{
  switch (lang) {
  case HB_GPU_SHADER_GLSL_330:
    *count = 1;
    return fragment_sources_330;
  default:
    *count = 0;
    return nullptr;
  }
}

/**
 * hb_gpu_shader_vertex_sources:
 * @lang: shader language variant
 * @count: (out): number of source strings returned
 *
 * Returns the vertex shader source strings for the specified
 * shader language.  The returned array and strings are static and
 * must not be freed.
 *
 * Return value: (transfer none) (array length=count):
 * An array of source strings, or `NULL` if @lang is unsupported.
 *
 * XSince: REPLACEME
 **/
const char * const *
hb_gpu_shader_vertex_sources (hb_gpu_shader_lang_t  lang,
			      unsigned int         *count)
{
  switch (lang) {
  case HB_GPU_SHADER_GLSL_330:
    *count = 1;
    return vertex_sources_330;
  default:
    *count = 0;
    return nullptr;
  }
}
