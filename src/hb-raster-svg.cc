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

#include "hb-face.hh"
#include "hb-raster.h"
#include "hb-raster-paint.hh"
#include "hb-raster-svg.hh"
#include "hb-raster-svg-base.hh"
#include "hb-raster-svg-parse.hh"
#include "hb-raster-svg-defs.hh"
#include "hb-raster-svg-gradient.hh"
#include "hb-raster-svg-clip.hh"
#include "hb-raster-svg-bbox.hh"
#include "hb-raster-svg-fill.hh"
#include "OT/Color/svg/svg.hh"
#include "hb-draw.h"
#include "hb-ot-color.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>


/*
 * 1. String / style helpers live in hb-raster-svg-base.*
 */


/*
 * 2. XML tokenizer / transform / shape parsing live in hb-raster-svg-parse.*
 */


/*
 * 3. SVG Color parser
 */

struct hb_svg_named_color_t
{
  const char *name;
  uint32_t rgb; /* 0xRRGGBB */
};

static const hb_svg_named_color_t svg_named_colors[] = {
  {"aliceblue", 0xF0F8FF},
  {"antiquewhite", 0xFAEBD7},
  {"aqua", 0x00FFFF},
  {"aquamarine", 0x7FFFD4},
  {"azure", 0xF0FFFF},
  {"beige", 0xF5F5DC},
  {"bisque", 0xFFE4C4},
  {"black", 0x000000},
  {"blanchedalmond", 0xFFEBCD},
  {"blue", 0x0000FF},
  {"blueviolet", 0x8A2BE2},
  {"brown", 0xA52A2A},
  {"burlywood", 0xDEB887},
  {"cadetblue", 0x5F9EA0},
  {"chartreuse", 0x7FFF00},
  {"chocolate", 0xD2691E},
  {"coral", 0xFF7F50},
  {"cornflowerblue", 0x6495ED},
  {"cornsilk", 0xFFF8DC},
  {"crimson", 0xDC143C},
  {"cyan", 0x00FFFF},
  {"darkblue", 0x00008B},
  {"darkcyan", 0x008B8B},
  {"darkgoldenrod", 0xB8860B},
  {"darkgray", 0xA9A9A9},
  {"darkgreen", 0x006400},
  {"darkgrey", 0xA9A9A9},
  {"darkkhaki", 0xBDB76B},
  {"darkmagenta", 0x8B008B},
  {"darkolivegreen", 0x556B2F},
  {"darkorange", 0xFF8C00},
  {"darkorchid", 0x9932CC},
  {"darkred", 0x8B0000},
  {"darksalmon", 0xE9967A},
  {"darkseagreen", 0x8FBC8F},
  {"darkslateblue", 0x483D8B},
  {"darkslategray", 0x2F4F4F},
  {"darkslategrey", 0x2F4F4F},
  {"darkturquoise", 0x00CED1},
  {"darkviolet", 0x9400D3},
  {"deeppink", 0xFF1493},
  {"deepskyblue", 0x00BFFF},
  {"dimgray", 0x696969},
  {"dimgrey", 0x696969},
  {"dodgerblue", 0x1E90FF},
  {"firebrick", 0xB22222},
  {"floralwhite", 0xFFFAF0},
  {"forestgreen", 0x228B22},
  {"fuchsia", 0xFF00FF},
  {"gainsboro", 0xDCDCDC},
  {"ghostwhite", 0xF8F8FF},
  {"gold", 0xFFD700},
  {"goldenrod", 0xDAA520},
  {"gray", 0x808080},
  {"green", 0x008000},
  {"greenyellow", 0xADFF2F},
  {"grey", 0x808080},
  {"honeydew", 0xF0FFF0},
  {"hotpink", 0xFF69B4},
  {"indianred", 0xCD5C5C},
  {"indigo", 0x4B0082},
  {"ivory", 0xFFFFF0},
  {"khaki", 0xF0E68C},
  {"lavender", 0xE6E6FA},
  {"lavenderblush", 0xFFF0F5},
  {"lawngreen", 0x7CFC00},
  {"lemonchiffon", 0xFFFACD},
  {"lightblue", 0xADD8E6},
  {"lightcoral", 0xF08080},
  {"lightcyan", 0xE0FFFF},
  {"lightgoldenrodyellow", 0xFAFAD2},
  {"lightgray", 0xD3D3D3},
  {"lightgreen", 0x90EE90},
  {"lightgrey", 0xD3D3D3},
  {"lightpink", 0xFFB6C1},
  {"lightsalmon", 0xFFA07A},
  {"lightseagreen", 0x20B2AA},
  {"lightskyblue", 0x87CEFA},
  {"lightslategray", 0x778899},
  {"lightslategrey", 0x778899},
  {"lightsteelblue", 0xB0C4DE},
  {"lightyellow", 0xFFFFE0},
  {"lime", 0x00FF00},
  {"limegreen", 0x32CD32},
  {"linen", 0xFAF0E6},
  {"magenta", 0xFF00FF},
  {"maroon", 0x800000},
  {"mediumaquamarine", 0x66CDAA},
  {"mediumblue", 0x0000CD},
  {"mediumorchid", 0xBA55D3},
  {"mediumpurple", 0x9370DB},
  {"mediumseagreen", 0x3CB371},
  {"mediumslateblue", 0x7B68EE},
  {"mediumspringgreen", 0x00FA9A},
  {"mediumturquoise", 0x48D1CC},
  {"mediumvioletred", 0xC71585},
  {"midnightblue", 0x191970},
  {"mintcream", 0xF5FFFA},
  {"mistyrose", 0xFFE4E1},
  {"moccasin", 0xFFE4B5},
  {"navajowhite", 0xFFDEAD},
  {"navy", 0x000080},
  {"oldlace", 0xFDF5E6},
  {"olive", 0x808000},
  {"olivedrab", 0x6B8E23},
  {"orange", 0xFFA500},
  {"orangered", 0xFF4500},
  {"orchid", 0xDA70D6},
  {"palegoldenrod", 0xEEE8AA},
  {"palegreen", 0x98FB98},
  {"paleturquoise", 0xAFEEEE},
  {"palevioletred", 0xDB7093},
  {"papayawhip", 0xFFEFD5},
  {"peachpuff", 0xFFDAB9},
  {"peru", 0xCD853F},
  {"pink", 0xFFC0CB},
  {"plum", 0xDDA0DD},
  {"powderblue", 0xB0E0E6},
  {"purple", 0x800080},
  {"rebeccapurple", 0x663399},
  {"red", 0xFF0000},
  {"rosybrown", 0xBC8F8F},
  {"royalblue", 0x4169E1},
  {"saddlebrown", 0x8B4513},
  {"salmon", 0xFA8072},
  {"sandybrown", 0xF4A460},
  {"seagreen", 0x2E8B57},
  {"seashell", 0xFFF5EE},
  {"sienna", 0xA0522D},
  {"silver", 0xC0C0C0},
  {"skyblue", 0x87CEEB},
  {"slateblue", 0x6A5ACD},
  {"slategray", 0x708090},
  {"slategrey", 0x708090},
  {"snow", 0xFFFAFA},
  {"springgreen", 0x00FF7F},
  {"steelblue", 0x4682B4},
  {"tan", 0xD2B48C},
  {"teal", 0x008080},
  {"thistle", 0xD8BFD8},
  {"tomato", 0xFF6347},
  {"turquoise", 0x40E0D0},
  {"violet", 0xEE82EE},
  {"wheat", 0xF5DEB3},
  {"white", 0xFFFFFF},
  {"whitesmoke", 0xF5F5F5},
  {"yellow", 0xFFFF00},
  {"yellowgreen", 0x9ACD32},
};

