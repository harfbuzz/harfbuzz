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

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

typedef struct {
  int level;
  cairo_t *cr;
  hb_font_t *font;
  hb_font_t *unscaled_font;
  hb_color_t *colors;
  unsigned int num_colors;
  hb_color_t foreground_color;
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

typedef struct {
  float r, g, b, a;
} color_t;

static void
get_color (paint_data_t *data,
           unsigned int color_index,
           float alpha,
           color_t *c)
{
  hb_color_t color;

  if (color_index == 0xffff)
    color = data->foreground_color;
  else
    color = data->colors[color_index];

  c->r = hb_color_get_red (color) / 255.;
  c->g = hb_color_get_green (color) / 255.;
  c->b = hb_color_get_blue (color) / 255.;
  c->a = (hb_color_get_alpha (color) / 255.) * alpha;
}

static cairo_operator_t
to_operator (hb_paint_composite_mode_t mode)
{
  switch (mode)
    {
    case HB_PAINT_COMPOSITE_MODE_CLEAR: return CAIRO_OPERATOR_CLEAR;
    case HB_PAINT_COMPOSITE_MODE_SRC: return CAIRO_OPERATOR_SOURCE;
    case HB_PAINT_COMPOSITE_MODE_DEST: return CAIRO_OPERATOR_DEST;
    case HB_PAINT_COMPOSITE_MODE_SRC_OVER: return CAIRO_OPERATOR_OVER;
    case HB_PAINT_COMPOSITE_MODE_DEST_OVER: return CAIRO_OPERATOR_DEST_OVER;
    case HB_PAINT_COMPOSITE_MODE_SRC_IN: return CAIRO_OPERATOR_IN;
    case HB_PAINT_COMPOSITE_MODE_DEST_IN: return CAIRO_OPERATOR_DEST_IN;
    case HB_PAINT_COMPOSITE_MODE_SRC_OUT: return CAIRO_OPERATOR_OUT;
    case HB_PAINT_COMPOSITE_MODE_DEST_OUT: return CAIRO_OPERATOR_DEST_OUT;
    case HB_PAINT_COMPOSITE_MODE_SRC_ATOP: return CAIRO_OPERATOR_ATOP;
    case HB_PAINT_COMPOSITE_MODE_DEST_ATOP: return CAIRO_OPERATOR_DEST_ATOP;
    case HB_PAINT_COMPOSITE_MODE_XOR: return CAIRO_OPERATOR_XOR;
    case HB_PAINT_COMPOSITE_MODE_PLUS: return CAIRO_OPERATOR_ADD;
    case HB_PAINT_COMPOSITE_MODE_SCREEN: return CAIRO_OPERATOR_SCREEN;
    case HB_PAINT_COMPOSITE_MODE_OVERLAY: return CAIRO_OPERATOR_OVERLAY;
    case HB_PAINT_COMPOSITE_MODE_DARKEN: return CAIRO_OPERATOR_DARKEN;
    case HB_PAINT_COMPOSITE_MODE_LIGHTEN: return CAIRO_OPERATOR_LIGHTEN;
    case HB_PAINT_COMPOSITE_MODE_COLOR_DODGE: return CAIRO_OPERATOR_COLOR_DODGE;
    case HB_PAINT_COMPOSITE_MODE_COLOR_BURN: return CAIRO_OPERATOR_COLOR_BURN;
    case HB_PAINT_COMPOSITE_MODE_HARD_LIGHT: return CAIRO_OPERATOR_HARD_LIGHT;
    case HB_PAINT_COMPOSITE_MODE_SOFT_LIGHT: return CAIRO_OPERATOR_SOFT_LIGHT;
    case HB_PAINT_COMPOSITE_MODE_DIFFERENCE: return CAIRO_OPERATOR_DIFFERENCE;
    case HB_PAINT_COMPOSITE_MODE_EXCLUSION: return CAIRO_OPERATOR_EXCLUSION;
    case HB_PAINT_COMPOSITE_MODE_MULTIPLY: return CAIRO_OPERATOR_MULTIPLY;
    case HB_PAINT_COMPOSITE_MODE_HSL_HUE: return CAIRO_OPERATOR_HSL_HUE;
    case HB_PAINT_COMPOSITE_MODE_HSL_SATURATION: return CAIRO_OPERATOR_HSL_SATURATION;
    case HB_PAINT_COMPOSITE_MODE_HSL_COLOR: return CAIRO_OPERATOR_HSL_COLOR;
    case HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY: return CAIRO_OPERATOR_HSL_LUMINOSITY;
    default:;
    }

  return CAIRO_OPERATOR_SOURCE;
}

static cairo_extend_t
cairo_extend (hb_paint_extend_t extend)
{
  switch (extend)
    {
    case HB_PAINT_EXTEND_PAD: return CAIRO_EXTEND_PAD;
    case HB_PAINT_EXTEND_REPEAT: return CAIRO_EXTEND_REPEAT;
    case HB_PAINT_EXTEND_REFLECT: return CAIRO_EXTEND_REFLECT;
    default:;
    }

  return CAIRO_EXTEND_PAD;
}

static void
push_transform (hb_paint_funcs_t *funcs,
                void *paint_data,
                float xx, float yx,
                float xy, float yy,
                float dx, float dy,
                void *user_data)
{
  paint_data_t *data = user_data;
  cairo_matrix_t m;

  print (data, "start transform %f %f %f %f %f %f", xx, yx, xy, yy, dx, dy);

  cairo_save (data->cr);
  cairo_matrix_init (&m, xx, yx, xy, yy, dx, dy);
  cairo_transform (data->cr, &m);

  data->level++;
}

static void
pop_transform (hb_paint_funcs_t *funcs,
               void *paint_data,
               void *user_data)
{
  paint_data_t *data = user_data;

  data->level--;

  cairo_restore (data->cr);

  print (data, "end transform");
}

static void
move_to (hb_draw_funcs_t *funcs, void *draw_data,
         hb_draw_state_t *st,
         float x, float y,
         void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "move to %f %f", x, y);

  cairo_move_to (data->cr, x, y);
}

