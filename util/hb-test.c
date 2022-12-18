#include <hb.h>
#include <hb-ot.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <cairo.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include "hb-cairo-utils.h"

static void
move_to (hb_draw_funcs_t *dfuncs,
         cairo_t *cr,
         hb_draw_state_t *st,
         float to_x, float to_y,
         void *)
{
  cairo_move_to (cr,
                 (double) to_x, (double) to_y);
}

static void
line_to (hb_draw_funcs_t *dfuncs,
         cairo_t *cr,
         hb_draw_state_t *st,
         float to_x, float to_y,
         void *)
{
  cairo_line_to (cr,
                 (double) to_x, (double) to_y);
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
  cairo_curve_to (cr,
                  (double) control1_x, (double) control1_y,
                  (double) control2_x, (double) control2_y,
                  (double) to_x, (double) to_y);
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
get_cairo_draw_funcs (void)
{
  static hb_draw_funcs_t *funcs;

  if (!funcs)
  {
    funcs = hb_draw_funcs_create ();
    hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) move_to, NULL, NULL);
    hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) line_to, NULL, NULL);
    hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) cubic_to, NULL, NULL);
    hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) close_path, NULL, NULL);
  }

  return funcs;
}


typedef struct {
  cairo_t *cr;
  hb_font_t *font;
  hb_font_t *unscaled_font;
  int level;
} paint_data_t;

static void
push_transform (hb_paint_funcs_t *funcs,
                void *paint_data,
                float xx, float yx,
                float xy, float yy,
                float dx, float dy,
                void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;
  cairo_matrix_t m;

  cairo_save (cr);
  cairo_matrix_init (&m, xx, yx, xy, yy, dx, dy);
  cairo_transform (data->cr, &m);
}

static void
pop_transform (hb_paint_funcs_t *funcs,
               void *paint_data,
               void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;

  cairo_restore (cr);
}

static void
push_clip_glyph (hb_paint_funcs_t *funcs,
                 void *paint_data,
                 hb_codepoint_t glyph,
                 void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;

  cairo_save (cr);
  cairo_new_path (cr);
  hb_font_get_glyph_shape (data->unscaled_font, glyph, get_cairo_draw_funcs (), cr);
  cairo_close_path (cr);
  cairo_clip (cr);
}

static void
push_clip_rectangle (hb_paint_funcs_t *funcs,
                     void *paint_data,
                     float xmin, float ymin, float xmax, float ymax,
                     void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;

  cairo_save (cr);
  cairo_rectangle (cr, xmin, ymin, xmax - xmin, ymax - ymin);
  cairo_clip (cr);
}

static void
pop_clip (hb_paint_funcs_t *funcs,
          void *paint_data,
          void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;

  cairo_restore (cr);
}

static void
push_group (hb_paint_funcs_t *funcs,
            void *paint_data,
            void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;

  cairo_save (cr);
  cairo_push_group (cr);
}

static void
pop_group (hb_paint_funcs_t *funcs,
           void *paint_data,
           hb_paint_composite_mode_t mode,
           void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;

  cairo_pop_group_to_source (cr);
  cairo_set_operator (cr, hb_paint_composite_mode_to_cairo (mode));
  cairo_paint (cr);

  cairo_restore (cr);
}

static void
paint_color (hb_paint_funcs_t *funcs,
             void *paint_data,
             unsigned int color_index,
             float alpha,
             void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  hb_face_t *face = hb_font_get_face (data->font);
  cairo_t *cr = data->cr;
  float r, g, b, a;

  hb_face_get_color (face, 0, color_index, alpha, &r, &g, &b, &a);
  cairo_set_source_rgba (cr, r, g, b, a);
  cairo_paint (cr);
}

static void
paint_image (hb_paint_funcs_t *funcs,
             void *paint_data,
             hb_blob_t *blob,
             hb_tag_t format,
             hb_glyph_extents_t *extents,
             void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;
  hb_font_t *font = data->font;

  hb_cairo_paint_glyph_image (cr, font, blob, format, extents);
}

static void
paint_linear_gradient (hb_paint_funcs_t *funcs,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0,
                       float x1, float y1,
                       float x2, float y2,
                       void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;
  hb_font_t *font = data->font;

  hb_cairo_paint_linear_gradient (cr, font, color_line, x0, y0, x1, y1, x2, y2);
}

