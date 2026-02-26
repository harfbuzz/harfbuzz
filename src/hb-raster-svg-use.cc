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

hb_svg_str_t
hb_raster_svg_find_href_attr (const hb_svg_xml_parser_t &parser)
{
  hb_svg_str_t href = parser.find_attr ("href");
  if (href.is_null ())
    href = parser.find_attr ("xlink:href");
  return href;
}

static bool
svg_find_element_by_id (const hb_svg_use_context_t *ctx,
                        const char *id,
                        const char **found)
{
  *found = nullptr;
  if (ctx->doc_cache)
  {
    unsigned start = 0, end = 0;
    if (ctx->svg_accel->doc_cache_find_id_cstr (ctx->doc_cache, id, &start, &end))
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
    if (attr_id.len && attr_id.eq_cstr (id))
    {
      *found = search.tag_start;
      return true;
    }
  }
  return false;
}

static bool
svg_parse_id_ref (hb_svg_str_t s, char out_id[64])
{
  s = s.trim ();

  if (s.len && s.data[0] == '#')
  {
    unsigned n = hb_min (s.len - 1, (unsigned) 63);
    memcpy (out_id, s.data + 1, n);
    out_id[n] = '\0';
    return n > 0;
  }

  if (!svg_str_starts_with_ascii_ci (s, "url("))
    return false;

  const char *p = s.data + 4;
  const char *end = s.data + s.len;
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;

  const char *q = p;
  char quote = 0;
  while (q < end)
  {
    char c = *q;
    if (quote)
    {
      if (c == quote) quote = 0;
    }
    else
    {
      if (c == '"' || c == '\'') quote = c;
      else if (c == ')') break;
    }
    q++;
  }
  if (q >= end || *q != ')')
    return false;

  const char *id_b = p;
  const char *id_e = q;
  while (id_b < id_e && (*id_b == ' ' || *id_b == '\t' || *id_b == '\n' || *id_b == '\r')) id_b++;
  while (id_e > id_b && (*(id_e - 1) == ' ' || *(id_e - 1) == '\t' || *(id_e - 1) == '\n' || *(id_e - 1) == '\r')) id_e--;
  if (id_e > id_b && ((*id_b == '\'' && *(id_e - 1) == '\'') || (*id_b == '"' && *(id_e - 1) == '"')))
  {
    id_b++;
    id_e--;
  }
  while (id_b < id_e && (*id_b == ' ' || *id_b == '\t' || *id_b == '\n' || *id_b == '\r')) id_b++;
  while (id_e > id_b && (*(id_e - 1) == ' ' || *(id_e - 1) == '\t' || *(id_e - 1) == '\n' || *(id_e - 1) == '\r')) id_e--;
  if (id_b < id_e && *id_b == '#') id_b++;
  if (id_b >= id_e)
    return false;

  unsigned n = hb_min ((unsigned) (id_e - id_b), (unsigned) 63);
  memcpy (out_id, id_b, n);
  out_id[n] = '\0';
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

  char ref_id[64];
  if (!svg_parse_id_ref (href, ref_id))
    return;

  float use_x = svg_parse_float (parser.find_attr ("x"));
  float use_y = svg_parse_float (parser.find_attr ("y"));

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
      render_cb (render_user, ref_parser, state);
  }

  if (has_translate)
    hb_paint_pop_transform (ctx->pfuncs, ctx->paint);

  if (has_use_transform)
    hb_paint_pop_transform (ctx->pfuncs, ctx->paint);
}
