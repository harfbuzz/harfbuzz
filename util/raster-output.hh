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
#include "hb-raster.h"
#include "hb-ot.h"

#include <math.h>
#include <vector>


struct raster_output_t : output_options_t<true>
{
  static const bool repeat_shape = false;

  ~raster_output_t ()
  {
    hb_raster_draw_destroy (rdr);
    hb_raster_paint_destroy (pnt);
    hb_font_destroy (font);
    g_free (fore);
    g_free (back);
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Rasterize text with given font.");
    parser->set_description ("Renders shaped text as a PPM raster image.");
    refuse_tty = true;
    output_options_t::add_options (parser);

    GOptionEntry entries[] =
    {
      {"background",	0, 0, G_OPTION_ARG_STRING,	&this->back,	"Set background color (default: #FFFFFF)",	"rrggbb/rrggbbaa"},
      {"foreground",	0, 0, G_OPTION_ARG_STRING,	&this->fore,	"Set foreground color (default: #000000)",	"rrggbb/rrggbbaa"},
      {nullptr}
    };
    parser->add_group (entries,
		       "view",
		       "View options:",
		       "Options for output rendering",
		       this);
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
      const char *fg_spec = fore ? fore : "#000000";
      unsigned r, g, b, a;
      if (parse_color (fg_spec, r, g, b, a))
	fg_color = HB_COLOR ((uint8_t) b, (uint8_t) g, (uint8_t) r, (uint8_t) a);
    }
    {
      const char *bg_spec = back ? back : "#FFFFFF";
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

    hb_font_extents_t extents = {};
    hb_font_get_extents_for_direction (font, dir, &extents);
    float step = fabsf ((float) (extents.ascender - extents.descender + extents.line_gap));
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
    else
    {
      for (unsigned int iter = 0; iter < num_iterations; iter++)
      {
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
	    write_a8_as_ppm (img);
	  hb_raster_draw_recycle_image (rdr, img);
	}
      }
    }

    hb_raster_draw_destroy (rdr);  rdr      = nullptr;
    hb_raster_paint_destroy (pnt); pnt      = nullptr;
    hb_font_destroy (font);        font      = nullptr;
    lines.clear ();
    direction = HB_DIRECTION_INVALID;
  }

  private:

  struct glyph_instance_t { hb_codepoint_t gid; float x, y; };
  struct line_t
  {
    float advance_x = 0.f, advance_y = 0.f;
    std::vector<glyph_instance_t> glyphs;
  };

  void finish_paint (float sx, float sy, float step, bool vertical, unsigned int num_iterations)
  {
    /* First pass: compute bounding box in pixel space */
    float pmin_x = 1e30f, pmin_y = 1e30f;
    float pmax_x = -1e30f, pmax_y = -1e30f;

    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                 : -(step * (float) li);

      for (const auto &g : lines[li].glyphs)
      {
	hb_glyph_extents_t gext;
	if (!hb_font_get_glyph_extents (font, g.gid, &gext))
	  continue;

	/* SVG glyphs may report zero extents; fall back to advance + font extents. */
	if (!gext.width && !gext.height)
	{
	  hb_font_extents_t fext;
	  hb_font_get_extents_for_direction (font, direction, &fext);
	  hb_position_t adv_x, adv_y;
	  hb_font_get_glyph_advance_for_direction (font, g.gid, direction, &adv_x, &adv_y);
	  gext.x_bearing = 0;
	  gext.y_bearing = fext.ascender;
	  gext.width = adv_x ? adv_x : (hb_position_t) step;
	  gext.height = -(hb_position_t) step;
	}

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
      }
    }

    if (pmin_x >= pmax_x || pmin_y >= pmax_y) return;

    int ix0 = (int) floorf (pmin_x) - 1;
    int iy0 = (int) floorf (pmin_y) - 1;
    int ix1 = (int) ceilf (pmax_x) + 1;
    int iy1 = (int) ceilf (pmax_y) + 1;
    unsigned w = (unsigned) (ix1 - ix0);
    unsigned h = (unsigned) (iy1 - iy0);
    unsigned stride = w * 4;

    hb_raster_extents_t ext = {ix0, iy0, w, h, stride};
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
      for (unsigned li = 0; li < lines.size (); li++)
      {
	float off_x = vertical ? -(step * (float) li) : 0.f;
	float off_y = vertical ?  0.f                 : -(step * (float) li);

	for (const auto &g : lines[li].glyphs)
	{
	  float pen_x = g.x + off_x;
	  float pen_y = g.y + off_y;

	  hb_raster_paint_set_transform (pnt, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
	  hb_raster_paint_set_extents (pnt, &ext);

	  hb_raster_paint_glyph (pnt, font, g.gid, pen_x, pen_y, 0, fg_color);

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
	/* Write as PPM (RGB, Y-flipped). */
	write_ppm (out_buf, w, h, stride);
    }
    hb_raster_image_destroy (out_img);
  }

  /* Write an A8 alpha image as PPM; composited over fg/bg, Y-flipped. */
  void write_a8_as_ppm (hb_raster_image_t *img)
  {
    hb_raster_extents_t ext;
    hb_raster_image_get_extents (img, &ext);
    if (!ext.width || !ext.height) return;

    uint8_t fg_r = hb_color_get_red (fg_color);
    uint8_t fg_g = hb_color_get_green (fg_color);
    uint8_t fg_b = hb_color_get_blue (fg_color);

    const uint8_t *buf = hb_raster_image_get_buffer (img);
    fprintf (out_fp, "P6\n%u %u\n255\n", ext.width, ext.height);
    std::vector<uint8_t> row_buf (ext.width * 3);
    for (unsigned row = 0; row < ext.height; row++)
    {
      const uint8_t *src = buf + (ext.height - 1 - row) * ext.stride;
      for (unsigned x = 0; x < ext.width; x++)
      {
	uint8_t a = src[x];
	unsigned inv_a = 255 - a;
	row_buf[x * 3 + 0] = (uint8_t) ((fg_r * a + bg_r * inv_a + 127) / 255);
	row_buf[x * 3 + 1] = (uint8_t) ((fg_g * a + bg_g * inv_a + 127) / 255);
	row_buf[x * 3 + 2] = (uint8_t) ((fg_b * a + bg_b * inv_a + 127) / 255);
      }
      fwrite (row_buf.data (), 1, ext.width * 3, out_fp);
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

  char              *fore      = nullptr;
  char              *back      = nullptr;
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
