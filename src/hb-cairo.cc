/*
 * Copyright © 2022  Red Hat, Inc.
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
 * Red Hat Author(s): Matthias Clasen
 */

#include "hb.hh"

#ifdef HAVE_CAIRO

#include "hb-cairo.h"

#include "hb-cairo-utils.hh"

#include "hb-machinery.hh"
#include "hb-utf.hh"


/**
 * SECTION:hb-cairo
 * @title: hb-cairo
 * @short_description: Cairo integration
 * @include: hb-cairo.h
 *
 * Functions for using HarfBuzz with the cairo library.
 *
 * HarfBuzz supports using cairo for rendering.
 **/

static void
hb_cairo_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		  void *draw_data,
		  hb_draw_state_t *st HB_UNUSED,
		  float to_x, float to_y,
		  void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_move_to (cr, (double) to_x, (double) to_y);
}

static void
hb_cairo_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		  void *draw_data,
		  hb_draw_state_t *st HB_UNUSED,
		  float to_x, float to_y,
		  void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_line_to (cr, (double) to_x, (double) to_y);
}

static void
hb_cairo_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		   void *draw_data,
		   hb_draw_state_t *st HB_UNUSED,
		   float control1_x, float control1_y,
		   float control2_x, float control2_y,
		   float to_x, float to_y,
		   void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_curve_to (cr,
                  (double) control1_x, (double) control1_y,
                  (double) control2_x, (double) control2_y,
                  (double) to_x, (double) to_y);
}

static void
hb_cairo_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED,
		     void *draw_data,
		     hb_draw_state_t *st HB_UNUSED,
		     void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_close_path (cr);
}

static inline void free_static_cairo_draw_funcs ();

static struct hb_cairo_draw_funcs_lazy_loader_t : hb_draw_funcs_lazy_loader_t<hb_cairo_draw_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();

    hb_draw_funcs_set_move_to_func (funcs, hb_cairo_move_to, nullptr, nullptr);
    hb_draw_funcs_set_line_to_func (funcs, hb_cairo_line_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func (funcs, hb_cairo_cubic_to, nullptr, nullptr);
    hb_draw_funcs_set_close_path_func (funcs, hb_cairo_close_path, nullptr, nullptr);

    hb_draw_funcs_make_immutable (funcs);

    hb_atexit (free_static_cairo_draw_funcs);

    return funcs;
  }
} static_cairo_draw_funcs;

static inline
void free_static_cairo_draw_funcs ()
{
  static_cairo_draw_funcs.free_instance ();
}

static hb_draw_funcs_t *
hb_cairo_draw_get_funcs ()
{
  return static_cairo_draw_funcs.get_unconst ();
}


#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC

static void
hb_cairo_push_transform (hb_paint_funcs_t *pfuncs HB_UNUSED,
			 void *paint_data,
			 float xx, float yx,
			 float xy, float yy,
			 float dx, float dy,
			 void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;
  cairo_matrix_t m;

  cairo_save (cr);
  cairo_matrix_init (&m, (double) xx, (double) yx,
                         (double) xy, (double) yy,
                         (double) dx, (double) dy);
  cairo_transform (cr, &m);
}

static void
hb_cairo_pop_transform (hb_paint_funcs_t *pfuncs HB_UNUSED,
		        void *paint_data,
		        void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  cairo_restore (cr);
}

static void
hb_cairo_push_clip_glyph (hb_paint_funcs_t *pfuncs HB_UNUSED,
			  void *paint_data,
			  hb_codepoint_t glyph,
			  hb_font_t *font,
			  void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  cairo_save (cr);
  cairo_new_path (cr);
  hb_font_draw_glyph (font, glyph, hb_cairo_draw_get_funcs (), cr);
  cairo_close_path (cr);
  cairo_clip (cr);
}

static void
hb_cairo_push_clip_rectangle (hb_paint_funcs_t *pfuncs HB_UNUSED,
			      void *paint_data,
			      float xmin, float ymin, float xmax, float ymax,
			      void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  cairo_save (cr);
  cairo_rectangle (cr,
                   (double) xmin, (double) ymin,
                   (double) (xmax - xmin), (double) (ymax - ymin));
  cairo_clip (cr);
}

static void
hb_cairo_pop_clip (hb_paint_funcs_t *pfuncs HB_UNUSED,
		   void *paint_data,
		   void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  cairo_restore (cr);
}

static void
hb_cairo_push_group (hb_paint_funcs_t *pfuncs HB_UNUSED,
		     void *paint_data,
		     void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  cairo_save (cr);
  cairo_push_group (cr);
}

