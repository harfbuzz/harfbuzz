/*
 * Copyright © 2010  Behdad Esfahbod
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

#include "common.hh"


#include <cairo-ft.h>
#include <hb-ft.h>

#include "options.hh"


/* Ugh, global vars.  Ugly, but does the job */
static int width = 0;
static int height = 0;
static cairo_surface_t *surface = NULL;
static cairo_pattern_t *fore_pattern = NULL;
static cairo_pattern_t *back_pattern = NULL;
static cairo_font_face_t *cairo_face;



static cairo_glyph_t *
_hb_cr_text_glyphs (cairo_t *cr,
		    const char *utf8, int len,
		    unsigned int *pnum_glyphs)
{
  cairo_scaled_font_t *scaled_font = cairo_get_scaled_font (cr);
  FT_Face ft_face = cairo_ft_scaled_font_lock_face (scaled_font);
  hb_font_t *hb_font = hb_ft_font_create (ft_face, NULL);
  hb_buffer_t *hb_buffer;
  cairo_glyph_t *cairo_glyphs;
  hb_glyph_info_t *hb_glyph;
  hb_glyph_position_t *hb_position;
  unsigned int num_glyphs, i;
  hb_position_t x, y;

  hb_buffer = hb_buffer_create ();

  if (shape_opts->direction)
    hb_buffer_set_direction (hb_buffer, hb_direction_from_string (shape_opts->direction, -1));
  if (shape_opts->script)
    hb_buffer_set_script (hb_buffer, hb_script_from_string (shape_opts->script, -1));
  if (shape_opts->language)
    hb_buffer_set_language (hb_buffer, hb_language_from_string (shape_opts->language, -1));

  if (len < 0)
    len = strlen (utf8);
  hb_buffer_add_utf8 (hb_buffer, utf8, len, 0, len);

  if (!hb_shape_full (hb_font, hb_buffer, shape_opts->features, shape_opts->num_features, NULL, shape_opts->shapers))
    fail ("All shapers failed");

  num_glyphs = hb_buffer_get_length (hb_buffer);
  hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);
  cairo_glyphs = cairo_glyph_allocate (num_glyphs);
  x = 0;
  y = 0;
  for (i = 0; i < num_glyphs; i++)
    {
      cairo_glyphs[i].index = hb_glyph->codepoint;
      cairo_glyphs[i].x = ( hb_position->x_offset + x) * (1./64);
      cairo_glyphs[i].y = (-hb_position->y_offset + y) * (1./64);
      x +=  hb_position->x_advance;
      y += -hb_position->y_advance;

      hb_glyph++;
      hb_position++;
    }
  hb_buffer_destroy (hb_buffer);
  hb_font_destroy (hb_font);
  cairo_ft_scaled_font_unlock_face (scaled_font);

  if (pnum_glyphs)
    *pnum_glyphs = num_glyphs;
  return cairo_glyphs;
}

static cairo_t *
create_context (void)
{
  cairo_t *cr;
  unsigned int fr, fg, fb, fa, br, bg, bb, ba;

  if (surface)
    cairo_surface_destroy (surface);
  if (back_pattern)
    cairo_pattern_destroy (back_pattern);
  if (fore_pattern)
    cairo_pattern_destroy (fore_pattern);

  br = bg = bb = ba = 255;
  sscanf (view_opts->back + (*view_opts->back=='#'), "%2x%2x%2x%2x", &br, &bg, &bb, &ba);
  fr = fg = fb = 0; fa = 255;
  sscanf (view_opts->fore + (*view_opts->fore=='#'), "%2x%2x%2x%2x", &fr, &fg, &fb, &fa);

  if (!view_opts->annotate && ba == 255 && fa == 255 && br == bg && bg == bb && fr == fg && fg == fb) {
    /* grayscale.  use A8 surface */
    surface = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    cr = cairo_create (surface);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba (cr, 1., 1., 1., br / 255.);
    cairo_paint (cr);
    back_pattern = cairo_pattern_reference (cairo_get_source (cr));
    cairo_set_source_rgba (cr, 1., 1., 1., fr / 255.);
    fore_pattern = cairo_pattern_reference (cairo_get_source (cr));
  } else {
    /* color.  use (A)RGB surface */
    if (ba != 255)
      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    else
      surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
    cr = cairo_create (surface);
    cairo_set_source_rgba (cr, br / 255., bg / 255., bb / 255., ba / 255.);
    cairo_paint (cr);
    back_pattern = cairo_pattern_reference (cairo_get_source (cr));
    cairo_set_source_rgba (cr, fr / 255., fg / 255., fb / 255., fa / 255.);
    fore_pattern = cairo_pattern_reference (cairo_get_source (cr));
  }

  cairo_set_font_face (cr, cairo_face);

  return cr;
}