static int
hexval (char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

/* Parse SVG color value; returns HB_COLOR with alpha = 255.
 * Sets *is_none if "none". */
hb_color_t
svg_parse_color (hb_svg_str_t s,
		 hb_paint_funcs_t *pfuncs,
		 void *paint_data,
		 hb_color_t foreground,
		 hb_face_t *face,
		 unsigned palette,
		 bool *is_none)
{
  *is_none = false;
  s = s.trim ();
  if (!s.len) { *is_none = true; return HB_COLOR (0, 0, 0, 0); }

  if (svg_str_eq_ascii_ci (s, "none") || svg_str_eq_ascii_ci (s, "transparent"))
  {
    *is_none = true;
    return HB_COLOR (0, 0, 0, 0);
  }

  if (svg_str_eq_ascii_ci (s, "currentColor"))
    return foreground;

  /* var(--colorN) → CPAL palette color */
  if (svg_str_starts_with_ascii_ci (s, "var("))
  {
    const char *p = s.data + 4;
    const char *e = s.data + s.len;
    while (p < e && *p == ' ') p++;
    if (p + 7 < e && p[0] == '-' && p[1] == '-' && p[2] == 'c' &&
	p[3] == 'o' && p[4] == 'l' && p[5] == 'o' && p[6] == 'r')
    {
      p += 7;
      unsigned color_index = 0;
      while (p < e && *p >= '0' && *p <= '9')
	color_index = color_index * 10 + (*p++ - '0');

      hb_color_t palette_color;
      if (hb_paint_custom_palette_color (pfuncs, paint_data, color_index, &palette_color))
	return palette_color;

      unsigned count = 1;
      hb_ot_color_palette_get_colors (face, palette, color_index, &count, &palette_color);
      if (count)
	return palette_color;
    }

    /* Fallback value after comma: var(--colorN, fallback) */
    p = s.data + 4;
    while (p < e && *p != ',') p++;
    if (p < e)
    {
      p++;
      while (p < e && *p == ' ') p++;
      const char *val_start = p;
      /* Find closing paren */
      while (e > val_start && *(e - 1) != ')') e--;
      if (e > val_start) e--;
      hb_svg_str_t fallback = {val_start, (unsigned) (e - val_start)};
      return svg_parse_color (fallback, pfuncs, paint_data, foreground, face, palette, is_none);
    }

    return foreground;
  }

  /* #RGB or #RRGGBB */
  if (s.data[0] == '#')
  {
    if (s.len == 4) /* #RGB */
    {
      int r = hexval (s.data[1]);
      int g = hexval (s.data[2]);
      int b = hexval (s.data[3]);
      if (r < 0 || g < 0 || b < 0) return HB_COLOR (0, 0, 0, 255);
      return HB_COLOR (b * 17, g * 17, r * 17, 255);
    }
    if (s.len == 7) /* #RRGGBB */
    {
      int r = hexval (s.data[1]) * 16 + hexval (s.data[2]);
      int g = hexval (s.data[3]) * 16 + hexval (s.data[4]);
      int b = hexval (s.data[5]) * 16 + hexval (s.data[6]);
      return HB_COLOR (b, g, r, 255);
    }
    return HB_COLOR (0, 0, 0, 255);
  }

  /* rgb(r, g, b) or rgb(r%, g%, b%) */
  if (s.starts_with ("rgb"))
  {
    const char *p = s.data + 3;
    const char *e = s.data + s.len;
    while (p < e && *p != '(') p++;
    if (p < e) p++;

    auto read_component = [&] () -> int {
      while (p < e && (*p == ' ' || *p == ',')) p++;
      char buf[32];
      unsigned n = 0;
      while (p < e && n < sizeof (buf) - 1 && ((*p >= '0' && *p <= '9') || *p == '.' || *p == '-'))
	buf[n++] = *p++;
      bool is_pct = (p < e && *p == '%');
      if (is_pct) p++;
      buf[n] = '\0';
      float val = strtof (buf, nullptr);
      if (is_pct) val = val * 255.f / 100.f;
      return hb_clamp ((int) (val + 0.5f), 0, 255);
    };

    int r = read_component ();
    int g = read_component ();
    int b = read_component ();
    return HB_COLOR (b, g, r, 255);
  }

  /* Named colors (case-insensitive comparison) */
  {
    char lower[32];
    unsigned n = hb_min (s.len, (unsigned) sizeof (lower) - 1);
    for (unsigned i = 0; i < n; i++)
      lower[i] = (s.data[i] >= 'A' && s.data[i] <= 'Z') ? s.data[i] + 32 : s.data[i];
    lower[n] = '\0';

    for (unsigned i = 0; i < ARRAY_LENGTH (svg_named_colors); i++)
      if (strcmp (lower, svg_named_colors[i].name) == 0)
      {
	uint32_t rgb = svg_named_colors[i].rgb;
	return HB_COLOR (rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF, 255);
      }
  }

  return HB_COLOR (0, 0, 0, 255);
}


/*
 * 9. Defs store
 */

static bool
svg_parse_id_ref_with_fallback (hb_svg_str_t s,
				char out_id[64],
				hb_svg_str_t *fallback,
				bool allow_fragment_direct);

/*
 * 10. SVG rendering context
 */

#define SVG_MAX_DEPTH 32

struct hb_svg_render_context_t
{
  hb_raster_paint_t *paint;
  hb_paint_funcs_t *pfuncs;
  hb_font_t *font;
  unsigned palette;
  hb_color_t foreground;
  hb_svg_defs_t defs;
  int depth = 0;

  /* The full SVG document for <use> resolution */
  const char *doc_start;
  unsigned doc_len;
  const OT::SVG::accelerator_t *svg_accel = nullptr;
  const OT::SVG::svg_doc_cache_t *doc_cache = nullptr;

  void push_transform (float xx, float yx, float xy, float yy, float dx, float dy)
  {
    hb_paint_push_transform (pfuncs, paint, xx, yx, xy, yy, dx, dy);
  }
  void pop_transform ()
  {
    hb_paint_pop_transform (pfuncs, paint);
  }
  void push_group ()
  {
    hb_paint_push_group (pfuncs, paint);
  }
  void pop_group (hb_paint_composite_mode_t mode)
  {
    hb_paint_pop_group (pfuncs, paint, mode);
  }
  void paint_color (hb_color_t color)
  {
    hb_paint_color (pfuncs, paint, false, color);
  }
  void pop_clip ()
  {
    hb_paint_pop_clip (pfuncs, paint);
  }
};

struct hb_svg_cascade_t
{
  hb_svg_str_t fill;
  float fill_opacity = 1.f;
  hb_svg_str_t clip_path;
  hb_color_t color = HB_COLOR (0, 0, 0, 255);
  bool visibility = true;
  float opacity = 1.f;
};

static bool
svg_parse_id_ref_with_fallback (hb_svg_str_t s,
				char out_id[64],
				hb_svg_str_t *fallback,
				bool allow_fragment_direct)
{
  if (fallback) *fallback = {};
  s = s.trim ();

  if (allow_fragment_direct && s.len && s.data[0] == '#')
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

  if (fallback)
  {
    const char *f = q + 1;
    while (f < end && (*f == ' ' || *f == '\t' || *f == '\n' || *f == '\r')) f++;
    if (f < end) *fallback = {f, (unsigned) (end - f)};
  }
  return true;
}


/*
 * Emit fill (solid or gradient)
 */

/*
 * 11. Element renderer — recursive SVG rendering
 */

static void svg_render_element (hb_svg_render_context_t *ctx,
				hb_svg_xml_parser_t &parser,
				const struct hb_svg_cascade_t &inherited);

/* Gradient def parsing lives in hb-raster-svg-gradient.* */

/* Clip-path defs and push helpers live in hb-raster-svg-clip.* */

/* Process <defs> children — gradients, clipPaths */
static void
svg_process_defs (hb_svg_render_context_t *ctx, hb_svg_xml_parser_t &parser)
{
  int depth = 1;

  while (depth > 0)
  {
    hb_svg_token_type_t tok = parser.next ();
    if (tok == SVG_TOKEN_EOF) break;

    if (tok == SVG_TOKEN_CLOSE_TAG)
    {
      depth--;
      continue;
    }

    if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
    {
      if (parser.tag_name.eq ("linearGradient"))
	svg_process_gradient_def (&ctx->defs, parser, tok, SVG_GRADIENT_LINEAR,
				  ctx->pfuncs, ctx->paint,
				  ctx->foreground, hb_font_get_face (ctx->font),
				  ctx->palette);
      else if (parser.tag_name.eq ("radialGradient"))
	svg_process_gradient_def (&ctx->defs, parser, tok, SVG_GRADIENT_RADIAL,
				  ctx->pfuncs, ctx->paint,
				  ctx->foreground, hb_font_get_face (ctx->font),
				  ctx->palette);
      else if (parser.tag_name.eq ("clipPath"))
	svg_process_clip_path_def (&ctx->defs, parser, tok);
      else
      {
	if (tok == SVG_TOKEN_OPEN_TAG)
	  depth++;
      }
    }
  }
}

static bool
svg_find_element_by_id (const hb_svg_render_context_t *ctx,
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

/* Render a single shape element */
static void
svg_render_shape (hb_svg_render_context_t *ctx,
		  hb_svg_shape_emit_data_t &shape,
		  const hb_svg_cascade_t &state,
		  hb_svg_str_t transform_str)
{
  bool has_transform = transform_str.len > 0;
  bool has_opacity = state.opacity < 1.f;
  bool has_clip_path = false;

  if (has_opacity)
    ctx->push_group ();

  if (has_transform)
  {
    hb_svg_transform_t t;
    svg_parse_transform (transform_str, &t);
    ctx->push_transform (t.xx, t.yx, t.xy, t.yy, t.dx, t.dy);
  }

  hb_extents_t<> bbox;
  bool has_bbox = svg_compute_shape_bbox (shape, &bbox);
  has_clip_path = svg_push_clip_path_ref (ctx->paint, &ctx->defs, state.clip_path,
					  has_bbox ? &bbox : nullptr);

  /* Clip with shape path, then fill */
  hb_raster_paint_push_clip_path (ctx->paint, svg_shape_path_emit, &shape);

  /* Default fill is black */
  if (state.fill.is_null ())
  {
    hb_color_t black = HB_COLOR (0, 0, 0, 255);
    if (state.fill_opacity < 1.f)
      black = HB_COLOR (0, 0, 0, (uint8_t) (255 * state.fill_opacity + 0.5f));
    ctx->paint_color (black);
  }
  else
  {
    hb_svg_fill_context_t fill_ctx = {ctx->paint, ctx->pfuncs, ctx->font, ctx->palette, &ctx->defs};
    svg_emit_fill (&fill_ctx, state.fill, state.fill_opacity, has_bbox ? &bbox : nullptr, state.color);
  }

  ctx->pop_clip ();
  if (has_clip_path)
    ctx->pop_clip ();

  if (has_transform)
    ctx->pop_transform ();

  if (has_opacity)
    ctx->pop_group (HB_PAINT_COMPOSITE_MODE_SRC_OVER);
}

static void
svg_render_container_element (hb_svg_render_context_t *ctx,
			      hb_svg_xml_parser_t &parser,
			      hb_svg_str_t tag,
			      bool self_closing,
			      const hb_svg_cascade_t &state,
			      hb_svg_str_t transform_str,
			      hb_svg_str_t clip_path_str)
{
  bool has_transform = transform_str.len > 0;
  bool has_opacity = state.opacity < 1.f;
  bool has_clip = false;
  bool has_viewbox = false;
  float vb_x = 0, vb_y = 0, vb_w = 0, vb_h = 0;

  if (tag.eq ("svg"))
  {
    hb_svg_str_t viewbox_str = parser.find_attr ("viewBox");
    if (viewbox_str.len)
    {
      has_viewbox = true;
      hb_svg_float_parser_t vb_fp (viewbox_str);
      vb_x = vb_fp.next_float ();
      vb_y = vb_fp.next_float ();
      vb_w = vb_fp.next_float ();
      vb_h = vb_fp.next_float ();
    }
  }

  if (has_opacity)
    ctx->push_group ();

  if (has_transform)
  {
    hb_svg_transform_t t;
    svg_parse_transform (transform_str, &t);
    ctx->push_transform (t.xx, t.yx, t.xy, t.yy, t.dx, t.dy);
  }

  if (has_viewbox && vb_w > 0 && vb_h > 0)
    ctx->push_transform (1, 0, 0, 1, -vb_x, -vb_y);

  has_clip = svg_push_clip_path_ref (ctx->paint, &ctx->defs, clip_path_str, nullptr);

  if (!self_closing)
  {
    int depth = 1;
    while (depth > 0)
    {
      hb_svg_token_type_t tok = parser.next ();
      if (tok == SVG_TOKEN_EOF) break;

      if (tok == SVG_TOKEN_CLOSE_TAG)
      {
	depth--;
	continue;
      }

      if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
      {
	hb_svg_str_t child_tag = parser.tag_name;
	if (parser.tag_name.eq ("defs"))
	{
	  if (tok != SVG_TOKEN_SELF_CLOSE_TAG)
	    svg_process_defs (ctx, parser);
	  continue;
	}
	svg_render_element (ctx, parser, state);
	if (tok == SVG_TOKEN_OPEN_TAG && !child_tag.eq ("g") &&
	    !child_tag.eq ("svg") && !child_tag.eq ("use"))
	{
	  /* Skip children of non-container elements we don't handle. */
	  int skip_depth = 1;
	  while (skip_depth > 0)
	  {
	    hb_svg_token_type_t st = parser.next ();
	    if (st == SVG_TOKEN_EOF) break;
	    if (st == SVG_TOKEN_CLOSE_TAG) skip_depth--;
	    else if (st == SVG_TOKEN_OPEN_TAG) skip_depth++;
	  }
	}
      }
    }
  }

  if (has_viewbox && vb_w > 0 && vb_h > 0)
    ctx->pop_transform ();

  if (has_clip)
    ctx->pop_clip ();

  if (has_transform)
    ctx->pop_transform ();

  if (has_opacity)
    ctx->pop_group (HB_PAINT_COMPOSITE_MODE_SRC_OVER);
}

static bool
svg_render_primitive_shape_element (hb_svg_render_context_t *ctx,
				    hb_svg_xml_parser_t &parser,
				    hb_svg_str_t tag,
				    const hb_svg_cascade_t &state,
				    hb_svg_str_t transform_str)
{
  if (tag.eq ("path"))
  {
    hb_svg_str_t d = parser.find_attr ("d");
    if (d.len)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_PATH;
      shape.str_data = d;
      svg_render_shape (ctx, shape, state, transform_str);
    }
    return true;
  }
  if (tag.eq ("rect"))
  {
    float x = svg_parse_float (parser.find_attr ("x"));
    float y = svg_parse_float (parser.find_attr ("y"));
    float w = svg_parse_float (parser.find_attr ("width"));
    float h = svg_parse_float (parser.find_attr ("height"));
    float rx = svg_parse_float (parser.find_attr ("rx"));
    float ry = svg_parse_float (parser.find_attr ("ry"));

    if (w > 0 && h > 0)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_RECT;
      shape.params[0] = x;
      shape.params[1] = y;
      shape.params[2] = w;
      shape.params[3] = h;
      shape.params[4] = rx;
      shape.params[5] = ry;
      svg_render_shape (ctx, shape, state, transform_str);
    }
    return true;
  }
  if (tag.eq ("circle"))
  {
    float cx = svg_parse_float (parser.find_attr ("cx"));
    float cy = svg_parse_float (parser.find_attr ("cy"));
    float r = svg_parse_float (parser.find_attr ("r"));

    if (r > 0)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_CIRCLE;
      shape.params[0] = cx;
      shape.params[1] = cy;
      shape.params[2] = r;
      svg_render_shape (ctx, shape, state, transform_str);
    }
    return true;
  }
  if (tag.eq ("ellipse"))
  {
    float cx = svg_parse_float (parser.find_attr ("cx"));
    float cy = svg_parse_float (parser.find_attr ("cy"));
    float rx = svg_parse_float (parser.find_attr ("rx"));
    float ry = svg_parse_float (parser.find_attr ("ry"));

    if (rx > 0 && ry > 0)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_ELLIPSE;
      shape.params[0] = cx;
      shape.params[1] = cy;
      shape.params[2] = rx;
      shape.params[3] = ry;
      svg_render_shape (ctx, shape, state, transform_str);
    }
    return true;
  }
  if (tag.eq ("line"))
  {
    float x1 = svg_parse_float (parser.find_attr ("x1"));
    float y1 = svg_parse_float (parser.find_attr ("y1"));
    float x2 = svg_parse_float (parser.find_attr ("x2"));
    float y2 = svg_parse_float (parser.find_attr ("y2"));

    hb_svg_shape_emit_data_t shape;
    shape.type = hb_svg_shape_emit_data_t::SHAPE_LINE;
    shape.params[0] = x1;
    shape.params[1] = y1;
    shape.params[2] = x2;
    shape.params[3] = y2;
    svg_render_shape (ctx, shape, state, transform_str);
    return true;
  }
  if (tag.eq ("polyline"))
  {
    hb_svg_str_t points = parser.find_attr ("points");
    if (points.len)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_POLYLINE;
      shape.str_data = points;
      svg_render_shape (ctx, shape, state, transform_str);
    }
    return true;
  }
  if (tag.eq ("polygon"))
  {
    hb_svg_str_t points = parser.find_attr ("points");
    if (points.len)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_POLYGON;
      shape.str_data = points;
      svg_render_shape (ctx, shape, state, transform_str);
    }
    return true;
  }

  return false;
}