static void
hb_cairo_pop_group (hb_paint_funcs_t *pfuncs HB_UNUSED,
		    void *paint_data,
		    hb_paint_composite_mode_t mode,
		    void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  cairo_pop_group_to_source (cr);
  cairo_set_operator (cr, hb_paint_composite_mode_to_cairo (mode));
  cairo_paint (cr);

  cairo_restore (cr);
}

static void
hb_cairo_paint_color (hb_paint_funcs_t *pfuncs HB_UNUSED,
		      void *paint_data,
		      hb_bool_t use_foreground,
		      hb_color_t color,
		      void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  cairo_set_source_rgba (cr,
                         hb_color_get_red (color) / 255.,
                         hb_color_get_green (color) / 255.,
                         hb_color_get_blue (color) / 255.,
                         hb_color_get_alpha (color) / 255.);
  cairo_paint (cr);
}

static hb_bool_t
hb_cairo_paint_image (hb_paint_funcs_t *pfuncs HB_UNUSED,
		      void *paint_data,
		      hb_blob_t *blob,
		      unsigned width,
		      unsigned height,
		      hb_tag_t format,
		      float slant,
		      hb_glyph_extents_t *extents,
		      void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  return hb_cairo_paint_glyph_image (cr, blob, width, height, format, slant, extents);
}

static void
hb_cairo_paint_linear_gradient (hb_paint_funcs_t *pfuncs HB_UNUSED,
				void *paint_data,
				hb_color_line_t *color_line,
				float x0, float y0,
				float x1, float y1,
				float x2, float y2,
				void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *)paint_data;

  hb_cairo_paint_linear_gradient (cr, color_line, x0, y0, x1, y1, x2, y2);
}

static void
hb_cairo_paint_radial_gradient (hb_paint_funcs_t *pfuncs HB_UNUSED,
				void *paint_data,
				hb_color_line_t *color_line,
				float x0, float y0, float r0,
				float x1, float y1, float r1,
				void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *)paint_data;

  hb_cairo_paint_radial_gradient (cr, color_line, x0, y0, r0, x1, y1, r1);
}

static void
hb_cairo_paint_sweep_gradient (hb_paint_funcs_t *pfuncs HB_UNUSED,
			       void *paint_data,
			       hb_color_line_t *color_line,
			       float x0, float y0,
			       float start_angle, float end_angle,
			       void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) paint_data;

  hb_cairo_paint_sweep_gradient (cr, color_line, x0, y0, start_angle, end_angle);
}

static inline void free_static_cairo_paint_funcs ();