static void
paint_radial_gradient (hb_paint_funcs_t *funcs,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0, float r0,
                       float x1, float y1, float r1,
                       void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;
  hb_font_t *font = data->font;

  hb_cairo_paint_radial_gradient (cr, font, color_line, x0, y0, r0, x1, y1, r1);
}

static void
paint_sweep_gradient (hb_paint_funcs_t *funcs,
                      void *paint_data,
                      hb_color_line_t *color_line,
                      float x0, float y0,
                      float start_angle, float end_angle,
                      void *user_data)
{
  paint_data_t *data = (paint_data_t *) paint_data;
  cairo_t *cr = data->cr;
  hb_font_t *font = data->font;

  hb_cairo_paint_sweep_gradient (cr, font, color_line, x0, y0, start_angle, end_angle);
}


int main (int argc, char *argv[])
{
  paint_data_t data;
  hb_paint_funcs_t *funcs;
  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);
  hb_font_t *unscaled_font;
  hb_font_set_scale (font, 20, 20);
  hb_codepoint_t gid = atoi (argv[2]);
  hb_glyph_extents_t extents;
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  cairo_matrix_t m;
  float xmin, ymin, xmax, ymax;
  unsigned int upem;

  float size = 120.;

  hb_font_set_scale (font, size, size);
  hb_font_get_glyph_extents (font, gid, &extents);

  unscaled_font = hb_font_create (face);
  upem = hb_face_get_upem (face);
  hb_font_set_scale (unscaled_font, upem, upem);
  hb_font_set_synthetic_slant (unscaled_font, 0.);

  printf ("size %f upem %u\n", size, upem);

  xmin = extents.x_bearing;
  xmax = xmin + extents.width;
  ymin = - extents.y_bearing;
  ymax = - extents.y_bearing - extents.height;

  printf ("surface %f %f, offset %f %f\n", ceil (xmax - xmin), ceil (ymax - ymin), - xmin, - ymin);

  if ((int) ceil (xmax - xmin) == 0 ||
      (int) ceil (ymax - ymin) == 0)
  {
    printf ("ERROR: empty surface\n");
    return 1;
  }

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        (int) ceil (xmax - xmin),
                                        (int) ceil (ymax - ymin));

  cairo_surface_set_device_offset (surface, - xmin, ymax);

  data.level = 0;
  data.cr = cairo_create (surface);
  cairo_push_group (data.cr);

  data.font = font;
  data.unscaled_font = unscaled_font;

  funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_transform_func (funcs, push_transform, NULL, NULL);
  hb_paint_funcs_set_pop_transform_func (funcs, pop_transform, NULL, NULL);
  hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, NULL, NULL);
  hb_paint_funcs_set_push_clip_rectangle_func (funcs, push_clip_rectangle, NULL, NULL);
  hb_paint_funcs_set_pop_clip_func (funcs, pop_clip, NULL, NULL);
  hb_paint_funcs_set_push_group_func (funcs, push_group, NULL, NULL);
  hb_paint_funcs_set_pop_group_func (funcs, pop_group, NULL, NULL);
  hb_paint_funcs_set_color_func (funcs, paint_color, NULL, NULL);
  hb_paint_funcs_set_image_func (funcs, paint_image, NULL, NULL);
  hb_paint_funcs_set_linear_gradient_func (funcs, paint_linear_gradient, NULL, NULL);
  hb_paint_funcs_set_radial_gradient_func (funcs, paint_radial_gradient, NULL, NULL);
  hb_paint_funcs_set_sweep_gradient_func (funcs, paint_sweep_gradient, NULL, NULL);

  hb_font_paint_glyph (font, gid, funcs, &data);

  pattern = cairo_pop_group (data.cr);

  cairo_matrix_init_scale (&m, 1, -1);
  cairo_matrix_translate (&m, 0, (ymax - ymin) + 2 * ymin);
  cairo_pattern_set_matrix (pattern, &m);
  cairo_set_source (data.cr, pattern);

  cairo_paint (data.cr);

  cairo_surface_set_device_offset (surface, - xmin, - ymin);

  printf ("writing glyph.png\n");
  cairo_surface_write_to_png (surface, "glyph.png");

  execvp ("eog", (char *const[]) { "eog", "glyph.png", NULL });

  return 0;
}