static hb_svg_str_t
svg_find_href_attr (const hb_svg_xml_parser_t &parser)
{
  hb_svg_str_t href = parser.find_attr ("href");
  if (href.is_null ())
    href = parser.find_attr ("xlink:href");
  return href;
}

static void
svg_render_use_element (hb_svg_render_context_t *ctx,
			hb_svg_xml_parser_t &parser,
			const hb_svg_cascade_t &state,
			hb_svg_str_t transform_str)
{
  hb_svg_str_t href = svg_find_href_attr (parser);

  char ref_id[64];
  if (!svg_parse_id_ref_with_fallback (href, ref_id, nullptr, true))
    return;

  float use_x = svg_parse_float (parser.find_attr ("x"));
  float use_y = svg_parse_float (parser.find_attr ("y"));

  bool has_translate = (use_x != 0.f || use_y != 0.f);
  bool has_use_transform = transform_str.len > 0;

  if (has_use_transform)
  {
    hb_svg_transform_t t;
    svg_parse_transform (transform_str, &t);
    ctx->push_transform (t.xx, t.yx, t.xy, t.yy, t.dx, t.dy);
  }

  if (has_translate)
    ctx->push_transform (1, 0, 0, 1, use_x, use_y);

  const char *found = nullptr;
  svg_find_element_by_id (ctx, ref_id, &found);

  if (found)
  {
    unsigned remaining = ctx->doc_start + ctx->doc_len - found;
    hb_svg_xml_parser_t ref_parser (found, remaining);
    hb_svg_token_type_t tok = ref_parser.next ();
    if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
      svg_render_element (ctx, ref_parser, state);
  }

  if (has_translate)
    ctx->pop_transform ();

  if (has_use_transform)
    ctx->pop_transform ();
}

