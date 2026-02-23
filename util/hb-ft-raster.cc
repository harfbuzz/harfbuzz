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

/* hb-ft-raster: like hb-raster but uses FreeType's rasterizer for
   pixel-level comparison.  Same pipeline (shaping, glyph positioning),
   but calls FT_Outline_Render instead of hb_raster_draw_render. */

#include "batch.hh"
#include "font-options.hh"
#include "main-font-text.hh"
#include "output-options.hh"
#include "shape-consumer.hh"
#include "text-options.hh"

#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_BBOX_H
#include FT_MULTIPLE_MASTERS_H

#include <math.h>
#include <vector>


const unsigned DEFAULT_FONT_SIZE = FONT_SIZE_UPEM;
const unsigned SUBPIXEL_BITS = 6;

struct ft_raster_output_t : output_options_t<true>
{
  ~ft_raster_output_t ()
  {
    hb_font_destroy (font);
    if (ft_face) FT_Done_Face (ft_face);
    if (ft_lib)  FT_Done_FreeType (ft_lib);
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("Rasterize text with FreeType rasterizer.");
    parser->set_description ("Renders shaped text as a PPM raster image using FreeType.");
    output_options_t::add_options (parser);
  }

  template <typename app_t>
  void init (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts)
  {
    lines.clear ();
    direction = HB_DIRECTION_INVALID;

    font = hb_font_reference (font_opts->font);
    subpixel_bits = font_opts->subpixel_bits;

    /* Set up FreeType */
    FT_Init_FreeType (&ft_lib);

    /* Get the font file blob from hb_face */
    hb_face_t *face = hb_font_get_face (font);
    hb_blob_t *blob = hb_face_reference_blob (face);
    unsigned blob_len;
    const char *blob_data = hb_blob_get_data (blob, &blob_len);

    FT_New_Memory_Face (ft_lib, (const FT_Byte *) blob_data, blob_len,
			hb_face_get_index (face), &ft_face);
    hb_blob_destroy (blob);

    int x_scale = 0, y_scale = 0;
    hb_font_get_scale (font, &x_scale, &y_scale);
    unsigned ft_x_scale = x_scale ? (unsigned) fabsf ((float) x_scale) : 1;
    unsigned ft_y_scale = y_scale ? (unsigned) fabsf ((float) y_scale) : 1;

    /* Match FT outlines to the shaping font scale. */
    FT_Set_Char_Size (ft_face,
		      (FT_F26Dot6) ft_x_scale * 64,
		      (FT_F26Dot6) ft_y_scale * 64,
		      72, 72);

#ifndef HB_NO_VAR
    unsigned coords_len;
    const float *coords = hb_font_get_var_coords_design (font, &coords_len);
    if (coords_len)
    {
      FT_Fixed *ft_coords = (FT_Fixed *) malloc (coords_len * sizeof (FT_Fixed));
      for (unsigned i = 0; i < coords_len; i++)
	ft_coords[i] = (FT_Fixed) (coords[i] * 65536.f);
      FT_Set_Var_Design_Coordinates (ft_face, coords_len, ft_coords);
      free (ft_coords);
    }
#endif
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
  void finish (hb_buffer_t *buffer HB_UNUSED, const app_t *font_opts HB_UNUSED)
  {
    /* pixels per font unit */
    float sx = scalbnf (1.f, -(int) subpixel_bits);
    float sy = scalbnf (1.f, -(int) subpixel_bits);

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

    /* First pass: compute bounding box of all glyphs in pixel space */
    float img_xmin = 1e30f, img_ymin = 1e30f;
    float img_xmax = -1e30f, img_ymax = -1e30f;

    struct glyph_render_t {
      hb_codepoint_t gid;
      float pen_x, pen_y;
    };
    std::vector<glyph_render_t> renders;

    for (unsigned li = 0; li < lines.size (); li++)
    {
      float off_x = vertical ? -(step * (float) li) : 0.f;
      float off_y = vertical ?  0.f                 : -(step * (float) li);

      for (const auto &g : lines[li].glyphs)
      {
	float pen_x = (g.x + off_x) * sx;
	float pen_y = (g.y + off_y) * sy;

	FT_Load_Glyph (ft_face, g.gid, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
	FT_Outline *outline = &ft_face->glyph->outline;

	FT_BBox bbox;
	FT_Outline_Get_BBox (outline, &bbox);

	/* bbox is in 26.6 font-space; transform to pixel space */
	float gx0 = pen_x + (float) bbox.xMin * sx / 64.f;
	float gy0 = pen_y + (float) bbox.yMin * sy / 64.f;
	float gx1 = pen_x + (float) bbox.xMax * sx / 64.f;
	float gy1 = pen_y + (float) bbox.yMax * sy / 64.f;

	if (gx0 < img_xmin) img_xmin = gx0;
	if (gy0 < img_ymin) img_ymin = gy0;
	if (gx1 > img_xmax) img_xmax = gx1;
	if (gy1 > img_ymax) img_ymax = gy1;

	renders.push_back ({g.gid, pen_x, pen_y});
      }
    }

    if (renders.empty () || img_xmin >= img_xmax || img_ymin >= img_ymax)
    {
      cleanup ();
      return;
    }

    int x0 = (int) floorf (img_xmin);
    int y0 = (int) floorf (img_ymin);
    int x1 = (int) ceilf (img_xmax);
    int y1 = (int) ceilf (img_ymax);
    unsigned width  = (unsigned) (x1 - x0);
    unsigned height = (unsigned) (y1 - y0);
    unsigned stride = (width + 3u) & ~3u;

    std::vector<uint8_t> pixels (stride * height, 0);

    /* Second pass: rasterize each glyph with FreeType */
    for (const auto &r : renders)
    {
      FT_Load_Glyph (ft_face, r.gid, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
      FT_Outline *outline = &ft_face->glyph->outline;

      /* We need to transform the outline from font-space 26.6 to pixel space,
	 then translate so that pixel (x0, y0) maps to bitmap (0, 0). */
      FT_Matrix matrix;
      matrix.xx = (FT_Fixed) (sx * 65536.f);
      matrix.xy = 0;
      matrix.yx = 0;
      matrix.yy = (FT_Fixed) (sy * 65536.f);

      /* pen position in 26.6 pixel space */
      FT_Vector delta;
      delta.x = (FT_Pos) roundf ((r.pen_x - x0) * 64.f);
      delta.y = (FT_Pos) roundf ((r.pen_y - y0) * 64.f);

      FT_Outline_Transform (outline, &matrix);
      FT_Outline_Translate (outline, delta.x, delta.y);

      FT_Bitmap bitmap;
      FT_Bitmap_Init (&bitmap);
      bitmap.rows       = height;
      bitmap.width      = width;
      bitmap.pitch      = (int) stride;
      bitmap.buffer     = pixels.data ();
      bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
      bitmap.num_grays  = 256;

      FT_Raster_Params params = {};
      params.target = &bitmap;
      params.flags  = FT_RASTER_FLAG_AA;
      FT_Outline_Render (ft_lib, outline, &params);
    }

    /* Write PPM (black on white), y-flipped so text reads correctly. */
    fprintf (out_fp, "P6\n%u %u\n255\n", width, height);
    std::vector<uint8_t> rgb_row (width * 3);
    for (unsigned row = 0; row < height; row++)
    {
      const uint8_t *src = pixels.data () + row * stride;
      for (unsigned x = 0; x < width; x++)
	rgb_row[x * 3 + 0] = rgb_row[x * 3 + 1] = rgb_row[x * 3 + 2] = (uint8_t) (255 - src[x]);
      fwrite (rgb_row.data (), 1, width * 3, out_fp);
    }

    cleanup ();
  }

  private:

  void cleanup ()
  {
    if (ft_face) { FT_Done_Face (ft_face); ft_face = nullptr; }
    if (ft_lib)  { FT_Done_FreeType (ft_lib); ft_lib = nullptr; }
    hb_font_destroy (font);        font      = nullptr;
    lines.clear ();
    direction = HB_DIRECTION_INVALID;
  }

  struct glyph_instance_t { hb_codepoint_t gid; float x, y; };
  struct line_t
  {
    float advance_x = 0.f, advance_y = 0.f;
    std::vector<glyph_instance_t> glyphs;
  };

  hb_font_t        *font      = nullptr;
  unsigned          subpixel_bits = 0;
  hb_direction_t    direction     = HB_DIRECTION_INVALID;
  std::vector<line_t> lines;
  FT_Library ft_lib  = nullptr;
  FT_Face    ft_face = nullptr;
};


int
main (int argc, char **argv)
{
  using main_t = main_font_text_t<shape_consumer_t<ft_raster_output_t>, font_options_t, shape_text_options_t>;
  return batch_main<main_t, true> (argc, argv);
}