static void
line_to (hb_draw_funcs_t *funcs, void *draw_data,
         hb_draw_state_t *st,
         float x, float y,
         void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "line to %f %f", x, y);

  cairo_line_to (data->cr, x, y);
}

static void
cubic_to (hb_draw_funcs_t *funcs, void *draw_data,
          hb_draw_state_t *st,
          float c1x, float c1y,
          float c2x, float c2y,
          float x, float y,
          void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "curve to %f %f  %f %f  %f %f", c1x, c1y, c2x, c2y, x, y);

  cairo_curve_to (data->cr, c1x, c1y, c2x, c2y, x, y);
}

static void
close_path (hb_draw_funcs_t *funcs, void *draw_data,
            hb_draw_state_t *st,
            void *user_data)
{
  paint_data_t *data = user_data;

  print (data, "close path\n");

  cairo_close_path (data->cr);
}

static void
push_clip_glyph (hb_paint_funcs_t *funcs,
                 void *paint_data,
                 hb_codepoint_t glyph,
                 void *user_data)
{
  paint_data_t *data = user_data;
  hb_draw_funcs_t *dfuncs;
  print (data, "start clip glyph %u", glyph);
  data->level++;

  cairo_save (data->cr);

  dfuncs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (dfuncs, move_to, data, NULL);
  hb_draw_funcs_set_line_to_func (dfuncs, line_to, data, NULL);
  hb_draw_funcs_set_cubic_to_func (dfuncs, cubic_to, data, NULL);
  hb_draw_funcs_set_close_path_func (dfuncs, close_path, data, NULL);

  cairo_new_path (data->cr);
  /* Note: we need to use a upem-scaled, unslanted copy of the font here,
   * since hb has already applied the root transform.
   */
  hb_font_get_glyph_shape (data->unscaled_font, glyph, dfuncs, data);
  cairo_close_path (data->cr);

  cairo_clip (data->cr);

  hb_draw_funcs_destroy (dfuncs);
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

  cairo_save (data->cr);

  cairo_rectangle (data->cr, xmin, ymin, xmax - xmin, ymax - ymin);

  cairo_clip (data->cr);
}

static void
pop_clip (hb_paint_funcs_t *funcs,
          void *paint_data,
          void *user_data)
{
  paint_data_t *data = user_data;

  cairo_restore (data->cr);

  data->level--;
  print (data, "end clip");
}

