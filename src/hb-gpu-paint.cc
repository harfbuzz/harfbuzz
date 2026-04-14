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
#include "hb-gpu-paint.hh"
#include "hb-machinery.hh"


/* ---- Paint callbacks ---- */

/* Op-type tags.  Must match the layout documented in
 * hb-gpu-paint.hh. */
enum {
  HB_GPU_PAINT_OP_LAYER_SOLID    = 0,
  HB_GPU_PAINT_OP_LAYER_GRADIENT = 1,
  HB_GPU_PAINT_OP_PUSH_GROUP     = 2,
  HB_GPU_PAINT_OP_POP_GROUP      = 3,
};

static hb_bool_t
hb_gpu_paint_custom_palette_color (hb_paint_funcs_t *funcs HB_UNUSED,
				   void             *paint_data,
				   unsigned          color_index,
				   hb_color_t       *color,
				   void             *user_data HB_UNUSED)
{
  hb_gpu_paint_t *c = (hb_gpu_paint_t *) paint_data;
  if (likely (c->custom_palette && hb_map_has (c->custom_palette, color_index)))
  {
    *color = hb_map_get (c->custom_palette, color_index);
    return true;
  }
  return false;
}

static void
hb_gpu_paint_push_clip_glyph (hb_paint_funcs_t *funcs HB_UNUSED,
			      void             *paint_data,
			      hb_codepoint_t    glyph,
			      hb_font_t        *font,
			      void             *user_data HB_UNUSED)
{
  hb_gpu_paint_t *c = (hb_gpu_paint_t *) paint_data;

  c->pending_clip = true;
  c->pending_clip_glyph = glyph;
  c->pending_clip_font  = font;
}

static void
hb_gpu_paint_pop_clip (hb_paint_funcs_t *funcs HB_UNUSED,
		       void             *paint_data,
		       void             *user_data HB_UNUSED)
{
  hb_gpu_paint_t *c = (hb_gpu_paint_t *) paint_data;
  c->pending_clip = false;
}

/* Emit a 1-texel control op.  Used for PUSH_GROUP / POP_GROUP. */
static void
emit_control_op (hb_gpu_paint_t *c, int16_t op_type, int16_t aux)
{
  if (unlikely (!c->ops.resize (c->ops.length + 4)))
  {
    c->unsupported = true;
    return;
  }
  int16_t *o = &c->ops.arrayZ[c->ops.length - 4];
  o[0] = op_type;
  o[1] = aux;
  o[2] = 0;
  o[3] = 0;
  c->num_ops++;
}

static void
hb_gpu_paint_push_group (hb_paint_funcs_t *funcs HB_UNUSED,
			 void             *paint_data,
			 void             *user_data HB_UNUSED)
{
  emit_control_op ((hb_gpu_paint_t *) paint_data,
		   HB_GPU_PAINT_OP_PUSH_GROUP, 0);
}

static void
hb_gpu_paint_pop_group (hb_paint_funcs_t    *funcs HB_UNUSED,
			void                *paint_data,
			hb_paint_composite_mode_t mode,
			void                *user_data HB_UNUSED)
{
  emit_control_op ((hb_gpu_paint_t *) paint_data,
		   HB_GPU_PAINT_OP_POP_GROUP, (int16_t) mode);
}

/* Quantize unsigned byte 0..255 to signed Q15 0..32767. */
static inline int16_t
color_to_q15 (unsigned byte_value)
{
  return (int16_t) ((byte_value * 32767u + 127u) / 255u);
}

static inline void
color_to_q15_rgba (hb_color_t color, int16_t out[4])
{
  out[0] = color_to_q15 (hb_color_get_red   (color));
  out[1] = color_to_q15 (hb_color_get_green (color));
  out[2] = color_to_q15 (hb_color_get_blue  (color));
  out[3] = color_to_q15 (hb_color_get_alpha (color));
}

static inline int16_t
clamp_i16 (float v)
{
  if (v <= -32768.f) return -32768;
  if (v >=  32767.f) return  32767;
  return (int16_t) v;
}

/* Rasterize the pending clip glyph's outline into a new sub-blob
 * and accumulate its extents.  Returns the sub_blob index (>= 0)
 * on success, or -1 if the outline is empty / the layer should be
 * skipped.  Sets c->unsupported on hard failure. */
static int
emit_clip_sub_blob (hb_gpu_paint_t *c)
{
  if (unlikely (!c->scratch_draw))
  {
    c->scratch_draw = hb_gpu_draw_create_or_fail ();
    if (unlikely (!c->scratch_draw))
    {
      c->unsupported = true;
      return -1;
    }
  }
  hb_gpu_draw_clear (c->scratch_draw);
  if (!hb_gpu_draw_glyph (c->scratch_draw, c->pending_clip_font, c->pending_clip_glyph))
    return -1;  /* Clip glyph has no outline -- skip. */

  hb_glyph_extents_t ext;
  hb_blob_t *blob = hb_gpu_draw_encode (c->scratch_draw, &ext);
  if (unlikely (!blob || !c->sub_blobs.push (blob)))
  {
    hb_blob_destroy (blob);
    c->unsupported = true;
    return -1;
  }

  /* Accumulate extents: x_bearing/y_bearing are top-left, width
   * positive, height negative (growing down). */
  int x0 = ext.x_bearing;
  int x1 = ext.x_bearing + ext.width;
  int y0 = ext.y_bearing;
  int y1 = ext.y_bearing + ext.height;
  c->ext_min_x = hb_min (c->ext_min_x, hb_min (x0, x1));
  c->ext_max_x = hb_max (c->ext_max_x, hb_max (x0, x1));
  c->ext_min_y = hb_min (c->ext_min_y, hb_min (y0, y1));
  c->ext_max_y = hb_max (c->ext_max_y, hb_max (y0, y1));

  return (int) (c->sub_blobs.length - 1);
}

