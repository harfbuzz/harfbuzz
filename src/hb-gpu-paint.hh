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

#ifndef HB_GPU_PAINT_HH
#define HB_GPU_PAINT_HH

#include "hb.hh"
#include "hb-gpu.h"
#include "hb-object.hh"


/*
 * hb_gpu_paint_t encoded-blob design
 * ==================================
 *
 * One blob per glyph.  One quad per glyph.  The fragment shader
 * is the interpreter: it walks a flat op stream per pixel, maintains
 * a small group stack, and composites the result.
 *
 * The encoder turns a COLRv1 paint tree (visited via hb_paint_funcs_t
 * callbacks) into a pre-order op stream plus embedded sub-payloads,
 * all packed into a single RGBA16I blob suitable for
 * hb_blob_t-style upload into the same shared atlas used by the
 * draw renderer.
 *
 * --- Top-level layout --------------------------------------------
 *
 *   [header]
 *   [op stream]          -- variable number of op records
 *   [sub-payloads]       -- clip-glyph Slug draw blobs and
 *                           gradient parameter blobs, referenced
 *                           by intra-blob offsets from the ops
 *
 * Every offset in the blob is a texel index (RGBA16I, 8 bytes per
 * texel) relative to the blob's base address.  The caller uploads
 * the blob at some atlas_base offset and adds that base to the
 * glyph's vertex attribute; the fragment shader sees one absolute
 * atlas offset per glyph and discovers everything else by walking
 * inside.
 *
 * --- Header -------------------------------------------------------
 *
 *   texel 0: (num_ops:i16, reserved:i16, reserved:i16, reserved:i16)
 *   texel 1: (ext_min_x:i16, ext_min_y:i16,
 *             ext_max_x:i16, ext_max_y:i16)   -- in HB_GPU units
 *   texel 2: (ops_offset:i16, reserved:i16,
 *             reserved:i16, reserved:i16)
 *
 * --- Op record ----------------------------------------------------
 *
 * Each op is exactly one RGBA16I texel (4 int16s):
 *
 *   (op_type:i16, op_aux:i16, payload_hi:i16, payload_lo:i16)
 *
 * op_type values:
 *
 *   0  LAYER_SOLID
 *        aux      = 0
 *        payload  = clip_glyph_offset (texel index)
 *        followed by one extra texel:
 *          (palette_index:i16, flags:i16,
 *           alpha_q15:i16, reserved:i16)
 *        flags bit 0: is_foreground -- sample
 *                     hb_gpu_foreground uniform instead of
 *                     hb_gpu_palette[palette_index]
 *        alpha_q15:   modulation applied on top of the looked-up
 *                     color, 0..0x7FFF
 *
 *   1  LAYER_GRADIENT
 *        aux      = gradient subtype  (0=linear, 1=radial, 2=sweep)
 *        payload  = clip_glyph_offset (texel index)
 *        followed by one extra texel:
 *          (grad_offset_hi:i16, grad_offset_lo:i16,
 *           reserved:i16, reserved:i16)
 *
 *   2  PUSH_GROUP
 *        aux      = 0
 *        payload  = 0
 *
 *   3  POP_GROUP
 *        aux      = composite mode, enumerated 0..27 matching
 *                   hb_paint_composite_mode_t
 *        payload  = 0
 *
 * The fragment shader reads the first texel, inspects op_type,
 * and knows from the type whether a trailing texel follows.  Ops
 * are therefore mixed-size but self-delimiting.
 *
 * --- Sub-payload: clip-glyph Slug draw blob ----------------------
 *
 * Exactly the format produced by hb_gpu_draw_encode().  Referenced
 * by LAYER_SOLID and LAYER_GRADIENT.
 *
 * The outline fed into the draw encoder has the enclosing paint
 * transform baked in at CPU encode time.  v1 does not dedup
 * outlines that differ only by translation; v2 may keep the outline
 * in canonical pre-translate space and store a per-op (dx, dy)
 * offset in the aux/payload slots.
 *
 * --- Sub-payload: gradient parameter blob ------------------------
 *
 *   texels 0..2  : inverse affine transform, 6 components as
 *                  q15.16 fixed point (two int16 per component,
 *                  int part then fraction)
 *   texel 3      : (grad_subtype:i16, extend_mode:i16,
 *                   reserved:i16, reserved:i16)
 *   texels 4..N  : subtype-specific params
 *                  linear: (x0, y0, px, py)  -- two texels
 *                          where p is the foot of perpendicular
 *                          from p2 onto the p0..p1 line
 *                  radial: (x0, y0, x1, y1), (r0, r1, _, _)
 *                  sweep:  (x0, y0, start_q14, end_q14)
 *                          angles in q14 fractions of pi
 *   texels ...   : color stops, each one texel:
 *                    (offset_q8<<1 | last_stop_bit,
 *                     palette_index:i16,
 *                     flags:i16,
 *                     alpha_q15:i16)
 *                  flags / alpha_q15 semantics match LAYER_SOLID.
 *                  The last stop is marked by the LSB of
 *                  offset_q8 being 1 (offsets quantized to q8 so
 *                  the bit is free for the marker).
 *
 * --- Shader entry point -----------------------------------------
 *
 *   vec4 hb_gpu_paint (vec2 renderCoord, uint glyphLoc,
 *                      vec4 foreground);
 *
 * Returns premultiplied RGBA.  The caller passes foreground in
 * explicitly; sourcing it from a uniform, a per-quad flat varying,
 * or anywhere else is the caller's decision.  A per-quad attribute
 * is the natural choice for styled text runs (no state flushes
 * between colored glyphs); a uniform is fine for single-color
 * rendering.
 *
 * Palette lookups target:
 *
 *   uniform vec4 hb_gpu_palette[HB_GPU_PALETTE_SIZE];
 *
 * This is the same implicit-global relationship hb_gpu_draw has with
 * hb_gpu_atlas / hb_gpu_fetch.  HB_GPU_PALETTE_SIZE defaults to 256
 * -- enough for real COLRv1 fonts (Noto Color Emoji uses ~150
 * entries).  The caller fills the uniform from
 * hb_ot_color_palette_get_colors() for whichever palette is active
 * and updates it whenever the palette changes (dark/light mode,
 * user overrides) -- no re-encoding.
 *
 * Color stops and solid-fill colors in the blob carry a
 * palette_index + flags rather than resolved RGBA, so blobs are
 * palette-independent.  The is_foreground flag routes to the
 * foreground parameter instead of the palette array.
 *
 * --- Fragment-shader contract ------------------------------------
 *
 * Per pixel:
 *   acc    = vec4 (0)                    -- transparent
 *   stack  = array<vec4, HB_GPU_PAINT_GROUP_DEPTH>
 *   sp     = 0
 *
 *   for op in ops:
 *     LAYER_SOLID:
 *       cov = hb_gpu_draw (texcoord, clip_glyph_offset)
 *       col = is_foreground ? v_foreground
 *                           : hb_gpu_palette[palette_index]
 *       col.a *= alpha_q15 / 32767
 *       acc = src_over (col * cov, acc)
 *     LAYER_GRADIENT:
 *       cov = hb_gpu_draw (texcoord, clip_glyph_offset)
 *       col = sample_gradient (texcoord, grad_offset, subtype)
 *       acc = src_over (col * cov, acc)
 *     PUSH_GROUP:
 *       stack[sp++] = acc
 *       acc = vec4 (0)
 *     POP_GROUP:
 *       acc = composite (stack[--sp], acc, composite_mode)
 *
 *   write acc to gl_FragColor
 *
 * HB_GPU_PAINT_GROUP_DEPTH is compile-time fixed at 4.  Encoder
 * validates nesting depth and fails encode if a paint tree exceeds
 * it.  Typical COLRv1 emoji nest 0..2 groups deep.
 *
 * --- Encoder state ----------------------------------------------
 *
 * hb_gpu_paint_t accumulates, during hb_font_paint_glyph_or_fail:
 *
 *   - transform stack (visited push/pop_transform)
 *   - current clip-glyph snapshot captured on push_clip_glyph:
 *       (font, codepoint, transform_at_push)
 *     consumed by the next color / gradient callback to emit a
 *     LAYER_SOLID or LAYER_GRADIENT op
 *   - flat op buffer
 *   - flat sub-payload buffer (bytes; offsets recorded as ops emit)
 *   - scratch hb_gpu_draw_t used to rasterize each clip-glyph
 *     outline under its baked transform
 *   - scratch int16 vector for gradient encoding
 *   - current group depth counter (for validation against
 *     HB_GPU_PAINT_GROUP_DEPTH)
 *   - recycled output blob
 *
 * To preserve palette indices across harfbuzz's paint pipeline
 * (which resolves them to hb_color_t before the color / color-stop
 * callbacks fire), the encoder registers a custom_palette_color
 * callback that returns a sentinel hb_color_t with the palette
 * index embedded.  Downstream color / color-stop callbacks decode
 * the sentinel to recover the index.  Foreground colors are
 * signalled independently by the is_foreground flag that harfbuzz
 * supplies, so the sentinel never collides with a genuine
 * foreground request.
 *
 * On hb_gpu_paint_encode:
 *   - emit header (num_ops, extents, ops_offset)
 *   - emit op stream
 *   - append accumulated sub-payload buffer
 *   - patch any ops that held byte offsets to convert to texel
 *     offsets
 *   - return blob
 *
 * --- Out of scope for v1 ----------------------------------------
 *
 *   - Clip-rectangle paints (push_clip_rectangle / pop_clip).
 *     COLRv1 allows them but they are rare; v1 emits nothing for
 *     them and the caller loses clipping precision.  v2 adds a
 *     RECT_CLIP op with four fixed-point extents.
 *   - Paint images (hb_paint_funcs_set_image_func).  Always
 *     returns unhandled.  Images are better served by a separate
 *     bitmap path.
 */

struct hb_gpu_paint_t
{
  hb_object_header_t header;
};


#endif /* HB_GPU_PAINT_HH */