static void
paint_color (hb_paint_funcs_t *funcs,
             void *paint_data,
             unsigned int color_index,
             float alpha,
             void *user_data)
{
  paint_data_t *data = user_data;
  color_t c;

  print (data, "solid %u %f", color_index, alpha);

  get_color (data, color_index, alpha, &c);
  data->level++;
  print (data, "color %f %f %f %f", c.r, c.g, c.b, c.a);
  data->level--;
  cairo_set_source_rgba (data->cr, c.r, c.g, c.b, c.a);
  cairo_paint (data->cr);
}

typedef struct
{
  hb_blob_t *blob;
  unsigned int offset;
} read_blob_data_t;

cairo_status_t
read_blob (void *closure,
           unsigned char *data,
           unsigned int length)
{
  read_blob_data_t *r = closure;
  const char *d;
  unsigned int size;

  d = hb_blob_get_data (r->blob, &size);

  if (r->offset + length > size)
    return CAIRO_STATUS_READ_ERROR;

  memcpy (data, d + r->offset, length);
  r->offset += length;

  return CAIRO_STATUS_SUCCESS;
}

static void
paint_image (hb_paint_funcs_t *funcs,
             void *paint_data,
             hb_codepoint_t glyph,
             void *user_data)
{
  paint_data_t *data = user_data;
  read_blob_data_t r;
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;
  cairo_matrix_t m;
  hb_glyph_extents_t extents;

  hb_font_get_glyph_extents (data->font, glyph, &extents);
  r.blob = hb_ot_color_glyph_reference_png (data->font, glyph);
  r.offset = 0;
  surface = cairo_image_surface_create_from_png_stream (read_blob, &r);
  hb_blob_destroy (r.blob);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_matrix_init_scale (&m, 1, -1);
  cairo_matrix_translate (&m, extents.x_bearing, - extents.y_bearing);
  cairo_pattern_set_matrix (pattern, &m);
  cairo_set_source (data->cr, pattern);

  cairo_paint (data->cr);

  cairo_pattern_destroy (pattern);
  cairo_surface_destroy (surface);
}

static void
reduce_anchors (float x0, float y0,
                float x1, float y1,
                float x2, float y2,
                float *xx0, float *yy0,
                float *xx1, float *yy1)
{
  float q1x, q1y, q2x, q2y;
  float s;
  float k;

  q2x = x2 - x0;
  q2y = y2 - y0;
  q1x = y1 - x0;
  q1y = y1 - y0;

  s = q2x * q2x + q2y * q2y;
  if (s < 0.000001)
    {
      *xx0 = x0; *yy0 = y0;
      *xx1 = x1; *yy1 = y1;
      return;
    }

  k = (q2x * q1x + q2y * q1y) / s;
  *xx0 = x0;
  *yy0 = y0;
  *xx1 = x1 - k * q2x;
  *yy1 = y1 - k * q2y;
}

static int
cmp_color_stop (const void *p1,
                const void *p2)
{
  const hb_color_stop_t *c1 = p1;
  const hb_color_stop_t *c2 = p2;

  if (c1->offset < c2->offset)
    return -1;
  else if (c1->offset > c2->offset)
    return 1;
  else
    return 0;
}

static void
normalize_color_line (hb_color_stop_t *stops,
                      unsigned int len,
                      float *omin,
                      float *omax)
{
  float min, max;

  qsort (stops, len, sizeof (hb_color_stop_t), cmp_color_stop);

  min = max = stops[0].offset;
  for (unsigned int i = 0; i < len; i++)
    {
      min = MIN (min, stops[i].offset);
      max = MAX (max, stops[i].offset);
    }

  if (min != max)
    {
      for (unsigned int i = 0; i < len; i++)
        stops[i].offset = (stops[i].offset - min) / (max - min);
    }

  *omin = min;
  *omax = max;
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
  paint_data_t *data = user_data;
  unsigned int len;
  hb_color_stop_t *stops;
  float xx0, yy0, xx1, yy1;
  float xxx0, yyy0, xxx1, yyy1;
  float min, max;
  cairo_pattern_t *pattern;

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = alloca (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);

  print (data, "linear gradient");
  data->level += 1;
  print (data, "p0 %f %f", x0, y0);
  print (data, "p1 %f %f", x1, y1);
  print (data, "p2 %f %f", x2, y2);
  print (data, "colors");
  data->level += 1;
  for (unsigned int i = 0; i < len; i++)
    print (data, "%f %u %f", stops[i].offset, stops[i].color_index, stops[i].alpha);
  data->level -= 2;

  reduce_anchors (x0, y0, x1, y1, x2, y2, &xx0, &yy0, &xx1, &yy1);
  normalize_color_line (stops, len, &min, &max);

  xxx0 = xx0 + min * (xx1 - xx0);
  yyy0 = yy0 + min * (yy1 - yy0);
  xxx1 = xx0 + max * (xx1 - xx0);
  yyy1 = yy0 + max * (yy1 - yy0);

  pattern = cairo_pattern_create_linear (xxx0, yyy0, xxx1, yyy1);
  cairo_pattern_set_extend (pattern, cairo_extend (hb_color_line_get_extend (color_line)));
  for (unsigned int i = 0; i < len; i++)
    {
      color_t c;
      get_color (data, stops[i].color_index, stops[i].alpha, &c);
      cairo_pattern_add_color_stop_rgba (pattern, stops[i].offset, c.r, c.g, c.b, c.a);
    }

  cairo_set_source (data->cr, pattern);
  cairo_paint (data->cr);

  cairo_pattern_destroy (pattern);
}

