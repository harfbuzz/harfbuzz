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

#include "hb-raster.h"
#include "hb-raster-svg.hh"

#include <math.h>
#include <string.h>
#include <stdlib.h>


/*
 * 1. String view helper
 */

struct hb_svg_str_t
{
  const char *data;
  unsigned len;

  hb_svg_str_t () : data (nullptr), len (0) {}
  hb_svg_str_t (const char *d, unsigned l) : data (d), len (l) {}

  bool is_null () const { return !data; }

  bool eq (const char *s) const
  {
    unsigned slen = strlen (s);
    return len == slen && memcmp (data, s, len) == 0;
  }

  bool starts_with (const char *prefix) const
  {
    unsigned plen = strlen (prefix);
    return len >= plen && memcmp (data, prefix, plen) == 0;
  }

  float to_float () const
  {
    if (!data || !len) return 0.f;
    /* strtof needs null-terminated string; we use a small stack buffer */
    char buf[64];
    unsigned n = hb_min (len, (unsigned) sizeof (buf) - 1);
    memcpy (buf, data, n);
    buf[n] = '\0';
    return strtof (buf, nullptr);
  }

  int to_int () const
  {
    if (!data || !len) return 0;
    char buf[32];
    unsigned n = hb_min (len, (unsigned) sizeof (buf) - 1);
    memcpy (buf, data, n);
    buf[n] = '\0';
    return atoi (buf);
  }

  /* Skip leading whitespace */
  hb_svg_str_t trim_left () const
  {
    const char *p = data;
    unsigned l = len;
    while (l && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    { p++; l--; }
    return {p, l};
  }

  hb_svg_str_t trim () const
  {
    hb_svg_str_t s = trim_left ();
    while (s.len && (s.data[s.len - 1] == ' ' || s.data[s.len - 1] == '\t' ||
		     s.data[s.len - 1] == '\n' || s.data[s.len - 1] == '\r'))
      s.len--;
    return s;
  }
};


/*
 * 2. Minimal XML tokenizer
 */

enum hb_svg_token_type_t
{
  SVG_TOKEN_OPEN_TAG,
  SVG_TOKEN_CLOSE_TAG,
  SVG_TOKEN_SELF_CLOSE_TAG,
  SVG_TOKEN_TEXT,
  SVG_TOKEN_EOF
};

struct hb_svg_attr_t
{
  hb_svg_str_t name;
  hb_svg_str_t value;
};

struct hb_svg_xml_parser_t
{
  const char *p;
  const char *end;

  hb_svg_str_t tag_name;
  hb_vector_t<hb_svg_attr_t> attrs;
  bool self_closing;

  hb_svg_xml_parser_t (const char *data, unsigned len)
    : p (data), end (data + len) {}

  void skip_ws ()
  {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
      p++;
  }

  hb_svg_str_t read_name ()
  {
    const char *start = p;
    while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' &&
	   *p != '>' && *p != '/' && *p != '=')
      p++;
    return {start, (unsigned) (p - start)};
  }

  hb_svg_str_t read_attr_value ()
  {
    if (p >= end) return {};
    char quote = *p;
    if (quote != '"' && quote != '\'') return {};
    p++;
    const char *start = p;
    while (p < end && *p != quote)
      p++;
    hb_svg_str_t val = {start, (unsigned) (p - start)};
    if (p < end) p++; /* skip closing quote */
    return val;
  }

  void parse_attrs ()
  {
    attrs.clear ();
    while (p < end)
    {
      skip_ws ();
      if (p >= end || *p == '>' || *p == '/') break;
      hb_svg_str_t name = read_name ();
      if (!name.len) { p++; continue; }
      skip_ws ();
      if (p < end && *p == '=')
      {
	p++;
	skip_ws ();
	hb_svg_str_t val = read_attr_value ();
	hb_svg_attr_t attr = {name, val};
	attrs.push (attr);
      }
      else
      {
	hb_svg_attr_t attr = {name, {}};
	attrs.push (attr);
      }
    }
  }

  hb_svg_token_type_t next ()
  {
    while (p < end)
    {
      if (*p != '<')
      {
	/* Text content — skip for now */
	while (p < end && *p != '<') p++;
	continue;
      }

      p++; /* skip '<' */
      if (p >= end) return SVG_TOKEN_EOF;

      /* Comment or processing instruction */
      if (*p == '!')
      {
	/* Skip <!-- ... --> */
	if (p + 2 < end && p[1] == '-' && p[2] == '-')
	{
	  p += 3;
	  while (p + 2 < end && !(p[0] == '-' && p[1] == '-' && p[2] == '>'))
	    p++;
	  if (p + 2 < end) p += 3;
	}
	else
	{
	  /* Skip <!DOCTYPE ...> or similar */
	  while (p < end && *p != '>') p++;
	  if (p < end) p++;
	}
	continue;
      }
      if (*p == '?')
      {
	/* <?xml ... ?> */
	while (p + 1 < end && !(p[0] == '?' && p[1] == '>'))
	  p++;
	if (p + 1 < end) p += 2;
	continue;
      }

      /* Closing tag */
      if (*p == '/')
      {
	p++;
	tag_name = read_name ();
	while (p < end && *p != '>') p++;
	if (p < end) p++;
	attrs.clear ();
	self_closing = false;
	return SVG_TOKEN_CLOSE_TAG;
      }

      /* Opening tag */
      tag_name = read_name ();
      parse_attrs ();

      self_closing = false;
      skip_ws ();
      if (p < end && *p == '/')
      {
	self_closing = true;
	p++;
      }
      if (p < end && *p == '>')
	p++;

      return self_closing ? SVG_TOKEN_SELF_CLOSE_TAG : SVG_TOKEN_OPEN_TAG;
    }
    return SVG_TOKEN_EOF;
  }

