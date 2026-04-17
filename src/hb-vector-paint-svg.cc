/*
 * Copyright © 2026  Behdad Esfahbod
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

#include "hb-vector-paint.hh"
#include "hb-paint.hh"
#include "hb-vector-path.hh"

#include <math.h>

static void
hb_vector_svg_paint_append_global_transform_prefix (hb_vector_paint_t *paint, hb_vector_t<char> *buf)
{
  /* Skip when the paint's own transform is identity -- each
   * instance's <use> transform already divides by scale_factor,
   * so wrapping the body in another /scale_factor matrix would
   * double-scale.  Only emit when the user has set a non-
   * identity paint-level transform. */
  if (paint->transform.xx == 1.f && paint->transform.yx == 0.f &&
      paint->transform.xy == 0.f && paint->transform.yy == 1.f &&
      paint->transform.x0 == 0.f && paint->transform.y0 == 0.f)
    return;

  unsigned sprec = hb_vector_scale_precision (paint->precision);
  hb_buf_append_str (buf, "<g transform=\"matrix(");
  hb_buf_append_num (buf, paint->transform.xx, sprec, true);
  hb_buf_append_c (buf, ',');
  hb_buf_append_num (buf, paint->transform.yx, sprec, true);
  hb_buf_append_c (buf, ',');
  hb_buf_append_num (buf, paint->transform.xy, sprec, true);
  hb_buf_append_c (buf, ',');
  hb_buf_append_num (buf, paint->transform.yy, sprec, true);
  hb_buf_append_c (buf, ',');
  hb_buf_append_num (buf, paint->transform.x0, paint->precision);
  hb_buf_append_c (buf, ',');
  hb_buf_append_num (buf, paint->transform.y0, paint->precision);
  hb_buf_append_str (buf, ")\">\n");
}

static void
hb_vector_svg_paint_append_global_transform_suffix (hb_vector_paint_t *paint, hb_vector_t<char> *buf)
{
  if (paint->transform.xx == 1.f && paint->transform.yx == 0.f &&
      paint->transform.xy == 0.f && paint->transform.yy == 1.f &&
      paint->transform.x0 == 0.f && paint->transform.y0 == 0.f)
    return;
  hb_buf_append_str (buf, "</g>\n");
}


static hb_bool_t
hb_vector_get_color_stops (hb_vector_paint_t *paint,
                        hb_color_line_t *color_line,
                        hb_vector_t<hb_color_stop_t> *stops)
{
  unsigned len = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  if (unlikely (!len))
  {
    stops->length = 0;
    return false;
  }
  if (unlikely (!stops->resize (len)))
    return false;
  hb_color_line_get_color_stops (color_line, 0, &len, stops->arrayZ);
  stops->length = len;
  if (unlikely (!len))
    return false;

  for (unsigned i = 0; i < len; i++)
    if (stops->arrayZ[i].is_foreground)
      stops->arrayZ[i].color = HB_COLOR (hb_color_get_blue (paint->foreground),
                                         hb_color_get_green (paint->foreground),
                                         hb_color_get_red (paint->foreground),
                                         (unsigned) hb_color_get_alpha (stops->arrayZ[i].color) *
                                         hb_color_get_alpha (paint->foreground) / 255);
  return true;
}

static const char *
hb_vector_svg_extend_mode_str (hb_paint_extend_t ext)
{
  switch (ext)
  {
    case HB_PAINT_EXTEND_PAD: return "pad";
    case HB_PAINT_EXTEND_REPEAT: return "repeat";
    case HB_PAINT_EXTEND_REFLECT: return "reflect";
    default: return "pad";
  }
}

static void
hb_vector_svg_emit_color_stops (hb_vector_paint_t *paint,
                         hb_vector_t<char> *buf,
                         hb_vector_t<hb_color_stop_t> *stops)
{
  for (unsigned i = 0; i < stops->length; i++)
  {
    hb_color_t c = stops->arrayZ[i].color;
    hb_buf_append_str (buf, "<stop offset=\"");
    hb_buf_append_num (buf, stops->arrayZ[i].offset, 4);
    hb_buf_append_str (buf, "\" stop-color=\"rgb(");
    hb_buf_append_unsigned (buf, hb_color_get_red (c));
    hb_buf_append_c (buf, ',');
    hb_buf_append_unsigned (buf, hb_color_get_green (c));
    hb_buf_append_c (buf, ',');
    hb_buf_append_unsigned (buf, hb_color_get_blue (c));
    hb_buf_append_str (buf, ")\"");
    if (hb_color_get_alpha (c) != 255)
    {
      hb_buf_append_str (buf, " stop-opacity=\"");
      hb_buf_append_num (buf, hb_color_get_alpha (c) / 255.f, 4);
      hb_buf_append_c (buf, '"');
    }
    hb_buf_append_str (buf, "/>\n");
  }
}