static void
paint_radial_gradient (hb_paint_funcs_t *funcs,
                       void *paint_data,
                       hb_color_line_t *color_line,
                       float x0, float y0, float r0,
                       float x1, float y1, float r1,
                       void *user_data)
{
  paint_data_t *data = user_data;
  unsigned int len;
  hb_color_stop_t *stops;
  cairo_extend_t extend;
  float min, max;
  float xx0, yy0, xx1, yy1;
  float rr0, rr1;
  cairo_pattern_t *pattern;

  print (data, "radial gradient");

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = alloca (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);

  extend = cairo_extend (hb_color_line_get_extend (color_line));

  normalize_color_line (stops, len, &min, &max);
  xx0 = x0 + min * (x1 - x0);
  yy0 = y0 + min * (y1 - y0);
  xx1 = x0 + max * (x1 - x0);
  yy1 = y0 + max * (y1 - y0);
  rr0 = r0 + min * (r1 - r0);
  rr1 = r0 + max * (r1 - r0);

  pattern = cairo_pattern_create_radial (xx0, yy0, rr0, xx1, yy1, rr1);
  cairo_pattern_set_extend (pattern, extend);

  for (unsigned int i = 0; i < len; i++)
    {
      color_t c;

      get_color (data, stops[i].color_index, stops[i].alpha, &c);
      cairo_pattern_add_color_stop_rgba (pattern, stops[i].offset, c.r, c.g, c.b, c.a);
    }

  cairo_set_source (data->cr, pattern);
  cairo_paint (data->cr);

  cairo_pattern_destroy (pattern);
}

typedef struct {
  float x, y;
} Point;

static inline float
interpolate (float f0, float f1, float f)
{
  return f0 + f * (f1 - f0);
}

static inline void
interpolate_points (Point *p0, Point *p1, float f, Point *out)
{
  out->x = interpolate (p0->x, p1->x, f);
  out->y = interpolate (p0->y, p1->y, f);
}

void
interpolate_colors (color_t *c0, color_t *c1, float k, color_t *c)
{
  c->r = c0->r + k * (c1->r - c0->r);
  c->g = c0->g + k * (c1->g - c0->g);
  c->b = c0->b + k * (c1->b - c0->b);
  c->a = c0->a + k * (c1->a - c0->a);
}

static inline float
dot (Point p, Point q)
{
  return p.x * q.x + p.y * q.y;
}

static inline Point
normalize (Point p)
{
  float len = sqrt (dot (p, p));

  return (Point) { p.x / len, p.y / len };
}

static inline Point
sum (Point p, Point q)
{
  return (Point) { p.x + q.x, p.y + q.y };
}

static inline Point
difference (Point p, Point q)
{
  return (Point) { p.x - q.x, p.y - q.y };
}

static inline Point
scale (Point p, float f)
{
  return (Point) { p.x * f, p.y * f };
}

typedef struct {
  Point center, p0, c0, c1, p1;
  color_t color0, color1;
} Patch;