static void
hb_gpu_paint_emit_solid (hb_gpu_paint_t *c,
			 hb_bool_t       is_foreground,
			 hb_color_t      color)
{
  if (unlikely (!c->pending_clip))
    return;

  int clip_idx = emit_clip_sub_blob (c);
  if (clip_idx < 0)
    return;

  /* Emit LAYER_SOLID op: 2 texels (8 int16s).
   *   texel 0: op_type, flags, payload_hi, payload_lo
   *   texel 1: r_q15, g_q15, b_q15, a_q15
   * Payload holds the sub_blob index; hb_gpu_paint_encode()
   * patches it into a texel offset at serialize time.
   * flags bit 0: use shader foreground uniform instead of the
   * baked color (so runtime foreground-color changes still work). */
  if (unlikely (!c->ops.resize (c->ops.length + 8)))
  {
    c->unsupported = true;
    return;
  }
  int16_t *o = &c->ops.arrayZ[c->ops.length - 8];
  o[0] = HB_GPU_PAINT_OP_LAYER_SOLID;
  o[1] = (int16_t) (is_foreground ? 1 : 0);
  o[2] = (int16_t) ((clip_idx >> 16) & 0xffff);
  o[3] = (int16_t) (clip_idx & 0xffff);
  color_to_q15_rgba (color, o + 4);
  c->num_ops++;
}

/* Reduce the 3-point anchor COLRv1 linear-gradient spec to a
 * 2-point axis (the perpendicular foot of p1 onto the p0-p2
 * normal).  Same derivation as hb_raster_paint's reduce_anchors. */
static inline void
reduce_linear_anchors (float x0, float y0,
		       float x1, float y1,
		       float x2, float y2,
		       float *xx0, float *yy0,
		       float *xx1, float *yy1)
{
  float q2x = x2 - x0, q2y = y2 - y0;
  float q1x = x1 - x0, q1y = y1 - y0;
  float s = q2x * q2x + q2y * q2y;
  if (s < 1e-6f)
  {
    *xx0 = x0; *yy0 = y0;
    *xx1 = x1; *yy1 = y1;
    return;
  }
  float k = (q2x * q1x + q2y * q1y) / s;
  *xx0 = x0;
  *yy0 = y0;
  *xx1 = x1 - k * q2x;
  *yy1 = y1 - k * q2y;
}

/* Sort @stops by offset, find their [min, max] offset range, and
 * normalize offsets to [0, 1] by rescaling in place.  Returns
 * (mn, mx) so the caller can shift the gradient's geometry along
 * the axis (linear) / centers+radii (radial) / start+end angles
 * (sweep) to preserve the rendered gradient position.  When all
 * stops share the same offset the normalization is a no-op and the
 * resulting gradient is degenerate; callers treat that case as
 * transparent. */
static void
normalize_stops (hb_color_stop_t *stops, unsigned count,
		 float *omn, float *omx)
{
  if (unlikely (!count))
  {
    *omn = *omx = 0.f;
    return;
  }

  hb_array_t<hb_color_stop_t> (stops, count)
    .qsort ([] (const hb_color_stop_t &a, const hb_color_stop_t &b) {
      return a.offset < b.offset ? -1 : a.offset > b.offset ? 1 : 0;
    });

  float mn = stops[0].offset;
  float mx = stops[count - 1].offset;
  if (mn != mx)
    for (unsigned i = 0; i < count; i++)
      stops[i].offset = (stops[i].offset - mn) / (mx - mn);

  *omn = mn;
  *omx = mx;
}

/* Append a color line (@count stops) to @grad_data in the encoded
 * stop format used by gradient ops: 2 texels per stop.
 *   texel a: (offset_q15, flags, _, _) with flags bit 0 = is_foreground
 *   texel b: (r_q15, g_q15, b_q15, a_q15)
 * Returns false on allocation failure. */
static bool
append_color_stops (hb_vector_t<int16_t> &grad_data,
		    const hb_color_stop_t *stops,
		    unsigned count)
{
  unsigned base = grad_data.length;
  if (unlikely (!grad_data.resize (base + count * 8)))
    return false;
  int16_t *p = grad_data.arrayZ + base;
  for (unsigned i = 0; i < count; i++)
  {
    float off = stops[i].offset;
    if (off < -1.f) off = -1.f;
    else if (off > 1.f) off = 1.f;
    p[0] = (int16_t) (off * 32767.f);
    p[1] = (int16_t) (stops[i].is_foreground ? 1 : 0);
    p[2] = 0;
    p[3] = 0;
    color_to_q15_rgba (stops[i].color, p + 4);
    p += 8;
  }
  return true;
}

