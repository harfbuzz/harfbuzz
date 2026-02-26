/*
 * Copyright © 2026  Google, Inc.
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
 */

#ifndef HB_VECTOR_OUTPUT_HH
#define HB_VECTOR_OUTPUT_HH

#include <algorithm>
#include <math.h>
#include <string>
#include <vector>

#include "output-options.hh"
#include "view-options.hh"
#include "hb-vector.h"
#include "hb-ot.h"


struct vector_output_t : output_options_t<>, view_options_t
{
  static const bool repeat_shape = false;

  ~vector_output_t ()
  {
    hb_font_destroy (font);
    hb_font_destroy (upem_font);
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Draw text with given font.");
    parser->set_description ("Shows shaped glyph outlines as SVG.");

    output_options_t::add_options (parser);
    view_options_t::add_options (parser);

    GOptionEntry entries[] =
    {
      {"flat",       0, 0, G_OPTION_ARG_NONE,   &this->flat,          "Flatten geometry and disable reuse", nullptr},
      {"precision",  0, 0, G_OPTION_ARG_INT,    &this->precision,     "Decimal precision (default: 2)",     "N"},
      {nullptr}
    };
    parser->add_group (entries,
                       "vector",
                       "Vector options:",
                       "Options for vector output",
                       this);
  }