  hb_svg_str_t find_attr (const char *name) const
  {
    for (unsigned i = 0; i < attrs.length; i++)
      if (attrs[i].name.eq (name))
	return attrs[i].value;
    return {};
  }
};


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
static hb_color_t
svg_parse_color (hb_svg_str_t s, hb_color_t foreground, hb_face_t *face, bool *is_none)
{
  *is_none = false;
  s = s.trim ();
  if (!s.len) { *is_none = true; return HB_COLOR (0, 0, 0, 0); }

  if (s.eq ("none") || s.eq ("transparent"))
  {
    *is_none = true;
    return HB_COLOR (0, 0, 0, 0);
  }

  if (s.eq ("currentColor"))
    return foreground;

  /* var(--colorN) → CPAL palette color */
  if (s.starts_with ("var("))
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
      unsigned count = 1;
      hb_ot_color_palette_get_colors (face, 0, color_index, &count, &palette_color);
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
      return svg_parse_color (fallback, foreground, face, is_none);
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
 * 4. SVG Number/Float parser
 */

struct hb_svg_float_parser_t
{
  const char *p;
  const char *end;

  hb_svg_float_parser_t (hb_svg_str_t s) : p (s.data), end (s.data + s.len) {}
  hb_svg_float_parser_t (const char *d, unsigned l) : p (d), end (d + l) {}

  void skip_ws_comma ()
  {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ','))
      p++;
  }

  bool has_more () const { return p < end; }

  float next_float ()
  {
    skip_ws_comma ();
    if (p >= end) return 0.f;
    char buf[64];
    unsigned n = 0;
    if (p < end && (*p == '-' || *p == '+'))
      buf[n++] = *p++;
    while (p < end && n < sizeof (buf) - 1 && *p >= '0' && *p <= '9')
      buf[n++] = *p++;
    if (p < end && n < sizeof (buf) - 1 && *p == '.')
    {
      buf[n++] = *p++;
      while (p < end && n < sizeof (buf) - 1 && *p >= '0' && *p <= '9')
	buf[n++] = *p++;
    }
    if (p < end && n < sizeof (buf) - 1 && (*p == 'e' || *p == 'E'))
    {
      buf[n++] = *p++;
      if (p < end && n < sizeof (buf) - 1 && (*p == '+' || *p == '-'))
	buf[n++] = *p++;
      while (p < end && n < sizeof (buf) - 1 && *p >= '0' && *p <= '9')
	buf[n++] = *p++;
    }
    buf[n] = '\0';
    return strtof (buf, nullptr);
  }

  int next_int ()
  {
    return (int) next_float ();
  }

  bool next_flag ()
  {
    skip_ws_comma ();
    if (p >= end) return false;
    bool v = *p != '0';
    p++;
    return v;
  }
};

static float
svg_parse_float (hb_svg_str_t s)
{
  return s.to_float ();
}



/*
 * 5. SVG Transform parser
 */

struct hb_svg_transform_t
{
  float xx = 1, yx = 0, xy = 0, yy = 1, dx = 0, dy = 0;

  void multiply (const hb_svg_transform_t &other)
  {
    float nxx = xx * other.xx + xy * other.yx;
    float nyx = yx * other.xx + yy * other.yx;
    float nxy = xx * other.xy + xy * other.yy;
    float nyy = yx * other.xy + yy * other.yy;
    float ndx = xx * other.dx + xy * other.dy + dx;
    float ndy = yx * other.dx + yy * other.dy + dy;
    xx = nxx; yx = nyx; xy = nxy; yy = nyy; dx = ndx; dy = ndy;
  }
};

static bool
svg_parse_transform (hb_svg_str_t s, hb_svg_transform_t *out)
{
  hb_svg_float_parser_t fp (s);

  while (fp.p < fp.end)
  {
    fp.skip_ws_comma ();
    if (fp.p >= fp.end) break;

    const char *start = fp.p;
    /* Read function name */
    while (fp.p < fp.end && *fp.p != '(') fp.p++;
    hb_svg_str_t func_name = {start, (unsigned) (fp.p - start)};

    /* Trim trailing whitespace from function name */
    while (func_name.len && (func_name.data[func_name.len - 1] == ' ' ||
			     func_name.data[func_name.len - 1] == '\t'))
      func_name.len--;

    if (fp.p >= fp.end) break;
    fp.p++; /* skip '(' */

    hb_svg_transform_t t;

    if (func_name.eq ("matrix"))
    {
      t.xx = fp.next_float ();
      t.yx = fp.next_float ();
      t.xy = fp.next_float ();
      t.yy = fp.next_float ();
      t.dx = fp.next_float ();
      t.dy = fp.next_float ();
    }
    else if (func_name.eq ("translate"))
    {
      t.dx = fp.next_float ();
      fp.skip_ws_comma ();
      if (fp.p < fp.end && *fp.p != ')')
	t.dy = fp.next_float ();
    }
    else if (func_name.eq ("scale"))
    {
      t.xx = fp.next_float ();
      fp.skip_ws_comma ();
      if (fp.p < fp.end && *fp.p != ')')
	t.yy = fp.next_float ();
      else
	t.yy = t.xx;
    }
    else if (func_name.eq ("rotate"))
    {
      float angle = fp.next_float () * (float) M_PI / 180.f;
      float cs = cosf (angle), sn = sinf (angle);
      fp.skip_ws_comma ();
      if (fp.p < fp.end && *fp.p != ')')
      {
	float cx = fp.next_float ();
	float cy = fp.next_float ();
	t.xx = cs;  t.yx = sn;
	t.xy = -sn; t.yy = cs;
	t.dx = cx - cs * cx + sn * cy;
	t.dy = cy - sn * cx - cs * cy;
      }
      else
      {
	t.xx = cs;  t.yx = sn;
	t.xy = -sn; t.yy = cs;
      }
    }
    else if (func_name.eq ("skewX"))
    {
      float angle = fp.next_float () * (float) M_PI / 180.f;
      t.xy = tanf (angle);
    }
    else if (func_name.eq ("skewY"))
    {
      float angle = fp.next_float () * (float) M_PI / 180.f;
      t.yx = tanf (angle);
    }

    /* Skip to closing ')' */
    while (fp.p < fp.end && *fp.p != ')') fp.p++;
    if (fp.p < fp.end) fp.p++;

    out->multiply (t);
  }
  return true;
}


/*
 * 6. SVG Path Data parser
 */

static void
svg_arc_to_cubics (hb_draw_funcs_t *dfuncs, void *draw_data, hb_draw_state_t *st,
		   float cx, float cy,
		   float rx, float ry,
		   float phi, float theta1, float dtheta)
{
  /* Approximate arc with cubic bezier segments.
   * Split into segments of at most 90 degrees. */
  int n_segs = (int) ceilf (fabsf (dtheta) / ((float) M_PI / 2.f));
  if (n_segs < 1) n_segs = 1;
  float seg_angle = dtheta / n_segs;

  float cos_phi = cosf (phi);
  float sin_phi = sinf (phi);

  for (int i = 0; i < n_segs; i++)
  {
    float t1 = theta1 + i * seg_angle;
    float t2 = t1 + seg_angle;
    float alpha = sinf (seg_angle) * (sqrtf (4.f + 3.f * tanf (seg_angle / 2.f) * tanf (seg_angle / 2.f)) - 1.f) / 3.f;

    float cos_t1 = cosf (t1), sin_t1 = sinf (t1);
    float cos_t2 = cosf (t2), sin_t2 = sinf (t2);

    /* Endpoint 1 (on unit circle) */
    float e1x = rx * cos_t1, e1y = ry * sin_t1;
    /* Endpoint 2 */
    float e2x = rx * cos_t2, e2y = ry * sin_t2;
    /* Derivative at t1 */
    float d1x = -rx * sin_t1, d1y = ry * cos_t1;
    /* Derivative at t2 */
    float d2x = -rx * sin_t2, d2y = ry * cos_t2;

    /* Control points */
    float cp1x = e1x + alpha * d1x;
    float cp1y = e1y + alpha * d1y;
    float cp2x = e2x - alpha * d2x;
    float cp2y = e2y - alpha * d2y;

    /* Rotate and translate */
    float r_cp1x = cos_phi * cp1x - sin_phi * cp1y + cx;
    float r_cp1y = sin_phi * cp1x + cos_phi * cp1y + cy;
    float r_cp2x = cos_phi * cp2x - sin_phi * cp2y + cx;
    float r_cp2y = sin_phi * cp2x + cos_phi * cp2y + cy;
    float r_e2x  = cos_phi * e2x  - sin_phi * e2y  + cx;
    float r_e2y  = sin_phi * e2x  + cos_phi * e2y  + cy;

    hb_draw_cubic_to (dfuncs, draw_data, st,
		      r_cp1x, r_cp1y,
		      r_cp2x, r_cp2y,
		      r_e2x, r_e2y);
  }
}

