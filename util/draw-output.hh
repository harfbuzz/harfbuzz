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

#include <unordered_set>
#include <utility>
#include <vector>

#include "output-options.hh"


struct draw_output_t : output_options_t<>
{
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
      ned = true;
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

    upem_font = hb_font_create_sub_font (font);
    hb_font_set_scale (upem_font, upem, upem);

    path = g_string_new (nullptr);
    draw_funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (draw_funcs, (hb_draw_move_to_func_t) move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (draw_funcs, (hb_draw_line_to_func_t) line_to, nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (draw_funcs, (hb_draw_quadratic_to_func_t) quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (draw_funcs, (hb_draw_cubic_to_func_t) cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (draw_funcs, (hb_draw_close_path_func_t) close_path, nullptr, nullptr);
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
    float x = box.x1;
    float y = box.y1;
    float w = box.x2 - box.x1;
    float h = box.y2 - box.y1;
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
	     "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"%s\" width=\"",
	     path->str);
    g_string_set_size (path, 0);
    append_num (w, precision, false);
    fprintf (out_fp, "%s\" height=\"", path->str);
    g_string_set_size (path, 0);
    append_num (h, precision, false);
    fprintf (out_fp, "%s\">\n", path->str);
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
    hb_position_t step = extents.ascender - extents.descender + extents.line_gap;
    if (!step)
      step = vertical ? abs (x_scale) : abs (y_scale);

    float step_upem = vertical ? fabsf ((float) step * upem / x_scale)
			       : fabsf ((float) step * upem / y_scale);
    if (!step_upem)
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

  bbox_t compute_bbox (const std::vector<std::pair<float, float>> &offsets) const
  {
    bbox_t box {};
    float sx = scalbnf ((float) x_scale, -(int) subpixel_bits);
    float sy = -scalbnf ((float) y_scale, -(int) subpixel_bits);

    for (unsigned li = 0; li < lines.size (); li++)
    {
      const line_t &line = lines[li];
      const auto &offset = offsets[li];
      for (const glyph_instance_t &glyph : line.glyphs)
      {
	hb_glyph_extents_t extents = {0, 0, 0, 0};
	if (!hb_font_get_glyph_extents (upem_font, glyph.gid, &extents))
	  continue;

	float x1 = offset.first + glyph.x + extents.x_bearing;
	float x2 = x1 + extents.width;
	float y2 = offset.second + glyph.y + extents.y_bearing;
	float y1 = y2 + extents.height;

	float fx1 = sx * x1 / upem;
	float fx2 = sx * x2 / upem;
	float fy1 = sy * y1 / upem;
	float fy2 = sy * y2 / upem;

	include_point (&box, fx1, fy1);
	include_point (&box, fx2, fy2);
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
	max_w = MAX (max_w, fabsf (sx * line.advance_x / upem));
	max_h = MAX (max_h, fabsf (sy * line.advance_y / upem));
      }
      if (!(max_w > 0.f))
	max_w = fabsf (sx);
      if (!(max_h > 0.f))
	max_h = fabsf (sy);
      include_point (&box, max_w, max_h);
    }
    return box;
  }

  void write_reuse_svg ()
  {
    std::vector<std::pair<float, float>> offsets = line_offsets ();
    bbox_t box = compute_bbox (offsets);
    emit_svg_header (box);

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
      if (!draw_glyph (upem_font, gid, defs_data))
	fprintf (out_fp, "<path id=\"g%u\" d=\"\"/>\n", gid);
      else
	fprintf (out_fp, "<path id=\"g%u\" d=\"%s\"/>\n", gid, path->str);
    }
    fprintf (out_fp, "</defs>\n");

    g_string_set_size (path, 0);
    append_num (scalbnf ((float) x_scale, -(int) subpixel_bits), precision, true);
    g_string_append_c (path, ' ');
    append_num (-scalbnf ((float) y_scale, -(int) subpixel_bits), precision, true);
    fprintf (out_fp, "<g transform=\"scale(%s)\">\n", path->str);

    g_string_set_size (path, 0);
    int upem_scale_precision = precision < 9 ? 9 : precision;
    append_num (1.f / upem, upem_scale_precision, true);
    fprintf (out_fp, "<g transform=\"scale(%s)\">\n", path->str);

    for (unsigned li = 0; li < lines.size (); li++)
    {
      const line_t &line = lines[li];
      const auto &offset = offsets[li];
      for (const glyph_instance_t &glyph : line.glyphs)
      {
	g_string_set_size (path, 0);
	append_num (offset.first + glyph.x, precision, false);
	g_string_append_c (path, ' ');
	append_num (offset.second + glyph.y, precision, false);
	fprintf (out_fp,
		 "<use href=\"#g%u\" transform=\"translate(%s)\"/>\n",
		 glyph.gid,
		 path->str);
      }
    }

    fprintf (out_fp, "</g>\n</g>\n</svg>\n");
  }

  void write_flat_svg ()
  {
    std::vector<std::pair<float, float>> offsets = line_offsets ();
    bbox_t box = compute_bbox (offsets);
    emit_svg_header (box);

    float sx = scalbnf ((float) x_scale, -(int) subpixel_bits) / upem;
    float sy = -scalbnf ((float) y_scale, -(int) subpixel_bits) / upem;
    for (unsigned li = 0; li < lines.size (); li++)
    {
      const line_t &line = lines[li];
      const auto &offset = offsets[li];
      for (const glyph_instance_t &glyph : line.glyphs)
      {
	draw_data_t data =
	{
	  this,
	  sx * (offset.first + glyph.x),
	  sy * (offset.second + glyph.y),
	  sx,
	  sy
	};
	if (draw_glyph (upem_font, glyph.gid, data))
	  fprintf (out_fp, "<path d=\"%s\"/>\n", path->str);
	else
	  fprintf (out_fp, "\n");
      }
    }

    fprintf (out_fp, "</svg>\n");
  }

  void write_raw_paths ()
  {
    std::vector<std::pair<float, float>> offsets = line_offsets ();
    float sx = scalbnf ((float) x_scale, -(int) subpixel_bits) / upem;
    float sy = -scalbnf ((float) y_scale, -(int) subpixel_bits) / upem;

    for (unsigned li = 0; li < lines.size (); li++)
    {
      const line_t &line = lines[li];
      const auto &offset = offsets[li];
      for (const glyph_instance_t &glyph : line.glyphs)
      {
	draw_data_t data =
	{
	  this,
	  sx * (offset.first + glyph.x),
	  sy * (offset.second + glyph.y),
	  sx,
	  sy
	};
	if (draw_glyph (upem_font, glyph.gid, data))
	  fprintf (out_fp, "%s\n", path->str);
	else
	  fprintf (out_fp, "\n");
      }
    }
  }

  hb_bool_t quiet = false;
  hb_bool_t ned = false;
  int precision = 2;

  hb_font_t *font = nullptr;
  hb_font_t *upem_font = nullptr;
  hb_draw_funcs_t *draw_funcs = nullptr;
  GString *path = nullptr;
  std::vector<line_t> lines;
  hb_direction_t direction = HB_DIRECTION_INVALID;

  unsigned upem = 0;
  int x_scale = 0;
  int y_scale = 0;
  unsigned subpixel_bits = 0;
};


#endif
