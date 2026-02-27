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

#ifndef RASTER_OUTPUT_HH
#define RASTER_OUTPUT_HH

#include "options.hh"
#include "output-options.hh"
#include "view-options.hh"
#include "hb-raster.h"
#include "hb-ot.h"

#include <math.h>
#include <vector>


struct raster_output_t : output_options_t<true>, view_options_t
{
  static const bool repeat_shape = false;

  ~raster_output_t ()
  {
    hb_raster_draw_destroy (rdr);
    hb_raster_paint_destroy (pnt);
    hb_font_destroy (font);
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Rasterize text with given font.");
    parser->set_description ("Renders shaped text as a PPM raster image.");
    refuse_tty = true;
    output_options_t::add_options (parser);
    view_options_t::add_options (parser);
  }

  template <typename app_t>
  void init (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts)
  {
    lines.clear ();
    direction = HB_DIRECTION_INVALID;

    font = hb_font_reference (font_opts->font);
    subpixel_bits = font_opts->subpixel_bits;

    hb_face_t *face = hb_font_get_face (font);
    has_color = hb_ot_color_has_paint (face) ||
		hb_ot_color_has_layers (face) ||
		hb_ot_color_has_svg (face);

    /* Parse foreground / background colors */
    {
      unsigned r, g, b, a;
      const char *fg_spec = fore ? fore : DEFAULT_FORE;
      if (foreground_use_palette && foreground_palette && foreground_palette->len)
      {
	auto &c = g_array_index (foreground_palette, rgba_color_t, 0);
	r = c.r; g = c.g; b = c.b; a = c.a;
	fg_color = HB_COLOR ((uint8_t) b, (uint8_t) g, (uint8_t) r, (uint8_t) a);
      }
      else if (parse_color (fg_spec, r, g, b, a))
	fg_color = HB_COLOR ((uint8_t) b, (uint8_t) g, (uint8_t) r, (uint8_t) a);
    }
    {
      const char *bg_spec = back ? back : DEFAULT_BACK;
      unsigned r, g, b, a;
      if (parse_color (bg_spec, r, g, b, a))
      {
	bg_r = (uint8_t) r;
	bg_g = (uint8_t) g;
	bg_b = (uint8_t) b;
	bg_a = (uint8_t) a;
      }
    }

    rdr = hb_raster_draw_create_or_fail ();
    if (has_color)
    {
      pnt = hb_raster_paint_create_or_fail ();
      hb_raster_paint_set_foreground (pnt, fg_color);
      hb_raster_paint_clear_custom_palette_colors (pnt);
      if (custom_palette_entries)
      {
	for (unsigned i = 0; i < custom_palette_entries->len; i++)
	{
	  auto &entry =
	    g_array_index (custom_palette_entries,
			   typename view_options_t::custom_palette_entry_t, i);
	  hb_raster_paint_set_custom_palette_color (pnt, entry.index,
						    HB_COLOR ((uint8_t) entry.color.b,
							      (uint8_t) entry.color.g,
							      (uint8_t) entry.color.r,
							      (uint8_t) entry.color.a));
	}
      }
    }
  }

  void new_line () { lines.emplace_back (); }

  void consume_text (hb_buffer_t  * HB_UNUSED,
		     const char   * HB_UNUSED,
		     unsigned int   HB_UNUSED,
		     hb_bool_t      HB_UNUSED) {}

  void error (const char *message)
  { g_printerr ("%s: %s\n", g_get_prgname (), message); }

  void consume_glyphs (hb_buffer_t  *buffer,
		       const char   * HB_UNUSED,
		       unsigned int   HB_UNUSED,
		       hb_bool_t      HB_UNUSED)
  {
    if (lines.empty ()) lines.emplace_back ();
    line_t &line = lines.back ();

    hb_direction_t line_dir = hb_buffer_get_direction (buffer);
    if (line_dir == HB_DIRECTION_INVALID)
      line_dir = HB_DIRECTION_LTR;
    if (direction == HB_DIRECTION_INVALID)
      direction = line_dir;

    unsigned count;
    const hb_glyph_info_t     *infos     = hb_buffer_get_glyph_infos     (buffer, &count);
    const hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (buffer, &count);

    hb_position_t pen_x = 0, pen_y = 0;
    line.glyphs.reserve (line.glyphs.size () + count);
    for (unsigned i = 0; i < count; i++)
    {
      line.glyphs.push_back ({
	infos[i].codepoint,
	(float) (pen_x + positions[i].x_offset),
	(float) (pen_y + positions[i].y_offset),
      });
      pen_x += positions[i].x_advance;
      pen_y += positions[i].y_advance;
    }
    line.advance_x = (float) pen_x;
    line.advance_y = (float) pen_y;
  }