static void
add_patch (cairo_pattern_t *pattern, Point *center, Patch *p)
{
  cairo_mesh_pattern_begin_patch (pattern);
  cairo_mesh_pattern_move_to (pattern, center->x, center->y);
  cairo_mesh_pattern_line_to (pattern, p->p0.x, p->p0.y);
  cairo_mesh_pattern_curve_to (pattern,
                               p->c0.x, p->c0.y,
                               p->c1.x, p->c1.y,
                               p->p1.x, p->p1.y);
  cairo_mesh_pattern_line_to (pattern, center->x, center->y);
  cairo_mesh_pattern_set_corner_color_rgba (pattern, 0,
                                            p->color0.r,
                                            p->color0.g,
                                            p->color0.b,
                                            p->color0.a);
  cairo_mesh_pattern_set_corner_color_rgba (pattern, 1,
                                            p->color0.r,
                                            p->color0.g,
                                            p->color0.b,
                                            p->color0.a);
  cairo_mesh_pattern_set_corner_color_rgba (pattern, 2,
                                            p->color1.r,
                                            p->color1.g,
                                            p->color1.b,
                                            p->color1.a);
  cairo_mesh_pattern_set_corner_color_rgba (pattern, 3,
                                            p->color1.r,
                                            p->color1.g,
                                            p->color1.b,
                                            p->color1.a);
  cairo_mesh_pattern_end_patch (pattern);
}

#define MAX_ANGLE (M_PI / 8.)

static void
add_sweep_gradient_patches1 (float cx, float cy, float radius,
                             float a0, color_t *c0,
                             float a1, color_t *c1,
                             cairo_pattern_t *pattern)
{
  Point center = (Point) { cx, cy };
  int num_splits;
  Point p0;
  color_t color0, color1;

  num_splits = ceilf (fabs (a1 - a0) / MAX_ANGLE);
  p0 = (Point) { cosf (a0), sinf (a0) };
  color0 = *c0;

  for (int a = 0; a < num_splits; a++)
    {
      float k = (a + 1.) / num_splits;
      float angle1;
      Point p1;
      Point A, U;
      Point C0, C1;
      Patch patch;

      angle1 = interpolate (a0, a1, k);
      interpolate_colors (c0, c1, k, &color1);

      patch.color0 = color0;
      patch.color1 = color1;

      p1 = (Point) { cosf (angle1), sinf (angle1) };
      patch.p0 = sum (center, scale (p0, radius));
      patch.p1 = sum (center, scale (p1, radius));

      A = normalize (sum (p0, p1));
      U = (Point) { -A.y, A.x };
      C0 = sum (A, scale (U, dot (difference (p0, A), p0) / dot (U, p0)));
      C1 = sum (A, scale (U, dot (difference (p1, A), p1) / dot (U, p1)));

      patch.c0 = sum (center, scale (sum (C0, scale (difference (C0, p0), 0.33333)), radius));
      patch.c1 = sum (center, scale (sum (C1, scale (difference (C1, p1), 0.33333)), radius));

      add_patch (pattern, &center, &patch);

      p0 = p1;
      color0 = color1;
    }
}

