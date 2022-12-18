/*
 * Copyright © 2022  Red Hat, Inc
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
 * Google Author(s): Matthias Clasen
 */

#include "hb-cairo-utils.h"

#include <cairo.h>
#include <hb-ot.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

typedef struct {
  float r, g, b, a;
} color_t;

typedef struct {
  cairo_t *cr;
  hb_font_t *font;
  hb_font_t *unscaled_font;
} paint_data_t;


static inline cairo_extend_t
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

void
hb_face_get_color (hb_face_t *face,
                   unsigned int palette_index,
                   unsigned int color_index,
                   float alpha,
                   float *r, float *g, float *b, float *a)
{
  if (color_index == 0xffff)
  {
    /* foreground color */
    *r = *g = *b = 0;
    *a = alpha;
  }
  else
  {
    hb_color_t color;
    unsigned int clen = 1;

    if (hb_ot_color_palette_get_colors (face, palette_index, color_index, &clen, &color))
      {
        *r = hb_color_get_red (color) / 255.f;
        *g = hb_color_get_green (color) / 255.f;
        *b = hb_color_get_blue (color) / 255.f;
        *a = (hb_color_get_alpha (color) / 255.f) * alpha;
      }
    else
      {
        *r = *g = *b = 0;
        *a = alpha;
      }
  }
}

typedef struct
{
  hb_blob_t *blob;
  unsigned int offset;
} read_blob_data_t;

static cairo_status_t
read_blob (void *closure,
           unsigned char *data,
           unsigned int length)
{
  read_blob_data_t *r = (read_blob_data_t *) closure;
  const char *d;
  unsigned int size;

  d = hb_blob_get_data (r->blob, &size);

  if (r->offset + length > size)
    return CAIRO_STATUS_READ_ERROR;

  memcpy (data, d + r->offset, length);
  r->offset += length;

  return CAIRO_STATUS_SUCCESS;
}

