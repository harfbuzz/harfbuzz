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

#include "view-cairo.hh"


#ifdef CAIRO_HAS_SVG_SURFACE
#  include <cairo-svg.h>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#  include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#  include <cairo-ps.h>
#  if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1,6,0)
#    define HAS_EPS 1

static cairo_surface_t *
_cairo_eps_surface_create_for_stream (cairo_write_func_t  write_func,
				      void               *closure,
				      double              width,
				      double              height)
{
  cairo_surface_t *surface;

  surface = cairo_ps_surface_create_for_stream (write_func, closure, width, height);
  cairo_ps_surface_set_eps (surface, TRUE);

  return surface;
}

#  else
#    undef HAS_EPS
#  endif
#endif

struct line_t {
  cairo_glyph_t *glyphs;
  unsigned int num_glyphs;
  char *utf8;
  unsigned int utf8_len;
  cairo_text_cluster_t *clusters;
  unsigned int num_clusters;
  cairo_text_cluster_flags_t cluster_flags;

  void finish (void) {
    if (glyphs)
      cairo_glyph_free (glyphs);
    if (clusters)
      cairo_text_cluster_free (clusters);
    if (utf8)
      g_free (utf8);
  }
};

void
view_cairo_t::init (const font_options_t *font_opts)
{
  lines = g_array_new (FALSE, FALSE, sizeof (line_t));
  scale = double (font_opts->font_size) / hb_face_get_upem (hb_font_get_face (font_opts->get_font ()));
}

void
view_cairo_t::consume_line (hb_buffer_t  *buffer,
			    const char   *text,
			    unsigned int  text_len)
{
  line_t l = {0};

  l.num_glyphs = hb_buffer_get_length (buffer);
  hb_glyph_info_t *hb_glyph = hb_buffer_get_glyph_infos (buffer, NULL);
  hb_glyph_position_t *hb_position = hb_buffer_get_glyph_positions (buffer, NULL);
  l.glyphs = cairo_glyph_allocate (l.num_glyphs + 1);
  l.utf8 = g_strndup (text, text_len);
  l.utf8_len = text_len;
  l.num_clusters = l.num_glyphs ? 1 : 0;
  for (unsigned int i = 1; i < l.num_glyphs; i++)
    if (hb_glyph[i].cluster != hb_glyph[i-1].cluster)
      l.num_clusters++;
  l.clusters = cairo_text_cluster_allocate (l.num_clusters);

  if ((l.num_glyphs && !l.glyphs) ||
      (l.utf8_len && !l.utf8) ||
      (l.num_clusters && !l.clusters))
  {
    l.finish ();
    return;
  }

  hb_position_t x = 0, y = 0;
  int i;
  for (i = 0; i < (int) l.num_glyphs; i++)
    {
      l.glyphs[i].index = hb_glyph[i].codepoint;
      l.glyphs[i].x = ( hb_position->x_offset + x) * scale;
      l.glyphs[i].y = (-hb_position->y_offset + y) * scale;
      x +=  hb_position->x_advance;
      y += -hb_position->y_advance;

      hb_position++;
    }
  l.glyphs[i].index = 0;
  l.glyphs[i].x = x;
  l.glyphs[i].y = y;

  memset ((void *) l.clusters, 0, l.num_clusters * sizeof (l.clusters[0]));
  bool backward = HB_DIRECTION_IS_BACKWARD (hb_buffer_get_direction (buffer));
  l.cluster_flags = backward ? CAIRO_TEXT_CLUSTER_FLAG_BACKWARD : (cairo_text_cluster_flags_t) 0;
  unsigned int cluster = 0;
  if (!l.num_glyphs)
    goto done;
  l.clusters[cluster].num_glyphs++;
  if (backward) {
    for (i = l.num_glyphs - 2; i >= 0; i--) {
      if (hb_glyph[i].cluster != hb_glyph[i+1].cluster) {
        g_assert (hb_glyph[i].cluster > hb_glyph[i+1].cluster);
	l.clusters[cluster].num_bytes += hb_glyph[i].cluster - hb_glyph[i+1].cluster;
        cluster++;
      }
      l.clusters[cluster].num_glyphs++;
    }
    l.clusters[cluster].num_bytes += text_len - hb_glyph[0].cluster;
  } else {
    for (i = 1; i < (int) l.num_glyphs; i++) {
      if (hb_glyph[i].cluster != hb_glyph[i-1].cluster) {
        g_assert (hb_glyph[i].cluster > hb_glyph[i-1].cluster);
	l.clusters[cluster].num_bytes += hb_glyph[i].cluster - hb_glyph[i-1].cluster;
        cluster++;
      }
      l.clusters[cluster].num_glyphs++;
    }
    l.clusters[cluster].num_bytes += text_len - hb_glyph[i - 1].cluster;
  }

done:
  g_array_append_val (lines, l);
}

