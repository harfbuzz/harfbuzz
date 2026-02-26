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

#include "hb-raster-svg-use.hh"

#include "hb-raster-svg-base.hh"

#include <string.h>

static bool
svg_find_element_by_id (const hb_svg_use_context_t *ctx,
                        hb_svg_str_t id,
                        const char **found)
{
  *found = nullptr;
  if (ctx->doc_cache)
  {
    unsigned start = 0, end = 0;
    OT::SVG::svg_id_span_t key = {id.data, id.len};
    if (ctx->svg_accel->doc_cache_find_id_span (ctx->doc_cache, key, &start, &end))
    {
      if (start < ctx->doc_len && end <= ctx->doc_len && start < end)
      {
        *found = ctx->doc_start + start;
        return true;
      }
    }
  }

  hb_svg_xml_parser_t search (ctx->doc_start, ctx->doc_len);
  while (true)
  {
    hb_svg_token_type_t tok = search.next ();
    if (tok == SVG_TOKEN_EOF) break;
    if (tok != SVG_TOKEN_OPEN_TAG && tok != SVG_TOKEN_SELF_CLOSE_TAG) continue;
    hb_svg_str_t attr_id = search.find_attr ("id");
    if (attr_id.len == id.len && 0 == memcmp (attr_id.data, id.data, id.len))
    {
      *found = search.tag_start;
      return true;
    }
  }
  return false;
}

static inline bool
svg_parse_viewbox (hb_svg_str_t viewbox_str,
                   float *x, float *y, float *w, float *h)
{
  if (!viewbox_str.len)
    return false;
  hb_svg_float_parser_t vb_fp (viewbox_str);
  float vb_x = vb_fp.next_float ();
  float vb_y = vb_fp.next_float ();
  float vb_w = vb_fp.next_float ();
  float vb_h = vb_fp.next_float ();
  if (vb_w <= 0.f || vb_h <= 0.f)
    return false;
  if (x) *x = vb_x;
  if (y) *y = vb_y;
  if (w) *w = vb_w;
  if (h) *h = vb_h;
  return true;
}

static inline float
svg_align_offset (hb_svg_str_t align,
                  float leftover,
                  char axis)
{
  if (leftover <= 0.f) return 0.f;
  if ((axis == 'x' && align.starts_with ("xMin")) ||
      (axis == 'y' && (align.eq ("xMinYMin") || align.eq ("xMidYMin") || align.eq ("xMaxYMin"))))
    return 0.f;
  if ((axis == 'x' && align.starts_with ("xMax")) ||
      (axis == 'y' && (align.eq ("xMinYMax") || align.eq ("xMidYMax") || align.eq ("xMaxYMax"))))
    return leftover;
  return leftover * 0.5f;
}

static bool
svg_compute_viewbox_transform (float viewport_w,
                               float viewport_h,
                               float vb_x,
                               float vb_y,
                               float vb_w,
                               float vb_h,
                               hb_svg_str_t preserve_aspect_ratio,
                               hb_svg_transform_t *out)
{
  if (!(viewport_w > 0.f && viewport_h > 0.f && vb_w > 0.f && vb_h > 0.f))
    return false;

  hb_svg_str_t par = preserve_aspect_ratio.trim ();
  if (!par.len)
    par = hb_svg_str_t ("xMidYMid meet", 12);

  bool is_none = false;
  bool is_slice = false;
  if (svg_str_starts_with_ascii_ci (par, "none"))
    is_none = true;
  else if (par.len >= 5)
  {
    if (svg_str_starts_with_ascii_ci (par.trim (), "x"))
    {
      const char *p = par.data;
      const char *end = par.data + par.len;
      while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
      while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
      if (p < end && svg_str_starts_with_ascii_ci (hb_svg_str_t (p, (unsigned) (end - p)), "slice"))
        is_slice = true;
    }
  }

  hb_svg_transform_t t;
  if (is_none)
  {
    t.xx = viewport_w / vb_w;
    t.yy = viewport_h / vb_h;
    t.dx = -vb_x * t.xx;
    t.dy = -vb_y * t.yy;
    *out = t;
    return true;
  }

  float sx = viewport_w / vb_w;
  float sy = viewport_h / vb_h;
  float s = is_slice ? hb_max (sx, sy) : hb_min (sx, sy);
  float scaled_w = vb_w * s;
  float scaled_h = vb_h * s;
  float leftover_x = viewport_w - scaled_w;
  float leftover_y = viewport_h - scaled_h;

  hb_svg_str_t align = par.trim ();
  if (!align.starts_with ("x"))
    align = hb_svg_str_t ("xMidYMid", 8);

  t.xx = s;
  t.yy = s;
  t.dx = svg_align_offset (align, leftover_x, 'x') - vb_x * s;
  t.dy = svg_align_offset (align, leftover_y, 'y') - vb_y * s;
  *out = t;
  return true;
}