void
hb_cairo_paint_glyph_image (cairo_t *cr,
                            hb_font_t *font,
                            hb_blob_t *blob,
                            hb_tag_t format,
                            hb_glyph_extents_t *extents)
{
  if (format != HB_PAINT_IMAGE_FORMAT_PNG || !extents)
    return;

  read_blob_data_t r;
  r.blob = blob;
  r.offset = 0;
  cairo_surface_t *surface = cairo_image_surface_create_from_png_stream (read_blob, &r);
  int width = cairo_image_surface_get_width (surface);
  int height = cairo_image_surface_get_width (surface);

  cairo_pattern_t *pattern = cairo_pattern_create_for_surface (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

  cairo_matrix_t matrix = {(double) width, 0, 0, (double) height, 0, 0};
  cairo_pattern_set_matrix (pattern, &matrix);

  cairo_translate (cr, extents->x_bearing, extents->y_bearing);
  cairo_scale (cr, extents->width, extents->height);
  cairo_set_source (cr, pattern);

  cairo_paint (cr);

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
  if (s < 0.000001f)
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
  const hb_color_stop_t *c1 = (const hb_color_stop_t *) p1;
  const hb_color_stop_t *c2 = (const hb_color_stop_t *) p2;

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

void
hb_cairo_paint_linear_gradient (cairo_t *cr,
                                hb_font_t *font,
                                hb_color_line_t *color_line,
                                float x0, float y0,
                                float x1, float y1,
                                float x2, float y2)
{
  hb_face_t *face = hb_font_get_face (font);
  unsigned int len;
  hb_color_stop_t *stops;
  float xx0, yy0, xx1, yy1;
  float xxx0, yyy0, xxx1, yyy1;
  float min, max;
  cairo_pattern_t *pattern;

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = (hb_color_stop_t *) alloca (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);
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
      float r, g, b, a;
      hb_face_get_color (face, 0, stops[i].color_index, stops[i].alpha, &r, &g, &b, &a);
      cairo_pattern_add_color_stop_rgba (pattern, stops[i].offset, r, g, b, a);
    }

  cairo_set_source (cr, pattern);
  cairo_paint (cr);

  cairo_pattern_destroy (pattern);
}

void
hb_cairo_paint_radial_gradient (cairo_t *cr,
                                hb_font_t *font,
                                hb_color_line_t *color_line,
                                float x0, float y0, float r0,
                                float x1, float y1, float r1)
{
  hb_face_t *face = hb_font_get_face (font);
  unsigned int len;
  hb_color_stop_t *stops;
  float min, max;
  float xx0, yy0, xx1, yy1;
  float rr0, rr1;
  cairo_pattern_t *pattern;

  len = hb_color_line_get_color_stops (color_line, 0, NULL, NULL);
  stops = (hb_color_stop_t *) alloca (len * sizeof (hb_color_stop_t));
  hb_color_line_get_color_stops (color_line, 0, &len, stops);
  normalize_color_line (stops, len, &min, &max);

  xx0 = x0 + min * (x1 - x0);
  yy0 = y0 + min * (y1 - y0);
  xx1 = x0 + max * (x1 - x0);
  yy1 = y0 + max * (y1 - y0);
  rr0 = r0 + min * (r1 - r0);
  rr1 = r0 + max * (r1 - r0);

  pattern = cairo_pattern_create_radial (xx0, yy0, rr0, xx1, yy1, rr1);
  cairo_pattern_set_extend (pattern, cairo_extend (hb_color_line_get_extend (color_line)));

  for (unsigned int i = 0; i < len; i++)
    {
      float r, g, b, a;
      hb_face_get_color (face, 0, stops[i].color_index, stops[i].alpha, &r, &g, &b, &a);
      cairo_pattern_add_color_stop_rgba (pattern, stops[i].offset, r, g, b, a);
    }

  cairo_set_source (cr, pattern);
  cairo_paint (cr);

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

static void
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
add_sweep_gradient_patches (hb_font_t *font,
                            hb_color_stop_t *stops,
                            unsigned int n_stops,
                            cairo_extend_t extend,
                            float cx, float cy,
                            float radius,
                            float start_angle,
                            float end_angle,
                            cairo_pattern_t *pattern)
{
  hb_face_t *face = hb_font_get_face (font);
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
              hb_face_get_color (face, 0, stops[0].color_index, stops[0].alpha, &c.r, &c.g, &c.b, &c.a);
              add_sweep_gradient_patches1 (cx, cy, radius,
                                           0.,          &c,
                                           start_angle, &c,
                                           pattern);
            }
          if (end_angle < 2 * M_PI)
            {
              hb_face_get_color (face, 0, stops[n_stops-1].color_index, stops[n_stops-1].alpha, &c.r, &c.g, &c.b, &c.a);
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

      for (unsigned i = 0; i < n_stops - 1 - i; i++)
        {
          hb_color_stop_t stop = stops[i];
          stops[i] = stops[n_stops - 1 - i];
          stops[n_stops - 1 - i] = stop;
        }
    }

  angles = alloca (sizeof (float) * n_stops);
  colors = alloca (sizeof (color_t) * n_stops);

  for (unsigned i = 0; i < n_stops; i++)
    {
      angles[i] = start_angle + stops[i].offset * (end_angle - start_angle);
      hb_face_get_color (face, 0, stops[i].color_index, stops[i].alpha, &colors[i].r, &colors[i].g, &colors[i].b, &colors[i].a);
    }

  if (extend == CAIRO_EXTEND_PAD)
    {
      unsigned pos;

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

     for (unsigned l = k; 1; l++)
        {
          for (unsigned i = 1; i < n_stops; i++)
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

void
hb_cairo_paint_sweep_gradient (cairo_t *cr,
                               hb_font_t *font,
                               hb_color_line_t *color_line,
                               float cx, float cy,
                               float start_angle,
                               float end_angle)
{
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

  cairo_clip_extents (cr, &x1, &y1, &x2, &y2);
  max_x = MAX ((x1 - cx) * (x1 - cx), (x2 - cx) * (x2 - cx));
  max_y = MAX ((y1 - cy) * (y1 - cy), (y2 - cy) * (y2 - cy));
  radius = sqrt (max_x + max_y);

  extend = cairo_extend (hb_color_line_get_extend (color_line));
  pattern = cairo_pattern_create_mesh ();

  add_sweep_gradient_patches (font, stops, len, extend, cx, cy,
                              radius, start_angle, end_angle, pattern);

  cairo_set_source (cr, pattern);
  cairo_paint (cr);

  cairo_pattern_destroy (pattern);
}
