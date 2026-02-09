/*
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef VIEW_CAIRO_HH
#define VIEW_CAIRO_HH

#include "view-options.hh"
#include "output-options.hh"
#include "helper-cairo.hh"

struct view_cairo_t : view_options_t, output_options_t<>
{
  ~view_cairo_t ()
  {
    cairo_debug_reset_static_data ();
  }

  void add_options (option_parser_t *parser)
  {
    parser->set_summary ("View text with given font.");
    parser->set_description ("Shows image of rendering text with a given font in various formats.");

    view_options_t::add_options (parser);
    output_options_t::add_options (parser, helper_cairo_supported_formats);
  }

  void init (hb_buffer_t *buffer, const font_options_t *font_opts)
  {
    lines = g_array_new (false, false, sizeof (helper_cairo_line_t));
    subpixel_bits = font_opts->subpixel_bits;
    setup_foreground ();
  }
  void new_line () {}
  void consume_text (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len,
		     hb_bool_t     utf8_clusters) {}
  void error (const char *message)
  { g_printerr ("%s: %s\n", g_get_prgname (), message); }
  void consume_glyphs (hb_buffer_t  *buffer,
		       const char   *text,
		       unsigned int  text_len,
		       hb_bool_t     utf8_clusters)
  {
    direction = hb_buffer_get_direction (buffer);
    helper_cairo_line_t l (text, text_len, buffer, utf8_clusters, subpixel_bits);
    g_array_append_val (lines, l);
  }
  void finish (hb_buffer_t *buffer, const font_options_t *font_opts)
  {
    render (font_opts);

    for (unsigned int i = 0; i < lines->len; i++) {
      helper_cairo_line_t &line = g_array_index (lines, helper_cairo_line_t, i);
      line.finish ();
    }
    g_array_unref (lines);
  }

  protected:

  void render (const font_options_t *font_opts);
  void draw_lines (cairo_t *cr, hb_font_t *font, double leading, int vert, int horiz);

  hb_direction_t direction = HB_DIRECTION_INVALID; // Remove this, make segment_properties accessible
  GArray *lines = nullptr;
  unsigned subpixel_bits = 0;
};

inline void
view_cairo_t::draw_lines (cairo_t *cr, hb_font_t *font, double leading, int vert, int horiz)
{
  const bool use_foreground_palette =
    foreground_use_palette && foreground_palette && foreground_palette->len;

  cairo_translate (cr, +vert * leading, -horiz * leading);
  for (unsigned int i = 0; i < lines->len; i++)
  {
    helper_cairo_line_t &l = g_array_index (lines, helper_cairo_line_t, i);

    cairo_translate (cr, -vert * leading, +horiz * leading);

    if (show_extents)
    {
      cairo_save (cr);

      cairo_set_source_rgba (cr, 1., 0., 0., .5);
      cairo_set_line_width (cr, 10);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      for (unsigned i = 0; i < l.num_glyphs; i++) {
	cairo_move_to (cr, l.glyphs[i].x, l.glyphs[i].y);
	cairo_rel_line_to (cr, 0, 0);
      }
      cairo_stroke (cr);

      cairo_restore (cr);
      cairo_save (cr);

      cairo_set_source_rgba (cr, 1., 0., 1., .5);
      cairo_set_line_width (cr, 3);
      for (unsigned i = 0; i < l.num_glyphs; i++)
      {
	hb_glyph_extents_t hb_extents;
	if (hb_font_get_glyph_extents (font, l.glyphs[i].index, &hb_extents))
	{
	  double x1 = scalbn ((double) hb_extents.x_bearing, - (int) subpixel_bits);
	  double y1 = -scalbn ((double) hb_extents.y_bearing, - (int) subpixel_bits);
	  double width = scalbn ((double) hb_extents.width, - (int) subpixel_bits);
	  double height = -scalbn ((double) hb_extents.height, - (int) subpixel_bits);

	  cairo_rectangle (cr, l.glyphs[i].x + x1, l.glyphs[i].y + y1, width, height);
	}
      }
      cairo_stroke (cr);

      cairo_restore (cr);
    }

    if (use_foreground_palette)
    {
      for (unsigned j = 0; j < l.num_glyphs; j++)
      {
	auto &color = g_array_index (foreground_palette, rgba_color_t,
				      j % foreground_palette->len);
	cairo_set_source_rgba (cr,
			       color.r / 255.,
			       color.g / 255.,
			       color.b / 255.,
			       color.a / 255.);
	cairo_show_glyphs (cr, l.glyphs + j, 1);
      }
    }
    else
    {
      // https://github.com/harfbuzz/harfbuzz/issues/4378
#if CAIRO_VERSION >= 11705
      if (l.num_clusters)
	cairo_show_text_glyphs (cr,
				l.utf8, l.utf8_len,
				l.glyphs, l.num_glyphs,
				l.clusters, l.num_clusters,
				l.cluster_flags);
      else
#endif
	cairo_show_glyphs (cr, l.glyphs, l.num_glyphs);
    }
  }
}

inline void
view_cairo_t::render (const font_options_t *font_opts)
{
  bool vertical = HB_DIRECTION_IS_VERTICAL (direction);
  int vert  = vertical ? 1 : 0;
  int horiz = vertical ? 0 : 1;

  int x_sign = font_opts->font_size_x < 0 ? -1 : +1;
  int y_sign = font_opts->font_size_y < 0 ? -1 : +1;

  hb_font_t *font = font_opts->font;

  if (!have_font_extents)
  {
    hb_font_extents_t hb_extents;
    hb_font_get_extents_for_direction (font, direction, &hb_extents);
    font_extents.ascent = scalbn ((double) hb_extents.ascender, - (int) subpixel_bits);
    font_extents.descent = -scalbn ((double) hb_extents.descender, - (int) subpixel_bits);
    font_extents.line_gap = scalbn ((double) hb_extents.line_gap, - (int) subpixel_bits);
    have_font_extents = true;
  }

  double ascent = y_sign * font_extents.ascent;
  double descent = y_sign * font_extents.descent;
  double line_gap = y_sign * font_extents.line_gap + line_space;
  double leading = ascent + descent + line_gap;

  /* Calculate logical surface size. */
  double logical_w = 0, logical_h = 0;
  (vertical ? logical_w : logical_h) = (int) lines->len * leading - (font_extents.line_gap + line_space);
  (vertical ? logical_h : logical_w) = 0;
  for (unsigned int i = 0; i < lines->len; i++) {
    helper_cairo_line_t &line = g_array_index (lines, helper_cairo_line_t, i);
    double x_advance, y_advance;
    line.get_advance (&x_advance, &y_advance);
    if (vertical)
      logical_h =  MAX (logical_h, y_sign * y_advance);
    else
      logical_w =  MAX (logical_w, x_sign * x_advance);
  }

  cairo_scaled_font_t *scaled_font = helper_cairo_create_scaled_font (font_opts,
								      this);

  /* See if font needs color. */
  cairo_content_t content = CAIRO_CONTENT_ALPHA;
  if (helper_cairo_scaled_font_has_color (scaled_font))
    content = CAIRO_CONTENT_COLOR;

  double surface_w = logical_w, surface_h = logical_h;
  double surface_shift_x = 0, surface_shift_y = 0;
  bool include_logical = logical || !ink;
  bool include_ink = ink || !logical;
  if (include_ink)
  {
    cairo_surface_t *ink_surface = cairo_recording_surface_create (content, nullptr);
    cairo_t *ink_cr = cairo_create (ink_surface);
    cairo_set_scaled_font (ink_cr, scaled_font);

    if (vertical)
      cairo_translate (ink_cr,
		       logical_w - ascent, /* We currently always stack lines right to left */
		       y_sign < 0 ? logical_h : 0);
    else
     {
      cairo_translate (ink_cr,
		       x_sign < 0 ? logical_w : 0,
		       y_sign < 0 ? descent : ascent);
     }

    draw_lines (ink_cr, font, leading, vert, horiz);

    double ink_x, ink_y, ink_w, ink_h;
    cairo_recording_surface_ink_extents (ink_surface, &ink_x, &ink_y, &ink_w, &ink_h);
    if (ink_w > 0 && ink_h > 0)
    {
      if (include_logical)
      {
	double x1 = MIN (0., ink_x);
	double y1 = MIN (0., ink_y);
	double x2 = MAX (logical_w, ink_x + ink_w);
	double y2 = MAX (logical_h, ink_y + ink_h);
	surface_w = x2 - x1;
	surface_h = y2 - y1;
	surface_shift_x = -x1;
	surface_shift_y = -y1;
      }
      else
      {
	surface_w = ink_w;
	surface_h = ink_h;
	surface_shift_x = -ink_x;
	surface_shift_y = -ink_y;
      }
    }

    cairo_destroy (ink_cr);
    cairo_surface_destroy (ink_surface);
  }

  /* Create surface. */
  cairo_t *cr = helper_cairo_create_context (surface_w + margin.l + margin.r,
					     surface_h + margin.t + margin.b,
					     this,
					     this,
					     content);
  cairo_set_scaled_font (cr, scaled_font);

  /* Setup coordinate system. */
  cairo_translate (cr, margin.l + surface_shift_x, margin.t + surface_shift_y);
  if (vertical)
    cairo_translate (cr,
		     logical_w - ascent, /* We currently always stack lines right to left */
		     y_sign < 0 ? logical_h : 0);
  else
   {
    cairo_translate (cr,
		     x_sign < 0 ? logical_w : 0,
		     y_sign < 0 ? descent : ascent);
   }

  /* Draw. */
  draw_lines (cr, font, leading, vert, horiz);

  /* Clean up. */
  helper_cairo_destroy_context (cr);
  cairo_scaled_font_destroy (scaled_font);
}

#endif