static void
svg_skip_subtree (hb_svg_xml_parser_t &parser)
{
  int depth = 1;
  while (depth > 0)
  {
    hb_svg_token_type_t tok = parser.next ();
    if (tok == SVG_TOKEN_EOF) break;
    if (tok == SVG_TOKEN_CLOSE_TAG) depth--;
    else if (tok == SVG_TOKEN_OPEN_TAG) depth++;
  }
}

/* Render one element (may be a container or shape) */
static void
svg_render_element (hb_svg_render_context_t *ctx,
		    hb_svg_xml_parser_t &parser,
		    const hb_svg_cascade_t &inherited)
{
  if (ctx->depth >= SVG_MAX_DEPTH) return;

  const HB_UNUSED unsigned transform_depth = ctx->paint->transform_stack.length;
  const HB_UNUSED unsigned clip_depth = ctx->paint->clip_stack.length;
  const HB_UNUSED unsigned surface_depth = ctx->paint->surface_stack.length;

  ctx->depth++;

  hb_svg_str_t tag = parser.tag_name;
  bool self_closing = parser.self_closing;

  /* Extract common attributes */
  hb_svg_str_t style = parser.find_attr ("style");
  hb_svg_style_props_t style_props;
  svg_parse_style_props (style, &style_props);
  hb_svg_str_t fill_attr = svg_pick_attr_or_style (parser, style_props.fill, "fill");
  hb_svg_str_t fill_opacity_str = svg_pick_attr_or_style (parser, style_props.fill_opacity, "fill-opacity");
  hb_svg_str_t opacity_str = svg_pick_attr_or_style (parser, style_props.opacity, "opacity");
  hb_svg_str_t transform_str = svg_pick_attr_or_style (parser, style_props.transform, "transform");
  hb_svg_str_t clip_path_attr = svg_pick_attr_or_style (parser, style_props.clip_path, "clip-path");
  hb_svg_str_t display_str = svg_pick_attr_or_style (parser, style_props.display, "display");
  hb_svg_str_t color_str = svg_pick_attr_or_style (parser, style_props.color, "color");
  hb_svg_str_t visibility_str = svg_pick_attr_or_style (parser, style_props.visibility, "visibility");

  hb_svg_cascade_t state = inherited;
  state.fill = (fill_attr.is_null () || svg_str_is_inherit (fill_attr)) ? inherited.fill : fill_attr;
  state.fill_opacity = (fill_opacity_str.len && !svg_str_is_inherit (fill_opacity_str))
		       ? svg_parse_float_clamped01 (fill_opacity_str)
		       : inherited.fill_opacity;
  state.opacity = (opacity_str.len && !svg_str_is_inherit (opacity_str) && !svg_str_is_none (opacity_str))
		  ? svg_parse_float_clamped01 (opacity_str)
		  : (svg_str_is_inherit (opacity_str) ? inherited.opacity : 1.f);
  state.clip_path = (clip_path_attr.is_null () || svg_str_is_inherit (clip_path_attr))
		    ? inherited.clip_path
		    : clip_path_attr;
  if (svg_str_is_inherit (transform_str) || svg_str_is_none (transform_str))
    transform_str = {};
  state.color = inherited.color;
  bool is_none = false;
  if (color_str.len && !svg_str_eq_ascii_ci (color_str.trim (), "inherit"))
    state.color = svg_parse_color (color_str, ctx->pfuncs, ctx->paint,
				   inherited.color, hb_font_get_face (ctx->font),
				   ctx->palette, &is_none);
  state.visibility = inherited.visibility;
  hb_svg_str_t visibility_trim = visibility_str.trim ();
  if (visibility_trim.len && !svg_str_eq_ascii_ci (visibility_trim, "inherit"))
    state.visibility = !(svg_str_eq_ascii_ci (visibility_trim, "hidden") ||
			 svg_str_eq_ascii_ci (visibility_trim, "collapse"));

  if (svg_str_eq_ascii_ci (display_str.trim (), "none"))
  {
    if (!self_closing)
      svg_skip_subtree (parser);
    ctx->depth--;
    assert (ctx->paint->transform_stack.length == transform_depth);
    assert (ctx->paint->clip_stack.length == clip_depth);
    assert (ctx->paint->surface_stack.length == surface_depth);
    return;
  }
  if (!state.visibility)
  {
    if (!self_closing)
      svg_skip_subtree (parser);
    ctx->depth--;
    assert (ctx->paint->transform_stack.length == transform_depth);
    assert (ctx->paint->clip_stack.length == clip_depth);
    assert (ctx->paint->surface_stack.length == surface_depth);
    return;
  }

  if (tag.eq ("g") || tag.eq ("svg"))
    svg_render_container_element (ctx, parser, tag, self_closing,
				  state, transform_str, state.clip_path);
  else if (svg_render_primitive_shape_element (ctx, parser, tag, state, transform_str))
    ;
  else if (tag.eq ("use"))
    svg_render_use_element (ctx, parser, state, transform_str);

  ctx->depth--;

  assert (ctx->paint->transform_stack.length == transform_depth);
  assert (ctx->paint->clip_stack.length == clip_depth);
  assert (ctx->paint->surface_stack.length == surface_depth);
}