void
hb_raster_svg_render_use_element (const hb_svg_use_context_t *ctx,
                        hb_svg_xml_parser_t &parser,
                        const void *state,
                        hb_svg_str_t transform_str,
                        hb_svg_use_render_cb_t render_cb,
                        void *render_user)
{
  hb_svg_str_t href = hb_raster_svg_find_href_attr (parser);

  hb_svg_str_t ref_id;
  if (!hb_raster_svg_parse_local_id_ref (href, &ref_id, nullptr))
    return;

  float use_x = svg_parse_float (parser.find_attr ("x"));
  float use_y = svg_parse_float (parser.find_attr ("y"));
  float use_w = svg_parse_float (parser.find_attr ("width"));
  float use_h = svg_parse_float (parser.find_attr ("height"));

  bool has_translate = (use_x != 0.f || use_y != 0.f);
  bool has_use_transform = transform_str.len > 0;

  if (has_use_transform)
  {
    hb_svg_transform_t t;
    hb_raster_svg_parse_transform (transform_str, &t);
    hb_paint_push_transform (ctx->pfuncs, ctx->paint, t.xx, t.yx, t.xy, t.yy, t.dx, t.dy);
  }

  if (has_translate)
    hb_paint_push_transform (ctx->pfuncs, ctx->paint, 1, 0, 0, 1, use_x, use_y);

  const char *found = nullptr;
  svg_find_element_by_id (ctx, ref_id, &found);

  if (found)
  {
    hb_decycler_node_t node (*ctx->use_decycler);
    if (unlikely (!node.visit ((uintptr_t) found)))
      return;

    unsigned remaining = ctx->doc_start + ctx->doc_len - found;
    hb_svg_xml_parser_t ref_parser (found, remaining);
    hb_svg_token_type_t tok = ref_parser.next ();
    if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
    {
      bool has_viewport_scale = false;
      if ((ref_parser.tag_name.eq ("svg") || ref_parser.tag_name.eq ("symbol")) &&
          use_w > 0.f && use_h > 0.f)
      {
        float vb_x = 0.f, vb_y = 0.f, vb_w = 0.f, vb_h = 0.f;
        hb_svg_transform_t t;
        if (svg_parse_viewbox (ref_parser.find_attr ("viewBox"), &vb_x, &vb_y, &vb_w, &vb_h) &&
            svg_compute_viewbox_transform (use_w, use_h, vb_x, vb_y, vb_w, vb_h,
                                           ref_parser.find_attr ("preserveAspectRatio"),
                                           &t))
        {
          hb_paint_push_transform (ctx->pfuncs, ctx->paint, t.xx, t.yx, t.xy, t.yy, t.dx, t.dy);
          has_viewport_scale = true;
        }
      }
      render_cb (render_user, ref_parser, state);
      if (has_viewport_scale)
        hb_paint_pop_transform (ctx->pfuncs, ctx->paint);
    }
  }

  if (has_translate)
    hb_paint_pop_transform (ctx->pfuncs, ctx->paint);

  if (has_use_transform)
    hb_paint_pop_transform (ctx->pfuncs, ctx->paint);
}
