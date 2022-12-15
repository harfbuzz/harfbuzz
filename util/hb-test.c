#include <hb.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

typedef struct {
  int level;
} paint_data_t;

#define INDENT 2

static void
print (paint_data_t *data,
       const char *format,
       ...)
{
  va_list args;

  printf ("%*s", 2 * data->level, "");

  va_start (args, format);
  vprintf (format, args);
  va_end (args);

  printf ("\n");
}

static void
push_transform (hb_paint_funcs_t *funcs,
                void *paint_data,
                float xx, float xy,
                float yx, float yy,
                float x0, float y0,
                void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "start transform %f %f %f %f %f %f", xx, xy, yx, yy, x0, y0);
  data->level++;
}

static void
pop_transform (hb_paint_funcs_t *funcs,
               void *paint_data,
               void *user_data)
{
  paint_data_t *data = user_data;
  data->level--;
  print (data, "end transform");
}

static void
push_clip_glyph (hb_paint_funcs_t *funcs,
                 void *paint_data,
                 hb_codepoint_t glyph,
                 void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "start clip glyph %u", glyph);
  data->level++;
}

static void
push_clip_rect (hb_paint_funcs_t *funcs,
                void *paint_data,
                float xmin, float ymin, float xmax, float ymax,
                void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "start clip rect %f %f %f %f", xmin, ymin, xmax, ymax);
  data->level++;
}

static void
pop_clip (hb_paint_funcs_t *funcs,
          void *paint_data,
          void *user_data)
{
  paint_data_t *data = user_data;
  data->level--;
  print (data, "end clip");
}

static void
solid (hb_paint_funcs_t *funcs,
       void *paint_data,
       unsigned int color_index,
       float alpha,
       void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "solid %u %f", color_index, alpha);
}

static void
linear_gradient (hb_paint_funcs_t *funcs,
                 void *paint_data,
                 hb_color_line_t *color_line,
                 float x0, float y0,
                 float x1, float y1,
                 float x2, float y2,
                 void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "linear gradient ");
}

static void
radial_gradient (hb_paint_funcs_t *funcs,
                 void *paint_data,
                 hb_color_line_t *color_line,
                 float x0, float y0, float r0,
                 float x1, float y1, float r1,
                 void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "radial gradient");
}

static void
sweep_gradient (hb_paint_funcs_t *funcs,
                void *paint_data,
                hb_color_line_t *color_line,
                float x0, float y0,
                float start_angle,
                float end_angle,
                void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "sweep gradient");
}

static void
push_group (hb_paint_funcs_t *funcs,
            void *paint_data,
            void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "push group");
  data->level++;
}

static void
pop_group (hb_paint_funcs_t *funcs,
           void *paint_data,
           hb_paint_composite_mode_t mode,
           void *user_data)
{
  paint_data_t *data = user_data;
  data->level--;
  print (data, "pop group mode %d", mode);
}

int main (int argc, char *argv[])
{
  paint_data_t data = { 0 };
  hb_paint_funcs_t *funcs;
  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);
  hb_font_set_scale (font, 20, 20);
  hb_codepoint_t gid = atoi (argv[2]);

  funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_transform_func (funcs, push_transform, &data, NULL);
  hb_paint_funcs_set_pop_transform_func (funcs, pop_transform, &data, NULL);
  hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, &data, NULL);
  hb_paint_funcs_set_push_clip_rect_func (funcs, push_clip_rect, &data, NULL);
  hb_paint_funcs_set_pop_clip_func (funcs, pop_clip, &data, NULL);
  hb_paint_funcs_set_push_group_func (funcs, push_group, &data, NULL);
  hb_paint_funcs_set_pop_group_func (funcs, pop_group, &data, NULL);
  hb_paint_funcs_set_solid_func (funcs, solid, &data, NULL);
  hb_paint_funcs_set_linear_gradient_func (funcs, linear_gradient, &data, NULL);
  hb_paint_funcs_set_radial_gradient_func (funcs, radial_gradient, &data, NULL);
  hb_paint_funcs_set_sweep_gradient_func (funcs, sweep_gradient, &data, NULL);

  hb_font_paint_glyph (font, gid, funcs, NULL);

  return 0;
}
