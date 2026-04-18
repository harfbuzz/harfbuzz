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

#include "helper-extents-overlay.hh"
#include "output-options.hh"
#include "view-options.hh"
#include "hb-vector.h"
#include "hb-ot.h"


static const char *vector_supported_formats[] = {
  "svg",
  "pdf",
  nullptr
};

struct vector_output_t : output_options_t<>, view_options_t
{
  static const bool repeat_shape = false;

  ~vector_output_t ()
  {
    hb_font_destroy (font);
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Draw text with given font.");
    parser->set_description ("Shows shaped glyph outlines as SVG or PDF.");

    output_options_t::add_options (parser, vector_supported_formats);
    view_options_t::add_options (parser);

    GOptionEntry entries[] =
    {
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

    if (output_format &&
        g_ascii_strcasecmp (output_format, "svg") != 0 &&
        g_ascii_strcasecmp (output_format, "pdf") != 0)
    {
      char *items = g_strjoinv ("/", const_cast<char **> (vector_supported_formats));
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                   "Unknown output format `%s'; supported formats are: %s",
                   output_format, items);
      g_free (items);
      return;
    }

    foreground = HB_COLOR (foreground_color.b,
                           foreground_color.g,
                           foreground_color.r,
                           foreground_color.a);
    background = HB_COLOR (background_color.b,
                           background_color.g,
                           background_color.r,
                           background_color.a);

    load_custom_palette_overrides_from_view ();
  }