static void
svg_arc_endpoint_to_center (float x1, float y1, float x2, float y2,
			    float rx, float ry, float phi_deg,
			    bool large_arc, bool sweep,
			    float *cx_out, float *cy_out,
			    float *theta1_out, float *dtheta_out,
			    float *rx_out, float *ry_out)
{
  float phi = phi_deg * (float) M_PI / 180.f;
  float cos_phi = cosf (phi), sin_phi = sinf (phi);

  /* Step 1: compute (x1', y1') */
  float mx = (x1 - x2) / 2.f;
  float my = (y1 - y2) / 2.f;
  float x1p =  cos_phi * mx + sin_phi * my;
  float y1p = -sin_phi * mx + cos_phi * my;

  /* Correct radii */
  rx = fabsf (rx); ry = fabsf (ry);
  if (rx < 1e-10f || ry < 1e-10f)
  {
    *cx_out = (x1 + x2) / 2.f;
    *cy_out = (y1 + y2) / 2.f;
    *theta1_out = 0;
    *dtheta_out = 0;
    *rx_out = rx; *ry_out = ry;
    return;
  }

  float x1p2 = x1p * x1p, y1p2 = y1p * y1p;
  float rx2 = rx * rx, ry2 = ry * ry;
  float lambda = x1p2 / rx2 + y1p2 / ry2;
  if (lambda > 1.f)
  {
    float sl = sqrtf (lambda);
    rx *= sl; ry *= sl;
    rx2 = rx * rx; ry2 = ry * ry;
  }

  /* Step 2: compute (cx', cy') */
  float num = rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2;
  float den = rx2 * y1p2 + ry2 * x1p2;
  float sq = (den > 0.f) ? sqrtf (hb_max (num / den, 0.f)) : 0.f;
  if (large_arc == sweep) sq = -sq;

  float cxp =  sq * rx * y1p / ry;
  float cyp = -sq * ry * x1p / rx;

  /* Step 3: compute (cx, cy) */
  float cx = cos_phi * cxp - sin_phi * cyp + (x1 + x2) / 2.f;
  float cy = sin_phi * cxp + cos_phi * cyp + (y1 + y2) / 2.f;

  /* Step 4: compute theta1 and dtheta */
  auto angle = [] (float ux, float uy, float vx, float vy) -> float {
    float dot = ux * vx + uy * vy;
    float len = sqrtf ((ux * ux + uy * uy) * (vx * vx + vy * vy));
    float a = acosf (hb_clamp (dot / len, -1.f, 1.f));
    if (ux * vy - uy * vx < 0.f) a = -a;
    return a;
  };

  float theta1 = angle (1.f, 0.f, (x1p - cxp) / rx, (y1p - cyp) / ry);
  float dtheta = angle ((x1p - cxp) / rx, (y1p - cyp) / ry,
			(-x1p - cxp) / rx, (-y1p - cyp) / ry);

  if (!sweep && dtheta > 0.f) dtheta -= 2.f * (float) M_PI;
  if (sweep && dtheta < 0.f)  dtheta += 2.f * (float) M_PI;

  *cx_out = cx; *cy_out = cy;
  *theta1_out = theta1; *dtheta_out = dtheta;
  *rx_out = rx; *ry_out = ry;
}

