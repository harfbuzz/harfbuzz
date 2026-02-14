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

#ifndef HB_DRAW_OUTPUT_HH
#define HB_DRAW_OUTPUT_HH

#include <cmath>
#include <unordered_set>
#include <utility>
#include <vector>

#include "output-options.hh"


struct draw_output_t : output_options_t<>
{
  struct margin_t {
    float t, r, b, l;
  };

  static gboolean
  parse_margin (const char *name G_GNUC_UNUSED,
		const char *arg,
		gpointer    data,
		GError    **error G_GNUC_UNUSED)
  {
    draw_output_t *draw_opts = (draw_output_t *) data;
    margin_t &m = draw_opts->margin;
    double t, r, b, l;
    switch (sscanf (arg, "%lf%*[ ,]%lf%*[ ,]%lf%*[ ,]%lf", &t, &r, &b, &l)) {
      case 1: r = t; HB_FALLTHROUGH;
      case 2: b = t; HB_FALLTHROUGH;
      case 3: l = r; HB_FALLTHROUGH;
      case 4:
	m.t = t; m.r = r; m.b = b; m.l = l;
	return true;
      default:
	g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		     "%s argument should be one to four space-separated numbers",
		     name);
	return false;
    }
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Draw text with given font.");
    parser->set_description ("Shows shaped glyph outlines as SVG or path data.");

    output_options_t::add_options (parser);

