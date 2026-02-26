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

#include "hb-raster-svg-clip.hh"

#include "hb-raster.h"
#include "hb-raster-paint.hh"
#include "hb-raster-svg.hh"
#include "hb-raster-svg-base.hh"

#include <string.h>

static inline bool
svg_transform_is_identity (const hb_svg_transform_t &t)
{
  return t.xx == 1.f && t.yx == 0.f &&
         t.xy == 0.f && t.yy == 1.f &&
         t.dx == 0.f && t.dy == 0.f;
}

static bool
svg_parse_element_transform (hb_svg_xml_parser_t &parser,
                             hb_svg_transform_t *out)
{
  hb_svg_style_props_t style_props;
  svg_parse_style_props (parser.find_attr ("style"), &style_props);
  hb_svg_str_t transform = svg_pick_attr_or_style (parser, style_props.transform, "transform");
  if (!transform.len)
    return false;
  hb_raster_svg_parse_transform (transform, out);
  return true;
}

static bool
svg_find_element_by_id (const char *doc_start,
                        unsigned doc_len,
                        const OT::SVG::accelerator_t *svg_accel,
                        const OT::SVG::svg_doc_cache_t *doc_cache,
                        hb_svg_str_t id,
                        const char **found)
{
  *found = nullptr;
  if (!doc_start || !doc_len || !id.len)
    return false;

  if (doc_cache && svg_accel)
  {
    unsigned start = 0, end = 0;
    OT::SVG::svg_id_span_t key = {id.data, id.len};
    if (svg_accel->doc_cache_find_id_span (doc_cache, key, &start, &end))
    {
      if (start < doc_len && end <= doc_len && start < end)
      {
        *found = doc_start + start;
        return true;
      }
    }
  }

  hb_svg_xml_parser_t search (doc_start, doc_len);
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

void
hb_raster_svg_process_clip_path_def (hb_svg_defs_t *defs,
                           hb_svg_xml_parser_t &parser,
                           hb_svg_token_type_t tok,
                           const char *doc_start,
                           unsigned doc_len,
                           const OT::SVG::accelerator_t *svg_accel,
                           const OT::SVG::svg_doc_cache_t *doc_cache)
{
  hb_svg_clip_path_def_t clip;
  hb_svg_str_t id = parser.find_attr ("id");
  hb_svg_str_t units = parser.find_attr ("clipPathUnits");
  if (units.eq ("objectBoundingBox"))
    clip.units_user_space = false;
  else
    clip.units_user_space = true;

  hb_svg_transform_t cp_t;
  if (svg_parse_element_transform (parser, &cp_t))
  {
    clip.has_clip_transform = true;
    clip.clip_transform = cp_t;
  }

  clip.first_shape = defs->clip_shapes.length;
  clip.shape_count = 0;

  if (tok == SVG_TOKEN_OPEN_TAG)
  {
    const unsigned SVG_MAX_CLIP_DEPTH = 64;
    hb_svg_transform_t inherited[SVG_MAX_CLIP_DEPTH];
    inherited[0] = hb_svg_transform_t ();
    inherited[1] = hb_svg_transform_t ();

    int cdepth = 1;
    bool had_alloc_failure = false;
    while (cdepth > 0)
    {
      hb_svg_token_type_t ct = parser.next ();
      if (ct == SVG_TOKEN_EOF) break;
      if (ct == SVG_TOKEN_CLOSE_TAG) { cdepth--; continue; }
      if (ct == SVG_TOKEN_OPEN_TAG || ct == SVG_TOKEN_SELF_CLOSE_TAG)
      {
        hb_svg_style_props_t visibility_style_props;
        svg_parse_style_props (parser.find_attr ("style"), &visibility_style_props);
        hb_svg_str_t display_str = svg_pick_attr_or_style (parser, visibility_style_props.display, "display");
        hb_svg_str_t visibility_str = svg_pick_attr_or_style (parser, visibility_style_props.visibility, "visibility");
        bool is_hidden = svg_str_eq_ascii_ci (display_str.trim (), "none") ||
                         svg_str_eq_ascii_ci (visibility_str.trim (), "hidden") ||
                         svg_str_eq_ascii_ci (visibility_str.trim (), "collapse");
        if (is_hidden)
        {
          if (ct == SVG_TOKEN_OPEN_TAG)
          {
            int skip_depth = 1;
            while (skip_depth > 0)
            {
              hb_svg_token_type_t st = parser.next ();
              if (st == SVG_TOKEN_EOF) break;
              if (st == SVG_TOKEN_CLOSE_TAG) skip_depth--;
              else if (st == SVG_TOKEN_OPEN_TAG) skip_depth++;
            }
          }
          continue;
        }

        hb_svg_transform_t effective = (unsigned) cdepth < SVG_MAX_CLIP_DEPTH
                                     ? inherited[cdepth]
                                     : hb_svg_transform_t ();

        hb_svg_transform_t local;
        bool has_local_transform = svg_parse_element_transform (parser, &local);
        if (has_local_transform)
        {
          effective.multiply (local);
        }

        if (parser.tag_name.eq ("use"))
        {
          hb_svg_str_t href = hb_raster_svg_find_href_attr (parser);
          hb_svg_str_t ref_id;
          if (hb_raster_svg_parse_local_id_ref (href, &ref_id, nullptr))
          {
            const char *found = nullptr;
            if (svg_find_element_by_id (doc_start, doc_len, svg_accel, doc_cache,
                                        ref_id, &found))
            {
              unsigned remaining = doc_len - (unsigned) (found - doc_start);
              hb_svg_xml_parser_t ref_parser (found, remaining);
              hb_svg_token_type_t rt = ref_parser.next ();
              if (rt == SVG_TOKEN_OPEN_TAG || rt == SVG_TOKEN_SELF_CLOSE_TAG)
              {
                hb_svg_shape_emit_data_t ref_shape;
                if (hb_raster_svg_parse_shape_tag (ref_parser, &ref_shape))
                {
                  hb_svg_transform_t use_t = effective;
                  float use_x = svg_parse_float (parser.find_attr ("x"));
                  float use_y = svg_parse_float (parser.find_attr ("y"));
                  if (use_x != 0.f || use_y != 0.f)
                  {
                    hb_svg_transform_t tr;
                    tr.dx = use_x;
                    tr.dy = use_y;
                    use_t.multiply (tr);
                  }
                  hb_svg_transform_t ref_t;
                  if (svg_parse_element_transform (ref_parser, &ref_t))
                    use_t.multiply (ref_t);

                  hb_svg_clip_shape_t clip_shape;
                  clip_shape.shape = ref_shape;
                  if (!svg_transform_is_identity (use_t))
                  {
                    clip_shape.has_transform = true;
                    clip_shape.transform = use_t;
                  }
                  defs->clip_shapes.push (clip_shape);
                  if (likely (!defs->clip_shapes.in_error ()))
                    clip.shape_count++;
                  else
                    had_alloc_failure = true;
                }
              }
            }
          }
        }
        else
        {
          hb_svg_shape_emit_data_t shape;
          if (hb_raster_svg_parse_shape_tag (parser, &shape))
          {
            hb_svg_clip_shape_t clip_shape;
            clip_shape.shape = shape;
            if (!svg_transform_is_identity (effective))
            {
              clip_shape.has_transform = true;
              clip_shape.transform = effective;
            }
            defs->clip_shapes.push (clip_shape);
            if (likely (!defs->clip_shapes.in_error ()))
              clip.shape_count++;
            else
              had_alloc_failure = true;
          }
        }

        if (ct == SVG_TOKEN_OPEN_TAG)
        {
          if ((unsigned) (cdepth + 1) < SVG_MAX_CLIP_DEPTH)
            inherited[cdepth + 1] = effective;
          cdepth++;
        }
      }
    }
    if (had_alloc_failure)
      id = {};
  }

  if (id.len)
    (void) defs->add_clip_path (hb_bytes_t (id.data, id.len), clip);
}

struct hb_svg_clip_emit_data_t
{
  const hb_svg_defs_t *defs;
  const hb_svg_clip_path_def_t *clip;
  hb_transform_t<> base_transform;
  hb_transform_t<> bbox_transform;
  bool has_bbox_transform = false;
};

static inline hb_transform_t<>
svg_to_hb_transform (const hb_svg_transform_t &t)
{
  return hb_transform_t<> (t.xx, t.yx, t.xy, t.yy, t.dx, t.dy);
}

static void
svg_clip_path_emit (hb_draw_funcs_t *dfuncs,
                    void *draw_data,
                    void *user_data)
{
  hb_raster_draw_t *rdr = (hb_raster_draw_t *) draw_data;
  hb_svg_clip_emit_data_t *ed = (hb_svg_clip_emit_data_t *) user_data;
  const hb_svg_clip_path_def_t *clip = ed->clip;

  for (unsigned i = 0; i < clip->shape_count; i++)
  {
    const hb_svg_clip_shape_t &s = ed->defs->clip_shapes[clip->first_shape + i];
    hb_transform_t<> t = ed->base_transform;
    if (ed->has_bbox_transform)
      t.multiply (ed->bbox_transform);
    if (clip->has_clip_transform)
      t.multiply (svg_to_hb_transform (clip->clip_transform));
    if (s.has_transform)
      t.multiply (svg_to_hb_transform (s.transform));

    hb_raster_draw_set_transform (rdr, t.xx, t.yx, t.xy, t.yy, t.x0, t.y0);

    hb_svg_shape_emit_data_t shape = s.shape;
    hb_raster_svg_shape_path_emit (dfuncs, draw_data, &shape);
  }
}

bool
hb_raster_svg_push_clip_path_ref (hb_raster_paint_t *paint,
                        hb_svg_defs_t *defs,
                        hb_svg_str_t clip_path_str,
                        const hb_extents_t<> *object_bbox)
{
  if (clip_path_str.is_null ()) return false;
  hb_svg_str_t trimmed = clip_path_str.trim ();
  if (!trimmed.len || trimmed.eq ("none")) return false;

  hb_svg_str_t clip_id;
  if (!hb_raster_svg_parse_local_id_ref (trimmed, &clip_id, nullptr))
    return false;

  const hb_svg_clip_path_def_t *clip = defs->find_clip_path (hb_bytes_t (clip_id.data, clip_id.len));
  if (!clip) return false;

  hb_svg_clip_emit_data_t ed;
  ed.defs = defs;
  ed.clip = clip;
  ed.base_transform = paint->current_effective_transform ();

  if (!clip->units_user_space)
  {
    if (!object_bbox || object_bbox->is_empty ())
      return false;
    float w = object_bbox->xmax - object_bbox->xmin;
    float h = object_bbox->ymax - object_bbox->ymin;
    if (w <= 0 || h <= 0)
      return false;
    ed.has_bbox_transform = true;
    ed.bbox_transform = hb_transform_t<> (w, 0, 0, h, object_bbox->xmin, object_bbox->ymin);
  }

  hb_raster_paint_push_clip_path (paint, svg_clip_path_emit, &ed);
  return true;
}