static const char *
hb_vector_svg_composite_mode_str (hb_paint_composite_mode_t mode)
{
  switch (mode)
  {
    case HB_PAINT_COMPOSITE_MODE_CLEAR:
    case HB_PAINT_COMPOSITE_MODE_SRC:
    case HB_PAINT_COMPOSITE_MODE_DEST:
    case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
    case HB_PAINT_COMPOSITE_MODE_SRC_IN:
    case HB_PAINT_COMPOSITE_MODE_DEST_IN:
    case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
    case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
    case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
    case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
    case HB_PAINT_COMPOSITE_MODE_XOR:
    case HB_PAINT_COMPOSITE_MODE_PLUS:
      return nullptr;
    case HB_PAINT_COMPOSITE_MODE_SRC_OVER: return "normal";
    case HB_PAINT_COMPOSITE_MODE_SCREEN: return "screen";
    case HB_PAINT_COMPOSITE_MODE_OVERLAY: return "overlay";
    case HB_PAINT_COMPOSITE_MODE_DARKEN: return "darken";
    case HB_PAINT_COMPOSITE_MODE_LIGHTEN: return "lighten";
    case HB_PAINT_COMPOSITE_MODE_COLOR_DODGE: return "color-dodge";
    case HB_PAINT_COMPOSITE_MODE_COLOR_BURN: return "color-burn";
    case HB_PAINT_COMPOSITE_MODE_HARD_LIGHT: return "hard-light";
    case HB_PAINT_COMPOSITE_MODE_SOFT_LIGHT: return "soft-light";
    case HB_PAINT_COMPOSITE_MODE_DIFFERENCE: return "difference";
    case HB_PAINT_COMPOSITE_MODE_EXCLUSION: return "exclusion";
    case HB_PAINT_COMPOSITE_MODE_MULTIPLY: return "multiply";
    case HB_PAINT_COMPOSITE_MODE_HSL_HUE: return "hue";
    case HB_PAINT_COMPOSITE_MODE_HSL_SATURATION: return "saturation";
    case HB_PAINT_COMPOSITE_MODE_HSL_COLOR: return "color";
    case HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY: return "luminosity";
    default: return nullptr;
  }
}

struct hb_vector_svg_point_t { float x, y; };
struct hb_vector_svg_rgba_t { float r, g, b, a; };

static inline float
hb_vector_svg_lerp (float a, float b, float t)
{ return a + (b - a) * t; }

static inline float
hb_vector_svg_clamp01 (float v)
{
  if (v < 0.f) return 0.f;
  if (v > 1.f) return 1.f;
  return v;
}

static inline hb_vector_svg_rgba_t
hb_vector_svg_rgba_from_hb_color (hb_color_t c)
{
  return {(float) hb_color_get_red (c) / 255.f,
          (float) hb_color_get_green (c) / 255.f,
          (float) hb_color_get_blue (c) / 255.f,
          (float) hb_color_get_alpha (c) / 255.f};
}

static inline hb_color_t
hb_vector_svg_hb_color_from_rgba (const hb_vector_svg_rgba_t &c)
{
  unsigned r = (unsigned) roundf (hb_vector_svg_clamp01 (c.r) * 255.f);
  unsigned g = (unsigned) roundf (hb_vector_svg_clamp01 (c.g) * 255.f);
  unsigned b = (unsigned) roundf (hb_vector_svg_clamp01 (c.b) * 255.f);
  unsigned a = (unsigned) roundf (hb_vector_svg_clamp01 (c.a) * 255.f);
  return HB_COLOR (b, g, r, a);
}

static inline hb_vector_svg_rgba_t
hb_vector_svg_lerp_rgba (const hb_vector_svg_rgba_t &c0,
                  const hb_vector_svg_rgba_t &c1,
                  float t)
{
  return {hb_vector_svg_lerp (c0.r, c1.r, t),
          hb_vector_svg_lerp (c0.g, c1.g, t),
          hb_vector_svg_lerp (c0.b, c1.b, t),
          hb_vector_svg_lerp (c0.a, c1.a, t)};
}

static inline float hb_vector_svg_dot (const hb_vector_svg_point_t &p, const hb_vector_svg_point_t &q) { return p.x * q.x + p.y * q.y; }
static inline hb_vector_svg_point_t hb_vector_svg_add (const hb_vector_svg_point_t &p, const hb_vector_svg_point_t &q) { return {p.x + q.x, p.y + q.y}; }
static inline hb_vector_svg_point_t hb_vector_svg_sub (const hb_vector_svg_point_t &p, const hb_vector_svg_point_t &q) { return {p.x - q.x, p.y - q.y}; }
static inline hb_vector_svg_point_t hb_vector_svg_scale (const hb_vector_svg_point_t &p, float f) { return {p.x * f, p.y * f}; }

static inline hb_vector_svg_point_t
hb_vector_svg_normalize (const hb_vector_svg_point_t &p)
{
  float len = sqrtf (hb_vector_svg_dot (p, p));
  if (len == 0.f) return {0.f, 0.f};
  return hb_vector_svg_scale (p, 1.f / len);
}

