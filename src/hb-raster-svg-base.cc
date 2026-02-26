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

#include "hb-raster-svg-base.hh"

static inline char
svg_ascii_lower (char c)
{
  if (c >= 'A' && c <= 'Z')
    return c + ('a' - 'A');
  return c;
}

bool
svg_str_eq_ascii_ci (hb_svg_str_t s, const char *lit)
{
  unsigned n = (unsigned) strlen (lit);
  if (s.len != n) return false;
  for (unsigned i = 0; i < n; i++)
    if (svg_ascii_lower (s.data[i]) != svg_ascii_lower (lit[i]))
      return false;
  return true;
}

bool
svg_str_starts_with_ascii_ci (hb_svg_str_t s, const char *lit)
{
  unsigned n = (unsigned) strlen (lit);
  if (s.len < n) return false;
  for (unsigned i = 0; i < n; i++)
    if (svg_ascii_lower (s.data[i]) != svg_ascii_lower (lit[i]))
      return false;
  return true;
}

void
svg_parse_style_props (hb_svg_str_t style, hb_svg_style_props_t *out)
{
  if (style.is_null ()) return;
  const char *p = style.data;
  const char *end = style.data + style.len;
  while (p < end)
  {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ';'))
      p++;
    if (p >= end) break;
    const char *name_start = p;
    while (p < end && *p != ':' && *p != ';')
      p++;
    const char *name_end = p;
    while (name_end > name_start &&
           (name_end[-1] == ' ' || name_end[-1] == '\t' || name_end[-1] == '\n' || name_end[-1] == '\r'))
      name_end--;
    if (p >= end || *p != ':')
    {
      while (p < end && *p != ';') p++;
      continue;
    }
    p++; /* skip ':' */
    const char *value_start = p;
    while (p < end && *p != ';')
      p++;
    const char *value_end = p;
    while (value_start < value_end &&
           (*value_start == ' ' || *value_start == '\t' || *value_start == '\n' || *value_start == '\r'))
      value_start++;
    while (value_end > value_start &&
           (value_end[-1] == ' ' || value_end[-1] == '\t' || value_end[-1] == '\n' || value_end[-1] == '\r'))
      value_end--;

    hb_svg_str_t prop_name = {name_start, (unsigned) (name_end - name_start)};
    hb_svg_str_t prop_value = {value_start, (unsigned) (value_end - value_start)};
    if (svg_str_eq_ascii_ci (prop_name, "fill")) out->fill = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "fill-opacity")) out->fill_opacity = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "opacity")) out->opacity = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "transform")) out->transform = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "clip-path")) out->clip_path = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "display")) out->display = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "color")) out->color = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "visibility")) out->visibility = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "offset")) out->offset = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "stop-color")) out->stop_color = prop_value;
    else if (svg_str_eq_ascii_ci (prop_name, "stop-opacity")) out->stop_opacity = prop_value;

    if (p < end && *p == ';') p++;
  }
}

float
svg_parse_number_or_percent (hb_svg_str_t s, bool *is_percent)
{
  if (is_percent) *is_percent = false;
  s = s.trim ();
  if (!s.len) return 0.f;
  if (s.data[s.len - 1] == '%')
  {
    if (is_percent) *is_percent = true;
    hb_svg_str_t n = {s.data, s.len - 1};
    return n.to_float () / 100.f;
  }
  return s.to_float ();
}