void
view_cairo_t::finish (const font_options_t *font_opts)
{
  render (font_opts);

  for (unsigned int i = 0; i < lines->len; i++) {
    line_t &line = g_array_index (lines, line_t, i);
    line.finish ();
  }

  g_array_unref (lines);
}

double
view_cairo_t::line_width (unsigned int i)
{
  line_t &line = g_array_index (lines, line_t, i);
  return line.glyphs[line.num_glyphs].x * scale;
}

void
view_cairo_t::get_surface_size (cairo_scaled_font_t *scaled_font,
				double *w, double *h)
{
  cairo_font_extents_t font_extents;

  cairo_scaled_font_extents (scaled_font, &font_extents);

  *h = font_extents.ascent
     + font_extents.descent
     + ((int) lines->len - 1) * font_extents.height;
  *w = 0;
  for (unsigned int i = 0; i < lines->len; i++)
    *w = MAX (*w, line_width (i));

  *w += margin.l + margin.r;
  *h += margin.t + margin.b;
}

cairo_scaled_font_t *
view_cairo_t::create_scaled_font (const font_options_t *font_opts)
{
  hb_font_t *font = hb_font_reference (font_opts->get_font ());

  cairo_font_face_t *cairo_face;
  FT_Face ft_face = hb_ft_font_get_face (font);
  if (!ft_face)
    /* This allows us to get some boxes at least... */
    cairo_face = cairo_toy_font_face_create ("sans",
					     CAIRO_FONT_SLANT_NORMAL,
					     CAIRO_FONT_WEIGHT_NORMAL);
  else
    cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);
  cairo_matrix_t ctm, font_matrix;
  cairo_font_options_t *font_options;

  cairo_matrix_init_identity (&ctm);
  cairo_matrix_init_scale (&font_matrix,
			   font_opts->font_size, font_opts->font_size);
  font_options = cairo_font_options_create ();
  cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);
  cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_OFF);

  cairo_scaled_font_t *scaled_font = cairo_scaled_font_create (cairo_face,
							       &font_matrix,
							       &ctm,
							       font_options);

  cairo_font_options_destroy (font_options);
  cairo_font_face_destroy (cairo_face);

  static cairo_user_data_key_t key;
  if (cairo_scaled_font_set_user_data (scaled_font,
				       &key,
				       (void *) font,
				       (cairo_destroy_func_t) hb_font_destroy))
    hb_font_destroy (font);

  return scaled_font;
}

struct finalize_closure_t {
  void (*callback)(finalize_closure_t *);
  cairo_surface_t *surface;
  cairo_write_func_t write_func;
  void *closure;
};
static cairo_user_data_key_t finalize_closure_key;

#ifdef CAIRO_HAS_PNG_FUNCTIONS