static void
svg_parse_path_data (hb_svg_str_t d, hb_draw_funcs_t *dfuncs, void *draw_data)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  hb_svg_float_parser_t fp (d);

  float cur_x = 0, cur_y = 0;
  float start_x = 0, start_y = 0;
  float last_cx = 0, last_cy = 0; /* for S/T smooth curves */
  char last_cmd = 0;
  char cmd = 0;

  while (fp.p < fp.end)
  {
    fp.skip_ws_comma ();
    if (fp.p >= fp.end) break;

    /* Check if next char is a command letter */
    char c = *fp.p;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
    {
      cmd = c;
      fp.p++;
    }
    /* else repeat previous command with new parameters */

    switch (cmd)
    {
    case 'M': case 'm':
    {
      float x = fp.next_float ();
      float y = fp.next_float ();
      if (cmd == 'm') { x += cur_x; y += cur_y; }
      hb_draw_move_to (dfuncs, draw_data, &st, x, y);
      cur_x = start_x = x;
      cur_y = start_y = y;
      /* Subsequent coordinates after M are treated as L */
      cmd = (cmd == 'M') ? 'L' : 'l';
      last_cmd = 'M';
      continue;
    }
    case 'L': case 'l':
    {
      float x = fp.next_float ();
      float y = fp.next_float ();
      if (cmd == 'l') { x += cur_x; y += cur_y; }
      hb_draw_line_to (dfuncs, draw_data, &st, x, y);
      cur_x = x; cur_y = y;
      break;
    }
    case 'H': case 'h':
    {
      float x = fp.next_float ();
      if (cmd == 'h') x += cur_x;
      hb_draw_line_to (dfuncs, draw_data, &st, x, cur_y);
      cur_x = x;
      break;
    }
    case 'V': case 'v':
    {
      float y = fp.next_float ();
      if (cmd == 'v') y += cur_y;
      hb_draw_line_to (dfuncs, draw_data, &st, cur_x, y);
      cur_y = y;
      break;
    }
    case 'C': case 'c':
    {
      float x1 = fp.next_float ();
      float y1 = fp.next_float ();
      float x2 = fp.next_float ();
      float y2 = fp.next_float ();
      float x  = fp.next_float ();
      float y  = fp.next_float ();
      if (cmd == 'c')
      { x1 += cur_x; y1 += cur_y; x2 += cur_x; y2 += cur_y; x += cur_x; y += cur_y; }
      hb_draw_cubic_to (dfuncs, draw_data, &st, x1, y1, x2, y2, x, y);
      last_cx = x2; last_cy = y2;
      cur_x = x; cur_y = y;
      break;
    }
    case 'S': case 's':
    {
      float cx1, cy1;
      if (last_cmd == 'C' || last_cmd == 'c' || last_cmd == 'S' || last_cmd == 's')
      { cx1 = 2 * cur_x - last_cx; cy1 = 2 * cur_y - last_cy; }
      else
      { cx1 = cur_x; cy1 = cur_y; }
      float x2 = fp.next_float ();
      float y2 = fp.next_float ();
      float x  = fp.next_float ();
      float y  = fp.next_float ();
      if (cmd == 's')
      { x2 += cur_x; y2 += cur_y; x += cur_x; y += cur_y; }
      hb_draw_cubic_to (dfuncs, draw_data, &st, cx1, cy1, x2, y2, x, y);
      last_cx = x2; last_cy = y2;
      cur_x = x; cur_y = y;
      break;
    }
    case 'Q': case 'q':
    {
      float x1 = fp.next_float ();
      float y1 = fp.next_float ();
      float x  = fp.next_float ();
      float y  = fp.next_float ();
      if (cmd == 'q')
      { x1 += cur_x; y1 += cur_y; x += cur_x; y += cur_y; }
      hb_draw_quadratic_to (dfuncs, draw_data, &st, x1, y1, x, y);
      last_cx = x1; last_cy = y1;
      cur_x = x; cur_y = y;
      break;
    }
    case 'T': case 't':
    {
      float cx1, cy1;
      if (last_cmd == 'Q' || last_cmd == 'q' || last_cmd == 'T' || last_cmd == 't')
      { cx1 = 2 * cur_x - last_cx; cy1 = 2 * cur_y - last_cy; }
      else
      { cx1 = cur_x; cy1 = cur_y; }
      float x = fp.next_float ();
      float y = fp.next_float ();
      if (cmd == 't') { x += cur_x; y += cur_y; }
      hb_draw_quadratic_to (dfuncs, draw_data, &st, cx1, cy1, x, y);
      last_cx = cx1; last_cy = cy1;
      cur_x = x; cur_y = y;
      break;
    }
    case 'A': case 'a':
    {
      float rx = fp.next_float ();
      float ry = fp.next_float ();
      float x_rot = fp.next_float ();
      bool large_arc = fp.next_flag ();
      bool sweep = fp.next_flag ();
      float x = fp.next_float ();
      float y = fp.next_float ();
      if (cmd == 'a') { x += cur_x; y += cur_y; }

      if (fabsf (x - cur_x) < 1e-6f && fabsf (y - cur_y) < 1e-6f)
      {
	cur_x = x; cur_y = y;
	break;
      }

      float cx, cy, theta1, dtheta, adj_rx, adj_ry;
      svg_arc_endpoint_to_center (cur_x, cur_y, x, y,
				  rx, ry, x_rot,
				  large_arc, sweep,
				  &cx, &cy, &theta1, &dtheta,
				  &adj_rx, &adj_ry);

      float phi = x_rot * (float) M_PI / 180.f;
      svg_arc_to_cubics (dfuncs, draw_data, &st,
			 cx, cy, adj_rx, adj_ry, phi, theta1, dtheta);
      cur_x = x; cur_y = y;
      break;
    }
    case 'Z': case 'z':
      hb_draw_close_path (dfuncs, draw_data, &st);
      cur_x = start_x;
      cur_y = start_y;
      break;

    default:
      /* Unknown command, skip a character */
      fp.p++;
      continue;
    }

    last_cmd = cmd;
  }
}


/*
 * 7. Shape-to-path converters
 */

static void
svg_rect_to_path (float x, float y, float w, float h, float rx, float ry,
		  hb_draw_funcs_t *dfuncs, void *draw_data)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

  if (rx <= 0 && ry <= 0)
  {
    /* Simple rectangle */
    hb_draw_move_to (dfuncs, draw_data, &st, x, y);
    hb_draw_line_to (dfuncs, draw_data, &st, x + w, y);
    hb_draw_line_to (dfuncs, draw_data, &st, x + w, y + h);
    hb_draw_line_to (dfuncs, draw_data, &st, x, y + h);
    hb_draw_close_path (dfuncs, draw_data, &st);
    return;
  }

  /* Rounded rectangle */
  if (rx <= 0) rx = ry;
  if (ry <= 0) ry = rx;
  rx = hb_min (rx, w / 2);
  ry = hb_min (ry, h / 2);

  /* Kappa for circular arcs approximated by cubic bezier */
  float kx = rx * 0.5522847498f;
  float ky = ry * 0.5522847498f;

  hb_draw_move_to (dfuncs, draw_data, &st, x + rx, y);
  hb_draw_line_to (dfuncs, draw_data, &st, x + w - rx, y);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    x + w - rx + kx, y,
		    x + w, y + ry - ky,
		    x + w, y + ry);
  hb_draw_line_to (dfuncs, draw_data, &st, x + w, y + h - ry);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    x + w, y + h - ry + ky,
		    x + w - rx + kx, y + h,
		    x + w - rx, y + h);
  hb_draw_line_to (dfuncs, draw_data, &st, x + rx, y + h);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    x + rx - kx, y + h,
		    x, y + h - ry + ky,
		    x, y + h - ry);
  hb_draw_line_to (dfuncs, draw_data, &st, x, y + ry);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    x, y + ry - ky,
		    x + rx - kx, y,
		    x + rx, y);
  hb_draw_close_path (dfuncs, draw_data, &st);
}

static void
svg_circle_to_path (float cx, float cy, float r,
		    hb_draw_funcs_t *dfuncs, void *draw_data)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  float k = r * 0.5522847498f;

  hb_draw_move_to (dfuncs, draw_data, &st, cx + r, cy);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx + r, cy + k,
		    cx + k, cy + r,
		    cx, cy + r);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx - k, cy + r,
		    cx - r, cy + k,
		    cx - r, cy);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx - r, cy - k,
		    cx - k, cy - r,
		    cx, cy - r);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx + k, cy - r,
		    cx + r, cy - k,
		    cx + r, cy);
  hb_draw_close_path (dfuncs, draw_data, &st);
}

static void
svg_ellipse_to_path (float cx, float cy, float rx, float ry,
		     hb_draw_funcs_t *dfuncs, void *draw_data)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  float kx = rx * 0.5522847498f;
  float ky = ry * 0.5522847498f;

  hb_draw_move_to (dfuncs, draw_data, &st, cx + rx, cy);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx + rx, cy + ky,
		    cx + kx, cy + ry,
		    cx, cy + ry);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx - kx, cy + ry,
		    cx - rx, cy + ky,
		    cx - rx, cy);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx - rx, cy - ky,
		    cx - kx, cy - ry,
		    cx, cy - ry);
  hb_draw_cubic_to (dfuncs, draw_data, &st,
		    cx + kx, cy - ry,
		    cx + rx, cy - ky,
		    cx + rx, cy);
  hb_draw_close_path (dfuncs, draw_data, &st);
}

