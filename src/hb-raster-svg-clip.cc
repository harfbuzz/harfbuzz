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

void
hb_raster_svg_process_clip_path_def (hb_svg_defs_t *defs,
                           hb_svg_xml_parser_t &parser,
                           hb_svg_token_type_t tok)
{
  hb_svg_clip_path_def_t clip;
  hb_svg_str_t id = parser.find_attr ("id");
  hb_svg_str_t units = parser.find_attr ("clipPathUnits");
  if (units.eq ("objectBoundingBox"))
    clip.units_user_space = false;
  else
    clip.units_user_space = true;

  hb_svg_str_t cp_transform = parser.find_attr ("transform");
  if (cp_transform.len)
  {
    clip.has_clip_transform = true;
    hb_raster_svg_parse_transform (cp_transform, &clip.clip_transform);
  }

  clip.first_shape = defs->clip_shapes.length;
  clip.shape_count = 0;

  if (tok == SVG_TOKEN_OPEN_TAG)
  {
    int cdepth = 1;
    bool had_alloc_failure = false;
    while (cdepth > 0)
    {
      hb_svg_token_type_t ct = parser.next ();
      if (ct == SVG_TOKEN_EOF) break;
      if (ct == SVG_TOKEN_CLOSE_TAG) { cdepth--; continue; }
      if (ct == SVG_TOKEN_OPEN_TAG || ct == SVG_TOKEN_SELF_CLOSE_TAG)
      {
        if (cdepth == 1)
        {
          hb_svg_shape_emit_data_t shape;
          if (hb_raster_svg_parse_shape_tag (parser, &shape))
          {
            hb_svg_clip_shape_t clip_shape;
            clip_shape.shape = shape;
            hb_svg_str_t sh_transform = parser.find_attr ("transform");
            if (sh_transform.len)
            {
              clip_shape.has_transform = true;
              hb_raster_svg_parse_transform (sh_transform, &clip_shape.transform);
            }
            defs->clip_shapes.push (clip_shape);
            if (likely (!defs->clip_shapes.in_error ()))
              clip.shape_count++;
            else
              had_alloc_failure = true;
          }
        }
        if (ct == SVG_TOKEN_OPEN_TAG)
          cdepth++;
      }
    }
    if (had_alloc_failure)
      id = {};
  }

  if (id.len && clip.shape_count)
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
  if (!hb_raster_svg_parse_id_ref (trimmed, &clip_id, nullptr))
    return false;
  if (svg_str_starts_with_ascii_ci (trimmed, "url(") &&
      !svg_str_starts_with_ascii_ci (trimmed, "url(#"))
    return false;

  const hb_svg_clip_path_def_t *clip = defs->find_clip_path (hb_bytes_t (clip_id.data, clip_id.len));
  if (!clip || !clip->shape_count) return false;

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