static struct hb_cairo_paint_funcs_lazy_loader_t : hb_paint_funcs_lazy_loader_t<hb_cairo_paint_funcs_lazy_loader_t>
{
  static hb_paint_funcs_t *create ()
  {
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();

    hb_paint_funcs_set_push_transform_func (funcs, hb_cairo_push_transform, nullptr, nullptr);
    hb_paint_funcs_set_pop_transform_func (funcs, hb_cairo_pop_transform, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, hb_cairo_push_clip_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, hb_cairo_push_clip_rectangle, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func (funcs, hb_cairo_pop_clip, nullptr, nullptr);
    hb_paint_funcs_set_push_group_func (funcs, hb_cairo_push_group, nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func (funcs, hb_cairo_pop_group, nullptr, nullptr);
    hb_paint_funcs_set_color_func (funcs, hb_cairo_paint_color, nullptr, nullptr);
    hb_paint_funcs_set_image_func (funcs, hb_cairo_paint_image, nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func (funcs, hb_cairo_paint_linear_gradient, nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func (funcs, hb_cairo_paint_radial_gradient, nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func (funcs, hb_cairo_paint_sweep_gradient, nullptr, nullptr);

    hb_paint_funcs_make_immutable (funcs);

    hb_atexit (free_static_cairo_paint_funcs);

    return funcs;
  }
} static_cairo_paint_funcs;

static inline
void free_static_cairo_paint_funcs ()
{
  static_cairo_paint_funcs.free_instance ();
}

static hb_paint_funcs_t *
hb_cairo_paint_get_funcs ()
{
  return static_cairo_paint_funcs.get_unconst ();
}

static cairo_status_t
hb_cairo_render_color_glyph (cairo_scaled_font_t  *scaled_font,
			     unsigned long         glyph,
			     cairo_t              *cr,
			     cairo_text_extents_t *extents);
#endif

static const cairo_user_data_key_t hb_cairo_face_user_data_key = {0};
static const cairo_user_data_key_t hb_cairo_font_user_data_key = {0};

static void hb_cairo_face_destroy (void *p) { hb_face_destroy ((hb_face_t *) p); }
static void hb_cairo_font_destroy (void *p) { hb_font_destroy ((hb_font_t *) p); }

static cairo_status_t
hb_cairo_init_scaled_font (cairo_scaled_font_t  *scaled_font,
			   cairo_t              *cr HB_UNUSED,
			   cairo_font_extents_t *extents)
{
  hb_font_t *font = (hb_font_t *) cairo_font_face_get_user_data (cairo_scaled_font_get_font_face (scaled_font),
								 &hb_cairo_font_user_data_key);

  if (!font)
  {
    hb_face_t *face = (hb_face_t *) cairo_font_face_get_user_data (cairo_scaled_font_get_font_face (scaled_font),
								   &hb_cairo_face_user_data_key);
    font = hb_font_create (face);

    cairo_font_options_t *font_options = cairo_font_options_create ();

    // Set variations
    cairo_scaled_font_get_font_options (scaled_font, font_options);
    const char *variations = cairo_font_options_get_variations (font_options);
    hb_vector_t<hb_variation_t> vars;
    const char *p = variations;
    while (p && *p)
    {
      const char *end = strpbrk ((char *) p, ", ");
      hb_variation_t var;
      if (hb_variation_from_string (p, end ? end - p : -1, &var))
	vars.push (var);
      p = end ? end + 1 : nullptr;
    }
    hb_font_set_variations (font, &vars[0], vars.length);

    cairo_font_options_destroy (font_options);

    // TODO Set (what?) scale; Note, should NOT set slant.

    hb_font_make_immutable (font);
  }

  cairo_scaled_font_set_user_data (scaled_font,
				   &hb_cairo_font_user_data_key,
				   (void *) hb_font_reference (font),
				   hb_cairo_font_destroy);

  hb_position_t x_scale, y_scale;
  hb_font_get_scale (font, &x_scale, &y_scale);

  hb_font_extents_t hb_extents;
  hb_font_get_h_extents (font, &hb_extents);

  extents->ascent  = (double)  hb_extents.ascender  / y_scale;
  extents->descent = (double) -hb_extents.descender / y_scale;
  extents->height  = extents->ascent + extents->descent;

  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
hb_cairo_render_glyph (cairo_scaled_font_t  *scaled_font,
		       unsigned long         glyph,
		       cairo_t              *cr,
		       cairo_text_extents_t *extents)
{
  hb_font_t *font = (hb_font_t *) cairo_scaled_font_get_user_data (scaled_font,
								   &hb_cairo_font_user_data_key);

  hb_position_t x_scale, y_scale;
  hb_font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, +1./x_scale, -1./y_scale);

  hb_font_draw_glyph (font, glyph, hb_cairo_draw_get_funcs (), cr);

  cairo_fill (cr);

  return CAIRO_STATUS_SUCCESS;
}

#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC

static cairo_status_t
hb_cairo_render_color_glyph (cairo_scaled_font_t  *scaled_font,
			     unsigned long         glyph,
			     cairo_t              *cr,
			     cairo_text_extents_t *extents)
{
  hb_font_t *font = (hb_font_t *) cairo_scaled_font_get_user_data (scaled_font,
								   &hb_cairo_font_user_data_key);

  unsigned int palette = 0;
#ifdef CAIRO_COLOR_PALETTE_DEFAULT
  cairo_font_options_t *options = cairo_font_options_create ();
  cairo_scaled_font_get_font_options (scaled_font, options);
  palette = cairo_font_options_get_color_palette (options);
  cairo_font_options_destroy (options);
#endif

  hb_color_t color = HB_COLOR (0, 0, 0, 255);
  cairo_pattern_t *pattern = cairo_get_source (cr);
  if (cairo_pattern_get_type (pattern) == CAIRO_PATTERN_TYPE_SOLID)
  {
    double r, g, b, a;
    cairo_pattern_get_rgba (pattern, &r, &g, &b, &a);
    color = HB_COLOR ((int)(b * 255.), (int)(g * 255.), (int) (r * 255.), (int)(a * 255.));
  }

  hb_position_t x_scale, y_scale;
  hb_font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, +1./x_scale, -1./y_scale);

  hb_font_paint_glyph (font, glyph, hb_cairo_paint_get_funcs (), cr, palette, color);

  return CAIRO_STATUS_SUCCESS;
}

#endif

static cairo_font_face_t *
user_font_face_create (hb_face_t *face)
{
  cairo_font_face_t *cairo_face;

  cairo_face = cairo_user_font_face_create ();
  cairo_user_font_face_set_init_func (cairo_face, hb_cairo_init_scaled_font);
  cairo_user_font_face_set_render_glyph_func (cairo_face, hb_cairo_render_glyph);
#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC
  if (hb_ot_color_has_png (face) || hb_ot_color_has_layers (face) || hb_ot_color_has_paint (face))
    cairo_user_font_face_set_render_color_glyph_func (cairo_face, hb_cairo_render_color_glyph);
#endif

  cairo_font_face_set_user_data (cairo_face,
                                 &hb_cairo_face_user_data_key,
                                 (void *) hb_face_reference (face),
                                 hb_cairo_face_destroy);

  return cairo_face;
}

/**
 * hb_cairo_font_face_create_for_face:
 * @face: a `hb_face_t`
 *
 * Creates a `cairo_font_face_t` for rendering text according
 * to @face.
 *
 * Returns: a newly created `cairo_font_face_t`
 *
 * Since: REPLACEME
 */
cairo_font_face_t *
hb_cairo_font_face_create_for_face (hb_face_t *face)
{
  hb_face_make_immutable (face);

  return user_font_face_create (face);
}

/**
 * hb_cairo_font_face_get_face:
 * @font_face: a `cairo_font_face_t`
 *
 * Gets the `hb_face_t` associated with @font_face.
 *
 * Returns: (nullable): the `hb_face_t` associated with @font_face
 *
 * Since: REPLACEME
 */
hb_face_t *
hb_cairo_font_face_get_face (cairo_font_face_t *font_face)
{
  return (hb_face_t *) cairo_font_face_get_user_data (font_face, &hb_cairo_face_user_data_key);
}

/**
 * hb_cairo_font_face_create_for_font:
 * @font: a `hb_font_t`
 *
 * Creates a `cairo_font_face_t` for rendering text according
 * to @font.
 *
 * Note that the scale of @font does not affect the rendering,
 * but the variations and slant that are set on @font do.
 *
 * Returns: a newly created `cairo_font_face_t`
 *
 * Since: REPLACEME
 */
cairo_font_face_t *
hb_cairo_font_face_create_for_font (hb_font_t *font)
{
  hb_font_make_immutable (font);

  auto *cairo_face =  user_font_face_create (font->face);

  cairo_font_face_set_user_data (cairo_face,
                                 &hb_cairo_font_user_data_key,
                                 (void *) hb_font_reference (font),
                                 hb_cairo_font_destroy);

  return cairo_face;
}

/**
 * hb_cairo_font_face_get_font:
 * @font_face: a `cairo_font_face_t`
 *
 * Gets the `hb_font_t` that @font_face was created from.
 *
 * Returns: (nullable): the `hb_font_t` that @font_face was created from
 *
 * Since: REPLACEME
 */
hb_font_t *
hb_cairo_font_face_get_font (cairo_font_face_t *font_face)
{
  return (hb_font_t *) cairo_font_face_get_user_data (font_face, &hb_cairo_font_user_data_key);
}

/**
 * hb_cairo_scaled_font_get_font:
 * @scaled_font: a `cairo_scaled_font_t`
 *
 * Gets the `hb_font_t` associated with @scaled_font.
 *
 * Returns: (nullable): the `hb_font_t` associated with @scaled_font
 *
 * Since: REPLACEME
 */
hb_font_t *
hb_cairo_scaled_font_get_font (cairo_scaled_font_t *scaled_font)
{
  return (hb_font_t *) cairo_scaled_font_get_user_data (scaled_font, &hb_cairo_font_user_data_key);
}

/**
 * hb_cairo_glyphs_from_buffer:
 * @buffer: a `hb_buffer_t` containing glyphs
 * @text: (nullable): the text that was shaped in @buffer
 * @text_len: the length of @text in bytes
 * @utf8_clusters: `true` to provide cluster positions in bytes, instead of characters
 * @scale_factor: scale factor to scale positions by
 * @glyphs: return location for an array of `cairo_glyph_t`
 * @num_glyphs: return location for the length of @glyphs
 * @clusters: return location for an array of cluster positions
 * @num_clusters: return location for the length of @clusters
 * @cluster_flags: return location for cluster flags
 *
 * Extracts information from @buffer in a form that can be
 * passed to cairo_show_text_glyphs().
 *
 * Since: REPLACEME
 */
void
hb_cairo_glyphs_from_buffer (hb_buffer_t *buffer,
			     const char *text,
			     int text_len,
			     hb_bool_t utf8_clusters,
			     int scale_factor,
			     cairo_glyph_t **glyphs,
			     unsigned int *num_glyphs,
			     cairo_text_cluster_t **clusters,
			     unsigned int *num_clusters,
			     cairo_text_cluster_flags_t *cluster_flags)
{
  if (text && text_len < 0)
    text_len = strlen (text);

  *num_glyphs = hb_buffer_get_length (buffer);
  hb_glyph_info_t *hb_glyph = hb_buffer_get_glyph_infos (buffer, nullptr);
  hb_glyph_position_t *hb_position = hb_buffer_get_glyph_positions (buffer, nullptr);
  *glyphs = cairo_glyph_allocate (*num_glyphs + 1);

  if (text)
  {
    *num_clusters = *num_glyphs ? 1 : 0;
    for (unsigned int i = 1; i < *num_glyphs; i++)
      if (hb_glyph[i].cluster != hb_glyph[i-1].cluster)
	(*num_clusters)++;
    *clusters = cairo_text_cluster_allocate (*num_clusters);
  }
  else if (clusters)
  {
    *clusters = nullptr;
    *num_clusters = 0;
    *cluster_flags = (cairo_text_cluster_flags_t) 0;
  }

  if ((*num_glyphs && !*glyphs) ||
      (clusters && *num_clusters && !*clusters))
  {
    if (*glyphs)
    {
      cairo_glyph_free (*glyphs);
      *glyphs = nullptr;
      *num_glyphs = 0;
    }
    if (clusters && *clusters)
    {
      cairo_text_cluster_free (*clusters);
      *clusters = nullptr;
      *num_clusters = 0;
      *cluster_flags = (cairo_text_cluster_flags_t) 0;
    }
    return;
  }

  double iscale = scale_factor ? 1. / scale_factor : 0.;
  hb_position_t x = 0, y = 0;
  int i;
  for (i = 0; i < (int) *num_glyphs; i++)
  {
    (*glyphs)[i].index = hb_glyph[i].codepoint;
    (*glyphs)[i].x = (+hb_position->x_offset + x) * iscale;
    (*glyphs)[i].y = (-hb_position->y_offset + y) * iscale;
    x +=  hb_position->x_advance;
    y += -hb_position->y_advance;

    hb_position++;
  }
  (*glyphs)[i].index = -1;
  (*glyphs)[i].x = x * iscale;
  (*glyphs)[i].y = y * iscale;

  if (*num_clusters)
  {
    memset ((void *) *clusters, 0, *num_clusters * sizeof ((*clusters)[0]));
    hb_bool_t backward = HB_DIRECTION_IS_BACKWARD (hb_buffer_get_direction (buffer));
    *cluster_flags = backward ? CAIRO_TEXT_CLUSTER_FLAG_BACKWARD : (cairo_text_cluster_flags_t) 0;
    unsigned int cluster = 0;
    const char *start = text, *end;
    (*clusters)[cluster].num_glyphs++;
    if (backward)
    {
      for (i = *num_glyphs - 2; i >= 0; i--)
      {
	if (hb_glyph[i].cluster != hb_glyph[i+1].cluster)
	{
	  assert (hb_glyph[i].cluster > hb_glyph[i+1].cluster);
	  if (utf8_clusters)
	    end = start + hb_glyph[i].cluster - hb_glyph[i+1].cluster;
	  else
	    end = (const char *) hb_utf_offset_to_pointer<hb_utf8_t> ((const uint8_t *) start,
								      (signed) (hb_glyph[i].cluster - hb_glyph[i+1].cluster));
	  (*clusters)[cluster].num_bytes = end - start;
	  start = end;
	  cluster++;
	}
	(*clusters)[cluster].num_glyphs++;
      }
      (*clusters)[cluster].num_bytes = text + text_len - start;
    }
    else
    {
      for (i = 1; i < (int) *num_glyphs; i++)
      {
	if (hb_glyph[i].cluster != hb_glyph[i-1].cluster)
	{
	  assert (hb_glyph[i].cluster > hb_glyph[i-1].cluster);
	  if (utf8_clusters)
	    end = start + hb_glyph[i].cluster - hb_glyph[i-1].cluster;
	  else
	    end = (const char *) hb_utf_offset_to_pointer<hb_utf8_t> ((const uint8_t *) start,
								      (signed) (hb_glyph[i].cluster - hb_glyph[i-1].cluster));
	  (*clusters)[cluster].num_bytes = end - start;
	  start = end;
	  cluster++;
	}
	(*clusters)[cluster].num_glyphs++;
      }
      (*clusters)[cluster].num_bytes = text + text_len - start;
    }
  }
}

#endif