static void
svg_line_to_path (float x1, float y1, float x2, float y2,
		  hb_draw_funcs_t *dfuncs, void *draw_data)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  hb_draw_move_to (dfuncs, draw_data, &st, x1, y1);
  hb_draw_line_to (dfuncs, draw_data, &st, x2, y2);
}

static void
svg_polygon_to_path (hb_svg_str_t points, bool close,
		     hb_draw_funcs_t *dfuncs, void *draw_data)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
  hb_svg_float_parser_t fp (points);
  bool first = true;
  while (fp.has_more ())
  {
    float x = fp.next_float ();
    float y = fp.next_float ();
    if (first)
    {
      hb_draw_move_to (dfuncs, draw_data, &st, x, y);
      first = false;
    }
    else
      hb_draw_line_to (dfuncs, draw_data, &st, x, y);
  }
  if (close && !first)
    hb_draw_close_path (dfuncs, draw_data, &st);
}


/*
 * 8. Defs store
 */

enum hb_svg_gradient_type_t
{
  SVG_GRADIENT_LINEAR,
  SVG_GRADIENT_RADIAL
};

struct hb_svg_gradient_stop_t
{
  float offset;
  hb_color_t color;
};

struct hb_svg_gradient_t
{
  hb_svg_gradient_type_t type;

  /* Common */
  hb_paint_extend_t spread = HB_PAINT_EXTEND_PAD;
  hb_svg_transform_t gradient_transform;
  bool has_gradient_transform = false;
  bool units_user_space = false; /* false = objectBoundingBox (default) */

  /* Linear gradient: x1, y1, x2, y2 */
  float x1 = 0, y1 = 0, x2 = 1, y2 = 0;

  /* Radial gradient: cx, cy, r, fx, fy */
  float cx = 0.5f, cy = 0.5f, r = 0.5f;
  float fx = -1.f, fy = -1.f; /* -1 = not set, use cx/cy */

  hb_vector_t<hb_svg_gradient_stop_t> stops;

  /* href reference to another gradient */
  char href_id[64] = {};
};

struct hb_svg_clip_path_def_t
{
  /* Store the raw XML content for deferred parsing */
  const char *content_start;
  unsigned content_len;
};

struct hb_svg_def_t
{
  char id[64];
  enum { DEF_GRADIENT, DEF_CLIP_PATH, DEF_ELEMENT } type;
  unsigned index; /* index into respective array */
};

struct hb_svg_defs_t
{
  hb_vector_t<hb_svg_gradient_t> gradients;
  hb_vector_t<hb_svg_def_t> defs;

  void add_gradient (const char *id, const hb_svg_gradient_t &grad)
  {
    unsigned idx = gradients.length;
    gradients.push (grad);
    hb_svg_def_t def;
    unsigned n = hb_min (strlen (id), (size_t) sizeof (def.id) - 1);
    memcpy (def.id, id, n);
    def.id[n] = '\0';
    def.type = hb_svg_def_t::DEF_GRADIENT;
    def.index = idx;
    defs.push (def);
  }

  const hb_svg_gradient_t *find_gradient (const char *id) const
  {
    for (unsigned i = 0; i < defs.length; i++)
      if (defs[i].type == hb_svg_def_t::DEF_GRADIENT &&
	  strcmp (defs[i].id, id) == 0)
	return &gradients[defs[i].index];
    return nullptr;
  }

  const hb_svg_gradient_t *find_gradient_str (hb_svg_str_t s) const
  {
    /* Handle url(#id) references */
    if (s.starts_with ("url("))
    {
      const char *p = s.data + 4;
      const char *e = s.data + s.len;
      while (p < e && *p == ' ') p++;
      if (p < e && *p == '#') p++;
      const char *start = p;
      while (p < e && *p != ')') p++;
      char buf[64];
      unsigned n = hb_min ((unsigned) (p - start), (unsigned) sizeof (buf) - 1);
      memcpy (buf, start, n);
      buf[n] = '\0';
      return find_gradient (buf);
    }
    return nullptr;
  }
};


/*
 * 9. Gradient handler — construct hb_color_line_t
 */

struct hb_svg_color_line_data_t
{
  const hb_svg_gradient_t *grad;
  hb_paint_extend_t extend;
};

static unsigned
svg_color_line_get_stops (hb_color_line_t *color_line HB_UNUSED,
			  void *color_line_data,
			  unsigned int start,
			  unsigned int *count,
			  hb_color_stop_t *color_stops,
			  void *user_data HB_UNUSED)
{
  hb_svg_color_line_data_t *cl = (hb_svg_color_line_data_t *) color_line_data;
  const hb_svg_gradient_t *grad = cl->grad;
  unsigned total = grad->stops.length;

  if (count)
  {
    unsigned n = hb_min (*count, total > start ? total - start : 0u);
    for (unsigned i = 0; i < n; i++)
    {
      color_stops[i].offset = grad->stops[start + i].offset;
      color_stops[i].is_foreground = false;
      color_stops[i].color = grad->stops[start + i].color;
    }
    *count = n;
  }
  return total;
}

static hb_paint_extend_t
svg_color_line_get_extend (hb_color_line_t *color_line HB_UNUSED,
			   void *color_line_data,
			   void *user_data HB_UNUSED)
{
  hb_svg_color_line_data_t *cl = (hb_svg_color_line_data_t *) color_line_data;
  return cl->extend;
}


/*
 * 10. SVG rendering context
 */

#define SVG_MAX_DEPTH 32

struct hb_svg_render_context_t
{
  hb_raster_paint_t *paint;
  hb_paint_funcs_t *pfuncs;
  hb_font_t *font;
  hb_color_t foreground;
  hb_svg_defs_t defs;
  int depth = 0;

