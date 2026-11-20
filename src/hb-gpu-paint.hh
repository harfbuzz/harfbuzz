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
#include "hb-geometry.hh"
#include "hb-map.hh"
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
 *   texel 0: (ext_min_x:i16, ext_min_y:i16,
 *             ext_max_x:i16, ext_max_y:i16)   -- in font units
 *   texel 1: (num_ops:i16, ops_offset:i16,
 *             x_scale:i16, y_scale:i16)
 *
 * The font scale is in the last two slots of texel 1, where a draw
 * blob has it too, so that the shader's hb_gpu_ppem() works on
 * either kind of blob.
 *
 * --- Op records ---------------------------------------------------
 *
 * Ops are RGBA16I texels, and start with (op_type:i16, aux:i16).
 * The op_type says how many texels the op takes, so ops are
 * mixed-size but self-delimiting:
 *
 *   0  LAYER_SOLID      three texels
 *   1  LAYER_GRADIENT   three texels
 *   2  PUSH_GROUP       one texel
 *   3  POP_GROUP        one texel; aux = composite mode, matching
 *                       hb_paint_composite_mode_t
 *
 * A layer op refers to up to three clip outlines, which the shader
 * intersects, and carries either a color baked as Q15 RGBA (with a
 * flag to use the shader's foreground instead) or a reference to a
 * gradient parameter blob.  The exact fields are documented where
 * they are written: hb_gpu_paint_emit_solid(), emit_gradient_op_header()
 * and emit_control_op() in hb-gpu-paint.cc.
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
 * The gradient's geometry, its extend mode and its color stops,
 * with the stops' colors resolved at encode time.  The layout is
 * documented where it is written, in the linear, radial and sweep
 * emitters in hb-gpu-paint.cc.
 *
 * --- Shader entry point -----------------------------------------
 *
 *   vec4 hb_gpu_paint (vec2 renderCoord, uint glyphLoc,
 *                      vec4 foreground, out float coverage);
 *
 * Returns premultiplied RGBA, and in coverage the edge coverage
 * across the layers.  The caller passes foreground in
 * explicitly; sourcing it from a uniform, a per-quad flat varying,
 * or anywhere else is the caller's decision.
 *
 * Colors are baked into the blob at encode time (as signed Q15
 * RGBA in a single texel per layer).  There is no runtime palette
 * buffer to bind or size.  Palette selection and per-run custom
 * palette overrides are set on the encoder via
 * hb_gpu_paint_set_palette() / hb_gpu_paint_set_custom_palette_color();
 * changing either invalidates previously encoded blobs and requires
 * re-encoding.  The is_foreground flag still routes through the
 * shader's foreground parameter so a dark-mode toggle does not
 * require re-encoding.
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
 *       cov = coverage of the layer's clip outlines, intersected
 *       col = is_foreground ? v_foreground : rgba_q15 / 32767
 *       acc = src_over (col * cov, acc)
 *     LAYER_GRADIENT:
 *       cov = coverage of the layer's clip outlines, intersected
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
 *   - stack of up to three pending clips, each captured on
 *     push_clip_glyph or push_clip_path with the transform at push;
 *     every color / gradient callback under them emits a
 *     LAYER_SOLID or LAYER_GRADIENT op that refers to all of them
 *   - flat op buffer
 *   - flat sub-payload buffer (bytes; offsets recorded as ops emit)
 *   - scratch hb_gpu_draw_t used to rasterize each clip-glyph
 *     outline under its baked transform
 *   - scratch int16 vector for gradient encoding
 *   - current group depth counter (for validation against
 *     HB_GPU_PAINT_GROUP_DEPTH)
 *   - recycled output blob
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
 *     RECT_CLIP op with four fixed-point extents.  The exception
 *     is a layer whose only clip is a rectangle: v1 encodes the
 *     rectangle as a path clip for it, since otherwise the layer
 *     has no shape and is lost.
 *   - Paint images (hb_paint_funcs_set_image_func).  Always
 *     returns unhandled.  Images are better served by a separate
 *     bitmap path.
 */

struct hb_gpu_paint_t
{
  hb_object_header_t header;

  /* Persistent configuration (survives hb_gpu_paint_clear). */
  unsigned   palette    = 0;
  hb_map_t  *custom_palette = nullptr;

  /* Current effective affine (stack is grown/shrunk by
   * push_transform / pop_transform callbacks). */
  hb_transform_t<float> cur_transform = {1, 0, 0, 1, 0, 0};
  hb_vector_t<hb_transform_t<float>> transform_stack;

  /* Font scale (set by hb_gpu_paint_glyph()). */
  int x_scale = 0;
  int y_scale = 0;

  /* Accumulator state (cleared by hb_gpu_paint_clear). */

  /* Flat int16 op stream.  Each op is a sequence of i16 words
   * whose total length is determined by the op type (see the
   * design notes at the top of this file). */
  hb_vector_t<int16_t> ops;
  unsigned num_ops = 0;

  /* Clip-glyph Slug sub-blobs collected during paint walk.
   * Referenced by sub_blob_index recorded inside ops;
   * hb_gpu_paint_encode() concatenates them after the op stream and
   * patches the recorded indices into texel offsets. */
  hb_vector_t<hb_blob_t *> sub_blobs;
  /* Total bytes held in sub_blobs; bounded by HB_GPU_PAINT_MAX_SUB_BYTES. */
  unsigned sub_bytes = 0;

  bool push_sub_blob (hb_blob_t *blob)
  {
    unsigned len = hb_blob_get_length (blob);
    if (unlikely (len > HB_GPU_PAINT_MAX_SUB_BYTES - sub_bytes ||
		  !sub_blobs.push_or_fail (blob)))
      return false;
    sub_bytes += len;
    return true;
  }

  /* Nesting depth of push_group / pop_group.  We bail (set
   * `unsupported`) if it exceeds HB_GPU_PAINT_MAX_GROUP_DEPTH,
   * which matches HB_GPU_PAINT_GROUP_DEPTH in the fragment shader. */
  unsigned group_depth = 0;

  /* Work budget for the current paint session; reset by
   * hb_gpu_paint_clear().  All the session's work is outline-derived
   * (outline traversal, the curves generated from it, and the clip
   * outline texels the fragment shader walks per layer -- the GPU
   * backend never rasterizes), so a single counter suffices.  It is the
   * public draw/paint budget: seeded into the scratch encoder through the
   * public draw-budget API before each glyph and read back afterwards. */
  int64_t budget = HB_BUDGET_DEFAULT;
  int64_t budget_remaining = HB_BUDGET_GLYPH;

  void recharge_budget ()
  {
    budget_remaining = budget == HB_BUDGET_DEFAULT ?
		       HB_BUDGET_GLYPH : budget;
  }

  /* Stack of pending clips.  Each color/gradient op consumes the
   * current state of this stack: the layer is rendered where ALL
   * stacked clips are opaque (intersection).  Capped at depth
   * HB_GPU_PAINT_MAX_CLIP_DEPTH; deeper pushes set `unsupported`.
   * The transform is the one current at push_clip_glyph time --
   * the clip outline is defined in that coord space, a subsequent
   * gradient params callback may run under deeper transforms which
   * we use for the gradient but not the clip outline. */
  struct pending_clip_t
  {
    hb_codepoint_t        glyph;    /* HB_CODEPOINT_INVALID for path clips */
    hb_font_t            *font;     /* borrowed; nullptr for path clips */
    hb_transform_t<float> transform;
    /* Path clips are encoded into a sub-blob at push_clip_path_end
     * time; glyph clips are encoded lazily on first consuming layer
     * and cached.  sub_blob_index is -1 until the encode happens,
     * then holds the index into c->sub_blobs and ext_* hold the
     * design-unit extents, or HB_GPU_PAINT_CLIP_EMPTY if the outline
     * has no ink and layers under it are to be skipped. */
    int                   sub_blob_index;
    int                   ext_x0, ext_y0, ext_x1, ext_y1;
  };
  pending_clip_t clip_stack[3];
  unsigned       clip_depth = 0;

  /* Rectangle clips are not encoded (see "Out of scope for v1"),
   * but their pop_clip must not pop the glyph/path clip below them.
   * rect_clips[d] counts the rectangle clips pushed while clip_depth
   * was d; the paint walk pops them before anything at depth d, so
   * a non-zero count at the current depth identifies a rectangle
   * pop.  The last slot collects everything pushed past the cap. */
  unsigned       rect_clips[3 + 1] = {};

  /* The exception: a layer painted with no glyph/path clip on the
   * stack has nothing to give it a shape, and would be dropped.  If
   * a rectangle clip is in effect there, the rectangle is encoded as
   * a path clip for that layer.  root_rects holds the rectangles
   * pushed while clip_depth was 0, innermost last, with the
   * transform each was pushed under. */
  struct root_rect_t
  {
    float                 xmin, ymin, xmax, ymax;
    hb_transform_t<float> transform;
  };
  root_rect_t    root_rects[4];

  /* Carries the cur_transform captured at push_clip_path_start
   * through to push_clip_path_end, where the encoded clip is
   * committed to clip_stack. */
  hb_transform_t<float> pending_clip_path_transform = {};
  bool                  pending_clip_path = false;

  /* Set when the paint walk emits v1-only callbacks we do not yet
   * support.  hb_gpu_paint_encode() returns NULL in that case. */
  bool unsupported = false;

  /* Extents in font design units, accumulated across layers. */
  int ext_min_x =  0x7fffffff;
  int ext_min_y =  0x7fffffff;
  int ext_max_x = -0x7fffffff;
  int ext_max_y = -0x7fffffff;

  /* Scratch: used to rasterize each clip-glyph outline. */
  hb_gpu_draw_t *scratch_draw = nullptr;

  /* Scratch: color stops fetched per gradient callback. */
  hb_vector_t<hb_color_stop_t> color_stops_scratch;

  /* Recycled output blob. */
  hb_blob_t *recycled_blob = nullptr;

  bool fetch_color_stops (hb_color_line_t *color_line)
  {
    unsigned count = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
    if (unlikely (!count || !color_stops_scratch.resize (count)))
    {
      color_stops_scratch.resize (0);
      return false;
    }
    hb_color_line_get_color_stops (color_line, 0, &count, color_stops_scratch.arrayZ);
    return true;
  }
};


#endif /* HB_GPU_PAINT_HH */