static void
hb_vector_svg_add_sweep_patch (hb_vector_t<char> *body,
                        unsigned precision,
                        float cx, float cy, float radius,
                        float a0, const hb_vector_svg_rgba_t &c0_in,
                        float a1, const hb_vector_svg_rgba_t &c1_in)
{
  static const float max_angle = HB_PI / 16.f;
  hb_vector_svg_point_t center = {cx, cy};
  int num_splits = (int) ceilf (fabsf (a1 - a0) / max_angle);
  if (num_splits < 1) num_splits = 1;

  hb_vector_svg_point_t p0 = {cosf (a0), sinf (a0)};
  hb_vector_svg_rgba_t color0 = c0_in;

  for (int a = 0; a < num_splits; a++)
  {
    float k = (a + 1.f) / num_splits;
    float angle1 = hb_vector_svg_lerp (a0, a1, k);
    hb_vector_svg_rgba_t color1 = hb_vector_svg_lerp_rgba (c0_in, c1_in, k);

    hb_vector_svg_point_t p1 = {cosf (angle1), sinf (angle1)};
    hb_vector_svg_point_t sp0 = hb_vector_svg_add (center, hb_vector_svg_scale (p0, radius));
    hb_vector_svg_point_t sp1 = hb_vector_svg_add (center, hb_vector_svg_scale (p1, radius));

    hb_vector_svg_point_t A = hb_vector_svg_normalize (hb_vector_svg_add (p0, p1));
    hb_vector_svg_point_t U = {-A.y, A.x};
    float up0 = hb_vector_svg_dot (U, p0);
    float up1 = hb_vector_svg_dot (U, p1);
    if (fabsf (up0) < 1e-6f || fabsf (up1) < 1e-6f)
    {
      p0 = p1;
      color0 = color1;
      continue;
    }
    hb_vector_svg_point_t C0 = hb_vector_svg_add (A, hb_vector_svg_scale (U, hb_vector_svg_dot (hb_vector_svg_sub (p0, A), p0) / up0));
    hb_vector_svg_point_t C1 = hb_vector_svg_add (A, hb_vector_svg_scale (U, hb_vector_svg_dot (hb_vector_svg_sub (p1, A), p1) / up1));

    hb_vector_svg_point_t sc0 = hb_vector_svg_add (center, hb_vector_svg_scale (hb_vector_svg_add (C0, hb_vector_svg_scale (hb_vector_svg_sub (C0, p0), 0.33333f)), radius));
    hb_vector_svg_point_t sc1 = hb_vector_svg_add (center, hb_vector_svg_scale (hb_vector_svg_add (C1, hb_vector_svg_scale (hb_vector_svg_sub (C1, p1), 0.33333f)), radius));

    hb_vector_svg_rgba_t mid_color = hb_vector_svg_lerp_rgba (color0, color1, 0.5f);
    hb_color_t mid = hb_vector_svg_hb_color_from_rgba (mid_color);

    hb_buf_append_str (body, "<path d=\"M");
    hb_buf_append_num (body, center.x, precision);
    hb_buf_append_c (body, ',');
    hb_buf_append_num (body, center.y, precision);
    hb_buf_append_str (body, "L");
    hb_buf_append_num (body, sp0.x, precision);
    hb_buf_append_c (body, ',');
    hb_buf_append_num (body, sp0.y, precision);
    hb_buf_append_str (body, "C");
    hb_buf_append_num (body, sc0.x, precision);
    hb_buf_append_c (body, ',');
    hb_buf_append_num (body, sc0.y, precision);
    hb_buf_append_c (body, ' ');
    hb_buf_append_num (body, sc1.x, precision);
    hb_buf_append_c (body, ',');
    hb_buf_append_num (body, sc1.y, precision);
    hb_buf_append_c (body, ' ');
    hb_buf_append_num (body, sp1.x, precision);
    hb_buf_append_c (body, ',');
    hb_buf_append_num (body, sp1.y, precision);
    hb_buf_append_str (body, "Z\" fill=\"");
    hb_buf_append_color (body, mid, true);
    hb_buf_append_str (body, "\"/>\n");

    p0 = p1;
    color0 = color1;
  }
}

/* Callback context + trampoline for hb_paint_sweep_gradient_tiles. */
struct hb_vector_svg_sweep_ctx_t {
  hb_vector_t<char> *body;
  unsigned precision;
  float cx, cy, radius;
};

static void
hb_vector_svg_sweep_emit_patch (float a0, hb_color_t c0,
				float a1, hb_color_t c1,
				void *user_data)
{
  auto *ctx = (hb_vector_svg_sweep_ctx_t *) user_data;
  hb_vector_svg_add_sweep_patch (ctx->body, ctx->precision,
				 ctx->cx, ctx->cy, ctx->radius,
				 a0, hb_vector_svg_rgba_from_hb_color (c0),
				 a1, hb_vector_svg_rgba_from_hb_color (c1));
}


static void hb_vector_paint_push_transform (hb_paint_funcs_t *, void *,
                                            float, float, float, float, float, float,
                                            void *);
