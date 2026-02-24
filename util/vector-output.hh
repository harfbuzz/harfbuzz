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

#include <math.h>
#include <string>
#include <vector>

#include "output-options.hh"
#include "hb-vector.h"
#include "hb-ot.h"


struct vector_output_t : output_options_t<>
{
  static const bool repeat_shape = false;

  ~vector_output_t ()
  {
    hb_font_destroy (font);
    hb_font_destroy (upem_font);
    g_free (foreground_str);
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Draw text with given font.");
    parser->set_description ("Shows shaped glyph outlines as SVG.");

    output_options_t::add_options (parser);

    GOptionEntry entries[] =
    {
      {"flat",       0, 0, G_OPTION_ARG_NONE,   &this->flat,          "Flatten geometry and disable reuse", nullptr},
      {"precision",  0, 0, G_OPTION_ARG_INT,    &this->precision,     "Decimal precision (default: 2)",     "N"},
      {"palette",    0, 0, G_OPTION_ARG_INT,    &this->palette,       "Color palette index (default: 0)",   "N"},
      {"foreground", 0, 0, G_OPTION_ARG_STRING, &this->foreground_str,"Foreground color (default: 000000)", "rrggbb[aa]"},
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
    if (precision < 0)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "precision must be non-negative");
      return;
    }

    if (foreground_str)
    {
      unsigned r, g, b, a;
      if (!parse_color (foreground_str, r, g, b, a))
      {
        g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                     "Invalid foreground color: %s", foreground_str);
        return;
      }
      foreground = HB_COLOR (b, g, r, a);
    }
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

    hb_vector_draw_t *draw = hb_vector_draw_create_or_fail ();
    hb_vector_paint_t *paint = hb_vector_paint_create_or_fail ();

    hb_vector_draw_set_scale_factor (draw, 1.f, 1.f);
    hb_vector_draw_set_extents (draw, &extents);
    hb_vector_draw_set_format (draw, HB_VECTOR_FORMAT_SVG);
    hb_vector_svg_set_precision (draw, precision);
    hb_vector_svg_set_flat (draw, flat);

    hb_vector_paint_set_scale_factor (paint, 1.f, 1.f);
    hb_vector_paint_set_extents (paint, &extents);
    hb_vector_paint_set_format (paint, HB_VECTOR_FORMAT_SVG);
    hb_vector_paint_set_foreground (paint, foreground);
    hb_vector_paint_set_palette (paint, palette);
    hb_vector_svg_paint_set_precision (paint, precision);
    hb_vector_svg_paint_set_flat (paint, flat);

    bool had_draw = false;
    bool had_paint = false;

    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID)
      dir = HB_DIRECTION_LTR;
    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);

    hb_font_extents_t fext = {0, 0, 0};
    hb_font_get_extents_for_direction (upem_font, dir, &fext);
    float step = fabsf ((float) (fext.ascender - fext.descender + fext.line_gap));
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

        hb_vector_paint_set_transform (paint, 1.f, 0.f, 0.f, 1.f, pen_x, pen_y);
        if (hb_font_paint_glyph_or_fail (upem_font, g.gid,
                                         hb_vector_paint_get_funcs (), paint,
                                         palette, foreground))
        {
          had_paint = true;
          continue;
        }

        if (hb_vector_draw_glyph (draw, upem_font, g.gid, pen_x, pen_y))
          had_draw = true;
      }
    }

    hb_blob_t *draw_blob = had_draw ? hb_vector_draw_render (draw) : nullptr;
    hb_blob_t *paint_blob = had_paint ? hb_vector_paint_render (paint) : nullptr;

    if (draw_blob && draw_blob != hb_blob_get_empty () &&
        paint_blob && paint_blob != hb_blob_get_empty ())
      write_combined_svg (draw_blob, paint_blob);
    else if (paint_blob && paint_blob != hb_blob_get_empty ())
      write_blob (paint_blob);
    else if (draw_blob && draw_blob != hb_blob_get_empty ())
      write_blob (draw_blob);

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

    hb_font_extents_t fext = {0, 0, 0};
    hb_font_get_extents_for_direction (upem_font, dir, &fext);

    float step = fabsf ((float) (fext.ascender - fext.descender + fext.line_gap));
    if (!(step > 0.f))
      step = (float) upem;

    bool valid = false;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;

    auto include_point = [&] (float x, float y)
    {
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

      float log_min = hb_min ((float) fext.descender, (float) fext.ascender);
      float log_max = hb_max ((float) fext.descender, (float) fext.ascender);
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

    const char *start = strstr (data, ">");
    const char *end = strstr (data, "</svg>");
    if (!start || !end || end <= start)
      return false;
    start += 1;

    const char *defs_start = strstr (start, "<defs>");
    const char *defs_end = defs_start ? strstr (defs_start, "</defs>") : nullptr;

    if (defs_start && defs_end && defs_start < end && defs_end < end)
    {
      defs->p = defs_start + strlen ("<defs>");
      defs->len = (unsigned) (defs_end - defs->p);

      body->p = start;
      body->len = (unsigned) (defs_start - start);
    }
    else
    {
      body->p = start;
      body->len = (unsigned) (end - start);
    }

    return true;
  }

  void write_blob (hb_blob_t *blob)
  {
    unsigned len = 0;
    const char *data = hb_blob_get_data (blob, &len);
    if (!data || !len)
      return;
    fwrite (data, 1, len, out_fp);
  }

  void write_combined_svg (hb_blob_t *draw_blob,
                           hb_blob_t *paint_blob)
  {
    unsigned draw_len = 0, paint_len = 0;
    const char *draw_data = hb_blob_get_data (draw_blob, &draw_len);
    const char *paint_data = hb_blob_get_data (paint_blob, &paint_len);
    if (!draw_data || !paint_data)
      return;

    const char *draw_hdr_end = strstr (draw_data, ">");
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
        fwrite (draw_defs.p, 1, draw_defs.len, out_fp);
      if (paint_defs.len)
        fwrite (paint_defs.p, 1, paint_defs.len, out_fp);
      fputs ("</defs>\n", out_fp);
    }

    if (draw_body.len)
      fwrite (draw_body.p, 1, draw_body.len, out_fp);
    if (paint_body.len)
      fwrite (paint_body.p, 1, paint_body.len, out_fp);

    fputs ("</svg>\n", out_fp);
  }

  hb_bool_t flat = false;
  int precision = 2;
  int palette = 0;
  char *foreground_str = nullptr;
  hb_color_t foreground = HB_COLOR (0, 0, 0, 255);

  hb_font_t *font = nullptr;
  hb_font_t *upem_font = nullptr;
  std::vector<line_t> lines;
  hb_direction_t direction = HB_DIRECTION_INVALID;

  int x_scale = 0;
  int y_scale = 0;
  unsigned upem = 0;
  unsigned subpixel_bits = 0;
};

#endif