static void
add_sweep_gradient_patches (paint_data_t *data,
                            hb_color_stop_t *stops,
                            unsigned int n_stops,
                            cairo_extend_t extend,
                            float cx, float cy,
                            float radius,
                            float start_angle,
                            float end_angle,
                            cairo_pattern_t *pattern)
{
  float *angles;
  color_t *colors;
  color_t color0, color1;

  if (start_angle == end_angle)
    {
      if (extend == CAIRO_EXTEND_PAD)
        {
          color_t c;
          if (start_angle > 0)
            {
              get_color (data, stops[0].color_index, stops[0].alpha, &c);
              add_sweep_gradient_patches1 (cx, cy, radius,
                                           0.,          &c,
                                           start_angle, &c,
                                           pattern);
            }
          if (end_angle < 2 * M_PI)
            {
              get_color (data, stops[n_stops - 1].color_index, stops[n_stops - 1].alpha, &c);
              add_sweep_gradient_patches1 (cx, cy, radius,
                                           end_angle, &c,
                                           2 * M_PI,  &c,
                                           pattern);
            }
        }
      return;
    }

  assert (start_angle != end_angle);

  /* handle directions */
  if (end_angle < start_angle)
    {
      float angle = end_angle;
      end_angle = start_angle;
      start_angle = angle;

      for (int i = 0; i < n_stops - 1 - i; i++)
        {
          hb_color_stop_t stop = stops[i];
          stops[i] = stops[n_stops - 1 - i];
          stops[n_stops - 1 - i] = stop;
        }
    }

  angles = alloca (sizeof (float) * n_stops);
  colors = alloca (sizeof (color_t) * n_stops);

  for (int i = 0; i < n_stops; i++)
    {
      angles[i] = start_angle + stops[i].offset * (end_angle - start_angle);
      get_color (data, stops[i].color_index, stops[i].alpha, &colors[i]);
    }

  if (extend == CAIRO_EXTEND_PAD)
    {
      int pos;

      color0 = colors[0];
      for (pos = 0; pos < n_stops; pos++)
        {
          if (angles[pos] >= 0)
            {
              if (pos > 0)
                {
                  float k = (0 - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
                  interpolate_colors (&colors[pos-1], &colors[pos], k, &color0);
                }
              break;
            }
        }
      if (pos == n_stops)
        {
          /* everything is below 0 */
          color0 = colors[n_stops-1];
          add_sweep_gradient_patches1 (cx, cy, radius,
                                       0.,       &color0,
                                       2 * M_PI, &color0,
                                       pattern);
          return;
        }

      add_sweep_gradient_patches1 (cx, cy, radius,
                                   0.,          &color0,
                                   angles[pos], &colors[pos],
                                   pattern);

      for (pos++; pos < n_stops; pos++)
        {
          if (angles[pos] <= 2 * M_PI)
            {
              add_sweep_gradient_patches1 (cx, cy, radius,
                                           angles[pos - 1], &colors[pos-1],
                                           angles[pos],     &colors[pos],
                                           pattern);
            }
          else
            {
              float k = (2 * M_PI - angles[pos - 1]) / (angles[pos] - angles[pos - 1]);
              interpolate_colors (&colors[pos - 1], &colors[pos], k, &color1);
              add_sweep_gradient_patches1 (cx, cy, radius,
                                           angles[pos - 1], &colors[pos - 1],
                                           2 * M_PI,        &color1,
                                           pattern);
              break;
            }
        }

      if (pos == n_stops)
        {
          /* everything is below 2*M_PI */
          color0 = colors[n_stops - 1];
          add_sweep_gradient_patches1 (cx, cy, radius,
                                       angles[n_stops - 1], &color0,
                                       2 * M_PI,            &color0,
                                       pattern);
          return;
        }
    }
  else
    {
      int k;
      float span;

      span = angles[n_stops - 1] - angles[0];
      k = 0;
      if (angles[0] >= 0)
        {
          float ss = angles[0];
          while (ss > 0)
            {
              if (span > 0)
                {
                  ss -= span;
                  k--;
                }
              else
                {
                  ss += span;
                  k++;
                }
            }
        }
      else if (angles[0] < 0)
        {
          float ee = angles[n_stops - 1];
          while (ee < 0)
            {
              if (span > 0)
                {
                  ee += span;
                  k++;
                }
              else
                {
                  ee -= span;
                  k--;
                }
            }
        }

      //assert (angles[0] + k * span <= 0 && 0 < angles[n_stops - 1] + k * span);

     for (int l = k; TRUE; l++)
        {
          for (int i = 1; i < n_stops; i++)
            {
              float a0, a1;
              color_t *c0, *c1;

              if ((l % 2 != 0) && (extend == CAIRO_EXTEND_REFLECT))
                {
                  a0 = angles[0] + angles[n_stops - 1] - angles[n_stops - 1 - (i-1)] + l * span;
                  a1 = angles[0] + angles[n_stops - 1] - angles[n_stops - 1 - i] + l * span;
                  c0 = &colors[n_stops - 1 - (i - 1)];
                  c1 = &colors[n_stops - 1 - i];
                }
              else
                {
                  a0 = angles[i-1] + l * span;
                  a1 = angles[i] + l * span;
                  c0 = &colors[i-1];
                  c1 = &colors[i];
                }

              if (a1 < 0)
                continue;
              if (a0 < 0)
                {
                  color_t color;
                  float f = (0 - a0)/(a1 - a0);
                  interpolate_colors (c0, c1, f, &color);
                  add_sweep_gradient_patches1 (cx, cy, radius,
                                               0,  &color,
                                               a1, c1,
                                               pattern);
                }
              else if (a1 >= 2 * M_PI)
                {
                  color_t color;
                  float f = (2 * M_PI - a0)/(a1 - a0);
                  interpolate_colors (c0, c1, f, &color);
                  add_sweep_gradient_patches1 (cx, cy, radius,
                                               a0,       c0,
                                               2 * M_PI, &color,
                                               pattern);
                  goto done;
                }
              else
                {
                  add_sweep_gradient_patches1 (cx, cy, radius,
                                               a0, c0,
                                               a1, c1,
                                               pattern);
                }
            }
        }
done: ;
    }
}

static void
paint_sweep_gradient (hb_paint_funcs_t *funcs,
                      void *paint_data,
                      hb_color_line_t *color_line,
                      float cx, float cy,
                      float start_angle,
                      float end_angle,
                      void *user_data)
{
  paint_data_t *data = user_data;
  unsigned int len;
  hb_color_stop_t *stops;
  cairo_extend_t extend;
  double x1, y1, x2, y2;
  float max_x, max_y, radius;
  cairo_pattern_t *pattern;

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = alloca (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);
  qsort (stops, len, sizeof (hb_color_stop_t), cmp_color_stop);

  print (data, "sweep gradient");
  data->level++;
  print (data, "center %f %f", cx, cy);
  print (data, "angles %f %f", start_angle, end_angle);

  data->level += 1;
  for (unsigned int i = 0; i < len; i++)
    print (data, "%f %u %f", stops[i].offset, stops[i].color_index, stops[i].alpha);
  data->level -= 2;

  cairo_clip_extents (data->cr, &x1, &y1, &x2, &y2);
  max_x = MAX ((x1 - cx) * (x1 - cx), (x2 - cx) * (x2 - cx));
  max_y = MAX ((y1 - cy) * (y1 - cy), (y2 - cy) * (y2 - cy));
  radius = sqrt (max_x + max_y);

  extend = cairo_extend (hb_color_line_get_extend (color_line));

  pattern = cairo_pattern_create_mesh ();

  add_sweep_gradient_patches (data, stops, len, extend, cx, cy,
                              radius, start_angle, end_angle, pattern);

  cairo_set_source (data->cr, pattern);
  cairo_paint (data->cr);

  cairo_pattern_destroy (pattern);

  data->level--;
}

static void
push_group (hb_paint_funcs_t *funcs,
            void *paint_data,
            void *user_data)
{
  paint_data_t *data = user_data;
  print (data, "push group");
  data->level++;

  cairo_save (data->cr);
  cairo_push_group (data->cr);
}

static void
pop_group (hb_paint_funcs_t *funcs,
           void *paint_data,
           hb_paint_composite_mode_t mode,
           void *user_data)
{
  paint_data_t *data = user_data;

  cairo_pop_group_to_source (data->cr);
  cairo_set_operator (data->cr, to_operator (mode));
  cairo_paint (data->cr);

  cairo_restore (data->cr);

  data->level--;
  print (data, "pop group mode %d", mode);
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
  data.foreground_color = HB_COLOR (0, 0, 0, 255);

  data.num_colors = hb_ot_color_palette_get_colors (hb_font_get_face (font),
                                                    0, 0, NULL, NULL);
  data.colors = alloca (data.num_colors * sizeof (hb_color_t));
  hb_ot_color_palette_get_colors (hb_font_get_face (font),
                                  0, 0, &data.num_colors, data.colors);

  funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_transform_func (funcs, push_transform, &data, NULL);
  hb_paint_funcs_set_pop_transform_func (funcs, pop_transform, &data, NULL);
  hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, &data, NULL);
  hb_paint_funcs_set_push_clip_rect_func (funcs, push_clip_rect, &data, NULL);
  hb_paint_funcs_set_pop_clip_func (funcs, pop_clip, &data, NULL);
  hb_paint_funcs_set_push_group_func (funcs, push_group, &data, NULL);
  hb_paint_funcs_set_pop_group_func (funcs, pop_group, &data, NULL);
  hb_paint_funcs_set_color_func (funcs, paint_color, &data, NULL);
  hb_paint_funcs_set_image_func (funcs, paint_image, &data, NULL);
  hb_paint_funcs_set_linear_gradient_func (funcs, paint_linear_gradient, &data, NULL);
  hb_paint_funcs_set_radial_gradient_func (funcs, paint_radial_gradient, &data, NULL);
  hb_paint_funcs_set_sweep_gradient_func (funcs, paint_sweep_gradient, &data, NULL);

  hb_font_paint_glyph (font, gid, funcs, NULL);

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
