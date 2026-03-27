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

/**
 * SECTION:hb-gpu
 * @title: hb-gpu
 * @short_description: GPU shape encoding
 * @include: hb-gpu.h
 *
 * Functions for encoding glyph outlines into compact blobs
 * for GPU rendering.  The encoded blob is an array of RGBA16I
 * texels suitable for upload to a GPU texture buffer object.
 *
 * Typical flow:
 *
 * |[<!-- language="plain" -->
 * hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
 * hb_gpu_draw_glyph (draw, font, gid);
 * hb_blob_t *blob = hb_gpu_draw_encode (draw);
 * // upload hb_blob_get_data(blob) to GPU
 * hb_gpu_draw_recycle_blob (draw, blob);
 * hb_gpu_draw_reset (draw);
 * // repeat for more glyphs...
 * hb_gpu_draw_destroy (draw);
 * ]|
 *
 * The companion GLSL shaders returned by
 * hb_gpu_shader_fragment_sources() and hb_gpu_shader_vertex_sources()
 * render the encoded blobs on the GPU.
 **/
