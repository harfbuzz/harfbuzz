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
 * This module implements the
 * [Slug](https://github.com/EricLengyel/Slug) GPU text
 * rasterization algorithm by Eric Lengyel.  Glyph outlines are
 * encoded on the CPU into compact blobs that the GPU decodes
 * and rasterizes directly in the fragment shader, with no
 * intermediate bitmap atlas.
 *
 * # Encoding
 *
 * Each glyph is encoded independently into an opaque blob of
 * RGBA16I texels (8 bytes each).  Typical encoding flow:
 *
 * |[<!-- language="plain" -->
 * hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
 *
 * hb_gpu_draw_glyph (draw, font, gid);
 * hb_blob_t *blob = hb_gpu_draw_encode (draw);
 *
 * // Upload hb_blob_get_data(blob) to the atlas at some offset.
 * unsigned atlas_offset = upload_to_atlas (blob);
 *
 * hb_gpu_draw_recycle_blob (draw, blob);
 * hb_gpu_draw_reset (draw);
 * // repeat for more glyphs...
 *
 * hb_gpu_draw_destroy (draw);
 * ]|
 *
 * The encoder object and its internal buffers are reused across
 * glyphs.  Call hb_gpu_draw_recycle_blob() after each encode to
 * allow buffer reuse; call hb_gpu_draw_reset() before each new
 * glyph.
 *
 * # Atlas setup
 *
 * All encoded blobs are uploaded into a single GL_TEXTURE_BUFFER
 * backed by a GL_RGBA16I format buffer.  Each glyph occupies a
 * contiguous range of texels at a known offset:
 *
 * |[<!-- language="plain" -->
 * GLuint buf, tex;
 * glGenBuffers (1, &buf);
 * glGenTextures (1, &tex);
 *
 * glBindBuffer (GL_TEXTURE_BUFFER, buf);
 * glBufferData (GL_TEXTURE_BUFFER, capacity * 8, NULL, GL_STATIC_DRAW);
 *
 * glBindTexture (GL_TEXTURE_BUFFER, tex);
 * glTexBuffer (GL_TEXTURE_BUFFER, GL_RGBA16I, buf);
 *
 * // Upload a glyph blob at 'offset' texels:
 * glBufferSubData (GL_TEXTURE_BUFFER,
 *                  offset * 8,
 *                  hb_blob_get_length (blob),
 *                  hb_blob_get_data (blob, NULL));
 * ]|
 *
 * # Shader compilation
 *
 * The library provides GLSL source strings for a vertex helper
 * function and a fragment rendering function.  Prepend a #version
 * directive and append your own main():
 *
 * |[<!-- language="plain" -->
 * unsigned vert_count, frag_count;
 * const char * const *vert_lib = hb_gpu_shader_vertex_sources (
 *     HB_GPU_SHADER_LANG_GLSL, &vert_count);
 * const char * const *frag_lib = hb_gpu_shader_fragment_sources (
 *     HB_GPU_SHADER_LANG_GLSL, &frag_count);
 *
 * const char *vert_sources[] = {
 *     "#version 330\n",
 *     hb_gpu_shader_vertex_source (HB_GPU_SHADER_LANG_GLSL),
 *     your_vertex_main
 * };
 * glShaderSource (vert_shader, 3, vert_sources, NULL);
 * // Same pattern for the fragment shader.
 * ]|
 *
 * # Vertex shader
 *
 * The vertex library provides one function:
 *
 * |[<!-- language="plain" -->
 * void hb_gpu_dilate (inout vec2 position, inout vec2 texcoord,
 *                     vec2 normal, vec4 jac,
 *                     mat4 m, vec2 viewport);
 * ]|
 *
 * This expands each glyph quad by approximately half a pixel on
 * screen to ensure proper antialiasing coverage at the edges.
 * It must be called in the vertex shader before computing
 * gl_Position.
 *
 * Per-vertex attributes for each glyph quad (2 triangles, 6
 * vertices, 4 unique corners):
 *
 * - **position** (vec2): object-space vertex position, computed from
 *   the glyph's bounding box.  For corner (cx, cy) where cx,cy
 *   are 0 or 1:
 *   `pos.x = pen_x + scale * lerp(extent_min_x, extent_max_x, cx)`
 *   `pos.y = pen_y - scale * lerp(extent_min_y, extent_max_y, cy)`
 *   where scale = font_size / upem.
 *
 * - **texcoord** (vec2): em-space texture coordinates, equal to the
 *   raw extent values: `(ex, ey)` where ex/ey are the glyph's
 *   bounding box corners in font design units.  The fragment shader
 *   samples the encoded blob in this coordinate space.
 *
 * - **normal** (vec2): outward normal at each corner.
 *   `(-1, +1)` for (0,0), `(+1, +1)` for (1,0),
 *   `(-1, -1)` for (0,1), `(+1, -1)` for (1,1).
 *   Note: y is negated relative to cx,cy because the common
 *   em-to-object transform flips y.
 *
 * - **jac** (vec4): maps object-space displacements back to
 *   em-space after dilation, stored row-major as
 *   (j00, j01, j10, j11).  For the common case of uniform
 *   scaling with y-flip:
 *   `jac = vec4(emPerPos, 0.0, 0.0, -emPerPos)`
 *   where emPerPos = upem / font_size.  For non-trivial
 *   transforms, see the hb_gpu_dilate source.
 *
 * - **m** (mat4): the model-view-projection matrix (uniform).
 *
 * - **viewport** (vec2): the viewport size in pixels (uniform).
 *
 * A typical vertex main:
 *
 * |[<!-- language="plain" -->
 * uniform mat4 u_matViewProjection;
 * uniform vec2 u_viewport;
 *
 * in vec2 a_position;
 * in vec2 a_texcoord;
 * in vec2 a_normal;
 * in float a_emPerPos;
 * in uint a_glyphLoc;
 *
 * out vec2 v_texcoord;
 * flat out uint v_glyphLoc;
 *
 * void main () {
 *   vec2 pos = a_position;
 *   vec2 tex = a_texcoord;
 *   vec4 jac = vec4 (a_emPerPos, 0.0, 0.0, -a_emPerPos);
 *   hb_gpu_dilate (pos, tex, a_normal, jac,
 *                  u_matViewProjection, u_viewport);
 *   gl_Position = u_matViewProjection * vec4 (pos, 0.0, 1.0);
 *   v_texcoord = tex;
 *   v_glyphLoc = a_glyphLoc;
 * }
 * ]|
 *
 * hb_gpu_dilate computes a perspective-correct half-pixel
 * expansion.  Applications that do not need perspective
 * correctness (e.g. strictly 2D rendering) can instead expand
 * each quad vertex by a fixed amount along its normal and
 * adjust the texcoord proportionally.
 *
 * # Fragment shader
 *
 * The fragment library provides one function:
 *
 * |[<!-- language="plain" -->
 * float hb_gpu_render (vec2 renderCoord, uint glyphLoc);
 * ]|
 *
 * It requires the `hb_gpu_atlas` uniform to be bound to the
 * texture containing the encoded glyph data.  By default this
 * is an `isamplerBuffer` (GL_TEXTURE_BUFFER).  For platforms
 * without texture buffers (e.g. WebGL2), define
 * `HB_GPU_ATLAS_2D` before including the shader source; the
 * atlas then becomes an `isampler2D` and a
 * `uniform int hb_gpu_atlas_width` must also be set to the
 * texture width.
 *
 * Parameters:
 *
 * - **renderCoord**: the interpolated em-space coordinate from
 *   the vertex shader (v_texcoord).
 *
 * - **glyphLoc**: the texel offset of this glyph's encoded blob
 *   in the atlas buffer.  Passed as a flat varying from the
 *   vertex shader.
 *
 * Returns the coverage in [0, 1]: 1.0 inside the glyph, 0.0
 * outside, with antialiased edges.
 *
 * A typical fragment main:
 *
 * |[<!-- language="plain" -->
 * in vec2 v_texcoord;
 * flat in uint v_glyphLoc;
 * out vec4 fragColor;
 *
 * void main () {
 *   float coverage = hb_gpu_render (v_texcoord, v_glyphLoc);
 *   fragColor = vec4 (0.0, 0.0, 0.0, coverage);
 * }
 * ]|
 *
 * The coverage value can be used with alpha blending
 * (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) to composite glyphs
 * over a background, or multiplied with any color.
 **/