  /* The full SVG document for <use> resolution */
  const char *doc_start;
  unsigned doc_len;

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


/*
 * Emit fill (solid or gradient)
 */

static void
svg_emit_fill (hb_svg_render_context_t *ctx,
	       hb_svg_str_t fill_str,
	       float fill_opacity)
{
  bool is_none = false;

  /* Check for gradient reference */
  const hb_svg_gradient_t *grad = ctx->defs.find_gradient_str (fill_str);
  if (grad)
  {
    /* Resolve href chain */
    const hb_svg_gradient_t *resolved = grad;
    unsigned max_chain = 8;
    while (resolved && resolved->stops.length == 0 && resolved->href_id[0] && max_chain--)
    {
      resolved = ctx->defs.find_gradient (resolved->href_id);
    }
    if (!resolved || resolved->stops.length == 0)
    {
      /* Empty gradient = transparent */
      return;
    }

    /* Merge attributes from referenced gradient if needed */
    const hb_svg_gradient_t *attr_grad = grad;
    const hb_svg_gradient_t *stop_grad = resolved;

    hb_svg_color_line_data_t cl_data;
    cl_data.grad = stop_grad;
    cl_data.extend = attr_grad->spread;

    hb_color_line_t cl = {
      &cl_data,
      svg_color_line_get_stops, nullptr,
      svg_color_line_get_extend, nullptr
    };

    if (attr_grad->has_gradient_transform)
      ctx->push_transform (attr_grad->gradient_transform.xx,
			    attr_grad->gradient_transform.yx,
			    attr_grad->gradient_transform.xy,
			    attr_grad->gradient_transform.yy,
			    attr_grad->gradient_transform.dx,
			    attr_grad->gradient_transform.dy);

    if (attr_grad->type == SVG_GRADIENT_LINEAR)
    {
      hb_paint_linear_gradient (ctx->pfuncs, ctx->paint, &cl,
				attr_grad->x1, attr_grad->y1,
				attr_grad->x2, attr_grad->y2,
				attr_grad->x2 - (attr_grad->y2 - attr_grad->y1),
				attr_grad->y2 + (attr_grad->x2 - attr_grad->x1));
    }
    else /* SVG_GRADIENT_RADIAL */
    {
      float fx = attr_grad->fx >= 0 ? attr_grad->fx : attr_grad->cx;
      float fy = attr_grad->fy >= 0 ? attr_grad->fy : attr_grad->cy;

      hb_paint_radial_gradient (ctx->pfuncs, ctx->paint, &cl,
				fx, fy, 0.f,
				attr_grad->cx, attr_grad->cy, attr_grad->r);
    }

    if (attr_grad->has_gradient_transform)
      ctx->pop_transform ();

    return;
  }

  /* Solid color */
  hb_color_t color = svg_parse_color (fill_str, ctx->foreground, hb_font_get_face (ctx->font), &is_none);
  if (is_none) return;

  if (fill_opacity < 1.f)
    color = HB_COLOR (hb_color_get_blue (color),
		      hb_color_get_green (color),
		      hb_color_get_red (color),
		      (uint8_t) (hb_color_get_alpha (color) * fill_opacity + 0.5f));

  ctx->paint_color (color);
}


/*
 * Shape path emitter callback
 */

struct hb_svg_shape_emit_data_t
{
  enum { SHAPE_PATH, SHAPE_RECT, SHAPE_CIRCLE, SHAPE_ELLIPSE,
	 SHAPE_LINE, SHAPE_POLYLINE, SHAPE_POLYGON } type;
  hb_svg_str_t str_data;    /* for path 'd' or polyline/polygon 'points' */
  float params[6];           /* for rect/circle/ellipse/line */
};

static void
svg_shape_path_emit (hb_draw_funcs_t *dfuncs, void *draw_data, void *user_data)
{
  hb_svg_shape_emit_data_t *shape = (hb_svg_shape_emit_data_t *) user_data;
  switch (shape->type)
  {
  case hb_svg_shape_emit_data_t::SHAPE_PATH:
    svg_parse_path_data (shape->str_data, dfuncs, draw_data);
    break;
  case hb_svg_shape_emit_data_t::SHAPE_RECT:
    svg_rect_to_path (shape->params[0], shape->params[1],
		      shape->params[2], shape->params[3],
		      shape->params[4], shape->params[5],
		      dfuncs, draw_data);
    break;
  case hb_svg_shape_emit_data_t::SHAPE_CIRCLE:
    svg_circle_to_path (shape->params[0], shape->params[1], shape->params[2],
			dfuncs, draw_data);
    break;
  case hb_svg_shape_emit_data_t::SHAPE_ELLIPSE:
    svg_ellipse_to_path (shape->params[0], shape->params[1],
			 shape->params[2], shape->params[3],
			 dfuncs, draw_data);
    break;
  case hb_svg_shape_emit_data_t::SHAPE_LINE:
    svg_line_to_path (shape->params[0], shape->params[1],
		      shape->params[2], shape->params[3],
		      dfuncs, draw_data);
    break;
  case hb_svg_shape_emit_data_t::SHAPE_POLYLINE:
    svg_polygon_to_path (shape->str_data, false, dfuncs, draw_data);
    break;
  case hb_svg_shape_emit_data_t::SHAPE_POLYGON:
    svg_polygon_to_path (shape->str_data, true, dfuncs, draw_data);
    break;
  }
}


/*
 * 11. Element renderer — recursive SVG rendering
 */

static void svg_render_element (hb_svg_render_context_t *ctx,
				hb_svg_xml_parser_t &parser,
				hb_svg_str_t inherited_fill,
				float inherited_fill_opacity);

/* Parse a gradient <stop> element */
static void
svg_parse_gradient_stop (hb_svg_xml_parser_t &parser,
			 hb_svg_gradient_t &grad,
			 hb_color_t foreground,
			 hb_face_t *face)
{
  hb_svg_str_t offset_str = parser.find_attr ("offset");
  hb_svg_str_t color_str = parser.find_attr ("stop-color");
  hb_svg_str_t opacity_str = parser.find_attr ("stop-opacity");

  float offset = 0;
  if (offset_str.len)
  {
    offset = svg_parse_float (offset_str);
    /* Check for percentage */
    if (offset_str.len && offset_str.data[offset_str.len - 1] == '%')
      offset /= 100.f;
  }

  bool is_none = false;
  hb_color_t color = HB_COLOR (0, 0, 0, 255);
  if (color_str.len)
    color = svg_parse_color (color_str, foreground, face, &is_none);

  if (opacity_str.len)
  {
    float opacity = svg_parse_float (opacity_str);
    color = HB_COLOR (hb_color_get_blue (color),
		      hb_color_get_green (color),
		      hb_color_get_red (color),
		      (uint8_t) (hb_color_get_alpha (color) * opacity + 0.5f));
  }

  hb_svg_gradient_stop_t stop = {offset, color};
  grad.stops.push (stop);
}

/* Parse gradient element attributes */
static void
svg_parse_gradient_attrs (hb_svg_xml_parser_t &parser,
			  hb_svg_gradient_t &grad)
{
  hb_svg_str_t spread_str = parser.find_attr ("spreadMethod");
  if (spread_str.eq ("reflect"))
    grad.spread = HB_PAINT_EXTEND_REFLECT;
  else if (spread_str.eq ("repeat"))
    grad.spread = HB_PAINT_EXTEND_REPEAT;

  hb_svg_str_t units_str = parser.find_attr ("gradientUnits");
  if (units_str.eq ("userSpaceOnUse"))
    grad.units_user_space = true;

  hb_svg_str_t transform_str = parser.find_attr ("gradientTransform");
  if (transform_str.len)
  {
    grad.has_gradient_transform = true;
    svg_parse_transform (transform_str, &grad.gradient_transform);
  }

  /* xlink:href or href */
  hb_svg_str_t href = parser.find_attr ("href");
  if (href.is_null ())
    href = parser.find_attr ("xlink:href");
  if (href.len && href.data[0] == '#')
  {
    unsigned n = hb_min (href.len - 1, (unsigned) sizeof (grad.href_id) - 1);
    memcpy (grad.href_id, href.data + 1, n);
    grad.href_id[n] = '\0';
  }
}

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
      {
	hb_svg_gradient_t grad;
	grad.type = SVG_GRADIENT_LINEAR;
	grad.x1 = svg_parse_float (parser.find_attr ("x1"));
	grad.y1 = svg_parse_float (parser.find_attr ("y1"));
	grad.x2 = svg_parse_float (parser.find_attr ("x2"));
	grad.y2 = svg_parse_float (parser.find_attr ("y2"));

	/* Default x2 to 1 if not explicitly set (objectBoundingBox default) */
	if (parser.find_attr ("x2").is_null ())
	  grad.x2 = 1.f;

	svg_parse_gradient_attrs (parser, grad);

	hb_svg_str_t id = parser.find_attr ("id");

	if (tok == SVG_TOKEN_OPEN_TAG)
	{
	  /* Parse child <stop> elements */
	  int gdepth = 1;
	  while (gdepth > 0)
	  {
	    hb_svg_token_type_t gt = parser.next ();
	    if (gt == SVG_TOKEN_EOF) break;
	    if (gt == SVG_TOKEN_CLOSE_TAG) { gdepth--; continue; }
	    if ((gt == SVG_TOKEN_OPEN_TAG || gt == SVG_TOKEN_SELF_CLOSE_TAG) &&
		parser.tag_name.eq ("stop"))
	      svg_parse_gradient_stop (parser, grad, ctx->foreground, hb_font_get_face (ctx->font));
	    if (gt == SVG_TOKEN_OPEN_TAG && !parser.tag_name.eq ("stop"))
	      gdepth++;
	  }
	}

	if (id.len)
	{
	  char id_buf[64];
	  unsigned n = hb_min (id.len, (unsigned) sizeof (id_buf) - 1);
	  memcpy (id_buf, id.data, n);
	  id_buf[n] = '\0';
	  ctx->defs.add_gradient (id_buf, grad);
	}
      }
      else if (parser.tag_name.eq ("radialGradient"))
      {
	hb_svg_gradient_t grad;
	grad.type = SVG_GRADIENT_RADIAL;

	hb_svg_str_t cx_str = parser.find_attr ("cx");
	hb_svg_str_t cy_str = parser.find_attr ("cy");
	hb_svg_str_t r_str = parser.find_attr ("r");
	hb_svg_str_t fx_str = parser.find_attr ("fx");
	hb_svg_str_t fy_str = parser.find_attr ("fy");

	if (cx_str.len) grad.cx = svg_parse_float (cx_str);
	if (cy_str.len) grad.cy = svg_parse_float (cy_str);
	if (r_str.len) grad.r = svg_parse_float (r_str);
	if (fx_str.len) grad.fx = svg_parse_float (fx_str);
	if (fy_str.len) grad.fy = svg_parse_float (fy_str);

	svg_parse_gradient_attrs (parser, grad);

	hb_svg_str_t id = parser.find_attr ("id");

	if (tok == SVG_TOKEN_OPEN_TAG)
	{
	  int gdepth = 1;
	  while (gdepth > 0)
	  {
	    hb_svg_token_type_t gt = parser.next ();
	    if (gt == SVG_TOKEN_EOF) break;
	    if (gt == SVG_TOKEN_CLOSE_TAG) { gdepth--; continue; }
	    if ((gt == SVG_TOKEN_OPEN_TAG || gt == SVG_TOKEN_SELF_CLOSE_TAG) &&
		parser.tag_name.eq ("stop"))
	      svg_parse_gradient_stop (parser, grad, ctx->foreground, hb_font_get_face (ctx->font));
	    if (gt == SVG_TOKEN_OPEN_TAG && !parser.tag_name.eq ("stop"))
	      gdepth++;
	  }
	}

	if (id.len)
	{
	  char id_buf[64];
	  unsigned n = hb_min (id.len, (unsigned) sizeof (id_buf) - 1);
	  memcpy (id_buf, id.data, n);
	  id_buf[n] = '\0';
	  ctx->defs.add_gradient (id_buf, grad);
	}
      }
      else
      {
	/* Skip unknown defs children */
	if (tok == SVG_TOKEN_OPEN_TAG)
	  depth++;
      }
    }
  }
}


