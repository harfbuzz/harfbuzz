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
				hb_vector_buf_t *buf,
				hb_vector_t<hb_color_stop_t> *stops)
{
  for (unsigned i = 0; i < stops->length; i++)
  {
    hb_color_t c = stops->arrayZ[i].color;
    buf->append_str ("<stop offset=\"");
    buf->append_num (stops->arrayZ[i].offset, 4);
    buf->append_str ("\" stop-color=\"rgb(");
    buf->append_unsigned (hb_color_get_red (c));
    buf->append_c (',');
    buf->append_unsigned (hb_color_get_green (c));
    buf->append_c (',');
    buf->append_unsigned (hb_color_get_blue (c));
    buf->append_str (")\"");
    if (hb_color_get_alpha (c) != 255)
    {
      buf->append_str (" stop-opacity=\"");
      buf->append_num (hb_color_get_alpha (c) / 255.f, 4);
      buf->append_c ('"');
    }
    buf->append_str ("/>\n");
  }
}

/* mix-blend-mode has one Porter-Duff operator besides source-over:
 * PLUS, as plus-lighter.  It is newer than the blend modes, so name
 * screen, the closest of them, first, for viewers that lack it. */
#define HB_VECTOR_SVG_BLEND_PLUS "screen;mix-blend-mode:plus-lighter"

/* The mix-blend-mode that paints a group in @mode, or nullptr if that
 * takes more than a blend mode. */
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
      return nullptr;
    case HB_PAINT_COMPOSITE_MODE_PLUS: return HB_VECTOR_SVG_BLEND_PLUS;
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
hb_vector_svg_add_sweep_patch (hb_vector_buf_t *body,
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

    body->append_str ("<path d=\"M");
    body->append_num (center.x, precision);
    body->append_c (',');
    body->append_num (center.y, precision);
    body->append_str ("L");
    body->append_num (sp0.x, precision);
    body->append_c (',');
    body->append_num (sp0.y, precision);
    body->append_str ("C");
    body->append_num (sc0.x, precision);
    body->append_c (',');
    body->append_num (sc0.y, precision);
    body->append_c (' ');
    body->append_num (sc1.x, precision);
    body->append_c (',');
    body->append_num (sc1.y, precision);
    body->append_c (' ');
    body->append_num (sp1.x, precision);
    body->append_c (',');
    body->append_num (sp1.y, precision);
    body->append_str ("Z\" fill=\"");
    body->append_svg_color (mid, true);
    body->append_str ("\"/>\n");

    p0 = p1;
    color0 = color1;
  }
}

/* Callback context + trampoline for hb_paint_sweep_gradient_tiles. */
struct hb_vector_svg_sweep_ctx_t
{
  hb_vector_buf_t *body;
  unsigned precision;
  float cx, cy, radius;
};

static void
hb_vector_svg_sweep_emit_patch (float a0, hb_color_t c0,
				float a1, hb_color_t c1,
				void *user_data)
{
  auto *ctx = (hb_vector_svg_sweep_ctx_t *) user_data;
  /* Sweep-gradient meshes are not outline-derived, so they are bounded by
   * the output document-size cap, not the outline work budget: stop once
   * the buffer trips its cap so per-gradient patch counts cannot multiply
   * with the paint-graph traversal limits of the font tables driving us. */
  if (unlikely (ctx->body->in_error ()))
    return;
  hb_vector_svg_add_sweep_patch (ctx->body, ctx->precision,
				 ctx->cx, ctx->cy, ctx->radius,
				 a0, hb_vector_svg_rgba_from_hb_color (c0),
				 a1, hb_vector_svg_rgba_from_hb_color (c1));
}


static void hb_vector_paint_push_transform (hb_paint_funcs_t *, void *,
                                            float, float, float, float, float, float,
                                            void *);