static void
hb_gpu_paint_emit_linear (hb_gpu_paint_t  *c,
			  hb_color_line_t *color_line,
			  float x0, float y0,
			  float x1, float y1,
			  float x2, float y2)
{
  if (unlikely (!c->pending_clip))
    return;

  /* Resolve stops (triggers custom_palette_color, etc). */
  unsigned count = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  if (unlikely (!count))
    return;
  hb_color_stop_t stack_stops[16];
  hb_color_stop_t *stops = stack_stops;
  hb_color_stop_t *heap_stops = nullptr;
  if (count > 16)
  {
    heap_stops = (hb_color_stop_t *) hb_malloc (count * sizeof (hb_color_stop_t));
    if (unlikely (!heap_stops)) { c->unsupported = true; return; }
    stops = heap_stops;
  }
  unsigned got = count;
  hb_color_line_get_color_stops (color_line, 0, &got, stops);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  /* Emit clip sub-blob first. */
  int clip_idx = emit_clip_sub_blob (c);
  if (clip_idx < 0) { hb_free (heap_stops); return; }

  /* Build gradient params sub-blob.
   *   texel 0: (lx0, ly0, lx1, ly1) in font units (i16)
   *   texels 1..N: color stops (2 texels each)
   */
  hb_vector_t<int16_t> grad_data;
  if (unlikely (!grad_data.resize (4)))
  { c->unsupported = true; hb_free (heap_stops); return; }

  float lx0, ly0, lx1, ly1;
  reduce_linear_anchors (x0, y0, x1, y1, x2, y2, &lx0, &ly0, &lx1, &ly1);
  /* Normalize stops to [0,1]; shift axis endpoints to compensate. */
  float mn, mx;
  normalize_stops (stops, got, &mn, &mx);
  float dx = lx1 - lx0, dy = ly1 - ly0;
  grad_data[0] = clamp_i16 (lx0 + mn * dx);
  grad_data[1] = clamp_i16 (ly0 + mn * dy);
  grad_data[2] = clamp_i16 (lx0 + mx * dx);
  grad_data[3] = clamp_i16 (ly0 + mx * dy);

  if (unlikely (!append_color_stops (grad_data, stops, got)))
  { c->unsupported = true; hb_free (heap_stops); return; }
  hb_free (heap_stops);

  unsigned grad_bytes = grad_data.length * sizeof (int16_t);
  hb_blob_t *grad_blob = hb_blob_create ((const char *) grad_data.arrayZ,
					 grad_bytes, HB_MEMORY_MODE_DUPLICATE,
					 nullptr, nullptr);
  if (unlikely (!grad_blob || !c->sub_blobs.push (grad_blob)))
  {
    hb_blob_destroy (grad_blob);
    c->unsupported = true;
    return;
  }
  unsigned grad_idx = c->sub_blobs.length - 1;

  /* Emit LAYER_GRADIENT op: 2 texels.
   *   texel 0: (op_type, subtype, clip_payload_hi, clip_payload_lo)
   *   texel 1: (grad_payload_hi, grad_payload_lo, extend_mode, stop_count)
   */
  if (unlikely (!c->ops.resize (c->ops.length + 8)))
  { c->unsupported = true; return; }
  int16_t *o = &c->ops.arrayZ[c->ops.length - 8];
  o[0] = HB_GPU_PAINT_OP_LAYER_GRADIENT;
  o[1] = 0;  /* subtype: 0 = linear */
  o[2] = (int16_t) ((clip_idx >> 16) & 0xffff);
  o[3] = (int16_t) (clip_idx & 0xffff);
  o[4] = (int16_t) ((grad_idx >> 16) & 0xffff);
  o[5] = (int16_t) (grad_idx & 0xffff);
  o[6] = (int16_t) extend;
  o[7] = (int16_t) got;
  c->num_ops++;
}