static void
draw (void)
{
  cairo_t *cr;
  cairo_font_extents_t font_extents;

  cairo_glyph_t *glyphs = NULL;
  unsigned int num_glyphs = 0;

  const char *end, *p = shape_opts->text;
  double x, y;

  cr= create_context ();

  cairo_set_font_size (cr, font_opts->font_size);
  cairo_font_extents (cr, &font_extents);

  height = 0;
  width = 0;

  x = view_opts->margin.l;
  y = view_opts->margin.t;

  do {
    cairo_text_extents_t extents;

    end = strchr (p, '\n');
    if (!end)
      end = p + strlen (p);

    if (p != shape_opts->text)
	y += view_opts->line_space;

    if (p != end) {
      glyphs = _hb_cr_text_glyphs (cr, p, end - p, &num_glyphs);

      cairo_glyph_extents (cr, glyphs, num_glyphs, &extents);

      y += ceil (font_extents.ascent);
      width = MAX (width, extents.x_advance);
      cairo_save (cr);
      cairo_translate (cr, x, y);
      if (view_opts->annotate) {
        unsigned int i;
        cairo_save (cr);

	/* Draw actual glyph origins */
	cairo_set_source_rgba (cr, 1., 0., 0., .5);
	cairo_set_line_width (cr, 5);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	for (i = 0; i < num_glyphs; i++) {
	  cairo_move_to (cr, glyphs[i].x, glyphs[i].y);
	  cairo_rel_line_to (cr, 0, 0);
	}
	cairo_stroke (cr);

        cairo_restore (cr);
      }
      cairo_show_glyphs (cr, glyphs, num_glyphs);
      cairo_restore (cr);
      y += ceil (font_extents.height - ceil (font_extents.ascent));

      cairo_glyph_free (glyphs);
    }

    p = end + 1;
  } while (*end);

  height = y + view_opts->margin.b;
  width += view_opts->margin.l + view_opts->margin.r;

  cairo_destroy (cr);
}



int
main (int argc, char **argv)
{
  static FT_Library ft_library;
  static FT_Face ft_face;
  cairo_status_t status;

  setlocale (LC_ALL, "");

  parse_options (argc, argv);

  FT_Init_FreeType (&ft_library);
  if (FT_New_Face (ft_library, font_opts->font_file, font_opts->face_index, &ft_face)) {
    fprintf (stderr, "Failed to open font file `%s'\n", font_opts->font_file);
    exit (1);
  }
  cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);

  draw ();
  draw ();

  status = cairo_surface_write_to_png (surface, out_file);
  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf (stderr, "Failed to write output file `%s': %s\n",
	     out_file, cairo_status_to_string (status));
    exit (1);
  }

  if (debug) {
    cairo_pattern_destroy (fore_pattern);
    cairo_pattern_destroy (back_pattern);
    cairo_surface_destroy (surface);
    cairo_font_face_destroy (cairo_face);
    cairo_debug_reset_static_data ();

    FT_Done_Face (ft_face);
    FT_Done_FreeType (ft_library);
  }

  return 0;
}
