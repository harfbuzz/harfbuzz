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

#include "options.hh"

#include <cairo-ft.h>
#include <hb-ft.h>

#ifndef VIEW_CAIRO_HH
#define VIEW_CAIRO_HH

struct view_cairo_t : output_options_t, view_options_t {
  view_cairo_t (option_parser_t *parser)
	       : output_options_t (parser),
	         view_options_t (parser) {}
  ~view_cairo_t (void) {
    if (debug)
      cairo_debug_reset_static_data ();
  }

  void init (const font_options_t *font_opts);
  void consume_line (hb_buffer_t  *buffer,
		     const char   *text,
		     unsigned int  text_len);
  void finish (const font_options_t *font_opts);

  protected:

  void render (const font_options_t *font_opts);
  cairo_scaled_font_t *create_scaled_font (const font_options_t *font_opts);
  void get_surface_size (cairo_scaled_font_t *scaled_font, double *w, double *h);
  cairo_t *create_context (double w, double h);
  void draw (cairo_t *cr);
  double line_width (unsigned int i);

  GArray *lines;
  unsigned int upem;
};

#endif