  void post_parse (GError **error)
  {
    view_options_t::post_parse (error);
    if (error && *error)
      return;

    if (precision < 0)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "precision must be non-negative");
      return;
    }

    foreground = HB_COLOR (0, 0, 0, 255);
    if (foreground_use_palette && foreground_palette && foreground_palette->len)
    {
      const rgba_color_t &c = g_array_index (foreground_palette, rgba_color_t, 0);
      foreground = HB_COLOR (c.b, c.g, c.r, c.a);
    }
    else if (fore && *fore)
    {
      unsigned r, g, b, a;
      if (!parse_color (fore, r, g, b, a))
        return;
      foreground = HB_COLOR (b, g, r, a);
    }

    has_background = false;
    if (back && *back)
    {
      unsigned r, g, b, a;
      if (!parse_color (back, r, g, b, a))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Invalid background color: %s", back);
        return;
      }
      background = HB_COLOR (b, g, r, a);
      has_background = true;
    }

    if (!parse_custom_palette_overrides (error))
      return;
  }

  template <typename app_t>
  void init (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts)
  {
    lines.clear ();
    direction = HB_DIRECTION_INVALID;

    font = hb_font_reference (font_opts->font);
    subpixel_bits = font_opts->subpixel_bits;
    hb_font_get_scale (font, &x_scale, &y_scale);
    upem = hb_face_get_upem (hb_font_get_face (font));

    upem_font = hb_font_create (hb_font_get_face (font));
    hb_font_set_scale (upem_font, upem, upem);
#ifndef HB_NO_VAR
    unsigned int coords_length = 0;
    const float *coords = hb_font_get_var_coords_design (font, &coords_length);
    if (coords_length)
      hb_font_set_var_coords_design (upem_font, coords, coords_length);
#endif
  }

  void new_line ()
  {
    lines.emplace_back ();
  }

  void consume_text (hb_buffer_t  * HB_UNUSED,
                     const char   * HB_UNUSED,
                     unsigned int   HB_UNUSED,
                     hb_bool_t      HB_UNUSED)
  {}

  void error (const char *message)
  {
    g_printerr ("%s: %s\n", g_get_prgname (), message);
  }

  void consume_glyphs (hb_buffer_t  *buffer,
                       const char   * HB_UNUSED,
                       unsigned int   HB_UNUSED,
                       hb_bool_t      HB_UNUSED)
  {
    if (lines.empty ())
      lines.emplace_back ();

    line_t &line = lines.back ();
    hb_direction_t line_direction = hb_buffer_get_direction (buffer);
    if (line_direction == HB_DIRECTION_INVALID)
      line_direction = HB_DIRECTION_LTR;
    if (direction == HB_DIRECTION_INVALID)
      direction = line_direction;

    unsigned int count = 0;
    const hb_glyph_info_t *infos = hb_buffer_get_glyph_infos (buffer, &count);
    const hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (buffer, &count);

    hb_position_t pen_x = 0;
    hb_position_t pen_y = 0;

    line.glyphs.reserve (line.glyphs.size () + count);
    for (unsigned i = 0; i < count; i++)
    {
      line.glyphs.push_back ({
        infos[i].codepoint,
        scale_to_upem_x (pen_x + positions[i].x_offset),
        scale_to_upem_y (pen_y + positions[i].y_offset),
        scale_to_upem_x (positions[i].x_advance),
        scale_to_upem_y (positions[i].y_advance),
      });

      pen_x += positions[i].x_advance;
      pen_y += positions[i].y_advance;
    }

    line.advance_x = scale_to_upem_x (pen_x);
    line.advance_y = scale_to_upem_y (pen_y);
  }

  template <typename app_t>
  void finish (hb_buffer_t *buffer HB_UNUSED, const app_t *app HB_UNUSED)
  {
    hb_vector_extents_t extents = {};
    if (!compute_extents (&extents))
      return;
    final_extents = extents;

    hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (HB_VECTOR_FORMAT_SVG);
    hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);

    hb_vector_draw_set_scale_factor (draw, 1.f, 1.f);
    hb_vector_draw_set_extents (draw, &extents);
    hb_vector_svg_set_precision (draw, precision);
    hb_vector_svg_set_flat (draw, flat);

    hb_vector_paint_set_scale_factor (paint, 1.f, 1.f);
    hb_vector_paint_set_extents (paint, &extents);
    hb_vector_paint_set_foreground (paint, foreground);
    hb_vector_paint_set_palette (paint, this->palette);
    apply_custom_palette (paint);
    init_palette_color_cache ();
    hb_vector_svg_paint_set_precision (paint, precision);
    hb_vector_svg_paint_set_flat (paint, flat);

    bool had_draw = false;
    bool had_paint = false;
    hb_vector_extents_mode_t extents_mode = ink ? HB_VECTOR_EXTENTS_MODE_EXPAND
                                                : HB_VECTOR_EXTENTS_MODE_NONE;
    const bool use_foreground_palette =
      foreground_use_palette && foreground_palette && foreground_palette->len;
    unsigned palette_glyph_index = 0;

    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID)
      dir = HB_DIRECTION_LTR;
    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);

    float asc = 0.f, desc = 0.f, gap = 0.f;
    get_line_metrics (dir, &asc, &desc, &gap);
    float step = fabsf (asc - desc + gap + (float) line_space);
    if (!(step > 0.f))
      step = (float) upem;

    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                  : -(step * (float) li);

      for (const auto &g : lines[li].glyphs)
      {
        float pen_x = g.x + off_x;
        float pen_y = g.y + off_y;

        if (use_foreground_palette)
        {
          const rgba_color_t &c =
            g_array_index (foreground_palette, rgba_color_t,
                           palette_glyph_index++ % foreground_palette->len);
          hb_vector_paint_set_foreground (paint, HB_COLOR (c.b, c.g, c.r, c.a));
        }

        hb_vector_paint_set_transform (paint, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
        if (hb_vector_paint_glyph (paint, upem_font, g.gid, pen_x, pen_y,
                                   extents_mode))
        {
          had_paint = true;
          continue;
        }

        if (g.gid == 1 && g.ax == 0.f && g.ay == 0.f)
          continue;

        if (hb_vector_draw_glyph (draw, upem_font, g.gid, pen_x, pen_y,
                                  extents_mode))
          had_draw = true;
      }
    }

    hb_blob_t *draw_blob = had_draw ? hb_vector_draw_render (draw) : nullptr;
    hb_blob_t *paint_blob = had_paint ? hb_vector_paint_render (paint) : nullptr;

    if (draw_blob && draw_blob != hb_blob_get_empty () &&
        paint_blob && paint_blob != hb_blob_get_empty ())
      write_combined_svg (draw_blob, paint_blob);
    else if (paint_blob && paint_blob != hb_blob_get_empty ())
      write_blob (paint_blob, false);
    else if (draw_blob && draw_blob != hb_blob_get_empty ())
      write_blob (draw_blob, true);

    if (show_extents)
      emit_extents_overlay (dir, step);

    hb_blob_destroy (draw_blob);
    hb_blob_destroy (paint_blob);
    hb_vector_draw_destroy (draw);
    hb_vector_paint_destroy (paint);

    hb_font_destroy (upem_font);
    upem_font = nullptr;
    hb_font_destroy (font);
    font = nullptr;
    lines.clear ();
  }

  private:

  struct glyph_instance_t
  {
    hb_codepoint_t gid;
    float x;
    float y;
    float ax;
    float ay;
  };

  struct line_t
  {
    float advance_x = 0.f;
    float advance_y = 0.f;
    std::vector<glyph_instance_t> glyphs;
  };

  struct slice_t
  {
    const char *p = nullptr;
    unsigned len = 0;
  };

  bool compute_extents (hb_vector_extents_t *extents)
  {
    if (lines.empty ())
    {
      extents->x = 0;
      extents->y = 0;
      extents->width = 1;
      extents->height = 1;
      return true;
    }

    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID)
      dir = HB_DIRECTION_LTR;
    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);

    float asc = 0.f, desc = 0.f, gap = 0.f;
    get_line_metrics (dir, &asc, &desc, &gap);
    float step = fabsf (asc - desc + gap + (float) line_space);
    if (!(step > 0.f))
      step = (float) upem;

    bool valid = false;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    bool include_logical = logical || (!logical && !ink);
    bool include_ink = ink;

    auto include_point = [&] (float x, float y)
    {
      y = -y;
      if (!valid)
      {
        x1 = x2 = x;
        y1 = y2 = y;
        valid = true;
        return;
      }
      x1 = hb_min (x1, x);
      y1 = hb_min (y1, y);
      x2 = hb_max (x2, x);
      y2 = hb_max (y2, y);
    };

    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                  : -(step * (float) li);

      const line_t &line = lines[li];

      if (include_logical)
      {
        float log_min = hb_min (desc, asc);
        float log_max = hb_max (desc, asc);
        if (vertical)
        {
          include_point (off_x + log_min, hb_min (0.f, line.advance_y));
          include_point (off_x + log_max, hb_max (0.f, line.advance_y));
        }
        else
        {
          include_point (hb_min (0.f, line.advance_x), off_y + log_min);
          include_point (hb_max (0.f, line.advance_x), off_y + log_max);
        }
      }

      if (include_ink)
        for (const auto &g : line.glyphs)
        {
          hb_glyph_extents_t ge;
          if (!hb_font_get_glyph_extents (upem_font, g.gid, &ge))
            continue;

          float gx = g.x + off_x;
          float gy = g.y + off_y;

          float xa = gx + ge.x_bearing;
          float ya = gy + ge.y_bearing;
          float xb = xa + ge.width;
          float yb = ya + ge.height;

          include_point (xa, ya);
          include_point (xb, yb);
        }
    }

    if (!valid)
    {
      extents->x = 0;
      extents->y = 0;
      extents->width = 1;
      extents->height = 1;
      return true;
    }

    if (x2 <= x1)
      x2 = x1 + 1;
    if (y2 <= y1)
      y2 = y1 + 1;

    x1 -= margin.l;
    y1 -= margin.t;
    x2 += margin.r;
    y2 += margin.b;

    extents->x = x1;
    extents->y = y1;
    extents->width = x2 - x1;
    extents->height = y2 - y1;
    return true;
  }

  float scale_to_upem_x (hb_position_t v) const
  {
    return (float) v * upem / x_scale;
  }

  float scale_to_upem_y (hb_position_t v) const
  {
    return (float) v * upem / y_scale;
  }

  static bool slice_svg (const char *data, unsigned len,
                         slice_t *defs, slice_t *body)
  {
    defs->p = nullptr;
    defs->len = 0;
    body->p = nullptr;
    body->len = 0;

    const char *data_end = data + len;
    const char *start = (const char *) memchr (data, '>', len);
    if (!start)
      return false;
    start += 1;

    if (len < 6)
      return false;

    const char *end = nullptr;
    for (const char *p = data_end - 6; p >= data; p--)
    {
      if (memcmp (p, "</svg>", 6) == 0)
      {
        end = p;
        break;
      }
      if (p == data)
        break;
    }
    if (!end || end <= start)
      return false;

    const char *defs_start = start;
    while (defs_start < end &&
           (*defs_start == ' ' || *defs_start == '\t' ||
            *defs_start == '\r' || *defs_start == '\n'))
      defs_start++;
    if (!(defs_start + 6 <= end && memcmp (defs_start, "<defs>", 6) == 0))
      defs_start = nullptr;

    const char *defs_end = nullptr;
    if (defs_start)
      for (const char *p = defs_start + 6; p + 7 <= end; p++)
        if (memcmp (p, "</defs>", 7) == 0)
        {
          defs_end = p;
          break;
        }

    if (defs_start && defs_end && defs_start < end && defs_end < end)
    {
      defs->p = defs_start + strlen ("<defs>");
      defs->len = (unsigned) (defs_end - defs->p);

      const char *body_start = defs_end + strlen ("</defs>");
      if (body_start < end)
      {
        body->p = body_start;
        body->len = (unsigned) (end - body_start);
      }
    }
    else
    {
      body->p = start;
      body->len = (unsigned) (end - start);
    }

    return true;
  }

  void emit_fill_group_open ()
  {
    const bool has_fill_override = fore || foreground_use_palette;
    const bool has_stroke = stroke_enabled && stroke_width > 0.;
    if (!has_fill_override && !has_stroke)
      return;

    fputs ("<g", out_fp);

    if (has_fill_override)
    {
      unsigned r = hb_color_get_red (foreground);
      unsigned g = hb_color_get_green (foreground);
      unsigned b = hb_color_get_blue (foreground);
      unsigned a = hb_color_get_alpha (foreground);
      fprintf (out_fp, " fill=\"#%02X%02X%02X\"", r, g, b);
      if (a != 255)
        fprintf (out_fp, " fill-opacity=\"%.3f\"", (double) a / 255.);
    }
    else
      fputs (" fill=\"none\"", out_fp);

    if (has_stroke)
      emit_stroke_attrs ();

    fprintf (out_fp, ">\n");
  }

  void emit_fill_group_close ()
  {
    if (!(fore || foreground_use_palette || (stroke_enabled && stroke_width > 0.)))
      return;
    fputs ("</g>\n", out_fp);
  }

  void emit_fill_stroke_attrs_for_color (hb_color_t color)
  {
    unsigned r = hb_color_get_red (color);
    unsigned g = hb_color_get_green (color);
    unsigned b = hb_color_get_blue (color);
    unsigned a = hb_color_get_alpha (color);
    fprintf (out_fp, " fill=\"#%02X%02X%02X\"", r, g, b);
    if (a != 255)
      fprintf (out_fp, " fill-opacity=\"%.3f\"", (double) a / 255.);

  }

  void emit_draw_body_with_palette (slice_t body)
  {
    if (!foreground_palette || !foreground_palette->len || !body.p || !body.len)
      return;

    const char *p = body.p;
    const char *end = body.p + body.len;
    unsigned palette_i = 0;
    const bool has_stroke = stroke_enabled && stroke_width > 0.;
    if (has_stroke)
    {
      fputs ("<g", out_fp);
      emit_stroke_attrs ();
      fputs (">\n", out_fp);
    }
    while (p < end)
    {
      const char *line_end = (const char *) memchr (p, '\n', (size_t) (end - p));
      if (!line_end)
        line_end = end;

      const char *s = p;
      while (s < line_end && (*s == ' ' || *s == '\t' || *s == '\r'))
        s++;
      bool is_elem = s < line_end && *s == '<' && (s + 1 < line_end && s[1] != '/');

      if (is_elem)
      {
        const rgba_color_t &c =
          g_array_index (foreground_palette, rgba_color_t,
                         palette_i++ % foreground_palette->len);
        hb_color_t col = HB_COLOR (c.b, c.g, c.r, c.a);
        fputs ("<g", out_fp);
        emit_fill_stroke_attrs_for_color (col);
        fputs (">", out_fp);
        fwrite (p, 1, (size_t) (line_end - p), out_fp);
        fputs ("</g>\n", out_fp);
      }
      else
      {
        fwrite (p, 1, (size_t) (line_end - p), out_fp);
        fputc ('\n', out_fp);
      }

      p = line_end;
      if (p < end && *p == '\n')
        p++;
    }
    if (has_stroke)
      fputs ("</g>\n", out_fp);
  }

  void emit_stroke_attrs ()
  {
    fprintf (out_fp, " stroke=\"#%02X%02X%02X\"",
             stroke_color.r, stroke_color.g, stroke_color.b);
    if (stroke_color.a != 255)
      fprintf (out_fp, " stroke-opacity=\"%.3f\"", (double) stroke_color.a / 255.);
    fprintf (out_fp, " stroke-width=\"%.*g\" stroke-linecap=\"round\" stroke-linejoin=\"round\"",
             precision + 4, stroke_width);
  }

  bool lookup_palette_color (unsigned idx, hb_color_t *color) const
  {
    if (idx >= palette_cache_has_value.size () || !palette_cache_has_value[idx])
      return false;
    *color = palette_cache_values[idx];
    return true;
  }

  static inline const char *
  skip_ascii_ws (const char *p, const char *end)
  {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
      p++;
    return p;
  }

  static inline const char *
  trim_ascii_ws_right (const char *begin, const char *end)
  {
    while (end > begin &&
           (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n' || end[-1] == '\r'))
      end--;
    return end;
  }

  struct parsed_var_color_expr_t
  {
    const char *expr_end = nullptr;
    const char *fallback_begin = nullptr;
    const char *fallback_end = nullptr;
    unsigned idx = 0;
    bool parse_ok = false;
    bool have_digits = false;
    bool has_fallback = false;
  };

  static inline const char *
  find_next_var_expr (const char *p, const char *end)
  {
    for (const char *q = p; q + 4 <= end; q++)
      if (memcmp (q, "var(", 4) == 0)
        return q;
    return nullptr;
  }

  static inline const char *
  find_matching_paren (const char *inside, const char *end)
  {
    unsigned depth = 1;
    for (const char *q = inside; q < end; q++)
    {
      if (*q == '(') depth++;
      else if (*q == ')')
      {
        depth--;
        if (!depth)
          return q;
      }
    }
    return nullptr;
  }

  static inline parsed_var_color_expr_t
  parse_var_color_expr (const char *match, const char *end)
  {
    parsed_var_color_expr_t parsed;
    const char *expr_close = find_matching_paren (match + 4, end);
    if (!expr_close)
      return parsed;

    parsed.expr_end = expr_close + 1;
    const char *inside = match + 4;
    const char *inside_end = expr_close;
    const char *q = skip_ascii_ws (inside, inside_end);
    parsed.parse_ok = true;

    if (!(q + 7 <= inside_end && memcmp (q, "--color", 7) == 0))
      parsed.parse_ok = false;
    if (parsed.parse_ok)
      q += 7;

    while (parsed.parse_ok && q < inside_end && *q >= '0' && *q <= '9')
    {
      parsed.have_digits = true;
      parsed.idx = parsed.idx * 10 + (unsigned) (*q - '0');
      q++;
    }
    q = skip_ascii_ws (q, inside_end);

    if (parsed.parse_ok && q < inside_end && *q == ',')
    {
      parsed.has_fallback = true;
      q++;
      parsed.fallback_begin = skip_ascii_ws (q, inside_end);
      parsed.fallback_end = trim_ascii_ws_right (parsed.fallback_begin, inside_end);
      q = inside_end;
    }

    if (parsed.parse_ok && q != inside_end)
      parsed.parse_ok = false;

    return parsed;
  }

  void emit_css_color (hb_color_t color)
  {
    unsigned r = hb_color_get_red (color);
    unsigned g = hb_color_get_green (color);
    unsigned b = hb_color_get_blue (color);
    unsigned a = hb_color_get_alpha (color);
    if (a == 255)
      fprintf (out_fp, "#%02X%02X%02X", r, g, b);
    else
      fprintf (out_fp, "rgba(%u,%u,%u,%.6g)", r, g, b, (double) a / 255.);
  }

  void write_slice_resolving_palette_vars (slice_t s)
  {
    if (!s.p || !s.len)
      return;

    const char *p = s.p;
    const char *end = s.p + s.len;
    while (p < end)
    {
      const char *match = find_next_var_expr (p, end);

      if (!match)
      {
        fwrite (p, 1, (size_t) (end - p), out_fp);
        return;
      }

      if (match > p)
        fwrite (p, 1, (size_t) (match - p), out_fp);

      parsed_var_color_expr_t parsed = parse_var_color_expr (match, end);
      if (!parsed.expr_end)
      {
        fwrite (match, 1, (size_t) (end - match), out_fp);
        return;
      }

      hb_color_t color;
      if (parsed.parse_ok && parsed.have_digits && lookup_palette_color (parsed.idx, &color))
      {
        emit_css_color (color);
      }
      else if (parsed.has_fallback && parsed.fallback_begin &&
               parsed.fallback_end >= parsed.fallback_begin)
      {
        fwrite (parsed.fallback_begin, 1,
                (size_t) (parsed.fallback_end - parsed.fallback_begin), out_fp);
      }
      else
      {
        fwrite (match, 1, (size_t) (parsed.expr_end - match), out_fp);
      }

      p = parsed.expr_end;
    }
  }

  void write_blob (hb_blob_t *blob,
                   hb_bool_t is_draw_blob)
  {
    unsigned in_len = 0;
    const char *in_data = hb_blob_get_data (blob, &in_len);
    if (!in_data || !in_len)
      return;

    const char *hdr_end = (const char *) memchr (in_data, '>', in_len);
    if (!hdr_end)
      return;

    slice_t defs, body;
    if (!slice_svg (in_data, in_len, &defs, &body))
      return;

    fwrite (in_data, 1, (hdr_end - in_data + 1), out_fp);
    fputc ('\n', out_fp);

    if (defs.len)
    {
      fputs ("<defs>\n", out_fp);
      write_slice_resolving_palette_vars (defs);
      fputs ("</defs>\n", out_fp);
    }

    emit_background_rect ();

    if (is_draw_blob && foreground_use_palette && foreground_palette && foreground_palette->len)
      emit_draw_body_with_palette (body);
    else if (is_draw_blob)
      emit_fill_group_open ();

    if (body.len && !(is_draw_blob && foreground_use_palette && foreground_palette && foreground_palette->len))
      write_slice_resolving_palette_vars (body);

    if (is_draw_blob && !(foreground_use_palette && foreground_palette && foreground_palette->len))
      emit_fill_group_close ();

    fputs ("</svg>\n", out_fp);
  }

  void write_combined_svg (hb_blob_t *draw_blob,
                           hb_blob_t *paint_blob)
  {
    unsigned draw_len = 0, paint_len = 0;
    const char *draw_data = hb_blob_get_data (draw_blob, &draw_len);
    const char *paint_data = hb_blob_get_data (paint_blob, &paint_len);
    if (!draw_data || !paint_data)
      return;

    const char *draw_hdr_end = (const char *) memchr (draw_data, '>', draw_len);
    if (!draw_hdr_end)
      return;

    slice_t draw_defs, draw_body;
    slice_t paint_defs, paint_body;
    if (!slice_svg (draw_data, draw_len, &draw_defs, &draw_body) ||
        !slice_svg (paint_data, paint_len, &paint_defs, &paint_body))
      return;

    fwrite (draw_data, 1, (draw_hdr_end - draw_data + 1), out_fp);
    fputc ('\n', out_fp);

    if (draw_defs.len || paint_defs.len)
    {
      fputs ("<defs>\n", out_fp);
      if (draw_defs.len)
        write_slice_resolving_palette_vars (draw_defs);
      if (paint_defs.len)
        write_slice_resolving_palette_vars (paint_defs);
      fputs ("</defs>\n", out_fp);
    }

    emit_background_rect ();

    if (foreground_use_palette && foreground_palette && foreground_palette->len)
      emit_draw_body_with_palette (draw_body);
    else
    {
      emit_fill_group_open ();
      if (draw_body.len)
        fwrite (draw_body.p, 1, draw_body.len, out_fp);
      emit_fill_group_close ();
    }
    if (paint_body.len)
      write_slice_resolving_palette_vars (paint_body);

    fputs ("</svg>\n", out_fp);
  }

  void emit_background_rect ()
  {
    if (!has_background)
      return;
    unsigned r = hb_color_get_red (background);
    unsigned g = hb_color_get_green (background);
    unsigned b = hb_color_get_blue (background);
    unsigned a = hb_color_get_alpha (background);
    fprintf (out_fp, "<rect x=\"%.*g\" y=\"%.*g\" width=\"%.*g\" height=\"%.*g\" fill=\"#%02X%02X%02X\"",
             precision + 4, (double) final_extents.x,
             precision + 4, (double) final_extents.y,
             precision + 4, (double) final_extents.width,
             precision + 4, (double) final_extents.height,
             r, g, b);
    if (a != 255)
      fprintf (out_fp, " fill-opacity=\"%.3f\"", (double) a / 255.);
    fputs ("/>\n", out_fp);
  }

  void apply_custom_palette (hb_vector_paint_t *paint)
  {
    hb_vector_paint_clear_custom_palette_colors (paint);
    for (unsigned idx = 0; idx < custom_palette_values.size (); idx++)
      if (idx < custom_palette_has_value.size () && custom_palette_has_value[idx])
        hb_vector_paint_set_custom_palette_color (paint, idx, custom_palette_values[idx]);
  }

  void init_palette_color_cache ()
  {
    palette_cache_values.clear ();
    palette_cache_has_value.clear ();

    hb_face_t *face = hb_font_get_face (upem_font ? upem_font : font);
    if (face)
    {
      unsigned palette_count = hb_ot_color_palette_get_count (face);
      if (palette_count)
      {
        unsigned palette_index = palette >= 0 ? (unsigned) palette : 0u;
        if (palette_index >= palette_count)
          palette_index = 0;

        unsigned color_count = hb_ot_color_palette_get_colors (face, palette_index, 0,
                                                                nullptr, nullptr);
        if (color_count)
        {
          palette_cache_values.resize (color_count, HB_COLOR (0, 0, 0, 255));
          palette_cache_has_value.resize (color_count, false);
          unsigned n = color_count;
          if (hb_ot_color_palette_get_colors (face, palette_index, 0, &n,
                                              palette_cache_values.data ()) && n)
          {
            if (n < palette_cache_values.size ())
              palette_cache_values.resize (n);
            palette_cache_has_value.resize (palette_cache_values.size (), false);
            std::fill (palette_cache_has_value.begin (),
                       palette_cache_has_value.end (), true);
          }
          else
          {
            palette_cache_values.clear ();
            palette_cache_has_value.clear ();
          }
        }
      }
    }

    for (unsigned idx = 0; idx < custom_palette_values.size (); idx++)
    {
      if (idx >= custom_palette_has_value.size () || !custom_palette_has_value[idx])
        continue;
      if (palette_cache_values.size () <= idx)
      {
        palette_cache_values.resize (idx + 1, HB_COLOR (0, 0, 0, 255));
        palette_cache_has_value.resize (idx + 1, false);
      }
      palette_cache_values[idx] = custom_palette_values[idx];
      palette_cache_has_value[idx] = true;
    }
  }

  bool parse_custom_palette_overrides (GError **error)
  {
    custom_palette_values.clear ();
    custom_palette_has_value.clear ();
    if (!custom_palette || !*custom_palette)
      return true;

    char **entries = g_strsplit (custom_palette, ",", -1);
    for (unsigned idx = 0; entries && entries[idx]; idx++)
    {
      char *entry = g_strstrip (entries[idx]);
      if (!*entry)
        continue;

      unsigned r = 0, g = 0, b = 0, a = 255;
      if (!parse_color (entry, r, g, b, a))
      {
        g_strfreev (entries);
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Invalid --custom-palette entry; expected rrggbb or rrggbbaa");
        return false;
      }

      if (custom_palette_values.size () <= idx)
      {
        custom_palette_values.resize (idx + 1, HB_COLOR (0, 0, 0, 255));
        custom_palette_has_value.resize (idx + 1, false);
      }
      custom_palette_values[idx] = HB_COLOR (b, g, r, a);
      custom_palette_has_value[idx] = true;
    }
    g_strfreev (entries);
    return true;
  }

  void emit_extents_overlay (hb_direction_t dir, float step)
  {
    const bool vertical = HB_DIRECTION_IS_VERTICAL (dir);
    const float dot_r = hb_max (1.f, step * 0.01f);
    fputs ("<g fill=\"#FF000080\" stroke=\"none\">\n", out_fp);
    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                  : -(step * (float) li);
      const line_t &line = lines[li];
      for (const auto &g : line.glyphs)
      {
        fprintf (out_fp, "<circle cx=\"%.*g\" cy=\"%.*g\" r=\"%.*g\"/>\n",
                 precision + 4, (double) (g.x + off_x),
                 precision + 4, (double) (-(g.y + off_y)),
                 precision + 4, (double) dot_r);
      }
    }
    fputs ("</g>\n", out_fp);

    fputs ("<g fill=\"none\" stroke=\"#FF00FF80\" stroke-width=\"1\">\n", out_fp);
    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                  : -(step * (float) li);
      const line_t &line = lines[li];
      for (const auto &g : line.glyphs)
      {
        hb_glyph_extents_t ge;
        if (!hb_font_get_glyph_extents (upem_font, g.gid, &ge))
          continue;
        float x = g.x + off_x + ge.x_bearing;
        float y = g.y + off_y + ge.y_bearing;
        float w = ge.width;
        float h = ge.height;
        if (!w || !h)
          continue;
        fprintf (out_fp, "<rect x=\"%.*g\" y=\"%.*g\" width=\"%.*g\" height=\"%.*g\"/>\n",
                 precision + 4, (double) x,
                 precision + 4, (double) (-y),
                 precision + 4, (double) w,
                 precision + 4, (double) (-h));
      }
    }
    fputs ("</g>\n", out_fp);
  }

  void get_line_metrics (hb_direction_t dir, float *asc, float *desc, float *gap) const
  {
    if (have_font_extents)
    {
      if (asc) *asc = (float) font_extents.ascent;
      if (desc) *desc = (float) (-font_extents.descent);
      if (gap) *gap = (float) font_extents.line_gap;
      return;
    }

    hb_font_extents_t fext = {0, 0, 0};
    hb_font_get_extents_for_direction (upem_font, dir, &fext);
    if (asc) *asc = (float) fext.ascender;
    if (desc) *desc = (float) fext.descender;
    if (gap) *gap = (float) fext.line_gap;
  }

  hb_bool_t flat = false;
  int precision = 2;
  hb_color_t background = HB_COLOR (255, 255, 255, 255);
  hb_color_t foreground = HB_COLOR (0, 0, 0, 255);
  hb_bool_t has_background = false;

  hb_font_t *font = nullptr;
  hb_font_t *upem_font = nullptr;
  std::vector<line_t> lines;
  hb_direction_t direction = HB_DIRECTION_INVALID;

  int x_scale = 0;
  int y_scale = 0;
  unsigned upem = 0;
  unsigned subpixel_bits = 0;
  hb_vector_extents_t final_extents = {0, 0, 1, 1};
  std::vector<hb_color_t> custom_palette_values;
  std::vector<bool> custom_palette_has_value;
  std::vector<hb_color_t> palette_cache_values;
  std::vector<bool> palette_cache_has_value;
};

#endif
