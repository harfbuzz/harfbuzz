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

#include "output-options.hh"
#include "hb-raster.h"

#include <math.h>
#include <vector>


struct raster_output_t : output_options_t<true>
{
  ~raster_output_t ()
  {
    hb_raster_draw_destroy (rdr);
    hb_font_destroy (upem_font);
    hb_font_destroy (font);
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Rasterize text with given font.");
    parser->set_description ("Renders shaped text as a PGM raster image.");
    output_options_t::add_options (parser);
  }

  template <typename app_t>
  void init (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts)
  {
    lines.clear ();
    direction = HB_DIRECTION_INVALID;

    font = hb_font_reference (font_opts->font);
    upem = hb_face_get_upem (hb_font_get_face (font));
    hb_font_get_scale (font, &x_scale, &y_scale);
    subpixel_bits = font_opts->subpixel_bits;

    upem_font = hb_font_create (hb_font_get_face (font));
    hb_font_set_scale (upem_font, (int) upem, (int) upem);
#ifndef HB_NO_VAR
    unsigned coords_len;
    const float *coords = hb_font_get_var_coords_design (font, &coords_len);
    if (coords_len)
      hb_font_set_var_coords_design (upem_font, coords, coords_len);
#endif

    rdr = hb_raster_draw_create ();
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
	to_upem_x (pen_x + positions[i].x_offset),
	to_upem_y (pen_y + positions[i].y_offset),
      });
      pen_x += positions[i].x_advance;
      pen_y += positions[i].y_advance;
    }
    line.advance_x = to_upem_x (pen_x);
    line.advance_y = to_upem_y (pen_y);
  }

  template <typename app_t>
  void finish (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts HB_UNUSED)
  {
    /* pixels per upem unit */
    float sx = scalbnf ((float) x_scale, -(int) subpixel_bits) / upem;
    float sy = scalbnf ((float) y_scale, -(int) subpixel_bits) / upem;

    /* line step in upem units */
    hb_direction_t dir = direction;
    if (dir == HB_DIRECTION_INVALID) dir = HB_DIRECTION_LTR;
    bool vertical = HB_DIRECTION_IS_VERTICAL (dir);

    hb_font_extents_t extents = {};
    hb_font_get_extents_for_direction (font, dir, &extents);
    int axis_scale = vertical ? x_scale : y_scale;
    if (!axis_scale) axis_scale = (int) upem;
    float step = fabsf ((float) (extents.ascender - extents.descender + extents.line_gap)
			* upem / axis_scale);
    if (!(step > 0.f)) step = (float) upem;

    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                 : -(step * (float) li);

      for (const auto &g : lines[li].glyphs)
      {
	float pen_x = (g.x + off_x) * sx;
	float pen_y = (g.y + off_y) * sy;
	hb_raster_draw_set_transform (rdr, sx, 0.f, 0.f, sy, pen_x, pen_y);
	hb_font_draw_glyph (upem_font, g.gid, hb_raster_draw_get_funcs (), rdr);
      }
    }

    hb_raster_image_t *img = hb_raster_draw_render (rdr);
    if (img)
    {
      write_pgm (img);
      hb_raster_image_destroy (img);
    }

    hb_raster_draw_destroy (rdr);  rdr      = nullptr;
    hb_font_destroy (upem_font);   upem_font = nullptr;
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

  float to_upem_x (hb_position_t v) const
  { return x_scale ? (float) v * upem / x_scale : (float) v; }
  float to_upem_y (hb_position_t v) const
  { return y_scale ? (float) v * upem / y_scale : (float) v; }

  /* Write a PGM file; Y-flipped so text reads correctly in viewers. */
  void write_pgm (hb_raster_image_t *img)
  {
    hb_raster_extents_t ext;
    hb_raster_image_get_extents (img, &ext);
    if (!ext.width || !ext.height) return;

    const uint8_t *buf = hb_raster_image_get_buffer (img);
    fprintf (out_fp, "P5\n%u %u\n255\n", ext.width, ext.height);
    for (unsigned row = 0; row < ext.height; row++)
      fwrite (buf + (ext.height - 1 - row) * ext.stride, 1, ext.width, out_fp);
  }

  hb_raster_draw_t *rdr    = nullptr;
  hb_font_t        *font      = nullptr;
  hb_font_t        *upem_font = nullptr;
  unsigned          upem          = 0;
  int               x_scale       = 0;
  int               y_scale       = 0;
  unsigned          subpixel_bits = 0;
  hb_direction_t    direction     = HB_DIRECTION_INVALID;
  std::vector<line_t> lines;
};

#endif