    GOptionEntry entries[] =
    {
      {"quiet",		'q', 0, G_OPTION_ARG_NONE,	&this->quiet,		"Path-data only; implies --ned",			nullptr},
      {"ned",		0, 0, G_OPTION_ARG_NONE,	&this->ned,		"No extra data: flatten geometry and disable reuse",	nullptr},
      {"precision",	0, 0, G_OPTION_ARG_INT,		&this->precision,	"Decimal precision (default: 2)",			"N"},
      {"no-color",	0, 0, G_OPTION_ARG_NONE,	&this->no_color,	"Disable color font rendering",				nullptr},
      {"palette",	0, 0, G_OPTION_ARG_INT,		&this->palette,		"Color palette index (default: 0)",			"N"},
      {"background",	0, 0, G_OPTION_ARG_STRING,	&this->background_str,	"Background color",					"rrggbb[aa]"},
      {"foreground",	0, 0, G_OPTION_ARG_STRING,	&this->foreground_str,	"Foreground color (default: 000000)",			"rrggbb[aa]"},
      {"margin",	0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_margin, "Margin around output (default: 0)",		"one to four numbers"},
      {"logical",	0, 0, G_OPTION_ARG_NONE,	&this->logical,		"Use logical extents for bounding box (default)",	nullptr},
      {"ink",		0, 0, G_OPTION_ARG_NONE,	&this->ink,		"Use ink extents for bounding box",			nullptr},
      {nullptr}
    };
    parser->add_group (entries,
		       "draw",
		       "Draw options:",
		       "Options for SVG/path output",
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

    if (quiet)
    {
      ned = true;
      no_color = true;
    }

    if (background_str)
    {
      unsigned r, g, b, a;
      if (!parse_color (background_str, r, g, b, a))
      {
	g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		     "Invalid background color: %s", background_str);
	return;
      }
      background = HB_COLOR (b, g, r, a);
      has_background = true;
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

    if (!x_scale || !y_scale)
      fail (false, "Font scale cannot be zero.");
    if (!upem)
      fail (false, "Face UPEM cannot be zero.");

    upem_font = hb_font_create (hb_font_get_face (font));
    hb_font_set_scale (upem_font, upem, upem);

    path = g_string_new (nullptr);
    draw_funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (draw_funcs, (hb_draw_move_to_func_t) move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (draw_funcs, (hb_draw_line_to_func_t) line_to, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (draw_funcs, (hb_draw_quadratic_to_func_t) quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (draw_funcs, (hb_draw_cubic_to_func_t) cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (draw_funcs, (hb_draw_close_path_func_t) close_path, nullptr, nullptr);

    if (!no_color)
    {
      probe_paint_funcs = hb_paint_funcs_create ();

      paint_funcs = hb_paint_funcs_create ();
      hb_paint_funcs_set_push_transform_func (paint_funcs, (hb_paint_push_transform_func_t) paint_push_transform, nullptr, nullptr);
      hb_paint_funcs_set_pop_transform_func (paint_funcs, (hb_paint_pop_transform_func_t) paint_pop_transform, nullptr, nullptr);
      hb_paint_funcs_set_push_clip_glyph_func (paint_funcs, (hb_paint_push_clip_glyph_func_t) paint_push_clip_glyph, nullptr, nullptr);
      hb_paint_funcs_set_push_clip_rectangle_func (paint_funcs, (hb_paint_push_clip_rectangle_func_t) paint_push_clip_rectangle, nullptr, nullptr);
      hb_paint_funcs_set_pop_clip_func (paint_funcs, (hb_paint_pop_clip_func_t) paint_pop_clip, nullptr, nullptr);
      hb_paint_funcs_set_color_func (paint_funcs, (hb_paint_color_func_t) paint_color, nullptr, nullptr);
      hb_paint_funcs_set_image_func (paint_funcs, (hb_paint_image_func_t) paint_image, nullptr, nullptr);
      hb_paint_funcs_set_linear_gradient_func (paint_funcs, (hb_paint_linear_gradient_func_t) paint_linear_gradient, nullptr, nullptr);
      hb_paint_funcs_set_radial_gradient_func (paint_funcs, (hb_paint_radial_gradient_func_t) paint_radial_gradient, nullptr, nullptr);
      hb_paint_funcs_set_sweep_gradient_func (paint_funcs, (hb_paint_sweep_gradient_func_t) paint_sweep_gradient, nullptr, nullptr);
      hb_paint_funcs_set_push_group_func (paint_funcs, (hb_paint_push_group_func_t) paint_push_group, nullptr, nullptr);
      hb_paint_funcs_set_pop_group_func (paint_funcs, (hb_paint_pop_group_func_t) paint_pop_group, nullptr, nullptr);
      hb_paint_funcs_set_color_glyph_func (paint_funcs, (hb_paint_color_glyph_func_t) paint_color_glyph, nullptr, nullptr);
    }
  }

  void new_line ()
  {
    lines.emplace_back ();
  }

  void consume_text (hb_buffer_t  *buffer HB_UNUSED,
		     const char   *text HB_UNUSED,
		     unsigned int  text_len HB_UNUSED,
		     hb_bool_t     utf8_clusters HB_UNUSED)
  {}

  void error (const char *message)
  {
    g_printerr ("%s: %s\n", g_get_prgname (), message);
  }

  void consume_glyphs (hb_buffer_t  *buffer,
		       const char   *text HB_UNUSED,
		       unsigned int  text_len HB_UNUSED,
		       hb_bool_t     utf8_clusters HB_UNUSED)
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
  void finish (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts HB_UNUSED)
  {
    if (quiet)
      write_raw_paths ();
    else if (ned)
      write_flat_svg ();
    else
      write_reuse_svg ();

    hb_draw_funcs_destroy (draw_funcs);
    draw_funcs = nullptr;
    if (paint_funcs)
    {
      hb_paint_funcs_destroy (paint_funcs);
      paint_funcs = nullptr;
    }
    if (probe_paint_funcs)
    {
      hb_paint_funcs_destroy (probe_paint_funcs);
      probe_paint_funcs = nullptr;
    }
    g_string_free (path, true);
    path = nullptr;
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

  struct draw_data_t
  {
    draw_output_t *that;
    float tx;
    float ty;
    float sx;
    float sy;
  };

  struct bbox_t
  {
    bool valid = false;
    float x1 = 0.f;
    float y1 = 0.f;
    float x2 = 0.f;
    float y2 = 0.f;
  };

  struct layout_t
  {
    std::vector<std::pair<float, float>> offsets;
    bbox_t box {};
    float font_scale_x = 1.f;
    float font_scale_y = 1.f;
    float glyph_scale_x = 1.f;
    float glyph_scale_y = -1.f;
    bool upem_scale = false;
  };

  /* Color font rendering: paint callbacks, SVG generation, gradient helpers. */
#include "draw-output-color.hh"

  /* ===== Draw callbacks ===== */

  static void
  move_to (hb_draw_funcs_t *, draw_data_t *data,
	   hb_draw_state_t *,
	   float to_x, float to_y,
	   void *)
  {
    data->that->append_cmd_xy ('M', *data, to_x, to_y);
  }

  static void
  line_to (hb_draw_funcs_t *, draw_data_t *data,
	   hb_draw_state_t *,
	   float to_x, float to_y,
	   void *)
  {
    data->that->append_cmd_xy ('L', *data, to_x, to_y);
  }

  static void
  quadratic_to (hb_draw_funcs_t *, draw_data_t *data,
		hb_draw_state_t *,
		float control_x, float control_y,
		float to_x, float to_y,
		void *)
  {
    g_string_append_c (data->that->path, 'Q');
    data->that->append_xy (*data, control_x, control_y);
    g_string_append_c (data->that->path, ' ');
    data->that->append_xy (*data, to_x, to_y);
  }

  static void
  cubic_to (hb_draw_funcs_t *, draw_data_t *data,
	    hb_draw_state_t *,
	    float control1_x, float control1_y,
	    float control2_x, float control2_y,
	    float to_x, float to_y,
	    void *)
  {
    g_string_append_c (data->that->path, 'C');
    data->that->append_xy (*data, control1_x, control1_y);
    g_string_append_c (data->that->path, ' ');
    data->that->append_xy (*data, control2_x, control2_y);
    g_string_append_c (data->that->path, ' ');
    data->that->append_xy (*data, to_x, to_y);
  }

  static void
  close_path (hb_draw_funcs_t *, draw_data_t *data,
	      hb_draw_state_t *,
	      void *)
  {
    g_string_append_c (data->that->path, 'Z');
  }

  void append_cmd_xy (char cmd, const draw_data_t &data, float x, float y)
  {
    g_string_append_c (path, cmd);
    append_xy (data, x, y);
  }

  void append_xy (const draw_data_t &data, float x, float y)
  {
    append_num (data.tx + data.sx * x, precision, false);
    g_string_append_c (path, ',');
    append_num (data.ty + data.sy * y, precision, false);
  }

  void append_num (float v, int requested_precision, bool keep_nonzero)
  {
    int effective_precision = requested_precision;
    if (keep_nonzero && v != 0.f)
    {
      while (effective_precision < 12)
      {
	float rounded_zero_threshold = 0.5f;
	for (int i = 0; i < effective_precision; i++)
	  rounded_zero_threshold *= 0.1f;
	if (fabsf (v) >= rounded_zero_threshold)
	  break;
	effective_precision++;
      }
    }

    float rounded_zero_threshold = 0.5f;
    for (int i = 0; i < effective_precision; i++)
      rounded_zero_threshold *= 0.1f;
    if (fabsf (v) < rounded_zero_threshold)
      v = 0.f;

    char fmt[20];
    g_snprintf (fmt, sizeof (fmt), "%%.%df", effective_precision);
    char buf[128];
    g_ascii_formatd (buf, sizeof (buf), fmt, (double) v);

    char *dot = strchr (buf, '.');
    if (dot)
    {
      char *end = buf + strlen (buf) - 1;
      while (end > dot && *end == '0')
	*end-- = '\0';
      if (end == dot)
	*end = '\0';
    }

    g_string_append (path, buf);
  }

  bool
  draw_glyph (hb_font_t *draw_font,
	      hb_codepoint_t gid,
	      const draw_data_t &data)
  {
    g_string_set_size (path, 0);
    draw_data_t d = data;
    hb_font_draw_glyph (draw_font, gid, draw_funcs, &d);
    return path->len != 0;
  }

  float scale_to_upem_x (hb_position_t v) const
  {
    return (float) v * upem / x_scale;
  }

  float scale_to_upem_y (hb_position_t v) const
  {
    return (float) v * upem / y_scale;
  }

  void emit_svg_header (const bbox_t &box)
  {
    float x = box.x1 - margin.l;
    float y = box.y1 - margin.t;
    float w = box.x2 - box.x1 + margin.l + margin.r;
    float h = box.y2 - box.y1 + margin.t + margin.b;
    if (!(w > 0.f))
      w = 1.f;
    if (!(h > 0.f))
      h = 1.f;

    g_string_set_size (path, 0);
    append_num (x, precision, false);
    g_string_append_c (path, ' ');
    append_num (y, precision, false);
    g_string_append_c (path, ' ');
    append_num (w, precision, false);
    g_string_append_c (path, ' ');
    append_num (h, precision, false);
    fprintf (out_fp,
	     "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"%s\" width=\"",
	     path->str);
    g_string_set_size (path, 0);
    append_num (w, precision, false);
    fprintf (out_fp, "%s\" height=\"", path->str);
    g_string_set_size (path, 0);
    append_num (h, precision, false);
    fprintf (out_fp, "%s\">\n", path->str);

    if (has_background)
      emit_svg_background (x, y, w, h);
  }

  void emit_svg_background (float x, float y, float w, float h)
  {
    unsigned r = hb_color_get_red (background);
    unsigned g = hb_color_get_green (background);
    unsigned b = hb_color_get_blue (background);
    unsigned a = hb_color_get_alpha (background);

    fprintf (out_fp, "<rect x=\"");
    g_string_set_size (path, 0);
    append_num (x, precision, false);
    fprintf (out_fp, "%s\" y=\"", path->str);
    g_string_set_size (path, 0);
    append_num (y, precision, false);
    fprintf (out_fp, "%s\" width=\"", path->str);
    g_string_set_size (path, 0);
    append_num (w, precision, false);
    fprintf (out_fp, "%s\" height=\"", path->str);
    g_string_set_size (path, 0);
    append_num (h, precision, false);
    fprintf (out_fp, "%s\" fill=\"#%02X%02X%02X\"", path->str, r, g, b);
    if (a != 255)
      fprintf (out_fp, " fill-opacity=\"%.3f\"", a / 255.);
    fprintf (out_fp, "/>\n");
  }

  void emit_fill_group_open ()
  {
    if (!foreground_str)
      return;

    unsigned r = hb_color_get_red (foreground);
    unsigned g = hb_color_get_green (foreground);
    unsigned b = hb_color_get_blue (foreground);
    unsigned a = hb_color_get_alpha (foreground);

    fprintf (out_fp, "<g fill=\"#%02X%02X%02X\"", r, g, b);
    if (a != 255)
      fprintf (out_fp, " fill-opacity=\"%.3f\"", a / 255.);
    fprintf (out_fp, ">\n");
  }

  void emit_fill_group_close ()
  {
    if (!foreground_str)
      return;

    fprintf (out_fp, "</g>\n");
  }

  static void include_point (bbox_t *box, float x, float y)
  {
    if (!box->valid)
    {
      box->valid = true;
      box->x1 = box->x2 = x;
      box->y1 = box->y2 = y;
      return;
    }

    box->x1 = MIN (box->x1, x);
    box->y1 = MIN (box->y1, y);
    box->x2 = MAX (box->x2, x);
    box->y2 = MAX (box->y2, y);
  }

  static void include_scaled_point (bbox_t *box,
				    float sx, float sy,
				    float x, float y)
  {
    include_point (box, sx * x, sy * y);
  }

  static void include_scaled_rect (bbox_t *box,
				   float sx, float sy,
				   float x1, float y1,
				   float x2, float y2)
  {
    include_scaled_point (box, sx, sy, x1, y1);
    include_scaled_point (box, sx, sy, x2, y2);
  }

  std::vector<std::pair<float, float>> line_offsets () const
  {
    std::vector<std::pair<float, float>> offsets;
    offsets.reserve (lines.size ());

    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID)
      dir = HB_DIRECTION_LTR;

    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);
    hb_font_extents_t extents = {0, 0, 0};
    hb_font_get_extents_for_direction (font, dir, &extents);
    int axis_scale = vertical ? x_scale : y_scale;
    if (!axis_scale)
      axis_scale = upem;
    float step_upem = fabsf ((float) (extents.ascender - extents.descender + extents.line_gap) * upem / axis_scale);
    if (!(step_upem > 0.f))
      step_upem = upem;

    for (unsigned i = 0; i < lines.size (); i++)
    {
      if (vertical)
	offsets.emplace_back (-step_upem * i, 0.f);
      else
	offsets.emplace_back (0.f, -step_upem * i);
    }
    return offsets;
  }

  bbox_t compute_bbox (const std::vector<std::pair<float, float>> &offsets,
		       float glyph_scale_x,
		       float glyph_scale_y) const
  {
    bbox_t box {};
    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID)
      dir = HB_DIRECTION_LTR;
    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);

    bool include_logical = logical || (!logical && !ink);
    bool include_ink = ink;

    if (include_ink)
    {
      // Include ink (actual glyph) extents
      for (unsigned li = 0; li < lines.size (); li++)
      {
	const line_t &line = lines[li];
	const auto &offset = offsets[li];

	for (const glyph_instance_t &glyph : line.glyphs)
	{
	  hb_glyph_extents_t extents;
	  if (hb_font_get_glyph_extents (upem_font, glyph.gid, &extents))
	  {
	    float x1 = offset.first + glyph.x + extents.x_bearing;
	    float y1 = offset.second + glyph.y - extents.y_bearing;
	    float x2 = x1 + extents.width;
	    float y2 = y1 - extents.height;
	    include_scaled_rect (&box, glyph_scale_x, glyph_scale_y, x1, y1, x2, y2);
	  }
	}
      }
    }

    if (include_logical)
    {
      // Include logical extents and advances
      hb_font_extents_t logical_extents = {0, 0, 0};
      hb_font_get_extents_for_direction (font, dir, &logical_extents);
      int axis_scale = vertical ? x_scale : y_scale;
      if (!axis_scale)
	axis_scale = upem;
      float logical_asc = (float) logical_extents.ascender * upem / axis_scale;
      float logical_desc = (float) logical_extents.descender * upem / axis_scale;
      float logical_min = MIN (logical_desc, logical_asc);
      float logical_max = MAX (logical_desc, logical_asc);

      for (unsigned li = 0; li < lines.size (); li++)
      {
	const line_t &line = lines[li];
	const auto &offset = offsets[li];

	float x1, y1, x2, y2;
	if (vertical)
	{
	  x1 = offset.first + logical_min;
	  x2 = offset.first + logical_max;
	  y1 = MIN (0.f, line.advance_y);
	  y2 = MAX (0.f, line.advance_y);
	}
	else
	{
	  x1 = MIN (0.f, line.advance_x);
	  x2 = MAX (0.f, line.advance_x);
	  y1 = offset.second + logical_min;
	  y2 = offset.second + logical_max;
	}
	include_scaled_rect (&box, glyph_scale_x, glyph_scale_y, x1, y1, x2, y2);
      }
    }

    if (box.valid)
      return box;

    include_point (&box, 0.f, 0.f);
    if (lines.empty ())
      include_point (&box, 1.f, 1.f);
    else
    {
      float max_w = 0.f;
      float max_h = 0.f;
      for (unsigned li = 0; li < lines.size (); li++)
      {
	const line_t &line = lines[li];
	max_w = MAX (max_w, fabsf (glyph_scale_x * line.advance_x));
	max_h = MAX (max_h, fabsf (glyph_scale_y * line.advance_y));
      }
      if (!(max_w > 0.f))
	max_w = fabsf (glyph_scale_x);
      if (!(max_h > 0.f))
	max_h = fabsf (glyph_scale_y);
      include_point (&box, max_w, max_h);
    }
    return box;
  }

  layout_t compute_layout (bool include_bbox) const
  {
    layout_t layout {};
    layout.offsets = line_offsets ();
    layout.font_scale_x = scalbnf ((float) x_scale, -(int) subpixel_bits);
    layout.font_scale_y = scalbnf ((float) y_scale, -(int) subpixel_bits);
    layout.glyph_scale_x = layout.font_scale_x / upem;
    layout.glyph_scale_y = -layout.font_scale_y / upem;

    int upem_scaled = (int) ((unsigned) upem << subpixel_bits);
    layout.upem_scale = x_scale == upem_scaled && y_scale == upem_scaled;

    if (include_bbox)
      layout.box = compute_bbox (layout.offsets,
				 layout.glyph_scale_x,
				 layout.glyph_scale_y);
    return layout;
  }

  draw_data_t glyph_draw_data (const layout_t &layout,
			       const std::pair<float, float> &offset,
			       const glyph_instance_t &glyph)
  {
    draw_data_t data =
    {
      this,
      layout.glyph_scale_x * (offset.first + glyph.x),
      layout.glyph_scale_y * (offset.second + glyph.y),
      layout.glyph_scale_x,
      layout.glyph_scale_y
    };
    return data;
  }

  void write_reuse_svg ()
  {
    layout_t layout = compute_layout (true);

    if (!paint_funcs)
    {
      /* No color: original reuse path. */
      emit_svg_header (layout.box);

      std::vector<hb_codepoint_t> unique_gids;
      unique_gids.reserve (128);
      std::unordered_set<hb_codepoint_t> seen;

      for (const line_t &line : lines)
	for (const glyph_instance_t &glyph : line.glyphs)
	  if (seen.insert (glyph.gid).second)
	    unique_gids.push_back (glyph.gid);

      fprintf (out_fp, "<defs>\n");
      draw_data_t defs_data = {this, 0.f, 0.f, 1.f, 1.f};
      for (hb_codepoint_t gid : unique_gids)
      {
	draw_glyph (upem_font, gid, defs_data);
	fprintf (out_fp, "<path id=\"p%u\" d=\"%s\"/>\n", gid, path->str);
      }
      fprintf (out_fp, "</defs>\n");

      emit_fill_group_open ();

      if (!layout.upem_scale)
      {
	g_string_set_size (path, 0);
	append_num (layout.font_scale_x, scale_precision (), true);
	g_string_append_c (path, ' ');
	append_num (-layout.font_scale_y, scale_precision (), true);
	fprintf (out_fp, "<g transform=\"scale(%s)\">\n", path->str);

	g_string_set_size (path, 0);
	int upem_precision = scale_precision () < 9 ? 9 : scale_precision ();
	append_num (1.f / upem, upem_precision, true);
	fprintf (out_fp, "<g transform=\"scale(%s)\">\n", path->str);
      }

      for (unsigned li = 0; li < lines.size (); li++)
      {
	const line_t &line = lines[li];
	const auto &offset = layout.offsets[li];
	for (const glyph_instance_t &glyph : line.glyphs)
	{
	  g_string_set_size (path, 0);
	  append_num (offset.first + glyph.x, precision, false);
	  g_string_append_c (path, ' ');
	  append_num (layout.upem_scale ? -(offset.second + glyph.y)
					:  (offset.second + glyph.y),
		      precision,
		      false);
	  if (layout.upem_scale)
	    fprintf (out_fp,
		     "<use href=\"#p%u\" transform=\"translate(%s) scale(1,-1)\"/>\n",
		     glyph.gid,
		     path->str);
	  else
	    fprintf (out_fp,
		     "<use href=\"#p%u\" transform=\"translate(%s)\"/>\n",
		     glyph.gid,
		     path->str);
	}
      }

      if (!layout.upem_scale)
	fprintf (out_fp, "</g>\n</g>\n");
      emit_fill_group_close ();
      fprintf (out_fp, "</svg>\n");
      return;
    }

    /* Color mode with reuse: color glyphs rendered inline,
     * non-color glyphs use <defs>/<use> reuse. */

    rect_clip_counter = 0;
    gradient_counter = 0;
    defined_outlines.clear ();
    defined_clips.clear ();

    std::unordered_set<hb_codepoint_t> color_gids;
    std::vector<hb_codepoint_t> unique_gids;
    unique_gids.reserve (128);
    std::unordered_set<hb_codepoint_t> seen;

    for (const line_t &line : lines)
      for (const glyph_instance_t &glyph : line.glyphs)
	if (seen.insert (glyph.gid).second)
	  unique_gids.push_back (glyph.gid);

    /* Probe for color glyphs using no-op paint funcs (no side effects). */
    for (hb_codepoint_t gid : unique_gids)
      if (hb_font_paint_glyph_or_fail (upem_font, gid,
					probe_paint_funcs, nullptr,
					palette, foreground))
	color_gids.insert (gid);

    GString *all_defs = g_string_new (nullptr);
    GString *all_body = g_string_new (nullptr);

    /* Non-color glyph outline defs for reuse.
     * Stored as raw outlines (no Y-flip) so clips can share them. */
    {
      draw_data_t defs_data = {this, 0.f, 0.f, 1.f, 1.f};
      for (hb_codepoint_t gid : unique_gids)
      {
	if (color_gids.count (gid))
	  continue;
	draw_glyph (upem_font, gid, defs_data);
	g_string_append_printf (all_defs,
				"<path id=\"p%u\" d=\"%s\"/>\n",
				gid,
				path->str);
	defined_outlines.insert (gid);
      }
    }

    /* Color glyph defs for reuse. */
    for (hb_codepoint_t gid : unique_gids)
    {
      if (!color_gids.count (gid))
	continue;

      GString *body = g_string_new (nullptr);
      GString *defs = g_string_new (nullptr);

      if (paint_glyph_to_svg (gid, defs, body))
      {
	if (defs->len)
	  g_string_append_len (all_defs, defs->str, defs->len);

	g_string_append_printf (all_defs, "<g id=\"cg%u\">\n", gid);
	g_string_append_len (all_defs, body->str, body->len);
	g_string_append (all_defs, "</g>\n");
      }

      g_string_free (body, true);
      g_string_free (defs, true);
    }

    if (!layout.upem_scale)
    {
      g_string_set_size (path, 0);
      append_num (layout.font_scale_x, scale_precision (), true);
      g_string_append_c (path, ' ');
      append_num (-layout.font_scale_y, scale_precision (), true);
      g_string_append_printf (all_body, "<g transform=\"scale(%s)\">\n", path->str);

      g_string_set_size (path, 0);
      int upem_precision = scale_precision () < 9 ? 9 : scale_precision ();
      append_num (1.f / upem, upem_precision, true);
      g_string_append_printf (all_body, "<g transform=\"scale(%s)\">\n", path->str);
    }

    for (unsigned li = 0; li < lines.size (); li++)
    {
      const line_t &line = lines[li];
      const auto &offset = layout.offsets[li];
      for (const glyph_instance_t &glyph : line.glyphs)
      {
	g_string_set_size (path, 0);
	append_num (offset.first + glyph.x, precision, false);
	g_string_append_c (path, ' ');
	append_num (layout.upem_scale ? -(offset.second + glyph.y)
				      :  (offset.second + glyph.y),
		    precision,
		    false);

	if (color_gids.count (glyph.gid))
	{
	  /* Color: use shared color glyph definition with Y-flip on <use>. */
	  if (layout.upem_scale)
	    g_string_append_printf (all_body,
				    "<use href=\"#cg%u\" transform=\"translate(%s) scale(1,-1)\"/>\n",
				    glyph.gid,
				    path->str);
	  else
	    g_string_append_printf (all_body,
				    "<use href=\"#cg%u\" transform=\"translate(%s)\"/>\n",
				    glyph.gid,
				    path->str);
	}
	else
	{
	  /* Non-color: use shared outline with Y-flip on <use>. */
	  if (layout.upem_scale)
	    g_string_append_printf (all_body,
				    "<use href=\"#p%u\" transform=\"translate(%s) scale(1,-1)\"/>\n",
				    glyph.gid,
				    path->str);
	  else
	    g_string_append_printf (all_body,
				    "<use href=\"#p%u\" transform=\"translate(%s)\"/>\n",
				    glyph.gid,
				    path->str);
	}
      }
    }

    if (!layout.upem_scale)
      g_string_append (all_body, "</g>\n</g>\n");

    emit_svg_header (layout.box);
    if (all_defs->len)
    {
      fprintf (out_fp, "<defs>\n");
      fwrite (all_defs->str, 1, all_defs->len, out_fp);
      fprintf (out_fp, "</defs>\n");
    }
    emit_fill_group_open ();
    fwrite (all_body->str, 1, all_body->len, out_fp);
    emit_fill_group_close ();
    fprintf (out_fp, "</svg>\n");

    g_string_free (all_defs, true);
    g_string_free (all_body, true);
  }

  void write_flat_svg ()
  {
    layout_t layout = compute_layout (true);

    if (!paint_funcs)
    {
      emit_svg_header (layout.box);
      emit_fill_group_open ();
      for (unsigned li = 0; li < lines.size (); li++)
      {
	const line_t &line = lines[li];
	const auto &offset = layout.offsets[li];
	for (const glyph_instance_t &glyph : line.glyphs)
	{
	  draw_data_t data = glyph_draw_data (layout, offset, glyph);
	  if (draw_glyph (upem_font, glyph.gid, data))
	    fprintf (out_fp, "<path d=\"%s\"/>\n", path->str);
	  else
	    fprintf (out_fp, "\n");
	}
      }
      emit_fill_group_close ();
      fprintf (out_fp, "</svg>\n");
      return;
    }

    /* Color mode: buffer everything so defs come first. */
    rect_clip_counter = 0;
    gradient_counter = 0;
    defined_outlines.clear ();
    defined_clips.clear ();

    GString *all_defs = g_string_new (nullptr);
    GString *all_body = g_string_new (nullptr);

    for (unsigned li = 0; li < lines.size (); li++)
    {
      const line_t &line = lines[li];
      const auto &offset = layout.offsets[li];
      for (const glyph_instance_t &glyph : line.glyphs)
      {
	GString *body = g_string_new (nullptr);
	GString *defs = g_string_new (nullptr);

	if (paint_glyph_to_svg (glyph.gid, defs, body))
	{
	  if (defs->len)
	    g_string_append_len (all_defs, defs->str, defs->len);

	  float tx = layout.glyph_scale_x * (offset.first + glyph.x);
	  float ty = layout.glyph_scale_y * (offset.second + glyph.y);
	  float sx = layout.glyph_scale_x;
	  float sy = layout.glyph_scale_y;

	  g_string_append (all_body, "<g transform=\"translate(");
	  append_num_to (all_body, tx, scale_precision ());
	  g_string_append_c (all_body, ',');
	  append_num_to (all_body, ty, scale_precision ());
	  g_string_append (all_body, ") scale(");
	  append_num_to (all_body, sx, scale_precision ());
	  g_string_append_c (all_body, ',');
	  append_num_to (all_body, sy, scale_precision ());
	  g_string_append (all_body, ")\">\n");
	  g_string_append_len (all_body, body->str, body->len);
	  g_string_append (all_body, "</g>\n");
	}
	else
	{
	  draw_data_t data = glyph_draw_data (layout, offset, glyph);
	  if (draw_glyph (upem_font, glyph.gid, data))
	    g_string_append_printf (all_body, "<path d=\"%s\"/>\n", path->str);
	  else
	    g_string_append (all_body, "\n");
	}

	g_string_free (body, true);
	g_string_free (defs, true);
      }
    }

    emit_svg_header (layout.box);
    if (all_defs->len)
    {
      fprintf (out_fp, "<defs>\n");
      fwrite (all_defs->str, 1, all_defs->len, out_fp);
      fprintf (out_fp, "</defs>\n");
    }
    emit_fill_group_open ();
    fwrite (all_body->str, 1, all_body->len, out_fp);
    emit_fill_group_close ();
    fprintf (out_fp, "</svg>\n");

    g_string_free (all_defs, true);
    g_string_free (all_body, true);
  }

  void write_raw_paths ()
  {
    layout_t layout = compute_layout (false);

    for (unsigned li = 0; li < lines.size (); li++)
    {
      const line_t &line = lines[li];
      const auto &offset = layout.offsets[li];
      for (const glyph_instance_t &glyph : line.glyphs)
      {
	draw_data_t data = glyph_draw_data (layout, offset, glyph);
	if (draw_glyph (upem_font, glyph.gid, data))
	  fprintf (out_fp, "%s\n", path->str);
	else
	  fprintf (out_fp, "\n");
      }
    }
  }

  hb_bool_t quiet = false;
  hb_bool_t ned = false;
  hb_bool_t no_color = false;
  int precision = 2;
  int scale_precision () const { return precision < 5 ? 5 : precision; }
  int palette = 0;
  char *background_str = nullptr;
  hb_color_t background = HB_COLOR (255, 255, 255, 255);
  hb_bool_t has_background = false;
  char *foreground_str = nullptr;
  hb_color_t foreground = HB_COLOR (0, 0, 0, 255);
  margin_t margin = {0., 0., 0., 0.};
  hb_bool_t logical = false;
  hb_bool_t ink = false;

  hb_font_t *font = nullptr;
  hb_font_t *upem_font = nullptr;
  hb_draw_funcs_t *draw_funcs = nullptr;
  hb_paint_funcs_t *paint_funcs = nullptr;
  hb_paint_funcs_t *probe_paint_funcs = nullptr;
  GString *path = nullptr;
  std::vector<line_t> lines;
  hb_direction_t direction = HB_DIRECTION_INVALID;

  unsigned upem = 0;
  int x_scale = 0;
  int y_scale = 0;
  unsigned subpixel_bits = 0;

  /* Persistent state across paint calls within one SVG. */
  unsigned rect_clip_counter = 0;
  unsigned gradient_counter = 0;
  std::unordered_set<hb_codepoint_t> defined_outlines;
  std::unordered_set<hb_codepoint_t> defined_clips;
};


#endif