/* Render a single shape element */
static void
svg_render_shape (hb_svg_render_context_t *ctx,
		  hb_svg_shape_emit_data_t &shape,
		  hb_svg_str_t fill_str,
		  float fill_opacity,
		  float opacity,
		  hb_svg_str_t transform_str)
{
  bool has_transform = transform_str.len > 0;
  bool has_opacity = opacity < 1.f;

  if (has_opacity)
    ctx->push_group ();

  if (has_transform)
  {
    hb_svg_transform_t t;
    svg_parse_transform (transform_str, &t);
    ctx->push_transform (t.xx, t.yx, t.xy, t.yy, t.dx, t.dy);
  }

  /* Clip with shape path, then fill */
  hb_raster_paint_push_clip_path (ctx->paint, svg_shape_path_emit, &shape);

  /* Default fill is black */
  if (fill_str.is_null ())
  {
    hb_color_t black = HB_COLOR (0, 0, 0, 255);
    if (fill_opacity < 1.f)
      black = HB_COLOR (0, 0, 0, (uint8_t) (255 * fill_opacity + 0.5f));
    ctx->paint_color (black);
  }
  else
    svg_emit_fill (ctx, fill_str, fill_opacity);

  ctx->pop_clip ();

  if (has_transform)
    ctx->pop_transform ();

  if (has_opacity)
    ctx->pop_group (HB_PAINT_COMPOSITE_MODE_SRC_OVER);
}