  template <typename app_t>
  void finish (hb_buffer_t *buffer HB_UNUSED, const app_t *app)
  {
    unsigned int num_iterations = app->num_iterations ? app->num_iterations : 1;

    /* pixels per font unit */
    float sx = scalbnf (1.f, -(int) subpixel_bits);
    float sy = scalbnf (1.f, -(int) subpixel_bits);
    float scale_factor = scalbnf (1.f, (int) subpixel_bits);

    hb_raster_draw_set_scale_factor (rdr, scale_factor, scale_factor);
    if (pnt)
      hb_raster_paint_set_scale_factor (pnt, scale_factor, scale_factor);

    /* line step in font units */
    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID) dir = HB_DIRECTION_LTR;
    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);

    float step = 0.f;
    if (have_font_extents)
      step = fabsf (scalbnf ((float) (font_extents.ascent + font_extents.descent + font_extents.line_gap),
			     (int) subpixel_bits));
    else
    {
      hb_font_extents_t extents = {};
      hb_font_get_extents_for_direction (font, dir, &extents);
      step = fabsf ((float) (extents.ascender - extents.descender + extents.line_gap));
    }
    step += scalbnf ((float) line_space, (int) subpixel_bits);
    if (step < 0.f)
      step = 0.f;
    if (!(step > 0.f))
    {
      int x_scale = 0, y_scale = 0;
      hb_font_get_scale (font, &x_scale, &y_scale);
      int axis_scale = vertical ? x_scale : y_scale;
      step = axis_scale ? fabsf ((float) axis_scale) : 1.f;
    }

    if (has_color && pnt)
    {
      finish_paint (sx, sy, step, vertical, num_iterations);
    }
    else if (foreground_use_palette && foreground_palette && foreground_palette->len > 1)
    {
      finish_draw_palette (sx, sy, step, vertical, num_iterations);
    }
    else
    {
      hb_raster_extents_t ext;
      if (!compute_extents (sx, sy, step, vertical, &ext))
      {
	cleanup ();
	return;
      }

      for (unsigned int iter = 0; iter < num_iterations; iter++)
      {
	hb_raster_draw_set_extents (rdr, &ext);
	for (unsigned li = 0; li < lines.size (); li++)
	{
	  float off_x = vertical ? -(step * (float) li) : 0.f;
	  float off_y = vertical ?  0.f                 : -(step * (float) li);

	  for (const auto &g : lines[li].glyphs)
	  {
	    float pen_x = g.x + off_x;
	    float pen_y = g.y + off_y;
	    hb_raster_draw_set_transform (rdr, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
	    hb_raster_draw_glyph (rdr, font, g.gid, pen_x, pen_y);
	  }
	}

	hb_raster_image_t *img = hb_raster_draw_render (rdr);
	if (img)
	{
	  if (iter + 1 == num_iterations)
	    write_a8_as_ppm (img, sx, sy, step, vertical, ext);
	  hb_raster_draw_recycle_image (rdr, img);
	}
      }
    }

    cleanup ();
  }

  private:

  struct glyph_instance_t { hb_codepoint_t gid; float x, y; };
  struct rect_i_t { int x0, y0, x1, y1; };
  struct line_t
  {
    float advance_x = 0.f, advance_y = 0.f;
    std::vector<glyph_instance_t> glyphs;
  };

  void finish_paint (float sx, float sy, float step, bool vertical, unsigned int num_iterations)
  {
    hb_raster_extents_t ext;
    if (!compute_extents (sx, sy, step, vertical, &ext))
      return;
    unsigned w = ext.width;
    unsigned h = ext.height;
    unsigned stride = w * 4;
    ext.stride = stride;
    hb_raster_image_t *out_img = hb_raster_image_create_or_fail ();
    if (!out_img) return;
    if (!hb_raster_image_configure (out_img, HB_RASTER_FORMAT_BGRA32, &ext))
    {
      hb_raster_image_destroy (out_img);
      return;
    }
    uint8_t *out_buf = const_cast<uint8_t *> (hb_raster_image_get_buffer (out_img));
    if (!out_buf)
    {
      hb_raster_image_destroy (out_img);
      return;
    }

    for (unsigned int iter = 0; iter < num_iterations; iter++)
    {
      hb_raster_image_clear (out_img);
      /* Second pass: paint each glyph */
      unsigned glyph_index = 0;
      for (unsigned li = 0; li < lines.size (); li++)
      {
	float off_x = vertical ? -(step * (float) li) : 0.f;
	float off_y = vertical ?  0.f                 : -(step * (float) li);

	for (const auto &g : lines[li].glyphs)
	{
	  hb_color_t glyph_fg = foreground_for_glyph (glyph_index++);
	  float pen_x = g.x + off_x;
	  float pen_y = g.y + off_y;

	  hb_raster_paint_set_transform (pnt, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
	  hb_raster_paint_set_extents (pnt, &ext);

	  hb_raster_paint_glyph (pnt, font, g.gid, pen_x, pen_y, palette, glyph_fg);

	  hb_raster_image_t *img = hb_raster_paint_render (pnt);
	  if (img)
	  {
	    /* Composite onto output buffer (SRC_OVER) */
	    const uint8_t *src = hb_raster_image_get_buffer (img);
	    hb_raster_extents_t img_ext;
	    hb_raster_image_get_extents (img, &img_ext);

	    for (unsigned y = 0; y < h; y++)
	      for (unsigned x = 0; x < w; x++)
	      {
		uint32_t s;
		hb_memcpy (&s, src + y * img_ext.stride + x * 4, 4);
		if (!s) continue;
		uint8_t sa = (uint8_t) (s >> 24);
		uint32_t d;
		hb_memcpy (&d, out_buf + y * stride + x * 4, 4);
		if (sa == 255) { d = s; }
		else
		{
		  unsigned inv_sa = 255 - sa;
		  uint8_t rb = (uint8_t) (((d & 0xFF) * inv_sa + 128 + (((d & 0xFF) * inv_sa + 128) >> 8)) >> 8) + (uint8_t) (s & 0xFF);
		  uint8_t rg = (uint8_t) ((((d >> 8) & 0xFF) * inv_sa + 128 + ((((d >> 8) & 0xFF) * inv_sa + 128) >> 8)) >> 8) + (uint8_t) ((s >> 8) & 0xFF);
		  uint8_t rr = (uint8_t) ((((d >> 16) & 0xFF) * inv_sa + 128 + ((((d >> 16) & 0xFF) * inv_sa + 128) >> 8)) >> 8) + (uint8_t) ((s >> 16) & 0xFF);
		  uint8_t ra = (uint8_t) ((((d >> 24) & 0xFF) * inv_sa + 128 + ((((d >> 24) & 0xFF) * inv_sa + 128) >> 8)) >> 8) + sa;
		  d = (uint32_t) rb | ((uint32_t) rg << 8) | ((uint32_t) rr << 16) | ((uint32_t) ra << 24);
		}
		hb_memcpy (out_buf + y * stride + x * 4, &d, 4);
	      }

	    hb_raster_paint_recycle_image (pnt, img);
	  }
	}
      }

      if (iter + 1 == num_iterations)
      {
	if (show_extents)
	{
	  std::vector<rect_i_t> rects;
	  collect_ink_extents_rects (sx, sy, step, vertical, ext, rects);
	  overlay_rects_bgra (out_buf, w, h, stride, rects);
	}
	/* Write as PPM (RGB, Y-flipped). */
	write_ppm (out_buf, w, h, stride);
      }
    }
    hb_raster_image_destroy (out_img);
  }

  void finish_draw_palette (float sx, float sy, float step, bool vertical, unsigned int num_iterations)
  {
    hb_raster_extents_t ext;
    if (!compute_extents (sx, sy, step, vertical, &ext))
      return;
    unsigned w = ext.width;
    unsigned h = ext.height;
    unsigned stride = w * 4;
    ext.stride = stride;

    hb_raster_image_t *out_img = hb_raster_image_create_or_fail ();
    if (!out_img) return;
    if (!hb_raster_image_configure (out_img, HB_RASTER_FORMAT_BGRA32, &ext))
    {
      hb_raster_image_destroy (out_img);
      return;
    }
    uint8_t *out_buf = const_cast<uint8_t *> (hb_raster_image_get_buffer (out_img));
    if (!out_buf)
    {
      hb_raster_image_destroy (out_img);
      return;
    }

    for (unsigned int iter = 0; iter < num_iterations; iter++)
    {
      hb_raster_image_clear (out_img);
      unsigned glyph_index = 0;
      for (unsigned li = 0; li < lines.size (); li++)
      {
	float off_x = vertical ? -(step * (float) li) : 0.f;
	float off_y = vertical ?  0.f                 : -(step * (float) li);

	for (const auto &g : lines[li].glyphs)
	{
	  hb_raster_draw_reset (rdr);
	  hb_raster_draw_set_scale_factor (rdr, scalbnf (1.f, (int) subpixel_bits),
					       scalbnf (1.f, (int) subpixel_bits));
	  hb_raster_draw_set_extents (rdr, &ext);
	  hb_raster_draw_set_transform (rdr, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
	  hb_raster_draw_glyph (rdr, font, g.gid, g.x + off_x, g.y + off_y);

	  hb_raster_image_t *img = hb_raster_draw_render (rdr);
	  if (!img) { glyph_index++; continue; }

	  hb_color_t glyph_fg = foreground_for_glyph (glyph_index++);
	  uint8_t fr = hb_color_get_red (glyph_fg);
	  uint8_t fg = hb_color_get_green (glyph_fg);
	  uint8_t fb = hb_color_get_blue (glyph_fg);
	  uint8_t fa = hb_color_get_alpha (glyph_fg);

	  const uint8_t *src = hb_raster_image_get_buffer (img);
	  hb_raster_extents_t img_ext;
	  hb_raster_image_get_extents (img, &img_ext);
	  for (unsigned y = 0; y < h; y++)
	    for (unsigned x = 0; x < w; x++)
	    {
	      uint8_t cov = src[y * img_ext.stride + x];
	      if (!cov) continue;

	      uint8_t sa = (uint8_t) ((cov * fa + 127) / 255);
	      uint8_t sb = (uint8_t) ((fb * sa + 127) / 255);
	      uint8_t sg = (uint8_t) ((fg * sa + 127) / 255);
	      uint8_t sr = (uint8_t) ((fr * sa + 127) / 255);

	      uint32_t s = (uint32_t) sb |
			   ((uint32_t) sg << 8) |
			   ((uint32_t) sr << 16) |
			   ((uint32_t) sa << 24);

	      uint32_t d;
	      hb_memcpy (&d, out_buf + y * stride + x * 4, 4);
	      if (sa == 255) d = s;
	      else
	      {
		unsigned inv_sa = 255 - sa;
		uint8_t rb = (uint8_t) (((d & 0xFF) * inv_sa + 128 + (((d & 0xFF) * inv_sa + 128) >> 8)) >> 8) + sb;
		uint8_t rg = (uint8_t) ((((d >> 8) & 0xFF) * inv_sa + 128 + ((((d >> 8) & 0xFF) * inv_sa + 128) >> 8)) >> 8) + sg;
		uint8_t rr = (uint8_t) ((((d >> 16) & 0xFF) * inv_sa + 128 + ((((d >> 16) & 0xFF) * inv_sa + 128) >> 8)) >> 8) + sr;
		uint8_t ra = (uint8_t) ((((d >> 24) & 0xFF) * inv_sa + 128 + ((((d >> 24) & 0xFF) * inv_sa + 128) >> 8)) >> 8) + sa;
		d = (uint32_t) rb | ((uint32_t) rg << 8) | ((uint32_t) rr << 16) | ((uint32_t) ra << 24);
	      }
	      hb_memcpy (out_buf + y * stride + x * 4, &d, 4);
	    }

	  hb_raster_draw_recycle_image (rdr, img);
	}
      }

      if (iter + 1 == num_iterations)
      {
	if (show_extents)
	{
	  std::vector<rect_i_t> rects;
	  collect_ink_extents_rects (sx, sy, step, vertical, ext, rects);
	  overlay_rects_bgra (out_buf, w, h, stride, rects);
	}
	write_ppm (out_buf, w, h, stride);
      }
    }

    hb_raster_draw_reset (rdr);
    hb_raster_image_destroy (out_img);
  }

  void cleanup ()
  {
    hb_raster_draw_destroy (rdr);  rdr = nullptr;
    hb_raster_paint_destroy (pnt); pnt = nullptr;
    hb_font_destroy (font);        font = nullptr;
    lines.clear ();
    direction = HB_DIRECTION_INVALID;
  }

  bool compute_extents (float sx, float sy, float step, bool vertical, hb_raster_extents_t *ext)
  {
    /* Compute bounding box in pixel space from logical and/or ink extents. */
    const bool include_logical = logical || !ink;
    const bool include_ink = ink || !logical;

    float pmin_x = 1e30f, pmin_y = 1e30f;
    float pmax_x = -1e30f, pmax_y = -1e30f;
    bool have_extents = false;

    if (include_logical)
    {
      float asc = 0.f, desc = 0.f;
      if (have_font_extents)
      {
	asc  = scalbnf ((float) font_extents.ascent, (int) subpixel_bits);
	desc = -scalbnf ((float) font_extents.descent, (int) subpixel_bits);
      }
      else
      {
	hb_font_extents_t fext = {};
	hb_font_get_extents_for_direction (font, direction, &fext);
	asc = (float) fext.ascender;
	desc = (float) fext.descender;
      }

      for (unsigned li = 0; li < lines.size (); li++)
      {
	float off_x = vertical ? -(step * (float) li) : 0.f;
	float off_y = vertical ?  0.f                 : -(step * (float) li);

	float ax0 = off_x;
	float ay0 = off_y;
	float ax1 = off_x + lines[li].advance_x;
	float ay1 = off_y + lines[li].advance_y;

	float lx0, ly0, lx1, ly1;
	if (vertical)
	{
	  lx0 = off_x + desc;
	  lx1 = off_x + asc;
	  ly0 = hb_min (ay0, ay1);
	  ly1 = hb_max (ay0, ay1);
	}
	else
	{
	  lx0 = hb_min (ax0, ax1);
	  lx1 = hb_max (ax0, ax1);
	  ly0 = off_y + desc;
	  ly1 = off_y + asc;
	}

	lx0 *= sx; lx1 *= sx;
	ly0 *= sy; ly1 *= sy;

	pmin_x = hb_min (pmin_x, hb_min (lx0, lx1));
	pmin_y = hb_min (pmin_y, hb_min (ly0, ly1));
	pmax_x = hb_max (pmax_x, hb_max (lx0, lx1));
	pmax_y = hb_max (pmax_y, hb_max (ly0, ly1));
	have_extents = true;
      }
    }

    if (include_ink)
    {
    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                 : -(step * (float) li);

      for (const auto &g : lines[li].glyphs)
      {
	hb_glyph_extents_t gext;
	if (!hb_font_get_glyph_extents (font, g.gid, &gext))
	  continue;

	float gx = (g.x + off_x) * sx;
	float gy = (g.y + off_y) * sy;

	float x0 = gx + gext.x_bearing * sx;
	float y0 = gy + (gext.y_bearing + gext.height) * sy;
	float x1 = gx + (gext.x_bearing + gext.width) * sx;
	float y1 = gy + gext.y_bearing * sy;

	pmin_x = hb_min (pmin_x, hb_min (x0, x1));
	pmin_y = hb_min (pmin_y, hb_min (y0, y1));
	pmax_x = hb_max (pmax_x, hb_max (x0, x1));
	pmax_y = hb_max (pmax_y, hb_max (y0, y1));
	have_extents = true;
      }
    }
    }

    if (!have_extents || pmin_x >= pmax_x || pmin_y >= pmax_y)
      return false;

    int ix0 = (int) floorf (pmin_x) - (int) floor (margin.l);
    int iy0 = (int) floorf (pmin_y) - (int) floor (margin.b);
    int ix1 = (int) ceilf  (pmax_x) + (int) ceil  (margin.r);
    int iy1 = (int) ceilf  (pmax_y) + (int) ceil  (margin.t);
    if (ix1 <= ix0 || iy1 <= iy0)
      return false;

    ext->x_origin = ix0;
    ext->y_origin = iy0;
    ext->width = (unsigned) (ix1 - ix0);
    ext->height = (unsigned) (iy1 - iy0);
    ext->stride = 0;
    return true;
  }

  hb_color_t foreground_for_glyph (unsigned glyph_index) const
  {
    if (foreground_use_palette && foreground_palette && foreground_palette->len)
    {
      auto &c = g_array_index (foreground_palette, rgba_color_t,
			       glyph_index % foreground_palette->len);
      return HB_COLOR ((uint8_t) c.b, (uint8_t) c.g, (uint8_t) c.r, (uint8_t) c.a);
    }
    return fg_color;
  }

  void collect_ink_extents_rects (float sx, float sy, float step, bool vertical,
				  const hb_raster_extents_t &ext,
				  std::vector<rect_i_t> &rects) const
  {
    rects.clear ();
    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                 : -(step * (float) li);

      for (const auto &g : lines[li].glyphs)
      {
	hb_glyph_extents_t gext;
	if (!hb_font_get_glyph_extents (font, g.gid, &gext))
	  continue;

	float gx = (g.x + off_x) * sx;
	float gy = (g.y + off_y) * sy;
	float x0 = gx + gext.x_bearing * sx;
	float y0 = gy + (gext.y_bearing + gext.height) * sy;
	float x1 = gx + (gext.x_bearing + gext.width) * sx;
	float y1 = gy + gext.y_bearing * sy;

	int ix0 = (int) floorf (hb_min (x0, x1)) - ext.x_origin;
	int iy0 = (int) floorf (hb_min (y0, y1)) - ext.y_origin;
	int ix1 = (int) ceilf  (hb_max (x0, x1)) - ext.x_origin;
	int iy1 = (int) ceilf  (hb_max (y0, y1)) - ext.y_origin;
	rects.push_back ({ix0, iy0, ix1, iy1});
      }
    }
  }

  static void overlay_rects_bgra (uint8_t *buf, unsigned w, unsigned h, unsigned stride,
				  const std::vector<rect_i_t> &rects)
  {
    for (const auto &rc : rects)
    {
      int x0 = hb_max (0, hb_min ((int) w, rc.x0));
      int x1 = hb_max (0, hb_min ((int) w, rc.x1));
      int y0 = hb_max (0, hb_min ((int) h, rc.y0));
      int y1 = hb_max (0, hb_min ((int) h, rc.y1));
      if (x1 - x0 < 1 || y1 - y0 < 1) continue;

      auto blend_at = [&] (int x, int y)
      {
	uint32_t d;
	hb_memcpy (&d, buf + y * stride + x * 4, 4);
	uint8_t db = (uint8_t) (d & 0xFF);
	uint8_t dg = (uint8_t) ((d >> 8) & 0xFF);
	uint8_t dr = (uint8_t) ((d >> 16) & 0xFF);
	uint8_t da = (uint8_t) (d >> 24);
	uint8_t ob = (uint8_t) ((db + 255) / 2);
	uint8_t og = (uint8_t) (dg / 2);
	uint8_t orr = (uint8_t) ((dr + 255) / 2);
	uint8_t oa = hb_max (da, (uint8_t) 128);
	uint32_t o = (uint32_t) ob | ((uint32_t) og << 8) | ((uint32_t) orr << 16) | ((uint32_t) oa << 24);
	hb_memcpy (buf + y * stride + x * 4, &o, 4);
      };

      for (int x = x0; x < x1; x++)
      {
	blend_at (x, y0);
	blend_at (x, y1 - 1);
      }
      for (int y = y0; y < y1; y++)
      {
	blend_at (x0, y);
	blend_at (x1 - 1, y);
      }
    }
  }

  static void overlay_rects_rgb (std::vector<uint8_t> &buf, unsigned w, unsigned h,
				 const std::vector<rect_i_t> &rects)
  {
    for (const auto &rc : rects)
    {
      int x0 = hb_max (0, hb_min ((int) w, rc.x0));
      int x1 = hb_max (0, hb_min ((int) w, rc.x1));
      int y0 = hb_max (0, hb_min ((int) h, rc.y0));
      int y1 = hb_max (0, hb_min ((int) h, rc.y1));
      if (x1 - x0 < 1 || y1 - y0 < 1) continue;

      auto blend_at = [&] (int x, int y)
      {
	uint8_t *p = &buf[(y * w + x) * 3];
	p[0] = (uint8_t) ((p[0] + 255) / 2);
	p[1] = (uint8_t) (p[1] / 2);
	p[2] = (uint8_t) ((p[2] + 255) / 2);
      };

      for (int x = x0; x < x1; x++)
      {
	blend_at (x, y0);
	blend_at (x, y1 - 1);
      }
      for (int y = y0; y < y1; y++)
      {
	blend_at (x0, y);
	blend_at (x1 - 1, y);
      }
    }
  }

  /* Write an A8 alpha image as PPM; composited over fg/bg, Y-flipped. */
  void write_a8_as_ppm (hb_raster_image_t *img,
			float sx, float sy, float step, bool vertical,
			const hb_raster_extents_t &fallback_ext)
  {
    hb_raster_extents_t ext = fallback_ext;
    hb_raster_image_get_extents (img, &ext);
    if (!ext.width || !ext.height) return;

    uint8_t fg_r = hb_color_get_red (fg_color);
    uint8_t fg_g = hb_color_get_green (fg_color);
    uint8_t fg_b = hb_color_get_blue (fg_color);

    const uint8_t *src = hb_raster_image_get_buffer (img);
    std::vector<uint8_t> rgb (ext.width * ext.height * 3);
    for (unsigned y = 0; y < ext.height; y++)
      for (unsigned x = 0; x < ext.width; x++)
      {
	uint8_t a = src[y * ext.stride + x];
	unsigned inv_a = 255 - a;
	rgb[(y * ext.width + x) * 3 + 0] = (uint8_t) ((fg_r * a + bg_r * inv_a + 127) / 255);
	rgb[(y * ext.width + x) * 3 + 1] = (uint8_t) ((fg_g * a + bg_g * inv_a + 127) / 255);
	rgb[(y * ext.width + x) * 3 + 2] = (uint8_t) ((fg_b * a + bg_b * inv_a + 127) / 255);
      }

    if (show_extents)
    {
      std::vector<rect_i_t> rects;
      collect_ink_extents_rects (sx, sy, step, vertical, ext, rects);
      overlay_rects_rgb (rgb, ext.width, ext.height, rects);
    }

    fprintf (out_fp, "P6\n%u %u\n255\n", ext.width, ext.height);
    for (unsigned row = 0; row < ext.height; row++)
    {
      const uint8_t *row_buf = &rgb[((ext.height - 1 - row) * ext.width) * 3];
      fwrite (row_buf, 1, ext.width * 3, out_fp);
    }
  }

  /* Write a PPM file from BGRA32 buffer; Y-flipped, composited over white. */
  void write_ppm (const uint8_t *buf, unsigned w, unsigned h, unsigned stride)
  {
    if (!w || !h) return;

    fprintf (out_fp, "P6\n%u %u\n255\n", w, h);
    std::vector<uint8_t> row_buf (w * 3);
    for (unsigned row = 0; row < h; row++)
    {
      const uint8_t *src = buf + (h - 1 - row) * stride;
      for (unsigned x = 0; x < w; x++)
      {
	uint32_t px;
	hb_memcpy (&px, src + x * 4, 4);
	uint8_t b = (uint8_t) (px & 0xFF);
	uint8_t g = (uint8_t) ((px >> 8) & 0xFF);
	uint8_t r = (uint8_t) ((px >> 16) & 0xFF);
	uint8_t a = (uint8_t) (px >> 24);
	/* Composite over background color */
	unsigned inv_a = 255 - a;
	row_buf[x * 3 + 0] = (uint8_t) (r + ((bg_r * inv_a + 128 + ((bg_r * inv_a + 128) >> 8)) >> 8));
	row_buf[x * 3 + 1] = (uint8_t) (g + ((bg_g * inv_a + 128 + ((bg_g * inv_a + 128) >> 8)) >> 8));
	row_buf[x * 3 + 2] = (uint8_t) (b + ((bg_b * inv_a + 128 + ((bg_b * inv_a + 128) >> 8)) >> 8));
      }
      fwrite (row_buf.data (), 1, w * 3, out_fp);
    }
  }

  hb_color_t         fg_color  = HB_COLOR (0, 0, 0, 255);
  uint8_t            bg_r = 255, bg_g = 255, bg_b = 255, bg_a = 255;
  hb_raster_draw_t  *rdr       = nullptr;
  hb_raster_paint_t *pnt       = nullptr;
  hb_font_t         *font      = nullptr;
  unsigned           subpixel_bits = 0;
  bool               has_color     = false;
  hb_direction_t     direction     = HB_DIRECTION_INVALID;
  std::vector<line_t> lines;
};

#endif