static void
finalize_png (finalize_closure_t *closure)
{
  cairo_status_t status;
  status = cairo_surface_write_to_png_stream (closure->surface,
					      closure->write_func,
					      closure->closure);
  if (status != CAIRO_STATUS_SUCCESS)
    fail (FALSE, "Failed to write output: %s",
	  cairo_status_to_string (status));
}

static cairo_surface_t *
_cairo_png_surface_create_for_stream (cairo_write_func_t write_func,
				      void *closure,
				      double width,
				      double height,
				      cairo_content_t content)
{
  cairo_surface_t *surface;
  int w = ceil (width);
  int h = ceil (height);

  switch (content) {
    case CAIRO_CONTENT_ALPHA:
      surface = cairo_image_surface_create (CAIRO_FORMAT_A8, w, h);
      break;
    default:
    case CAIRO_CONTENT_COLOR:
      surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, w, h);
      break;
    case CAIRO_CONTENT_COLOR_ALPHA:
      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
      break;
  }
  cairo_status_t status = cairo_surface_status (surface);
  if (status != CAIRO_STATUS_SUCCESS)
    fail (FALSE, "Failed to create cairo surface: %s",
	  cairo_status_to_string (status));

  finalize_closure_t *png_closure = g_new0 (finalize_closure_t, 1);
  png_closure->callback = finalize_png;
  png_closure->surface = surface;
  png_closure->write_func = write_func;
  png_closure->closure = closure;

  if (cairo_surface_set_user_data (surface,
				   &finalize_closure_key,
				   (void *) png_closure,
				   (cairo_destroy_func_t) g_free))
    g_free ((void *) closure);

  return surface;
}

#endif

void
view_cairo_t::render (const font_options_t *font_opts)
{
  cairo_scaled_font_t *scaled_font = create_scaled_font (font_opts);
  double w, h;
  get_surface_size (scaled_font, &w, &h);
  cairo_t *cr = create_context (w, h);
  cairo_set_scaled_font (cr, scaled_font);
  cairo_scaled_font_destroy (scaled_font);

  draw (cr);

  finalize_closure_t *closure = (finalize_closure_t *)
				cairo_surface_get_user_data (cairo_get_target (cr),
							     &finalize_closure_key);
  if (closure)
    closure->callback (closure);

  cairo_status_t status = cairo_status (cr);
  if (status != CAIRO_STATUS_SUCCESS)
    fail (FALSE, "Failed: %s",
	  cairo_status_to_string (status));
  cairo_destroy (cr);
}

static cairo_status_t
stdio_write_func (void                *closure,
		  const unsigned char *data,
		  unsigned int         size)
{
  FILE *fp = (FILE *) closure;

  while (size) {
    size_t ret = fwrite (data, 1, size, fp);
    size -= ret;
    data += ret;
    if (size && ferror (fp))
      fail (FALSE, "Failed to write output: %s", strerror (errno));
  }

  return CAIRO_STATUS_SUCCESS;
}