static void hb_vector_paint_pop_transform (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_push_clip_glyph (hb_paint_funcs_t *, void *, hb_codepoint_t, hb_font_t *, void *);
static void hb_vector_paint_push_clip_rectangle (hb_paint_funcs_t *, void *, float, float, float, float, void *);
static hb_draw_funcs_t * hb_vector_paint_push_clip_path_start (hb_paint_funcs_t *, void *, void **, void *);
static void hb_vector_paint_push_clip_path_end (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_pop_clip (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_color (hb_paint_funcs_t *, void *, hb_bool_t, hb_color_t, void *);
static hb_bool_t hb_vector_paint_image (hb_paint_funcs_t *, void *, hb_blob_t *, unsigned, unsigned, hb_tag_t, float, hb_glyph_extents_t *, void *);
static void hb_vector_paint_linear_gradient (hb_paint_funcs_t *, void *, hb_color_line_t *, float, float, float, float, float, float, void *);
static void hb_vector_paint_radial_gradient (hb_paint_funcs_t *, void *, hb_color_line_t *, float, float, float, float, float, float, void *);
static void hb_vector_paint_sweep_gradient (hb_paint_funcs_t *, void *, hb_color_line_t *, float, float, float, float, void *);
static void hb_vector_paint_push_group (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_pop_group (hb_paint_funcs_t *, void *, hb_paint_composite_mode_t, void *);
static hb_bool_t hb_vector_paint_color_glyph (hb_paint_funcs_t *, void *, hb_codepoint_t, hb_font_t *, void *);
static hb_bool_t
hb_vector_paint_custom_palette_color (hb_paint_funcs_t *pfuncs HB_UNUSED,
                                      void *paint_data,
                                      unsigned color_index,
                                      hb_color_t *color,
                                      void *user_data HB_UNUSED)
{
  hb_vector_paint_t *paint = (hb_vector_paint_t *) paint_data;
  if (!paint || !color)
    return false;

  hb_color_t *value = nullptr;
  if (!paint->custom_palette_colors.has (color_index, &value) || !value)
    return false;
  *color = *value;
  return true;
}

static inline void free_static_vector_paint_funcs ();
static struct hb_vector_paint_funcs_lazy_loader_t
  : hb_paint_funcs_lazy_loader_t<hb_vector_paint_funcs_lazy_loader_t>
{
  static hb_paint_funcs_t *create ()
  {
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
    hb_paint_funcs_set_push_transform_func (funcs, (hb_paint_push_transform_func_t) hb_vector_paint_push_transform, nullptr, nullptr);
    hb_paint_funcs_set_pop_transform_func (funcs, (hb_paint_pop_transform_func_t) hb_vector_paint_pop_transform, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, (hb_paint_push_clip_glyph_func_t) hb_vector_paint_push_clip_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, (hb_paint_push_clip_rectangle_func_t) hb_vector_paint_push_clip_rectangle, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_path_start_func (funcs, (hb_paint_push_clip_path_start_func_t) hb_vector_paint_push_clip_path_start, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_path_end_func (funcs, (hb_paint_push_clip_path_end_func_t) hb_vector_paint_push_clip_path_end, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func (funcs, (hb_paint_pop_clip_func_t) hb_vector_paint_pop_clip, nullptr, nullptr);
    hb_paint_funcs_set_color_func (funcs, (hb_paint_color_func_t) hb_vector_paint_color, nullptr, nullptr);
    hb_paint_funcs_set_image_func (funcs, (hb_paint_image_func_t) hb_vector_paint_image, nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func (funcs, (hb_paint_linear_gradient_func_t) hb_vector_paint_linear_gradient, nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func (funcs, (hb_paint_radial_gradient_func_t) hb_vector_paint_radial_gradient, nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func (funcs, (hb_paint_sweep_gradient_func_t) hb_vector_paint_sweep_gradient, nullptr, nullptr);
    hb_paint_funcs_set_push_group_func (funcs, (hb_paint_push_group_func_t) hb_vector_paint_push_group, nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func (funcs, (hb_paint_pop_group_func_t) hb_vector_paint_pop_group, nullptr, nullptr);
    hb_paint_funcs_set_color_glyph_func (funcs, (hb_paint_color_glyph_func_t) hb_vector_paint_color_glyph, nullptr, nullptr);
    hb_paint_funcs_set_custom_palette_color_func (funcs, (hb_paint_custom_palette_color_func_t) hb_vector_paint_custom_palette_color, nullptr, nullptr);
    hb_paint_funcs_make_immutable (funcs);
    hb_atexit (free_static_vector_paint_funcs);
    return funcs;
  }
} static_vector_paint_funcs;

static inline void
free_static_vector_paint_funcs ()
{
  static_vector_paint_funcs.free_instance ();
}

hb_paint_funcs_t *
hb_vector_paint_svg_funcs_get ()
{
  return static_vector_paint_funcs.get_unconst ();
}


static void
hb_vector_paint_push_transform (hb_paint_funcs_t *,
                                void *paint_data,
                                float xx, float yx,
                                float xy, float yy,
                                float dx, float dy,
                                void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  if (unlikely (paint->transform_group_overflow_depth))
  {
    paint->transform_group_overflow_depth++;
    return;
  }
  if (unlikely (paint->transform_group_depth >= 64))
  {
    paint->transform_group_overflow_depth = 1;
    return;
  }

  hb_bool_t opened =
    !(fabsf (xx - 1.f) < 1e-6f && fabsf (yx) < 1e-6f &&
      fabsf (xy) < 1e-6f && fabsf (yy - 1.f) < 1e-6f &&
      fabsf (dx) < 1e-6f && fabsf (dy) < 1e-6f);
  paint->transform_group_open_mask = (paint->transform_group_open_mask << 1) | (opened ? 1ull : 0ull);
  paint->transform_group_depth++;

  if (!opened)
    return;

  auto &body = paint->current_body ();
  unsigned sprec = hb_vector_scale_precision (paint->precision);
  hb_buf_append_str (&body, "<g transform=\"matrix(");
  hb_buf_append_num (&body, xx, sprec, true);
  hb_buf_append_c (&body, ',');
  hb_buf_append_num (&body, yx, sprec, true);
  hb_buf_append_c (&body, ',');
  hb_buf_append_num (&body, xy, sprec, true);
  hb_buf_append_c (&body, ',');
  hb_buf_append_num (&body, yy, sprec, true);
  hb_buf_append_c (&body, ',');
  hb_buf_append_num (&body, dx, paint->precision);
  hb_buf_append_c (&body, ',');
  hb_buf_append_num (&body, dy, paint->precision);
  hb_buf_append_str (&body, ")\">\n");
}

static void
hb_vector_paint_pop_transform (hb_paint_funcs_t *,
                               void *paint_data,
                               void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  if (unlikely (paint->transform_group_overflow_depth))
  {
    paint->transform_group_overflow_depth--;
    return;
  }
  if (!paint->transform_group_depth)
    return;
  paint->transform_group_depth--;
  hb_bool_t opened = !!(paint->transform_group_open_mask & 1ull);
  paint->transform_group_open_mask >>= 1;
  if (opened)
    hb_buf_append_str (&paint->current_body (), "</g>\n");
}

static void
hb_vector_paint_push_clip_glyph (hb_paint_funcs_t *,
                                 void *paint_data,
                                 hb_codepoint_t glyph,
                                 hb_font_t *font,
                                 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  const char *pfx = paint->id_prefix.arrayZ;
  unsigned pfx_len = paint->id_prefix.length;

  if (!hb_set_has (paint->defined_outlines, glyph))
  {
    hb_set_add (paint->defined_outlines, glyph);
    paint->path.clear ();
    hb_vector_path_sink_t sink = {&paint->path, paint->precision, 1.f, 1.f};
    hb_font_draw_glyph (font, glyph, hb_vector_svg_path_draw_funcs_get (), &sink);
    hb_buf_append_str (&paint->defs, "<path id=\"");
    hb_buf_append_len (&paint->defs, pfx, pfx_len);
    hb_buf_append_c  (&paint->defs, 'p');
    hb_buf_append_unsigned (&paint->defs, glyph);
    hb_buf_append_str (&paint->defs, "\" d=\"");
    hb_buf_append_len (&paint->defs, paint->path.arrayZ, paint->path.length);
    hb_buf_append_str (&paint->defs, "\"/>\n");
  }

  if (!hb_set_has (paint->defined_clips, glyph))
  {
    hb_set_add (paint->defined_clips, glyph);
    hb_buf_append_str (&paint->defs, "<clipPath id=\"");
    hb_buf_append_len (&paint->defs, pfx, pfx_len);
    hb_buf_append_str (&paint->defs, "clip-g");
    hb_buf_append_unsigned (&paint->defs, glyph);
    hb_buf_append_str (&paint->defs, "\"><use href=\"#");
    hb_buf_append_len (&paint->defs, pfx, pfx_len);
    hb_buf_append_c  (&paint->defs, 'p');
    hb_buf_append_unsigned (&paint->defs, glyph);
    hb_buf_append_str (&paint->defs, "\"/></clipPath>\n");
  }

  hb_buf_append_str (&paint->current_body (), "<g clip-path=\"url(#");
  hb_buf_append_len (&paint->current_body (), pfx, pfx_len);
  hb_buf_append_str (&paint->current_body (), "clip-g");
  hb_buf_append_unsigned (&paint->current_body (), glyph);
  hb_buf_append_str (&paint->current_body (), ")\">\n");
}

static void
hb_vector_paint_push_clip_rectangle (hb_paint_funcs_t *,
                                     void *paint_data,
                                     float xmin, float ymin,
                                     float xmax, float ymax,
                                     void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  const char *pfx = paint->id_prefix.arrayZ;
  unsigned pfx_len = paint->id_prefix.length;
  unsigned clip_id = paint->clip_rect_counter++;
  hb_buf_append_str (&paint->defs, "<clipPath id=\"");
  hb_buf_append_len (&paint->defs, pfx, pfx_len);
  hb_buf_append_c  (&paint->defs, 'c');
  hb_buf_append_unsigned (&paint->defs, clip_id);
  hb_buf_append_str (&paint->defs, "\"><rect x=\"");
  hb_buf_append_num (&paint->defs, xmin, paint->precision);
  hb_buf_append_str (&paint->defs, "\" y=\"");
  hb_buf_append_num (&paint->defs, ymin, paint->precision);
  hb_buf_append_str (&paint->defs, "\" width=\"");
  hb_buf_append_num (&paint->defs, xmax - xmin, paint->precision);
  hb_buf_append_str (&paint->defs, "\" height=\"");
  hb_buf_append_num (&paint->defs, ymax - ymin, paint->precision);
  hb_buf_append_str (&paint->defs, "\"/></clipPath>\n");

  hb_buf_append_str (&paint->current_body (), "<g clip-path=\"url(#");
  hb_buf_append_len (&paint->current_body (), pfx, pfx_len);
  hb_buf_append_c  (&paint->current_body (), 'c');
  hb_buf_append_unsigned (&paint->current_body (), clip_id);
  hb_buf_append_str (&paint->current_body (), ")\">\n");
}

static hb_draw_funcs_t *
hb_vector_paint_push_clip_path_start (hb_paint_funcs_t *,
                                      void *paint_data,
                                      void **draw_data,
                                      void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
  {
    *draw_data = nullptr;
    return nullptr;
  }

  paint->path.clear ();
  paint->clip_path_sink = {&paint->path, paint->precision,
			   paint->x_scale_factor,
			   paint->y_scale_factor};
  *draw_data = &paint->clip_path_sink;
  return hb_vector_svg_path_draw_funcs_get ();
}

static void
hb_vector_paint_push_clip_path_end (hb_paint_funcs_t *,
                                    void *paint_data,
                                    void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  const char *pfx = paint->id_prefix.arrayZ;
  unsigned pfx_len = paint->id_prefix.length;
  unsigned clip_id = paint->clip_path_counter++;

  /* The accumulated path is in font Y-up coords (the
   * convention used inside per-glyph <use scale(_,-sy)>
   * wrappers); this clip is emitted at base body level
   * (free-form between glyphs), so flip its geometry inside
   * the clipPath via the path's own transform attribute,
   * keeping the body's <g clip-path> at body Y-down. */
  hb_buf_append_str (&paint->defs, "<clipPath id=\"");
  hb_buf_append_len (&paint->defs, pfx, pfx_len);
  hb_buf_append_str (&paint->defs, "cp");
  hb_buf_append_unsigned (&paint->defs, clip_id);
  hb_buf_append_str (&paint->defs, "\"><path transform=\"scale(1,-1)\" d=\"");
  hb_buf_append_len (&paint->defs, paint->path.arrayZ, paint->path.length);
  hb_buf_append_str (&paint->defs, "\"/></clipPath>\n");

  hb_buf_append_str (&paint->current_body (), "<g clip-path=\"url(#");
  hb_buf_append_len (&paint->current_body (), pfx, pfx_len);
  hb_buf_append_str (&paint->current_body (), "cp");
  hb_buf_append_unsigned (&paint->current_body (), clip_id);
  hb_buf_append_str (&paint->current_body (), ")\">\n");
}

static void
hb_vector_paint_pop_clip (hb_paint_funcs_t *,
                          void *paint_data,
                          void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  hb_buf_append_str (&paint->current_body (), "</g>\n");
}

static void
hb_vector_paint_color (hb_paint_funcs_t *,
                       void *paint_data,
                       hb_bool_t is_foreground,
                       hb_color_t color,
                       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  hb_color_t c = color;
  if (is_foreground)
    c = HB_COLOR (hb_color_get_blue (paint->foreground),
                  hb_color_get_green (paint->foreground),
                  hb_color_get_red (paint->foreground),
                  (unsigned) hb_color_get_alpha (paint->foreground) * hb_color_get_alpha (color) / 255);

  auto &body = paint->current_body ();
  hb_buf_append_str (&body, "<rect x=\"-1000000\" y=\"-1000000\" width=\"2000000\" height=\"2000000\" fill=\"");
  hb_buf_append_color (&body, c, true);
  hb_buf_append_str (&body, "\"/>\n");
}

static hb_bool_t
hb_vector_paint_image (hb_paint_funcs_t *,
                       void *paint_data,
                       hb_blob_t *image,
                       unsigned width,
                       unsigned height,
                       hb_tag_t format,
                       float slant HB_UNUSED,
                       hb_glyph_extents_t *extents,
                       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return false;

  auto &body = paint->current_body ();
  if (format == HB_TAG ('p','n','g',' '))
  {
    if (!extents || !width || !height)
      return false;

    unsigned len = 0;
    const char *png_data = hb_blob_get_data (image, &len);
    if (!png_data || !len)
      return false;

    hb_buf_append_str (&body, "<g transform=\"translate(");
    hb_buf_append_num (&body, (float) extents->x_bearing, paint->precision);
    hb_buf_append_c (&body, ',');
    hb_buf_append_num (&body, (float) extents->y_bearing, paint->precision);
    hb_buf_append_str (&body, ") scale(");
    hb_buf_append_num (&body, (float) extents->width / width, paint->precision);
    hb_buf_append_c (&body, ',');
    hb_buf_append_num (&body, (float) extents->height / height, paint->precision);
    hb_buf_append_str (&body, ")\">\n");

    hb_buf_append_str (&body, "<image href=\"data:image/png;base64,");
    hb_buf_append_base64 (&body, (const uint8_t *) png_data, len);
    hb_buf_append_str (&body, "\" width=\"");
    hb_buf_append_num (&body, (float) width, paint->precision);
    hb_buf_append_str (&body, "\" height=\"");
    hb_buf_append_num (&body, (float) height, paint->precision);
    hb_buf_append_str (&body, "\"/>\n</g>\n");

    return true;
  }

  return false;
}

static void
hb_vector_paint_linear_gradient (hb_paint_funcs_t *,
                                 void *paint_data,
                                 hb_color_line_t *color_line,
                                 float x0, float y0,
                                 float x1, float y1,
                                 float x2, float y2,
                                 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;
  if (!hb_vector_get_color_stops (paint, color_line, &stops) || !stops.length)
    return;

  /* Sort + rescale stops to [0, 1]; shift the gradient axis
   * by the original (mn, mx) so the visible gradient stays
   * put.  SVG <stop offset="..."> requires offsets in [0,1];
   * out-of-range stops would otherwise be silently clamped. */
  float mn, mx;
  hb_paint_normalize_color_line (stops.arrayZ, stops.length, &mn, &mx);

  const char *pfx = paint->id_prefix.arrayZ;
  unsigned pfx_len = paint->id_prefix.length;
  unsigned grad_id = paint->gradient_counter++;

  /* Reduce COLR's 3-anchor (P0, P1, P2) to SVG's 2-point
   * (start, end) gradient. */
  float lx0, ly0, lx1, ly1;
  hb_paint_reduce_linear_anchors (x0, y0, x1, y1, x2, y2,
				  &lx0, &ly0, &lx1, &ly1);
  float gx0 = lx0 + mn * (lx1 - lx0);
  float gy0 = ly0 + mn * (ly1 - ly0);
  float gx1 = lx0 + mx * (lx1 - lx0);
  float gy1 = ly0 + mx * (ly1 - ly0);

  hb_buf_append_str (&paint->defs, "<linearGradient id=\"");
  hb_buf_append_len (&paint->defs, pfx, pfx_len);
  hb_buf_append_str (&paint->defs, "gr");
  hb_buf_append_unsigned (&paint->defs, grad_id);
  hb_buf_append_str (&paint->defs, "\" gradientUnits=\"userSpaceOnUse\" x1=\"");
  hb_buf_append_num (&paint->defs, gx0, paint->precision);
  hb_buf_append_str (&paint->defs, "\" y1=\"");
  hb_buf_append_num (&paint->defs, gy0, paint->precision);
  hb_buf_append_str (&paint->defs, "\" x2=\"");
  hb_buf_append_num (&paint->defs, gx1, paint->precision);
  hb_buf_append_str (&paint->defs, "\" y2=\"");
  hb_buf_append_num (&paint->defs, gy1, paint->precision);
  hb_buf_append_str (&paint->defs, "\" spreadMethod=\"");
  hb_buf_append_str (&paint->defs, hb_vector_svg_extend_mode_str (hb_color_line_get_extend (color_line)));
  hb_buf_append_str (&paint->defs, "\">\n");
  hb_vector_svg_emit_color_stops (paint, &paint->defs, &stops);
  hb_buf_append_str (&paint->defs, "</linearGradient>\n");

  hb_buf_append_str (&paint->current_body (),
                     "<rect x=\"-1000000\" y=\"-1000000\" width=\"2000000\" height=\"2000000\" fill=\"url(#");
  hb_buf_append_len (&paint->current_body (), pfx, pfx_len);
  hb_buf_append_str (&paint->current_body (), "gr");
  hb_buf_append_unsigned (&paint->current_body (), grad_id);
  hb_buf_append_str (&paint->current_body (), ")\"/>\n");
}

static void
hb_vector_paint_radial_gradient (hb_paint_funcs_t *,
                                 void *paint_data,
                                 hb_color_line_t *color_line,
                                 float x0, float y0, float r0,
                                 float x1, float y1, float r1,
                                 void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;
  if (!hb_vector_get_color_stops (paint, color_line, &stops) || !stops.length)
    return;

  float mn, mx;
  hb_paint_normalize_color_line (stops.arrayZ, stops.length, &mn, &mx);

  /* Shift centers + radii by (mn, mx) along the gradient axis
   * to compensate for rescaling stops to [0, 1]. */
  float gx0 = x0 + mn * (x1 - x0);
  float gy0 = y0 + mn * (y1 - y0);
  float gr0 = r0 + mn * (r1 - r0);
  float gx1 = x0 + mx * (x1 - x0);
  float gy1 = y0 + mx * (y1 - y0);
  float gr1 = r0 + mx * (r1 - r0);

  const char *pfx = paint->id_prefix.arrayZ;
  unsigned pfx_len = paint->id_prefix.length;
  unsigned grad_id = paint->gradient_counter++;

  hb_buf_append_str (&paint->defs, "<radialGradient id=\"");
  hb_buf_append_len (&paint->defs, pfx, pfx_len);
  hb_buf_append_str (&paint->defs, "gr");
  hb_buf_append_unsigned (&paint->defs, grad_id);
  hb_buf_append_str (&paint->defs, "\" gradientUnits=\"userSpaceOnUse\" cx=\"");
  hb_buf_append_num (&paint->defs, gx1, paint->precision);
  hb_buf_append_str (&paint->defs, "\" cy=\"");
  hb_buf_append_num (&paint->defs, gy1, paint->precision);
  hb_buf_append_str (&paint->defs, "\" r=\"");
  hb_buf_append_num (&paint->defs, gr1, paint->precision);
  hb_buf_append_str (&paint->defs, "\" fx=\"");
  hb_buf_append_num (&paint->defs, gx0, paint->precision);
  hb_buf_append_str (&paint->defs, "\" fy=\"");
  hb_buf_append_num (&paint->defs, gy0, paint->precision);
  if (gr0 > 0)
  {
    hb_buf_append_str (&paint->defs, "\" fr=\"");
    hb_buf_append_num (&paint->defs, gr0, paint->precision);
  }
  hb_buf_append_str (&paint->defs, "\" spreadMethod=\"");
  hb_buf_append_str (&paint->defs, hb_vector_svg_extend_mode_str (hb_color_line_get_extend (color_line)));
  hb_buf_append_str (&paint->defs, "\">\n");
  hb_vector_svg_emit_color_stops (paint, &paint->defs, &stops);
  hb_buf_append_str (&paint->defs, "</radialGradient>\n");

  hb_buf_append_str (&paint->current_body (),
                     "<rect x=\"-1000000\" y=\"-1000000\" width=\"2000000\" height=\"2000000\" fill=\"url(#");
  hb_buf_append_len (&paint->current_body (), pfx, pfx_len);
  hb_buf_append_str (&paint->current_body (), "gr");
  hb_buf_append_unsigned (&paint->current_body (), grad_id);
  hb_buf_append_str (&paint->current_body (), ")\"/>\n");
}

static void
hb_vector_paint_sweep_gradient (hb_paint_funcs_t *,
                                void *paint_data,
                                hb_color_line_t *color_line,
                                float cx, float cy,
                                float start_angle, float end_angle,
                                void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;
  if (!hb_vector_get_color_stops (paint, color_line, &stops) || !stops.length)
    return;

  float mn, mx;
  hb_paint_normalize_color_line (stops.arrayZ, stops.length, &mn, &mx);

  /* Shift the angle range to compensate for rescaling stops
   * to [0, 1]. */
  float ga0 = start_angle + mn * (end_angle - start_angle);
  float ga1 = start_angle + mx * (end_angle - start_angle);

  hb_vector_svg_sweep_ctx_t ctx {
    &paint->current_body (), paint->precision, cx, cy, 1000000.f
  };
  hb_paint_sweep_gradient_tiles (stops.arrayZ, stops.length,
				 hb_color_line_get_extend (color_line),
				 ga0, ga1,
				 hb_vector_svg_sweep_emit_patch,
				 &ctx);
}

static void
hb_vector_paint_push_group (hb_paint_funcs_t *,
                            void *paint_data,
                            void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  if (unlikely (!paint->group_stack.push_or_fail (hb_vector_t<char> {})))
    return;
}

static void
hb_vector_paint_pop_group (hb_paint_funcs_t *,
                           void *paint_data,
                           hb_paint_composite_mode_t mode,
                           void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  if (paint->group_stack.length < 2)
    return;

  hb_vector_t<char> group = paint->group_stack.pop ();
  auto &body = paint->current_body ();

  const char *blend = hb_vector_svg_composite_mode_str (mode);
  if (blend)
  {
    hb_buf_append_str (&body, "<g style=\"mix-blend-mode:");
    hb_buf_append_str (&body, blend);
    hb_buf_append_str (&body, "\">\n");
    hb_buf_append_len (&body, group.arrayZ, group.length);
    hb_buf_append_str (&body, "</g>\n");
  }
  else
    hb_buf_append_len (&body, group.arrayZ, group.length);
}

static hb_bool_t
hb_vector_paint_color_glyph (hb_paint_funcs_t *,
                             void *paint_data,
                             hb_codepoint_t glyph,
                             hb_font_t *font,
                             void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return false;
  if (unlikely (paint->color_glyph_depth >= HB_MAX_NESTING_LEVEL))
    return false;
  if (unlikely (hb_set_has (paint->active_color_glyphs, glyph)))
    return false;

  paint->color_glyph_depth++;
  hb_set_add (paint->active_color_glyphs, glyph);

  hb_font_paint_glyph (font, glyph,
		       hb_vector_paint_svg_funcs_get (),
		       paint,
		       paint->palette,
		       paint->foreground);
  hb_set_del (paint->active_color_glyphs, glyph);
  paint->color_glyph_depth--;
  return true;
}




hb_blob_t *
hb_vector_paint_render_svg (hb_vector_paint_t *paint)
{
  if (!paint->has_extents)
    return nullptr;

  if (unlikely (!paint->ensure_initialized ()))
    return nullptr;

  hb_vector_t<char> out;
  hb_buf_recover_recycled (paint->recycled_blob, &out);
  unsigned estimated = paint->defs.length +
		       paint->group_stack.arrayZ[0].length +
		       320;
  out.alloc (estimated);
  hb_buf_append_str (&out, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"");
  hb_buf_append_num (&out, paint->extents.x, paint->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, paint->extents.y, paint->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, paint->extents.width, paint->precision);
  hb_buf_append_c (&out, ' ');
  hb_buf_append_num (&out, paint->extents.height, paint->precision);
  hb_buf_append_str (&out, "\" width=\"");
  hb_buf_append_num (&out, paint->extents.width, paint->precision);
  hb_buf_append_str (&out, "\" height=\"");
  hb_buf_append_num (&out, paint->extents.height, paint->precision);
  hb_buf_append_str (&out, "\">\n");

  if (paint->defs.length)
  {
    hb_buf_append_str (&out, "<defs>\n");
    hb_buf_append_len (&out, paint->defs.arrayZ, paint->defs.length);
    hb_buf_append_str (&out, "</defs>\n");
  }

  if (hb_color_get_alpha (paint->background))
  {
    hb_buf_append_str (&out, "<rect x=\"");
    hb_buf_append_num (&out, paint->extents.x, paint->precision);
    hb_buf_append_str (&out, "\" y=\"");
    hb_buf_append_num (&out, paint->extents.y, paint->precision);
    hb_buf_append_str (&out, "\" width=\"");
    hb_buf_append_num (&out, paint->extents.width, paint->precision);
    hb_buf_append_str (&out, "\" height=\"");
    hb_buf_append_num (&out, paint->extents.height, paint->precision);
    hb_buf_append_str (&out, "\" fill=\"rgb(");
    hb_buf_append_unsigned (&out, hb_color_get_red (paint->background));
    hb_buf_append_c (&out, ',');
    hb_buf_append_unsigned (&out, hb_color_get_green (paint->background));
    hb_buf_append_c (&out, ',');
    hb_buf_append_unsigned (&out, hb_color_get_blue (paint->background));
    hb_buf_append_str (&out, ")\"");
    if (hb_color_get_alpha (paint->background) < 255)
    {
      hb_buf_append_str (&out, " fill-opacity=\"");
      hb_buf_append_num (&out, hb_color_get_alpha (paint->background) / 255.f, 4);
      hb_buf_append_c (&out, '"');
    }
    hb_buf_append_str (&out, "/>\n");
  }

  hb_vector_svg_paint_append_global_transform_prefix (paint, &out);
  hb_buf_append_len (&out, paint->group_stack.arrayZ[0].arrayZ, paint->group_stack.arrayZ[0].length);
  hb_vector_svg_paint_append_global_transform_suffix (paint, &out);

  hb_buf_append_str (&out, "</svg>\n");

  hb_blob_t *blob = hb_buf_blob_from (&paint->recycled_blob, &out);

  hb_vector_paint_clear (paint);

  return blob;
}