static void
hb_gpu_paint_emit_radial (hb_gpu_paint_t  *c,
			  hb_color_line_t *color_line,
			  float x0, float y0, float r0,
			  float x1, float y1, float r1)
{
  if (unlikely (!c->pending_clip))
    return;

  unsigned count = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  if (unlikely (!count))
    return;
  hb_color_stop_t stack_stops[16];
  hb_color_stop_t *stops = stack_stops;
  hb_color_stop_t *heap_stops = nullptr;
  if (count > 16)
  {
    heap_stops = (hb_color_stop_t *) hb_malloc (count * sizeof (hb_color_stop_t));
    if (unlikely (!heap_stops)) { c->unsupported = true; return; }
    stops = heap_stops;
  }
  unsigned got = count;
  hb_color_line_get_color_stops (color_line, 0, &got, stops);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  int clip_idx = emit_clip_sub_blob (c);
  if (clip_idx < 0) { hb_free (heap_stops); return; }

  /* Build gradient params sub-blob.
   *   texel 0: (x0, y0, r0, _) in font units (i16)
   *   texel 1: (x1, y1, r1, _) in font units (i16)
   *   texels 2..N: color stops (2 texels each)
   */
  hb_vector_t<int16_t> grad_data;
  if (unlikely (!grad_data.resize (8)))
  { c->unsupported = true; hb_free (heap_stops); return; }
  float mn, mx;
  normalize_stops (stops, got, &mn, &mx);
  float dx = x1 - x0, dy = y1 - y0, dr = r1 - r0;
  grad_data[0] = clamp_i16 (x0 + mn * dx);
  grad_data[1] = clamp_i16 (y0 + mn * dy);
  grad_data[2] = clamp_i16 (r0 + mn * dr);
  grad_data[3] = 0;
  grad_data[4] = clamp_i16 (x0 + mx * dx);
  grad_data[5] = clamp_i16 (y0 + mx * dy);
  grad_data[6] = clamp_i16 (r0 + mx * dr);
  grad_data[7] = 0;

  if (unlikely (!append_color_stops (grad_data, stops, got)))
  { c->unsupported = true; hb_free (heap_stops); return; }
  hb_free (heap_stops);

  unsigned grad_bytes = grad_data.length * sizeof (int16_t);
  hb_blob_t *grad_blob = hb_blob_create ((const char *) grad_data.arrayZ,
					 grad_bytes, HB_MEMORY_MODE_DUPLICATE,
					 nullptr, nullptr);
  if (unlikely (!grad_blob || !c->sub_blobs.push (grad_blob)))
  {
    hb_blob_destroy (grad_blob);
    c->unsupported = true;
    return;
  }
  unsigned grad_idx = c->sub_blobs.length - 1;

  if (unlikely (!c->ops.resize (c->ops.length + 8)))
  { c->unsupported = true; return; }
  int16_t *o = &c->ops.arrayZ[c->ops.length - 8];
  o[0] = HB_GPU_PAINT_OP_LAYER_GRADIENT;
  o[1] = 1;  /* subtype: 1 = radial */
  o[2] = (int16_t) ((clip_idx >> 16) & 0xffff);
  o[3] = (int16_t) (clip_idx & 0xffff);
  o[4] = (int16_t) ((grad_idx >> 16) & 0xffff);
  o[5] = (int16_t) (grad_idx & 0xffff);
  o[6] = (int16_t) extend;
  o[7] = (int16_t) got;
  c->num_ops++;
}

static void
hb_gpu_paint_color (hb_paint_funcs_t *funcs HB_UNUSED,
		    void             *paint_data,
		    hb_bool_t         is_foreground,
		    hb_color_t        color,
		    void             *user_data HB_UNUSED)
{
  hb_gpu_paint_emit_solid ((hb_gpu_paint_t *) paint_data, is_foreground, color);
}

static void
hb_gpu_paint_linear_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
			      void             *paint_data,
			      hb_color_line_t  *color_line,
			      float x0, float y0,
			      float x1, float y1,
			      float x2, float y2,
			      void             *user_data HB_UNUSED)
{
  hb_gpu_paint_emit_linear ((hb_gpu_paint_t *) paint_data, color_line,
			    x0, y0, x1, y1, x2, y2);
}

static void
hb_gpu_paint_radial_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
			      void             *paint_data,
			      hb_color_line_t  *color_line,
			      float x0, float y0, float r0,
			      float x1, float y1, float r1,
			      void             *user_data HB_UNUSED)
{
  hb_gpu_paint_emit_radial ((hb_gpu_paint_t *) paint_data, color_line,
			    x0, y0, r0, x1, y1, r1);
}

static void
hb_gpu_paint_emit_sweep (hb_gpu_paint_t  *c,
			 hb_color_line_t *color_line,
			 float cx, float cy,
			 float start_angle, float end_angle)
{
  if (unlikely (!c->pending_clip))
    return;

  unsigned count = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  if (unlikely (!count))
    return;
  hb_color_stop_t stack_stops[16];
  hb_color_stop_t *stops = stack_stops;
  hb_color_stop_t *heap_stops = nullptr;
  if (count > 16)
  {
    heap_stops = (hb_color_stop_t *) hb_malloc (count * sizeof (hb_color_stop_t));
    if (unlikely (!heap_stops)) { c->unsupported = true; return; }
    stops = heap_stops;
  }
  unsigned got = count;
  hb_color_line_get_color_stops (color_line, 0, &got, stops);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  int clip_idx = emit_clip_sub_blob (c);
  if (clip_idx < 0) { hb_free (heap_stops); return; }

  /* Build gradient params sub-blob.
   *   texel 0: (cx, cy, start_q14, end_q14)
   *     angles in radians, stored as Q14 fractions of pi (so the
   *     shader can compare directly against atan2(P-c) / pi).
   *   texels 1..N: color stops (2 texels each) */
  hb_vector_t<int16_t> grad_data;
  if (unlikely (!grad_data.resize (4)))
  { c->unsupported = true; hb_free (heap_stops); return; }

  float mn, mx;
  normalize_stops (stops, got, &mn, &mx);
  float a_span = end_angle - start_angle;
  float a_lo = start_angle + mn * a_span;
  float a_hi = start_angle + mx * a_span;
  auto rad_to_q14_pi = [] (float rad) -> int16_t {
    float v = rad * (float) (1.0 / M_PI);  /* fraction of pi */
    if (v <= -2.f) return -32768;
    if (v >=  2.f) return  32767;
    return (int16_t) (v * 16384.f);
  };
  grad_data[0] = clamp_i16 (cx);
  grad_data[1] = clamp_i16 (cy);
  grad_data[2] = rad_to_q14_pi (a_lo);
  grad_data[3] = rad_to_q14_pi (a_hi);

  if (unlikely (!append_color_stops (grad_data, stops, got)))
  { c->unsupported = true; hb_free (heap_stops); return; }
  hb_free (heap_stops);

  unsigned grad_bytes = grad_data.length * sizeof (int16_t);
  hb_blob_t *grad_blob = hb_blob_create ((const char *) grad_data.arrayZ,
					 grad_bytes, HB_MEMORY_MODE_DUPLICATE,
					 nullptr, nullptr);
  if (unlikely (!grad_blob || !c->sub_blobs.push (grad_blob)))
  {
    hb_blob_destroy (grad_blob);
    c->unsupported = true;
    return;
  }
  unsigned grad_idx = c->sub_blobs.length - 1;

  if (unlikely (!c->ops.resize (c->ops.length + 8)))
  { c->unsupported = true; return; }
  int16_t *o = &c->ops.arrayZ[c->ops.length - 8];
  o[0] = HB_GPU_PAINT_OP_LAYER_GRADIENT;
  o[1] = 2;  /* subtype: 2 = sweep */
  o[2] = (int16_t) ((clip_idx >> 16) & 0xffff);
  o[3] = (int16_t) (clip_idx & 0xffff);
  o[4] = (int16_t) ((grad_idx >> 16) & 0xffff);
  o[5] = (int16_t) (grad_idx & 0xffff);
  o[6] = (int16_t) extend;
  o[7] = (int16_t) got;
  c->num_ops++;
}