cairo_t *
view_cairo_t::create_context (double w, double h)
{
  cairo_surface_t *(*constructor) (cairo_write_func_t write_func,
				   void *closure,
				   double width,
				   double height) = NULL;
  cairo_surface_t *(*constructor2) (cairo_write_func_t write_func,
				    void *closure,
				    double width,
				    double height,
				    cairo_content_t content) = NULL;

  const char *extension = output_format;
  if (!extension)
    extension = "png";
  if (0)
    ;
  #ifdef CAIRO_HAS_PNG_FUNCTIONS
    else if (0 == strcasecmp (extension, "png"))
      constructor2 = _cairo_png_surface_create_for_stream;
  #endif
  #ifdef CAIRO_HAS_SVG_SURFACE
    else if (0 == strcasecmp (extension, "svg"))
      constructor = cairo_svg_surface_create_for_stream;
  #endif
  #ifdef CAIRO_HAS_PDF_SURFACE
    else if (0 == strcasecmp (extension, "pdf"))
      constructor = cairo_pdf_surface_create_for_stream;
  #endif
  #ifdef CAIRO_HAS_PS_SURFACE
    else if (0 == strcasecmp (extension, "ps"))
      constructor = cairo_ps_surface_create_for_stream;
   #ifdef HAS_EPS
    else if (0 == strcasecmp (extension, "eps"))
      constructor = _cairo_eps_surface_create_for_stream;
   #endif
  #endif


  unsigned int fr, fg, fb, fa, br, bg, bb, ba;
  br = bg = bb = ba = 255;
  sscanf (back + (*back=='#'), "%2x%2x%2x%2x", &br, &bg, &bb, &ba);
  fr = fg = fb = 0; fa = 255;
  sscanf (fore + (*fore=='#'), "%2x%2x%2x%2x", &fr, &fg, &fb, &fa);

  cairo_content_t content;
  if (!annotate && ba == 255 && br == bg && bg == bb && fr == fg && fg == fb)
    content = CAIRO_CONTENT_ALPHA;
  else if (ba == 255)
    content = CAIRO_CONTENT_COLOR;
  else
    content = CAIRO_CONTENT_COLOR_ALPHA;

  cairo_surface_t *surface;
  FILE *f = get_file_handle ();
  if (constructor)
    surface = constructor (stdio_write_func, f, w, h);
  else if (constructor2)
    surface = constructor2 (stdio_write_func, f, w, h, content);
  else
    fail (FALSE, "Unknown output format `%s'", extension);

  cairo_t *cr = cairo_create (surface);
  content = cairo_surface_get_content (surface);

  switch (content) {
    case CAIRO_CONTENT_ALPHA:
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr, 1., 1., 1., br / 255.);
      cairo_paint (cr);
      cairo_set_source_rgba (cr, 1., 1., 1.,
			     (fr / 255.) * (fa / 255.) + (br / 255) * (1 - (fa / 255.)));
      break;
    default:
    case CAIRO_CONTENT_COLOR:
    case CAIRO_CONTENT_COLOR_ALPHA:
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr, br / 255., bg / 255., bb / 255., ba / 255.);
      cairo_paint (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_set_source_rgba (cr, fr / 255., fg / 255., fb / 255., fa / 255.);
      break;
  }

  cairo_surface_destroy (surface);
  return cr;
}

void
view_cairo_t::draw (cairo_t *cr)
{
  cairo_save (cr);

  cairo_font_extents_t font_extents;
  cairo_font_extents (cr, &font_extents);
  cairo_translate (cr, margin.l, margin.t);
  for (unsigned int i = 0; i < lines->len; i++)
  {
    line_t &l = g_array_index (lines, line_t, i);

    if (i)
      cairo_translate (cr, 0, line_space);

    cairo_translate (cr, 0, font_extents.ascent);

    if (annotate) {
      cairo_save (cr);

      /* Draw actual glyph origins */
      cairo_set_source_rgba (cr, 1., 0., 0., .5);
      cairo_set_line_width (cr, 5);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      for (unsigned i = 0; i < l.num_glyphs; i++) {
	cairo_move_to (cr, l.glyphs[i].x, l.glyphs[i].y);
	cairo_rel_line_to (cr, 0, 0);
      }
      cairo_stroke (cr);

      cairo_restore (cr);
    }

    if (cairo_surface_get_type (cairo_get_target (cr)) == CAIRO_SURFACE_TYPE_IMAGE) {
      /* cairo_show_glyphs() doesn't support subpixel positioning */
      cairo_glyph_path (cr, l.glyphs, l.num_glyphs);
      cairo_fill (cr);
    } else
      cairo_show_text_glyphs (cr,
			      l.utf8, l.utf8_len,
			      l.glyphs, l.num_glyphs,
			      l.clusters, l.num_clusters,
			      l.cluster_flags);

    cairo_translate (cr, 0, font_extents.height - font_extents.ascent);
  }

  cairo_restore (cr);
}