/*
 * 12. Entry point
 */

hb_bool_t
hb_raster_svg_render (hb_raster_paint_t *paint,
		      hb_blob_t *blob,
		      hb_codepoint_t glyph,
		      hb_font_t *font,
		      unsigned palette,
		      hb_color_t foreground)
{
  unsigned data_len;
  const char *data = hb_blob_get_data (blob, &data_len);
  if (!data || !data_len) return false;

  hb_face_t *face = hb_font_get_face (font);
  const OT::SVG::svg_doc_cache_t *doc_cache = nullptr;
  unsigned doc_index = 0;
  hb_codepoint_t start_glyph = HB_CODEPOINT_INVALID;
  hb_codepoint_t end_glyph = HB_CODEPOINT_INVALID;

  if (face &&
      hb_ot_color_glyph_get_svg_document_index (face, glyph, &doc_index) &&
      hb_ot_color_get_svg_document_glyph_range (face, doc_index, &start_glyph, &end_glyph))
    doc_cache = face->table.SVG->get_or_create_doc_cache (blob, data, data_len,
                                                          doc_index, start_glyph, end_glyph);

  if (doc_cache)
    data = face->table.SVG->doc_cache_get_svg (doc_cache, &data_len);

  hb_paint_funcs_t *pfuncs = hb_raster_paint_get_funcs ();

  hb_svg_render_context_t ctx;
  ctx.paint = paint;
  ctx.pfuncs = pfuncs;
  ctx.font = font;
  ctx.palette = palette;
  ctx.foreground = foreground;
  ctx.doc_start = data;
  ctx.doc_len = data_len;
  ctx.svg_accel = face ? face->table.SVG.get () : nullptr;
  ctx.doc_cache = doc_cache;

  hb_svg_cascade_t initial_state;
  initial_state.color = foreground;

  /* First pass: collect defs (gradients, clip-paths) from the entire document,
   * including entries that are authored outside <defs>. */
  {
    hb_svg_xml_parser_t defs_parser (data, data_len);
    while (true)
    {
      hb_svg_token_type_t tok = defs_parser.next ();
      if (tok == SVG_TOKEN_EOF) break;
      if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
      {
	if (defs_parser.tag_name.eq ("linearGradient"))
	  svg_process_gradient_def (&ctx.defs, defs_parser, tok, SVG_GRADIENT_LINEAR,
				    ctx.pfuncs, ctx.paint,
				    ctx.foreground, hb_font_get_face (ctx.font),
				    ctx.palette);
	else if (defs_parser.tag_name.eq ("radialGradient"))
	  svg_process_gradient_def (&ctx.defs, defs_parser, tok, SVG_GRADIENT_RADIAL,
				    ctx.pfuncs, ctx.paint,
				    ctx.foreground, hb_font_get_face (ctx.font),
				    ctx.palette);
	else if (defs_parser.tag_name.eq ("clipPath"))
	  svg_process_clip_path_def (&ctx.defs, defs_parser, tok);
      }
    }
  }

  bool found_glyph = false;
  unsigned glyph_start = 0, glyph_end = 0;
  if (doc_cache && face->table.SVG->doc_cache_get_glyph_span (doc_cache, glyph, &glyph_start, &glyph_end))
  {
    hb_svg_xml_parser_t parser (data + glyph_start, glyph_end - glyph_start);
    hb_svg_token_type_t tok = parser.next ();
    if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
    {
      hb_paint_push_font_transform (ctx.pfuncs, ctx.paint, font);
      ctx.push_transform (1, 0, 0, -1, 0, 0);
	      svg_render_element (&ctx, parser, initial_state);
      ctx.pop_transform ();
      hb_paint_pop_transform (ctx.pfuncs, ctx.paint);
      found_glyph = true;
    }
  }
  else
  {
    /* Fallback for malformed/uncached docs: linear scan by glyph id. */
    char glyph_id_str[32];
    snprintf (glyph_id_str, sizeof (glyph_id_str), "glyph%u", glyph);
    hb_svg_xml_parser_t parser (data, data_len);
    while (true)
    {
      hb_svg_token_type_t tok = parser.next ();
      if (tok == SVG_TOKEN_EOF) break;

      if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
      {
        hb_svg_str_t id = parser.find_attr ("id");
        if (id.len)
        {
          char id_buf[64];
          unsigned n = hb_min (id.len, (unsigned) sizeof (id_buf) - 1);
          memcpy (id_buf, id.data, n);
          id_buf[n] = '\0';

          if (strcmp (id_buf, glyph_id_str) == 0)
          {
            hb_paint_push_font_transform (ctx.pfuncs, ctx.paint, font);
            ctx.push_transform (1, 0, 0, -1, 0, 0);
	            svg_render_element (&ctx, parser, initial_state);
            ctx.pop_transform ();
            hb_paint_pop_transform (ctx.pfuncs, ctx.paint);
            found_glyph = true;
            break;
          }
        }
      }
    }
  }

  return found_glyph;
}
