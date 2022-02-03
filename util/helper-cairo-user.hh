/*
 * Copyright © 2022  Google, Inc.
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

#ifndef HELPER_CAIRO_USER_HH
#define HELPER_CAIRO_USER_HH

#include "font-options.hh"

#include <cairo.h>
#include <hb.h>

static void
move_to (hb_draw_funcs_t *dfuncs,
	 cairo_t *cr,
	 hb_draw_state_t *st,
	 float to_x, float to_y,
	 void *)
{
  cairo_move_to (cr, to_x, to_y);
}

static void
line_to (hb_draw_funcs_t *dfuncs,
	 cairo_t *cr,
	 hb_draw_state_t *st,
	 float to_x, float to_y,
	 void *)
{
  cairo_line_to (cr, to_x, to_y);
}

static void
cubic_to (hb_draw_funcs_t *dfuncs,
	  cairo_t *cr,
	  hb_draw_state_t *st,
	  float control1_x, float control1_y,
	  float control2_x, float control2_y,
	  float to_x, float to_y,
	  void *)
{
  cairo_curve_to (cr, control1_x, control1_y, control2_x, control2_y, to_x, to_y);
}

static void
close_path (hb_draw_funcs_t *dfuncs,
	    cairo_t *cr,
	    hb_draw_state_t *st,
	    void *)
{
  cairo_close_path (cr);
}


static hb_draw_funcs_t *
get_cairo_draw_funcs ()
{
  static hb_draw_funcs_t *funcs;

  if (!funcs)
  {
    funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) line_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) close_path, nullptr, nullptr);
  }

  return funcs;
}

static const cairo_user_data_key_t _hb_font_cairo_user_data_key = {0};

static cairo_status_t
render_glyph (cairo_scaled_font_t  *scaled_font,
	      unsigned long         glyph,
	      cairo_t              *cr,
	      cairo_text_extents_t *extents)
{
  hb_font_t *font = (hb_font_t *) (cairo_font_face_get_user_data (cairo_scaled_font_get_font_face (scaled_font),
								  &_hb_font_cairo_user_data_key));

  hb_position_t x_scale, y_scale;
  hb_font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, +1./x_scale, -1./y_scale);

  hb_font_get_glyph_shape (font, glyph, get_cairo_draw_funcs (), cr);
  cairo_fill (cr);

  return CAIRO_STATUS_SUCCESS;
}

static inline cairo_font_face_t *
helper_cairo_create_user_font_face (const font_options_t *font_opts)
{
  cairo_font_face_t *cairo_face = cairo_user_font_face_create ();
  cairo_user_font_face_set_render_glyph_func (cairo_face, render_glyph);
  cairo_font_face_set_user_data (cairo_face,
				 &_hb_font_cairo_user_data_key,
				 hb_font_reference (font_opts->font),
				 (cairo_destroy_func_t) hb_font_destroy);
  return cairo_face;
}

static inline bool
helper_cairo_user_font_face_has_data (cairo_font_face_t *font_face)
{
  return cairo_font_face_get_user_data (font_face, &_hb_font_cairo_user_data_key);
}

static inline bool
helper_cairo_user_scaled_font_has_color (cairo_scaled_font_t *scaled_font)
{
  /* Ignoring SVG for now, since we cannot render it. */
  hb_font_t *font = (hb_font_t *) (cairo_font_face_get_user_data (cairo_scaled_font_get_font_face (scaled_font),
								  &_hb_font_cairo_user_data_key));
  return hb_ot_color_has_png (hb_font_get_face (font));
}

#endif