  template <typename app_t>
  void init (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts)
  {
    lines.clear ();
    direction = HB_DIRECTION_INVALID;

    font = hb_font_reference (font_opts->font);
    subpixel_bits = font_opts->subpixel_bits;
    normalized_stroke_width = get_normalized_stroke_width ();
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
        (float) (pen_x + positions[i].x_offset),
        (float) (pen_y + positions[i].y_offset),
        (float) positions[i].x_advance,
        (float) positions[i].y_advance,
      });

      pen_x += positions[i].x_advance;
      pen_y += positions[i].y_advance;
    }

    line.advance_x = (float) pen_x;
    line.advance_y = (float) pen_y;
  }

  template <typename app_t>
  void finish (hb_buffer_t *buffer HB_UNUSED, const app_t *app HB_UNUSED)
  {
    hb_vector_extents_t extents = {};
    if (!compute_extents (&extents))
      return;
    final_extents = extents;

    hb_vector_format_t fmt = HB_VECTOR_FORMAT_SVG;
    if (output_format && g_ascii_strcasecmp (output_format, "pdf") == 0)
      fmt = HB_VECTOR_FORMAT_PDF;

    hb_vector_draw_t *draw = hb_vector_draw_create_or_fail (fmt);
    hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (fmt);

    float scale = scalbnf (1.f, (int) subpixel_bits);

    hb_vector_draw_set_scale_factor (draw, scale, scale);
    hb_vector_draw_set_extents (draw, &extents);
    hb_vector_draw_set_precision (draw, precision);
    hb_vector_draw_set_foreground (draw, foreground);
    hb_vector_draw_set_background (draw, background);

    if (paint)
    {
      hb_vector_paint_set_scale_factor (paint, scale, scale);
      hb_vector_paint_set_extents (paint, &extents);
      hb_vector_paint_set_foreground (paint, foreground);
      hb_vector_paint_set_background (paint, background);
      hb_vector_paint_set_palette (paint, this->palette);
      apply_custom_palette (paint);
      hb_vector_paint_set_precision (paint, precision);
    }

    bool had_draw = false;
    bool had_paint = false;
    hb_vector_extents_mode_t extents_mode = HB_VECTOR_EXTENTS_MODE_NONE;
    const bool use_foreground_palette =
      foreground_use_palette && foreground_palette && foreground_palette->len;
    unsigned palette_glyph_index = 0;

    /* Pick draw vs paint mode.  --paint wins over --draw;
     * otherwise auto-detect via the font's color tables.
     * hb_vector_paint_glyph synthesizes paint from outlines
     * for mono fonts, so --paint works even without color. */
    hb_face_t *face = hb_font_get_face (font);
    bool font_has_color = hb_ot_color_has_paint (face) ||
			  hb_ot_color_has_layers (face) ||
			  hb_ot_color_has_png (face);
    bool use_paint = force_paint ? true
		   : force_draw  ? false
		   : font_has_color;

    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID)
      dir = HB_DIRECTION_LTR;
    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);

    float asc = 0.f, desc = 0.f, gap = 0.f;
    get_line_metrics (dir, &asc, &desc, &gap);
    float step = fabsf (asc - desc + gap + (float) line_space);
    if (!(step > 0.f))
      step = scale;
    line_step = step;

    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                  : -(step * (float) li);

      for (const auto &g : lines[li].glyphs)
      {
        float pen_x = g.x + off_x;
        float pen_y = g.y + off_y;

        if (use_paint && paint)
        {
          if (use_foreground_palette)
          {
            const rgba_color_t &c =
              g_array_index (foreground_palette, rgba_color_t,
                             palette_glyph_index++ % foreground_palette->len);
            hb_vector_paint_set_foreground (paint, HB_COLOR (c.b, c.g, c.r, c.a));
          }

          hb_vector_paint_set_transform (paint, 1.f, 0.f, 0.f, 1.f, pen_x, pen_y);
          hb_vector_paint_glyph (paint, font, g.gid,
                                 extents_mode);
          had_paint = true;
          if (show_extents)
          {
            hb_glyph_extents_t ge;
            if (hb_font_get_glyph_extents (font, g.gid, &ge))
              util_emit_extents_overlay_into_paint (
                hb_vector_paint_get_funcs (paint), paint,
                &ge, pen_x, pen_y, (float) normalized_stroke_width);
          }
        }
        else
        {
          if (use_foreground_palette)
          {
            const rgba_color_t &c =
              g_array_index (foreground_palette, rgba_color_t,
                             palette_glyph_index++ % foreground_palette->len);
            hb_vector_draw_set_foreground (draw, HB_COLOR (c.b, c.g, c.r, c.a));
          }

          hb_vector_draw_set_transform (draw, 1.f, 0.f, 0.f, 1.f, pen_x, pen_y);
          hb_vector_draw_glyph (draw, font, g.gid,
                                extents_mode);
          had_draw = true;
          if (show_extents)
          {
            hb_glyph_extents_t ge;
            if (hb_font_get_glyph_extents (font, g.gid, &ge))
              util_emit_extents_overlay_into_draw (
                hb_vector_draw_get_funcs (draw), draw,
                &ge, pen_x, pen_y, (float) normalized_stroke_width);
          }
        }
      }
    }

    hb_blob_t *blob = had_paint ? hb_vector_paint_render (paint)
				: had_draw  ? hb_vector_draw_render (draw)
				: nullptr;
    if (blob && blob != hb_blob_get_empty ())
      write_blob (blob, !had_paint);
    hb_blob_destroy (blob);
    hb_vector_draw_destroy (draw);
    hb_vector_paint_destroy (paint);

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

    float scale = scalbnf (1.f, (int) subpixel_bits);

    float asc = 0.f, desc = 0.f, gap = 0.f;
    get_line_metrics (dir, &asc, &desc, &gap);
    float step = fabsf (asc - desc + gap + (float) line_space);
    if (!(step > 0.f))
      step = scale;

    bool valid = false;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    bool include_logical = include_logical_extents ();
    bool include_ink = include_ink_extents ();

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
          if (!hb_font_get_glyph_extents (font, g.gid, &ge))
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

    /* Margin is in pixels; extents are in font-scale (pixel * SCALE). */
    x1 -= (float) margin.l * scale;
    y1 -= (float) margin.t * scale;
    x2 += (float) margin.r * scale;
    y2 += (float) margin.b * scale;

    extents->x = x1;
    extents->y = y1;
    extents->width = x2 - x1;
    extents->height = y2 - y1;
    return true;
  }


  double get_normalized_stroke_width () const
  {
    if (stroke_width <= 0.)
      return stroke_width;

    /* Stroke width is in pixels; geometry is in font-scale
     * (pixel * SCALE), so multiply by SCALE. */
    return stroke_width * scalbn (1., (int) subpixel_bits);
  }

  void write_blob (hb_blob_t *blob,
                   hb_bool_t is_draw_blob HB_UNUSED)
  {
    unsigned in_len = 0;
    const char *in_data = hb_blob_get_data (blob, &in_len);
    if (!in_data || !in_len)
      return;
    fwrite (in_data, 1, in_len, out_fp);
  }

  void apply_custom_palette (hb_vector_paint_t *paint)
  {
    hb_vector_paint_clear_custom_palette_colors (paint);
    for (unsigned idx = 0; idx < custom_palette_values.size (); idx++)
      if (idx < custom_palette_has_value.size () && custom_palette_has_value[idx])
        hb_vector_paint_set_custom_palette_color (paint, idx, custom_palette_values[idx]);
  }

  void load_custom_palette_overrides_from_view ()
  {
    custom_palette_values.clear ();
    custom_palette_has_value.clear ();
    if (!custom_palette_entries)
      return;

    for (unsigned i = 0; i < custom_palette_entries->len; i++)
    {
      const custom_palette_entry_t &entry =
        g_array_index (custom_palette_entries, custom_palette_entry_t, i);
      unsigned idx = entry.index;
      if (custom_palette_values.size () <= idx)
      {
        custom_palette_values.resize (idx + 1, HB_COLOR (0, 0, 0, 255));
        custom_palette_has_value.resize (idx + 1, false);
      }
      custom_palette_values[idx] = HB_COLOR (entry.color.b,
                                             entry.color.g,
                                             entry.color.r,
                                             entry.color.a);
      custom_palette_has_value[idx] = true;
    }
  }

  void get_line_metrics (hb_direction_t dir, float *asc, float *desc, float *gap) const
  {
    if (have_font_extents)
    {
      float s = scalbnf (1.f, (int) subpixel_bits);
      if (asc) *asc = (float) font_extents.ascent * s;
      if (desc) *desc = (float) (-font_extents.descent) * s;
      if (gap) *gap = (float) font_extents.line_gap * s;
      return;
    }

    hb_font_extents_t fext = {0, 0, 0};
    hb_font_get_extents_for_direction (font, dir, &fext);
    if (asc) *asc = (float) fext.ascender;
    if (desc) *desc = (float) fext.descender;
    if (gap) *gap = (float) fext.line_gap;
  }

  int precision = 2;
  hb_color_t background = HB_COLOR (255, 255, 255, 255);
  hb_color_t foreground = HB_COLOR (0, 0, 0, 255);

  hb_font_t *font = nullptr;
  std::vector<line_t> lines;
  hb_direction_t direction = HB_DIRECTION_INVALID;

  unsigned subpixel_bits = 0;
  double normalized_stroke_width = DEFAULT_STROKE_WIDTH;
  float line_step = 0.f;
  hb_vector_extents_t final_extents = {0, 0, 1, 1};
  std::vector<hb_color_t> custom_palette_values;
  std::vector<bool> custom_palette_has_value;
};

#endif