static void hb_vector_paint_pop_transform (hb_paint_funcs_t *, void *, void *);
static void hb_vector_paint_fill_glyph (hb_paint_funcs_t *, void *, hb_codepoint_t, hb_font_t *, hb_bool_t, hb_color_t, void *);
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
static inline void free_static_vector_paint_funcs ();
static struct hb_vector_paint_funcs_lazy_loader_t
  : hb_paint_funcs_lazy_loader_t<hb_vector_paint_funcs_lazy_loader_t>
{
  static hb_paint_funcs_t *create ()
  {
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
    hb_paint_funcs_set_push_transform_func (funcs, hb_vector_paint_push_transform, nullptr, nullptr);
    hb_paint_funcs_set_pop_transform_func (funcs, hb_vector_paint_pop_transform, nullptr, nullptr);
    hb_paint_funcs_set_fill_glyph_func (funcs, hb_vector_paint_fill_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, hb_vector_paint_push_clip_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, hb_vector_paint_push_clip_rectangle, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_path_start_func (funcs, hb_vector_paint_push_clip_path_start, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_path_end_func (funcs, hb_vector_paint_push_clip_path_end, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func (funcs, hb_vector_paint_pop_clip, nullptr, nullptr);
    hb_paint_funcs_set_color_func (funcs, hb_vector_paint_color, nullptr, nullptr);
    hb_paint_funcs_set_image_func (funcs, hb_vector_paint_image, nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func (funcs, hb_vector_paint_linear_gradient, nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func (funcs, hb_vector_paint_radial_gradient, nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func (funcs, hb_vector_paint_sweep_gradient, nullptr, nullptr);
    hb_paint_funcs_set_push_group_func (funcs, hb_vector_paint_push_group, nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func (funcs, hb_vector_paint_pop_group, nullptr, nullptr);
    hb_paint_funcs_set_color_glyph_func (funcs, hb_vector_paint_color_glyph, nullptr, nullptr);
    hb_paint_funcs_set_custom_palette_color_func (funcs, hb_vector_paint_custom_palette_color, nullptr, nullptr);
    hb_paint_funcs_set_set_budget_func (funcs, hb_vector_paint_set_budget_callback, nullptr, nullptr);
    hb_paint_funcs_set_get_budget_func (funcs, hb_vector_paint_get_budget_callback, nullptr, nullptr);
    hb_paint_funcs_set_get_budget_remaining_func (funcs, hb_vector_paint_get_budget_remaining_callback, nullptr, nullptr);
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
      fabsf (paint->sx (dx)) < 1e-6f && fabsf (paint->sy (dy)) < 1e-6f);
  paint->transform_group_open_mask = (paint->transform_group_open_mask << 1) | (opened ? 1ull : 0ull);
  paint->transform_group_depth++;

  if (!opened)
    return;

  auto &body = paint->current_body ();
  unsigned start = body.length;
  unsigned sprec = paint->defs.scale_precision ();
  body.append_str ("<g transform=\"matrix(");
  body.append_num (xx, sprec);
  body.append_c (',');
  body.append_num (yx, sprec);
  body.append_c (',');
  body.append_num (xy, sprec);
  body.append_c (',');
  body.append_num (yy, sprec);
  body.append_c (',');
  body.append_num (paint->sx (dx));
  body.append_c (',');
  body.append_num (paint->sy (dy));
  body.append_str (")\">\n");
  paint->scope_opened (start);
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
  if (opened && paint->scope_closed ())
    paint->current_body ().append_str ("</g>\n");
}

static void
hb_vector_paint_fill_glyph (hb_paint_funcs_t *,
                            void *paint_data,
                            hb_codepoint_t glyph,
                            hb_font_t *font,
                            hb_bool_t,
                            hb_color_t color,
                            void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  paint->path.clear ();
  /* Skip the outline extraction when the session work budget is
   * spent; an empty path keeps the document structure intact. */
  if (likely (paint->budget_remaining >= 0))
  {
    hb_vector_path_sink_t sink = {&paint->path, paint->get_precision (),
				 paint->x_scale_factor, paint->y_scale_factor,
				 &paint->budget_remaining};
    hb_font_draw_glyph (font, glyph, hb_vector_svg_path_draw_funcs_get (), &sink);
  }

  auto &body = paint->current_body ();
  body.append_str ("<path d=\"");
  body.append_len (paint->path.arrayZ, paint->path.length);
  body.append_str ("\" fill=\"");
  body.append_svg_color (color, true);
  body.append_str ("\"/>\n");
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

  const char *pfx = paint->id_prefix;
  unsigned pfx_len = paint->id_prefix_length;

  paint->path.clear ();
  /* Skip the outline extraction when the session work budget is
   * spent; an empty clip path clips everything out. */
  if (likely (paint->budget_remaining >= 0))
  {
    hb_vector_path_sink_t sink = {&paint->path, paint->get_precision (),
				 paint->x_scale_factor, paint->y_scale_factor,
				 &paint->budget_remaining};
    hb_font_draw_glyph (font, glyph, hb_vector_svg_path_draw_funcs_get (), &sink);
  }

  unsigned def_id = paint->path_def_count++;
  paint->defs.append_str ("<path id=\"");
  paint->defs.append_len (pfx, pfx_len);
  paint->defs.append_c ('p');
  paint->defs.append_unsigned (def_id);
  paint->defs.append_str ("\" d=\"");
  paint->defs.append_len (paint->path.arrayZ, paint->path.length);
  paint->defs.append_str ("\"/>\n");
  paint->defs.append_str ("<clipPath id=\"");
  paint->defs.append_len (pfx, pfx_len);
  paint->defs.append_str ("clip-p");
  paint->defs.append_unsigned (def_id);
  paint->defs.append_str ("\"><use href=\"#");
  paint->defs.append_len (pfx, pfx_len);
  paint->defs.append_c ('p');
  paint->defs.append_unsigned (def_id);
  paint->defs.append_str ("\"/></clipPath>\n");

  unsigned start = paint->current_body ().length;
  paint->current_body ().append_str ("<g clip-path=\"url(#");
  paint->current_body ().append_len (pfx, pfx_len);
  paint->current_body ().append_str ("clip-p");
  paint->current_body ().append_unsigned (def_id);
  paint->current_body ().append_str (")\">\n");
  paint->scope_opened (start);
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

  const char *pfx = paint->id_prefix;
  unsigned pfx_len = paint->id_prefix_length;
  unsigned clip_id = paint->clip_rect_counter++;
  paint->defs.append_str ("<clipPath id=\"");
  paint->defs.append_len (pfx, pfx_len);
  paint->defs.append_c ('c');
  paint->defs.append_unsigned (clip_id);
  paint->defs.append_str ("\"><rect x=\"");
  paint->defs.append_num (paint->sx (xmin));
  paint->defs.append_str ("\" y=\"");
  paint->defs.append_num (paint->sy (ymin));
  paint->defs.append_str ("\" width=\"");
  paint->defs.append_num (paint->sx (xmax - xmin));
  paint->defs.append_str ("\" height=\"");
  paint->defs.append_num (paint->sy (ymax - ymin));
  paint->defs.append_str ("\"/></clipPath>\n");

  unsigned start = paint->current_body ().length;
  paint->current_body ().append_str ("<g clip-path=\"url(#");
  paint->current_body ().append_len (pfx, pfx_len);
  paint->current_body ().append_c ('c');
  paint->current_body ().append_unsigned (clip_id);
  paint->current_body ().append_str (")\">\n");
  paint->scope_opened (start);
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
  paint->clip_path_sink = {&paint->path, paint->get_precision (),
			   paint->x_scale_factor,
			   paint->y_scale_factor,
			   &paint->budget_remaining};
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

  const char *pfx = paint->id_prefix;
  unsigned pfx_len = paint->id_prefix_length;
  unsigned clip_id = paint->clip_path_counter++;

  /* The accumulated path is in font Y-up coords, the same
   * space as the body's <g clip-path> that references it:
   * the whole body is wrapped in a single scale(1,-1) at
   * render time, so no per-clip flip here (matches the
   * glyph and rectangle clips above). */
  paint->defs.append_str ("<clipPath id=\"");
  paint->defs.append_len (pfx, pfx_len);
  paint->defs.append_str ("cp");
  paint->defs.append_unsigned (clip_id);
  paint->defs.append_str ("\"><path d=\"");
  paint->defs.append_len (paint->path.arrayZ, paint->path.length);
  paint->defs.append_str ("\"/></clipPath>\n");

  unsigned start = paint->current_body ().length;
  paint->current_body ().append_str ("<g clip-path=\"url(#");
  paint->current_body ().append_len (pfx, pfx_len);
  paint->current_body ().append_str ("cp");
  paint->current_body ().append_unsigned (clip_id);
  paint->current_body ().append_str (")\">\n");
  paint->scope_opened (start);
}

static void
hb_vector_paint_pop_clip (hb_paint_funcs_t *,
                          void *paint_data,
                          void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;
  if (paint->scope_closed ())
    paint->current_body ().append_str ("</g>\n");
}

static void
hb_vector_paint_color (hb_paint_funcs_t *,
                       void *paint_data,
                       hb_bool_t,
                       hb_color_t color,
                       void *)
{
  auto *paint = (hb_vector_paint_t *) paint_data;
  if (unlikely (!paint->ensure_initialized ()))
    return;

  auto &body = paint->current_body ();
  body.append_str ("<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" fill=\"");
  body.append_svg_color (color, true);
  body.append_str ("\"/>\n");
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

    body.append_str ("<g transform=\"translate(");
    body.append_num (paint->sx ((float) extents->x_bearing));
    body.append_c (',');
    body.append_num (paint->sy ((float) extents->y_bearing));
    body.append_str (") scale(");
    body.append_num (paint->sx ((float) extents->width) / width);
    body.append_c (',');
    body.append_num (paint->sy ((float) extents->height) / height);
    body.append_str (")\">\n");

    body.append_str ("<image href=\"data:image/png;base64,");
    body.append_base64 ((const uint8_t *) png_data, len);
    body.append_str ("\" width=\"");
    body.append_num ((float) width);
    body.append_str ("\" height=\"");
    body.append_num ((float) height);
    body.append_str ("\"/>\n</g>\n");

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

  if (!paint->fetch_color_stops (color_line))
    return;
  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;

  /* Sort + rescale stops to [0, 1]; shift the gradient axis
   * by the original (mn, mx) so the visible gradient stays
   * put.  SVG <stop offset="..."> requires offsets in [0,1];
   * out-of-range stops would otherwise be silently clamped. */
  float mn, mx;
  hb_paint_normalize_color_line (stops.arrayZ, stops.length, &mn, &mx);

  const char *pfx = paint->id_prefix;
  unsigned pfx_len = paint->id_prefix_length;
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

  paint->defs.append_str ("<linearGradient id=\"");
  paint->defs.append_len (pfx, pfx_len);
  paint->defs.append_str ("gr");
  paint->defs.append_unsigned (grad_id);
  paint->defs.append_str ("\" gradientUnits=\"userSpaceOnUse\" x1=\"");
  paint->defs.append_num (paint->sx (gx0));
  paint->defs.append_str ("\" y1=\"");
  paint->defs.append_num (paint->sy (gy0));
  paint->defs.append_str ("\" x2=\"");
  paint->defs.append_num (paint->sx (gx1));
  paint->defs.append_str ("\" y2=\"");
  paint->defs.append_num (paint->sy (gy1));
  paint->defs.append_str ("\" spreadMethod=\"");
  paint->defs.append_str (hb_vector_svg_extend_mode_str (hb_color_line_get_extend (color_line)));
  paint->defs.append_str ("\">\n");
  hb_vector_svg_emit_color_stops (paint, &paint->defs, &stops);
  paint->defs.append_str ("</linearGradient>\n");

  paint->current_body ().append_str (
                     "<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" fill=\"url(#");
  paint->current_body ().append_len (pfx, pfx_len);
  paint->current_body ().append_str ("gr");
  paint->current_body ().append_unsigned (grad_id);
  paint->current_body ().append_str (")\"/>\n");
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

  if (!paint->fetch_color_stops (color_line))
    return;
  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;

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

  const char *pfx = paint->id_prefix;
  unsigned pfx_len = paint->id_prefix_length;
  unsigned grad_id = paint->gradient_counter++;

  paint->defs.append_str ("<radialGradient id=\"");
  paint->defs.append_len (pfx, pfx_len);
  paint->defs.append_str ("gr");
  paint->defs.append_unsigned (grad_id);
  paint->defs.append_str ("\" gradientUnits=\"userSpaceOnUse\" cx=\"");
  paint->defs.append_num (paint->sx (gx1));
  paint->defs.append_str ("\" cy=\"");
  paint->defs.append_num (paint->sy (gy1));
  paint->defs.append_str ("\" r=\"");
  paint->defs.append_num (paint->sx (gr1));
  paint->defs.append_str ("\" fx=\"");
  paint->defs.append_num (paint->sx (gx0));
  paint->defs.append_str ("\" fy=\"");
  paint->defs.append_num (paint->sy (gy0));
  if (gr0 > 0)
  {
    paint->defs.append_str ("\" fr=\"");
    paint->defs.append_num (paint->sx (gr0));
  }
  paint->defs.append_str ("\" spreadMethod=\"");
  paint->defs.append_str (hb_vector_svg_extend_mode_str (hb_color_line_get_extend (color_line)));
  paint->defs.append_str ("\">\n");
  hb_vector_svg_emit_color_stops (paint, &paint->defs, &stops);
  paint->defs.append_str ("</radialGradient>\n");

  paint->current_body ().append_str (
                     "<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" fill=\"url(#");
  paint->current_body ().append_len (pfx, pfx_len);
  paint->current_body ().append_str ("gr");
  paint->current_body ().append_unsigned (grad_id);
  paint->current_body ().append_str (")\"/>\n");
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

  if (!paint->fetch_color_stops (color_line))
    return;
  hb_vector_t<hb_color_stop_t> &stops = paint->color_stops_scratch;

  float mn, mx;
  hb_paint_normalize_color_line (stops.arrayZ, stops.length, &mn, &mx);

  /* Shift the angle range to compensate for rescaling stops
   * to [0, 1]. */
  float ga0 = start_angle + mn * (end_angle - start_angle);
  float ga1 = start_angle + mx * (end_angle - start_angle);

  hb_vector_svg_sweep_ctx_t ctx {
    &paint->current_body (), paint->get_precision (), paint->sx (cx), paint->sy (cy), 32767.f
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
  paint->push_group ();
}

static void
hb_vector_svg_append_id (hb_vector_paint_t *paint,
			 hb_vector_buf_t &buf,
			 char kind, unsigned id)
{
  buf.append_len (paint->id_prefix, paint->id_prefix_length);
  buf.append_c (kind);
  buf.append_unsigned (id);
}

/* Appends @group to @buf, as a group of its own if blending inside it
 * has to be kept from what is beneath it. */
static void
hb_vector_svg_append_group (hb_vector_buf_t &buf,
			    const hb_vector_paint_group_t &group)
{
  if (group.blended)
    buf.append_str ("<g style=\"isolation:isolate\">\n");
  buf.append_len (group.arrayZ, group.length);
  if (group.blended)
    buf.append_str ("</g>\n");
}

/* Writes @group to <defs> as an element for the body and masks to
 * refer to, when it is needed more than once.  Returns its id. */
static unsigned
hb_vector_svg_define_group (hb_vector_paint_t *paint,
			    const hb_vector_paint_group_t &group)
{
  auto &defs = paint->defs;
  unsigned id = paint->group_def_count++;
  defs.append_str ("<g id=\"");
  hb_vector_svg_append_id (paint, defs, 'g', id);
  defs.append_str (group.blended ? "\" style=\"isolation:isolate\">\n" : "\">\n");
  defs.append_len (group.arrayZ, group.length);
  defs.append_str ("</g>\n");
  return id;
}

static void
hb_vector_svg_append_group_or_use (hb_vector_paint_t *paint,
				   hb_vector_buf_t &buf,
				   const hb_vector_paint_group_t *group,
				   unsigned group_id)
{
  if (group)
  {
    buf.append_len (group->arrayZ, group->length);
    return;
  }
  buf.append_str ("<use href=\"#");
  hb_vector_svg_append_id (paint, buf, 'g', group_id);
  buf.append_str ("\"/>\n");
}

/* Writes a mask to <defs> that is the alpha of a group or, with
 * @inverse, one minus it.  The group is @group, which is written into
 * the mask, or if that is null the defined group @group_id.  Returns
 * the mask's id.
 *
 * The alpha comes from a mask of mask-type alpha, but that is not the
 * mask returned.  It cannot be inverted, as no content has the inverse
 * of another's alpha.  And Chrome ignores one that paints nothing
 * when what it masks has clips of its own, where it should hide it.
 * So the alpha mask is only ever put on a rectangle, in a luminance
 * mask: white through it, or for the inverse, black through it over
 * white. */
static unsigned
hb_vector_svg_define_mask (hb_vector_paint_t *paint,
			   const hb_vector_paint_group_t *group,
			   unsigned group_id,
			   bool inverse)
{
  auto &defs = paint->defs;
  unsigned alpha_id = paint->mask_counter++;
  defs.append_str ("<mask id=\"");
  hb_vector_svg_append_id (paint, defs, 'm', alpha_id);
  defs.append_str ("\" mask-type=\"alpha\">\n");
  hb_vector_svg_append_group_or_use (paint, defs, group, group_id);
  defs.append_str ("</mask>\n");

  unsigned id = paint->mask_counter++;
  defs.append_str ("<mask id=\"");
  hb_vector_svg_append_id (paint, defs, 'm', id);
  defs.append_str ("\">\n");
  if (inverse)
    defs.append_str ("<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\" fill=\"#FFF\"/>\n");
  defs.append_str ("<rect x=\"-32767\" y=\"-32767\" width=\"65534\" height=\"65534\"");
  if (!inverse)
    defs.append_str (" fill=\"#FFF\"");
  defs.append_str (" mask=\"url(#");
  hb_vector_svg_append_id (paint, defs, 'm', alpha_id);
  defs.append_str (")\"/>\n"
		   "</mask>\n");
  return id;
}

/* Paints a group into the current one through a mask: @group, which is
 * written in place, or if that is null the defined group @group_id.
 * With @plus it is added to what is beneath it, not painted over it. */
static void
hb_vector_svg_paint_masked (hb_vector_paint_t *paint,
			    unsigned mask_id,
			    const hb_vector_paint_group_t *group,
			    unsigned group_id,
			    bool plus = false)
{
  auto &body = paint->current_body ();
  body.append_str ("<g mask=\"url(#");
  hb_vector_svg_append_id (paint, body, 'm', mask_id);
  body.append_str (plus ? ")\" style=\"mix-blend-mode:" HB_VECTOR_SVG_BLEND_PLUS "\">\n"
			: ")\">\n");
  hb_vector_svg_append_group_or_use (paint, body, group, group_id);
  body.append_str ("</g>\n");
}

/* Makes what the current group has painted so far a unit to composite
 * @src with, by closing the transforms and clips that are open in it.
 * The source was painted under them, so it is put under copies of
 * them; hb_vector_paint_t::reopen_scopes() opens them again, for what
 * is painted next. */
static void
hb_vector_svg_close_scopes (hb_vector_paint_t *paint,
			    hb_vector_paint_group_t &src,
			    hb_vector_buf_t *opening)
{
  unsigned count = paint->close_scopes ("</g>\n", opening);
  if (!count)
    return;

  hb_vector_paint_group_t under;
  under.base = src.base;
  under.referred = src.referred;
  under.blended = src.blended;
  under.append_len (opening->arrayZ, opening->length);
  under.append_len (src.arrayZ, src.length);
  for (unsigned i = 0; i < count; i++)
    under.append_str ("</g>\n");
  src = std::move (under);
}

/* Composites @src onto the current group in a Porter-Duff mode that
 * no blend mode stands for.  They all come to keeping, dropping,
 * reordering or masking the two groups:
 *
 *   CLEAR      nothing
 *   SRC        the source
 *   DEST_OVER  the source, then the backdrop
 *   SRC_IN     the source, masked by the backdrop's alpha
 *   DEST_IN    the backdrop, masked by the source's alpha
 *   SRC_OUT    the source, masked by the inverse of the backdrop's alpha
 *   DEST_OUT   the backdrop, masked by the inverse of the source's alpha
 *   SRC_ATOP   DEST_OUT plus SRC_IN
 *   DEST_ATOP  SRC_OUT plus DEST_IN
 *   XOR        DEST_OUT plus SRC_OUT
 *
 * The two parts of the last three must be added: painted one over the
 * other, the upper would hide some of the lower wherever both are
 * partly transparent.  A viewer without plus-lighter does that. */
static void
hb_vector_svg_composite (hb_vector_paint_t *paint,
			 hb_vector_paint_group_t &src,
			 hb_paint_composite_mode_t mode)
{
  /* The backdrop is all that the current group has painted, and the
   * result takes its place. */
  auto &dst = paint->current_group ();
  hb_vector_buf_t opening;
  hb_vector_svg_close_scopes (paint, src, &opening);
  if (unlikely (dst.in_error ()))
    return;
  hb_vector_paint_group_t backdrop = std::move (dst);
  dst.referred = 0;
  dst.blended = false;

  switch ((unsigned) mode)
  {
    case HB_PAINT_COMPOSITE_MODE_SRC:
      hb_vector_svg_append_group (dst, src);
      dst.referred = src.referred;
      break;

    case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
      hb_vector_svg_append_group (dst, src);
      hb_vector_svg_append_group (dst, backdrop);
      dst.referred = src.referred + backdrop.referred;
      break;

    case HB_PAINT_COMPOSITE_MODE_SRC_IN:
    case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
    {
      unsigned mask = hb_vector_svg_define_mask (paint, &backdrop, 0,
						 mode == HB_PAINT_COMPOSITE_MODE_SRC_OUT);
      hb_vector_svg_paint_masked (paint, mask, &src, 0);
      dst.referred = src.referred + backdrop.work ();
      break;
    }

    case HB_PAINT_COMPOSITE_MODE_DEST_IN:
    case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
    {
      unsigned mask = hb_vector_svg_define_mask (paint, &src, 0,
						 mode == HB_PAINT_COMPOSITE_MODE_DEST_OUT);
      hb_vector_svg_paint_masked (paint, mask, &backdrop, 0);
      dst.referred = backdrop.referred + src.work ();
      break;
    }

    case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
    case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
    case HB_PAINT_COMPOSITE_MODE_XOR:
    {
      /* The lower part is one group out of the other, and the upper
       * part the other group in it, or for XOR out of it. */
      unsigned lower = hb_vector_svg_define_group (paint, backdrop);
      unsigned upper = hb_vector_svg_define_group (paint, src);
      if (mode == HB_PAINT_COMPOSITE_MODE_DEST_ATOP)
	hb_swap (lower, upper);
      unsigned lower_mask = hb_vector_svg_define_mask (paint, nullptr, upper, true);
      unsigned upper_mask = hb_vector_svg_define_mask (paint, nullptr, lower,
						       mode == HB_PAINT_COMPOSITE_MODE_XOR);
      hb_vector_svg_paint_masked (paint, lower_mask, nullptr, lower);
      hb_vector_svg_paint_masked (paint, upper_mask, nullptr, upper, true);
      dst.referred = 2 * (src.work () + backdrop.work ());
      dst.blended = true;
      break;
    }

    case HB_PAINT_COMPOSITE_MODE_CLEAR:
    default:
      break;
  }

  paint->reopen_scopes (opening);

  if (unlikely (src.in_error () || backdrop.in_error () || paint->defs.in_error ()))
    dst.overflowed = true; /* Fails the render. */
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

  /* Close what the group left open, so that it is a unit. */
  while (paint->scope_closed ())
    paint->current_body ().append_str ("</g>\n");

  hb_vector_paint_group_t group = paint->group_stack.pop ();
  auto &body = paint->current_group ();

  if (mode == HB_PAINT_COMPOSITE_MODE_DEST)
    return; /* The source is not used. */

  /* What is not a mode is painted source-over. */
  if (unlikely ((unsigned) mode > HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY))
    mode = HB_PAINT_COMPOSITE_MODE_SRC_OVER;

  /* Three modes are written with both groups used twice.  When the
   * document cannot take that, write the mode nearest to them that
   * uses each once. */
  if ((mode == HB_PAINT_COMPOSITE_MODE_SRC_ATOP ||
       mode == HB_PAINT_COMPOSITE_MODE_DEST_ATOP ||
       mode == HB_PAINT_COMPOSITE_MODE_XOR) &&
      unlikely (!paint->affords (2 * group.work () + body.work ())))
    mode = mode == HB_PAINT_COMPOSITE_MODE_DEST_ATOP
	 ? HB_PAINT_COMPOSITE_MODE_DEST_OVER
	 : HB_PAINT_COMPOSITE_MODE_SRC_OVER;

  const char *blend = hb_vector_svg_composite_mode_str (mode);
  if (!blend)
  {
    hb_vector_svg_composite (paint, group, mode);
    return;
  }

  /* COLR composites a group onto what the enclosing group has painted
   * so far, and onto nothing else.  A <g> with a blend mode blends with
   * everything beneath it up to the nearest isolated group, and a <g> is
   * not one by default: without isolation the source would also blend
   * with earlier layers of the glyph, and with the page.  A clip is one,
   * though, and one that is open in the enclosing group would keep the
   * source from what was painted before it. */
  bool blending = mode != HB_PAINT_COMPOSITE_MODE_SRC_OVER;
  hb_vector_buf_t opening;
  if (blending)
    hb_vector_svg_close_scopes (paint, group, &opening);
  body.append_str ("<g style=\"mix-blend-mode:");
  body.append_str (blend);
  /* A blend mode other than normal isolates the group by itself. */
  if (group.blended && !blending)
    body.append_str (";isolation:isolate");
  body.append_str ("\">\n");
  body.append_len (group.arrayZ, group.length);
  body.append_str ("</g>\n");
  body.referred += group.referred;
  if (blending)
  {
    body.blended = true;
    paint->reopen_scopes (opening);
    if (unlikely (group.in_error ()))
      body.overflowed = true; /* Fails the render. */
  }
}

static hb_bool_t
hb_vector_paint_color_glyph (hb_paint_funcs_t *,
                             void *,
                             hb_codepoint_t,
                             hb_font_t *,
                             void *)
{
  /* Painting the nested glyph from here would re-enter
   * hb_font_paint_glyph(), with fresh cycle, depth, and edge limits
   * for every reference to it, so a DAG with fan-out f over n levels is
   * walked f^n times.  Decline, as hb-raster and the PDF backend do, so
   * the caller paints it inline under the limits of the current walk. */
  return false;
}




hb_blob_t *
hb_vector_paint_render_svg (hb_vector_paint_t *paint)
{
  if (!paint->has_extents)
    return nullptr;

  if (unlikely (!paint->ensure_initialized ()))
    return nullptr;

  hb_vector_buf_t out;
  hb_buf_recover_recycled (paint->recycled_blob, &out);
  unsigned estimated = paint->defs.length +
		       paint->group_stack.arrayZ[0].length +
		       320;
  out.alloc (estimated);
  float vb_x = paint->extents.x;
  float vb_y = -(paint->extents.y + paint->extents.height);
  float vb_w = paint->extents.width;
  float vb_h = paint->extents.height;
  out.append_str ("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"");
  out.append_num (vb_x);
  out.append_c (' ');
  out.append_num (vb_y);
  out.append_c (' ');
  out.append_num (vb_w);
  out.append_c (' ');
  out.append_num (vb_h);
  out.append_str ("\" width=\"");
  out.append_num (vb_w);
  out.append_str ("\" height=\"");
  out.append_num (vb_h);
  out.append_str ("\">\n");

  if (paint->defs.length)
  {
    out.append_str ("<defs>\n");
    out.append_len (paint->defs.arrayZ, paint->defs.length);
    out.append_str ("</defs>\n");
  }

  if (hb_color_get_alpha (paint->background))
  {
    out.append_str ("<rect x=\"");
    out.append_num (vb_x);
    out.append_str ("\" y=\"");
    out.append_num (vb_y);
    out.append_str ("\" width=\"");
    out.append_num (vb_w);
    out.append_str ("\" height=\"");
    out.append_num (vb_h);
    out.append_str ("\" fill=\"rgb(");
    out.append_unsigned (hb_color_get_red (paint->background));
    out.append_c (',');
    out.append_unsigned (hb_color_get_green (paint->background));
    out.append_c (',');
    out.append_unsigned (hb_color_get_blue (paint->background));
    out.append_str (")\"");
    if (hb_color_get_alpha (paint->background) < 255)
    {
      out.append_str (" fill-opacity=\"");
      out.append_num (hb_color_get_alpha (paint->background) / 255.f, 4);
      out.append_c ('"');
    }
    out.append_str ("/>\n");
  }

  /* The background is not part of what the glyphs blend with. */
  out.append_str (paint->group_stack.arrayZ[0].blended
		  ? "<g transform=\"scale(1,-1)\" style=\"isolation:isolate\">\n"
		  : "<g transform=\"scale(1,-1)\">\n");
  out.append_len (paint->group_stack.arrayZ[0].arrayZ, paint->group_stack.arrayZ[0].length);
  out.append_str ("</g>\n");

  out.append_str ("</svg>\n");

  hb_blob_t *blob = hb_buf_blob_from (&paint->recycled_blob, &out);

  return blob;
}