/* Render one element (may be a container or shape) */
static void
svg_render_element (hb_svg_render_context_t *ctx,
		    hb_svg_xml_parser_t &parser,
		    hb_svg_str_t inherited_fill,
		    float inherited_fill_opacity)
{
  if (ctx->depth >= SVG_MAX_DEPTH) return;
  ctx->depth++;

  hb_svg_str_t tag = parser.tag_name;
  bool self_closing = parser.self_closing;

  /* Extract common attributes */
  hb_svg_str_t fill_attr = parser.find_attr ("fill");
  hb_svg_str_t fill_opacity_str = parser.find_attr ("fill-opacity");
  hb_svg_str_t opacity_str = parser.find_attr ("opacity");
  hb_svg_str_t transform_str = parser.find_attr ("transform");

  hb_svg_str_t fill_str = fill_attr.is_null () ? inherited_fill : fill_attr;
  float fill_opacity = fill_opacity_str.len ? svg_parse_float (fill_opacity_str) : inherited_fill_opacity;
  float opacity = opacity_str.len ? svg_parse_float (opacity_str) : 1.f;

  if (tag.eq ("g") || tag.eq ("svg"))
  {
    bool has_transform = transform_str.len > 0;
    bool has_opacity = opacity < 1.f;
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
    {
      ctx->push_transform (1, 0, 0, 1, -vb_x, -vb_y);
    }

    /* Process children */
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
	  svg_render_element (ctx, parser, fill_str, fill_opacity);
	  if (tok == SVG_TOKEN_OPEN_TAG && !child_tag.eq ("g") &&
	      !child_tag.eq ("svg") && !child_tag.eq ("use"))
	  {
	    /* Skip children of non-container elements we don't handle */
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

    if (has_transform)
      ctx->pop_transform ();

    if (has_opacity)
      ctx->pop_group (HB_PAINT_COMPOSITE_MODE_SRC_OVER);
  }
  else if (tag.eq ("path"))
  {
    hb_svg_str_t d = parser.find_attr ("d");
    if (d.len)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_PATH;
      shape.str_data = d;
      svg_render_shape (ctx, shape, fill_str, fill_opacity, opacity, transform_str);
    }
  }
  else if (tag.eq ("rect"))
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
      svg_render_shape (ctx, shape, fill_str, fill_opacity, opacity, transform_str);
    }
  }
  else if (tag.eq ("circle"))
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
      svg_render_shape (ctx, shape, fill_str, fill_opacity, opacity, transform_str);
    }
  }
  else if (tag.eq ("ellipse"))
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
      svg_render_shape (ctx, shape, fill_str, fill_opacity, opacity, transform_str);
    }
  }
  else if (tag.eq ("line"))
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
    svg_render_shape (ctx, shape, fill_str, fill_opacity, opacity, transform_str);
  }
  else if (tag.eq ("polyline"))
  {
    hb_svg_str_t points = parser.find_attr ("points");
    if (points.len)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_POLYLINE;
      shape.str_data = points;
      svg_render_shape (ctx, shape, fill_str, fill_opacity, opacity, transform_str);
    }
  }
  else if (tag.eq ("polygon"))
  {
    hb_svg_str_t points = parser.find_attr ("points");
    if (points.len)
    {
      hb_svg_shape_emit_data_t shape;
      shape.type = hb_svg_shape_emit_data_t::SHAPE_POLYGON;
      shape.str_data = points;
      svg_render_shape (ctx, shape, fill_str, fill_opacity, opacity, transform_str);
    }
  }
  else if (tag.eq ("use"))
  {
    hb_svg_str_t href = parser.find_attr ("href");
    if (href.is_null ())
      href = parser.find_attr ("xlink:href");

    if (href.len && href.data[0] == '#')
    {
      /* Find referenced element in the document */
      char ref_id[64];
      unsigned n = hb_min (href.len - 1, (unsigned) sizeof (ref_id) - 1);
      memcpy (ref_id, href.data + 1, n);
      ref_id[n] = '\0';

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

      /* Search for element with matching id in document */
      char id_search[80];
      snprintf (id_search, sizeof (id_search), "id=\"%s\"", ref_id);

      const char *found = nullptr;
      const char *search_p = ctx->doc_start;
      unsigned search_len = ctx->doc_len;
      unsigned id_search_len = strlen (id_search);

      while (search_p + id_search_len <= ctx->doc_start + search_len)
      {
	const char *match = (const char *) memchr (search_p, 'i',
						   ctx->doc_start + search_len - search_p);
	if (!match) break;
	if ((unsigned) (ctx->doc_start + search_len - match) >= id_search_len &&
	    memcmp (match, id_search, id_search_len) == 0)
	{
	  /* Found it; backtrack to find the '<' */
	  const char *tag_start = match;
	  while (tag_start > ctx->doc_start && tag_start[-1] != '<')
	    tag_start--;
	  if (tag_start > ctx->doc_start)
	  {
	    found = tag_start - 1;
	    break;
	  }
	}
	search_p = match + 1;
      }

      if (found)
      {
	unsigned remaining = ctx->doc_start + ctx->doc_len - found;
	hb_svg_xml_parser_t ref_parser (found, remaining);
	hb_svg_token_type_t tok = ref_parser.next ();
	if (tok == SVG_TOKEN_OPEN_TAG || tok == SVG_TOKEN_SELF_CLOSE_TAG)
	  svg_render_element (ctx, ref_parser, fill_str, fill_opacity);
      }

      if (has_translate)
	ctx->pop_transform ();

      if (has_use_transform)
	ctx->pop_transform ();
    }
  }

  ctx->depth--;
}


/*
 * 12. Entry point
 */

hb_bool_t
hb_raster_svg_render (hb_raster_paint_t *paint,
		      hb_blob_t *blob,
		      hb_codepoint_t glyph,
		      hb_font_t *font,
		      hb_color_t foreground)
{
  unsigned data_len;
  const char *data = hb_blob_get_data (blob, &data_len);
  if (!data || !data_len) return false;

  /* The SVG document may contain multiple glyphs.
   * We need to find the element with id="glyphN" where N is the glyph ID. */
  char glyph_id_str[32];
  snprintf (glyph_id_str, sizeof (glyph_id_str), "glyph%u", glyph);

  hb_paint_funcs_t *pfuncs = hb_raster_paint_get_funcs ();

  hb_svg_render_context_t ctx;
  ctx.paint = paint;
  ctx.pfuncs = pfuncs;
  ctx.font = font;
  ctx.foreground = foreground;
  ctx.doc_start = data;
  ctx.doc_len = data_len;

  /* First pass: collect defs (gradients, clip-paths) from the entire document */
  {
    hb_svg_xml_parser_t defs_parser (data, data_len);
    while (true)
    {
      hb_svg_token_type_t tok = defs_parser.next ();
      if (tok == SVG_TOKEN_EOF) break;
      if ((tok == SVG_TOKEN_OPEN_TAG) && defs_parser.tag_name.eq ("defs"))
	svg_process_defs (&ctx, defs_parser);
    }
  }

  /* Second pass: find and render the target glyph element */
  hb_svg_xml_parser_t parser (data, data_len);
  bool found_glyph = false;

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
	  /* Push font transform (design units → scaled units) combined
	   * with y-flip (SVG y-down → paint y-up). */
	  hb_paint_push_font_transform (ctx.pfuncs, ctx.paint, font);
	  ctx.push_transform (1, 0, 0, -1, 0, 0);

	  svg_render_element (&ctx, parser, hb_svg_str_t (), 1.f);

	  ctx.pop_transform ();
	  hb_paint_pop_transform (ctx.pfuncs, ctx.paint);
	  found_glyph = true;
	  break;
	}
      }
    }
  }

  return found_glyph;
}