static void
hb_gpu_paint_sweep_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
			     void             *paint_data,
			     hb_color_line_t  *color_line,
			     float cx, float cy,
			     float start_angle,
			     float end_angle,
			     void             *user_data HB_UNUSED)
{
  hb_gpu_paint_emit_sweep ((hb_gpu_paint_t *) paint_data, color_line,
			   cx, cy, start_angle, end_angle);
}

static inline void free_static_gpu_paint_funcs ();

static struct hb_gpu_paint_funcs_lazy_loader_t
  : hb_paint_funcs_lazy_loader_t<hb_gpu_paint_funcs_lazy_loader_t>
{
  static hb_paint_funcs_t *create ()
  {
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();

    hb_paint_funcs_set_push_clip_glyph_func       (funcs, hb_gpu_paint_push_clip_glyph,       nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func              (funcs, hb_gpu_paint_pop_clip,              nullptr, nullptr);
    hb_paint_funcs_set_push_group_func            (funcs, hb_gpu_paint_push_group,            nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func             (funcs, hb_gpu_paint_pop_group,             nullptr, nullptr);
    hb_paint_funcs_set_color_func                 (funcs, hb_gpu_paint_color,                 nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func       (funcs, hb_gpu_paint_linear_gradient,       nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func       (funcs, hb_gpu_paint_radial_gradient,       nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func        (funcs, hb_gpu_paint_sweep_gradient,        nullptr, nullptr);
    hb_paint_funcs_set_custom_palette_color_func  (funcs, hb_gpu_paint_custom_palette_color,  nullptr, nullptr);

    hb_paint_funcs_make_immutable (funcs);

    hb_atexit (free_static_gpu_paint_funcs);

    return funcs;
  }
} static_gpu_paint_funcs;

static inline void
free_static_gpu_paint_funcs ()
{
  static_gpu_paint_funcs.free_instance ();
}


/* ---- Public API ---- */

/**
 * hb_gpu_paint_create_or_fail:
 *
 * Creates a new GPU color-glyph paint encoder.
 *
 * Return value: (transfer full):
 * A newly allocated #hb_gpu_paint_t, or `NULL` on allocation
 * failure.
 *
 * XSince: REPLACEME
 **/
hb_gpu_paint_t *
hb_gpu_paint_create_or_fail (void)
{
  return hb_object_create<hb_gpu_paint_t> ();
}

/**
 * hb_gpu_paint_reference: (skip)
 * @paint: a GPU color-glyph paint encoder
 *
 * Increases the reference count on @paint by one.
 *
 * Return value: (transfer full):
 * The referenced #hb_gpu_paint_t.
 *
 * XSince: REPLACEME
 **/
hb_gpu_paint_t *
hb_gpu_paint_reference (hb_gpu_paint_t *paint)
{
  return hb_object_reference (paint);
}

/**
 * hb_gpu_paint_destroy: (skip)
 * @paint: a GPU color-glyph paint encoder
 *
 * Decreases the reference count on @paint by one.  When the
 * reference count reaches zero, the encoder is freed.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_paint_destroy (hb_gpu_paint_t *paint)
{
  if (!hb_object_destroy (paint)) return;
  hb_free (paint);
}

/**
 * hb_gpu_paint_set_user_data: (skip)
 * @paint: a GPU color-glyph paint encoder
 * @key: the user-data key
 * @data: a pointer to the user data
 * @destroy: (nullable): a callback to call when @data is not needed anymore
 * @replace: whether to replace an existing data with the same key
 *
 * Attaches user data to @paint.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_gpu_paint_set_user_data (hb_gpu_paint_t     *paint,
			    hb_user_data_key_t *key,
			    void               *data,
			    hb_destroy_func_t   destroy,
			    hb_bool_t           replace)
{
  return hb_object_set_user_data (paint, key, data, destroy, replace);
}

/**
 * hb_gpu_paint_get_user_data: (skip)
 * @paint: a GPU color-glyph paint encoder
 * @key: the user-data key
 *
 * Fetches the user data associated with the specified key.
 *
 * Return value: (transfer none):
 * A pointer to the user data.
 *
 * XSince: REPLACEME
 **/
void *
hb_gpu_paint_get_user_data (const hb_gpu_paint_t *paint,
			    hb_user_data_key_t   *key)
{
  return hb_object_get_user_data (paint, key);
}

/**
 * hb_gpu_paint_get_funcs:
 *
 * Fetches the singleton #hb_paint_funcs_t that feeds paint data
 * into an #hb_gpu_paint_t.  Pass the #hb_gpu_paint_t as the
 * @paint_data argument when calling the paint functions.
 *
 * Return value: (transfer none):
 * The GPU paint functions
 *
 * XSince: REPLACEME
 **/
hb_paint_funcs_t *
hb_gpu_paint_get_funcs (void)
{
  return static_gpu_paint_funcs.get_unconst ();
}

/**
 * hb_gpu_paint_set_foreground:
 * @paint: a GPU color-glyph paint encoder
 * @foreground: the foreground color
 *
 * Sets the foreground color used to resolve palette references that
 * specify the foreground sentinel (palette index 0xFFFF).
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_paint_set_foreground (hb_gpu_paint_t *paint,
			     hb_color_t      foreground)
{
  paint->foreground = foreground;
}

/**
 * hb_gpu_paint_get_foreground:
 * @paint: a GPU color-glyph paint encoder
 *
 * Returns the foreground color previously set on @paint, or the
 * default opaque black if none was set.
 *
 * Return value: the foreground color.
 *
 * XSince: REPLACEME
 **/
hb_color_t
hb_gpu_paint_get_foreground (const hb_gpu_paint_t *paint)
{
  return paint->foreground;
}

/**
 * hb_gpu_paint_set_palette:
 * @paint: a GPU color-glyph paint encoder
 * @palette: palette index
 *
 * Selects which font palette is used when paint callbacks look up
 * indexed colors.  Default is palette 0.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_paint_set_palette (hb_gpu_paint_t *paint,
			  unsigned        palette)
{
  paint->palette = palette;
}

/**
 * hb_gpu_paint_get_palette:
 * @paint: a GPU color-glyph paint encoder
 *
 * Returns the palette index previously set on @paint, or 0 if none
 * was set.
 *
 * Return value: the palette index.
 *
 * XSince: REPLACEME
 **/
unsigned
hb_gpu_paint_get_palette (const hb_gpu_paint_t *paint)
{
  return paint->palette;
}

/**
 * hb_gpu_paint_clear_custom_palette_colors:
 * @paint: a GPU color-glyph paint encoder
 *
 * Clears all custom palette color overrides previously set on @paint.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_paint_clear_custom_palette_colors (hb_gpu_paint_t *paint)
{
  if (paint->custom_palette)
    hb_map_clear (paint->custom_palette);
}

/**
 * hb_gpu_paint_set_custom_palette_color:
 * @paint: a GPU color-glyph paint encoder
 * @color_index: palette index to override
 * @color: replacement color
 *
 * Overrides one font palette color entry.  Overrides are keyed by
 * @color_index and persist on @paint until cleared (or replaced for
 * the same index).
 *
 * Return value: `true` if the override was set; `false` on allocation failure.
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_gpu_paint_set_custom_palette_color (hb_gpu_paint_t *paint,
				       unsigned int    color_index,
				       hb_color_t      color)
{
  if (unlikely (!paint->custom_palette))
  {
    paint->custom_palette = hb_map_create ();
    if (unlikely (!paint->custom_palette))
      return false;
  }
  hb_map_set (paint->custom_palette, color_index, color);
  return hb_map_allocation_successful (paint->custom_palette);
}

/**
 * hb_gpu_paint_glyph:
 * @paint: a GPU color-glyph paint encoder
 * @font: font to paint from
 * @glyph: glyph ID to paint
 *
 * Convenience wrapper that feeds a color glyph's paint tree into
 * the encoder via hb_font_paint_glyph_or_fail().  The font's scale
 * is stashed on @paint for use by hb_gpu_paint_encode().
 *
 * Return value: `true` if the glyph has color paint data and was
 * accumulated without error, `false` otherwise.
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_gpu_paint_glyph (hb_gpu_paint_t *paint,
		    hb_font_t      *font,
		    hb_codepoint_t  glyph)
{
  hb_font_get_scale (font, &paint->x_scale, &paint->y_scale);
  /* Use hb_font_paint_glyph (not _or_fail) so that non-color
   * glyphs still produce a blob: harfbuzz synthesizes
   * push_clip_glyph + color(is_foreground=true) + pop_clip, which
   * our callbacks turn into a single LAYER_SOLID op. */
  hb_font_paint_glyph (font, glyph,
		       hb_gpu_paint_get_funcs (), paint,
		       paint->palette, paint->foreground);
  return !paint->unsupported;
}

/**
 * hb_gpu_paint_encode:
 * @paint: a GPU color-glyph paint encoder
 * @extents: (out): receives the glyph's ink extents in font units
 *
 * Encodes the accumulated paint state into a GPU-renderable blob
 * and writes the glyph's ink extents to @extents.
 *
 * Return value: (transfer full):
 * A newly allocated blob, or `NULL` if there is nothing to encode
 * or the accumulated paint used unsupported features.
 *
 * XSince: REPLACEME
 **/
hb_blob_t *
hb_gpu_paint_encode (hb_gpu_paint_t     *paint,
		     hb_glyph_extents_t *extents)
{
  if (unlikely (paint->unsupported || paint->num_ops == 0))
    return nullptr;

  /* Layout:
   *   header          (3 texels = 24 bytes)
   *   op stream       (ops.length i16 words, multiple of 4)
   *   sub-payloads    (concat of each sub_blob's data, 8-byte
   *                    aligned since draw blobs are RGBA16I)
   */
  const unsigned header_texels = 3;
  const unsigned texel_bytes   = 8;

  unsigned ops_texels = paint->ops.length / 4;
  unsigned sub_bytes = 0;
  for (hb_blob_t *b : paint->sub_blobs)
    sub_bytes += hb_blob_get_length (b);
  /* Sub-blobs come from the draw encoder which produces 8-byte
   * aligned blobs; assert so we notice if that ever changes. */
  if (unlikely (sub_bytes % texel_bytes))
    return nullptr;

  unsigned total_bytes = header_texels * texel_bytes
			+ paint->ops.length * 2
			+ sub_bytes;

  int16_t *buf = (int16_t *) hb_malloc (total_bytes);
  if (unlikely (!buf))
    return nullptr;

  /* Compute each sub_blob's texel offset (relative to blob base). */
  unsigned sub_offsets_count = paint->sub_blobs.length;
  hb_vector_t<unsigned> sub_offsets;
  if (unlikely (!sub_offsets.resize (sub_offsets_count)))
  {
    hb_free (buf);
    return nullptr;
  }
  unsigned cursor = header_texels + ops_texels;
  for (unsigned i = 0; i < sub_offsets_count; i++)
  {
    sub_offsets[i] = cursor;
    cursor += hb_blob_get_length (paint->sub_blobs[i]) / texel_bytes;
  }

  /* Header. */
  buf[0] = (int16_t) paint->num_ops;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  buf[4] = (int16_t) hb_clamp (paint->ext_min_x, -0x7fff, 0x7fff);
  buf[5] = (int16_t) hb_clamp (paint->ext_min_y, -0x7fff, 0x7fff);
  buf[6] = (int16_t) hb_clamp (paint->ext_max_x, -0x7fff, 0x7fff);
  buf[7] = (int16_t) hb_clamp (paint->ext_max_y, -0x7fff, 0x7fff);
  buf[8] = (int16_t) header_texels;
  buf[9]  = 0;
  buf[10] = 0;
  buf[11] = 0;

  /* Op stream: copy raw, then patch payload slots to texel
   * offsets where op_type indicates a sub_blob reference. */
  int16_t *ops_out = buf + header_texels * 4;
  hb_memcpy (ops_out, paint->ops.arrayZ, paint->ops.length * 2);

  unsigned i = 0;
  while (i < paint->ops.length)
  {
    int16_t op_type = ops_out[i];
    switch (op_type)
    {
      case HB_GPU_PAINT_OP_LAYER_SOLID:
      {
	unsigned idx = ((uint16_t) ops_out[i + 2] << 16)
		     |  (uint16_t) ops_out[i + 3];
	if (idx < sub_offsets_count)
	{
	  unsigned off = sub_offsets[idx];
	  ops_out[i + 2] = (int16_t) ((off >> 16) & 0xffff);
	  ops_out[i + 3] = (int16_t) (off & 0xffff);
	}
	i += 8;  /* 2 texels */
	break;
      }
      case HB_GPU_PAINT_OP_LAYER_GRADIENT:
      {
	/* Two sub-blob references: clip at [i+2,i+3], gradient
	 * params at [i+4,i+5]. */
	for (unsigned slot = 0; slot < 2; slot++)
	{
	  unsigned p = i + 2 + slot * 2;
	  unsigned idx = ((uint16_t) ops_out[p] << 16)
		       |  (uint16_t) ops_out[p + 1];
	  if (idx < sub_offsets_count)
	  {
	    unsigned off = sub_offsets[idx];
	    ops_out[p]     = (int16_t) ((off >> 16) & 0xffff);
	    ops_out[p + 1] = (int16_t) (off & 0xffff);
	  }
	}
	i += 8;  /* 2 texels */
	break;
      }
      case HB_GPU_PAINT_OP_PUSH_GROUP:
      case HB_GPU_PAINT_OP_POP_GROUP:
	i += 4;  /* 1 texel */
	break;
      default:
	hb_free (buf);
	return nullptr;
    }
  }

  /* Sub-payloads: concat in recorded order. */
  char *dst = (char *) buf + header_texels * texel_bytes
			+ paint->ops.length * 2;
  for (hb_blob_t *b : paint->sub_blobs)
  {
    unsigned len = hb_blob_get_length (b);
    hb_memcpy (dst, hb_blob_get_data (b, nullptr), len);
    dst += len;
  }

  if (extents)
  {
    extents->x_bearing = paint->ext_min_x;
    extents->y_bearing = paint->ext_max_y;
    extents->width     = paint->ext_max_x - paint->ext_min_x;
    extents->height    = paint->ext_min_y - paint->ext_max_y;
  }

  return hb_blob_create ((const char *) buf, total_bytes,
			 HB_MEMORY_MODE_WRITABLE,
			 buf, hb_free);
}

/**
 * hb_gpu_paint_clear:
 * @paint: a GPU color-glyph paint encoder
 *
 * Discards accumulated paint state so @paint can be reused for
 * another encode.  User configuration (font scale) is preserved.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_paint_clear (hb_gpu_paint_t *paint)
{
  paint->ops.resize (0);
  paint->num_ops = 0;
  for (hb_blob_t *b : paint->sub_blobs)
    hb_blob_destroy (b);
  paint->sub_blobs.resize (0);
  paint->pending_clip = false;
  paint->unsupported = false;
  paint->ext_min_x =  0x7fffffff;
  paint->ext_min_y =  0x7fffffff;
  paint->ext_max_x = -0x7fffffff;
  paint->ext_max_y = -0x7fffffff;
}

/**
 * hb_gpu_paint_reset:
 * @paint: a GPU color-glyph paint encoder
 *
 * Resets @paint, discarding accumulated state and user
 * configuration.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_paint_reset (hb_gpu_paint_t *paint)
{
  paint->x_scale = 0;
  paint->y_scale = 0;
  paint->foreground = HB_COLOR (0, 0, 0, 0xff);
  paint->palette = 0;
  hb_map_destroy (paint->custom_palette);
  paint->custom_palette = nullptr;
  hb_gpu_paint_clear (paint);
}

/**
 * hb_gpu_paint_recycle_blob:
 * @paint: a GPU color-glyph paint encoder
 * @blob: (transfer full): a blob previously returned by hb_gpu_paint_encode()
 *
 * Returns a blob to the encoder for potential reuse.  The caller
 * transfers ownership of @blob.
 *
 * The current implementation simply destroys the blob.  A future
 * version may reclaim the underlying buffer.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_paint_recycle_blob (hb_gpu_paint_t *paint HB_UNUSED,
			   hb_blob_t      *blob)
{
  hb_blob_destroy (blob);
}


/**
 * hb_gpu_paint_shader_source:
 * @stage: pipeline stage (vertex or fragment)
 * @lang: shader language variant
 *
 * Returns the paint-renderer-specific shader source for the
 * specified stage and language.  The returned string is static
 * and must not be freed.
 *
 * This source assumes both the shared helpers
 * (hb_gpu_shader_source()) and the draw-renderer helpers
 * (hb_gpu_draw_shader_source()) are concatenated ahead of it --
 * the paint interpreter invokes hb_gpu_draw() to compute
 * clip-glyph coverage.  Full assembly:
 *
 *   [#version] + hb_gpu_shader_source ()
 *             + hb_gpu_draw_shader_source ()
 *             + hb_gpu_paint_shader_source ()
 *             + caller's main ()
 *
 * Only GLSL fragment is implemented today; other languages and
 * the vertex stage return the empty string so callers can
 * concatenate unconditionally.
 *
 * Return value: (transfer none):
 * A shader source string, or `NULL` if @stage or @lang is
 * unsupported.
 *
 * XSince: REPLACEME
 **/
#include "hb-gpu-paint-fragment-glsl.hh"
#include "hb-gpu-paint-fragment-msl.hh"
#include "hb-gpu-paint-fragment-wgsl.hh"
#include "hb-gpu-paint-fragment-hlsl.hh"

const char *
hb_gpu_paint_shader_source (hb_gpu_shader_stage_t stage,
			    hb_gpu_shader_lang_t  lang)
{
  switch (stage) {
  case HB_GPU_SHADER_STAGE_FRAGMENT:
    switch (lang) {
    case HB_GPU_SHADER_LANG_GLSL: return hb_gpu_paint_fragment_glsl;
    case HB_GPU_SHADER_LANG_MSL:  return hb_gpu_paint_fragment_msl;
    case HB_GPU_SHADER_LANG_WGSL: return hb_gpu_paint_fragment_wgsl;
    case HB_GPU_SHADER_LANG_HLSL: return hb_gpu_paint_fragment_hlsl;
    default: return nullptr;
    }
  case HB_GPU_SHADER_STAGE_VERTEX:
    switch (lang) {
    case HB_GPU_SHADER_LANG_GLSL:
    case HB_GPU_SHADER_LANG_MSL:
    case HB_GPU_SHADER_LANG_WGSL:
    case HB_GPU_SHADER_LANG_HLSL: return "";
    default: return nullptr;
    }
  default:
    return nullptr;
  }
}
