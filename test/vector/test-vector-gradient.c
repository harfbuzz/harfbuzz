/*
 * Copyright © 2026  Behdad Esfahbod
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
 */

/* COLR interpolates the colors of a gradient with alpha multiplied
 * into them.  SVG and PDF viewers interpolate the color and the alpha
 * of each stop as written.  Check that the stops that are written make
 * the second come to the first. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

#define MAX_STOPS 256

typedef struct
{
  const hb_color_stop_t *stops;
  unsigned count;
} line_t;

static unsigned
gradient_color_stops (hb_color_line_t *cl, void *cl_data,
		      unsigned start, unsigned *count,
		      hb_color_stop_t *stops, void *user_data)
{
  const line_t *line = (const line_t *) cl_data;
  if (count && stops)
  {
    unsigned n = 0;
    for (; n < *count && start + n < line->count; n++)
      stops[n] = line->stops[start + n];
    *count = n;
  }
  return line->count;
}

static hb_paint_extend_t
gradient_extend (hb_color_line_t *cl, void *cl_data, void *user_data)
{ return HB_PAINT_EXTEND_PAD; }

/* Paints a linear gradient with the stops of @line, and returns the
 * document as a string. */
static char *
render (hb_vector_format_t format, line_t *line)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (paint);
  hb_vector_extents_t extents = {0, 0, 100, 100};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  hb_color_line_t color_line = {0};
  color_line.data = line;
  color_line.get_color_stops = gradient_color_stops;
  color_line.get_extend = gradient_extend;

  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 100, 100);
  hb_paint_linear_gradient (funcs, paint, &color_line, 0, 0, 100, 0, 0, 100);
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  unsigned len;
  const char *data = hb_blob_get_data (blob, &len);
  char *doc = (char *) malloc (len + 1);
  assert (doc);
  memcpy (doc, data, len);
  doc[len] = '\0';
  hb_blob_destroy (blob);
  hb_vector_paint_destroy (paint);
  return doc;
}

/* A stop as a viewer has it: red, green, blue and alpha, each 0 to 1. */
typedef struct
{
  float offset;
  float c[4];
} stop_t;

static unsigned
parse_svg_stops (const char *doc, stop_t *out)
{
  unsigned n = 0;
  for (const char *p = doc; (p = strstr (p, "<stop offset=\"")); p++)
  {
    unsigned r, g, b;
    float opacity = 1.f;
    assert (n < MAX_STOPS);
    assert (4 == sscanf (p, "<stop offset=\"%f\" stop-color=\"rgb(%u,%u,%u)\"",
			 &out[n].offset, &r, &g, &b));
    const char *end = strchr (p, '>');
    const char *o = strstr (p, "stop-opacity=\"");
    if (o && o < end)
      assert (1 == sscanf (o, "stop-opacity=\"%f\"", &opacity));
    out[n].c[0] = r / 255.f;
    out[n].c[1] = g / 255.f;
    out[n].c[2] = b / 255.f;
    out[n].c[3] = opacity;
    n++;
  }
  return n;
}

/* The premultiplied color at @t of a line of @n stops, interpolating
 * the colors premultiplied if @premultiplied, and as they are if not. */
static void
color_at (const stop_t *stops, unsigned n, float t, int premultiplied, float *out)
{
  /* The last segment that starts at or before t. */
  unsigned i = 0;
  while (i + 2 < n && stops[i + 1].offset <= t)
    i++;
  const stop_t *s0 = &stops[i], *s1 = &stops[i + 1];
  float u = s1->offset > s0->offset ? (t - s0->offset) / (s1->offset - s0->offset) : 0.f;
  float a = s0->c[3] + u * (s1->c[3] - s0->c[3]);
  for (unsigned k = 0; k < 3; k++)
    if (premultiplied)
      out[k] = s0->c[k] * s0->c[3] + u * (s1->c[k] * s1->c[3] - s0->c[k] * s0->c[3]);
    else
      out[k] = (s0->c[k] + u * (s1->c[k] - s0->c[k])) * a;
  out[3] = a;
}

/* Renders the stops, and checks what a viewer makes of what is written
 * against what COLR makes of the stops.  Returns how many stops were
 * written. */
static unsigned
check (const hb_color_stop_t *stops, unsigned count)
{
  line_t line = {stops, count};

  stop_t given[MAX_STOPS];
  for (unsigned i = 0; i < count; i++)
  {
    given[i].offset = stops[i].offset;
    given[i].c[0] = hb_color_get_red (stops[i].color) / 255.f;
    given[i].c[1] = hb_color_get_green (stops[i].color) / 255.f;
    given[i].c[2] = hb_color_get_blue (stops[i].color) / 255.f;
    given[i].c[3] = hb_color_get_alpha (stops[i].color) / 255.f;
  }

  char *svg = render (HB_VECTOR_FORMAT_SVG, &line);
  stop_t written[MAX_STOPS];
  unsigned n = parse_svg_stops (svg, written);
  assert (n >= 2);
  free (svg);

  for (unsigned i = 0; i < 1000; i++)
  {
    float t = (i + .5f) / 1000.f;
    float want[4], got[4];
    color_at (given, count, t, 1, want);
    color_at (written, n, t, 0, got);
    for (unsigned k = 0; k < 4; k++)
      assert (fabsf (want[k] - got[k]) < 4.f / 255.f);
  }

  /* PDF writes a color function and, with any alpha, an alpha function
   * for each pair of the same stops. */
  char *pdf = render (HB_VECTOR_FORMAT_PDF, &line);
  unsigned functions = 0;
  for (const char *p = pdf; (p = strstr (p, "/FunctionType 2")); p++)
    functions++;
  int translucent = 0;
  for (unsigned i = 0; i < count; i++)
    if (hb_color_get_alpha (stops[i].color) != 255)
      translucent = 1;
  assert (functions == (n - 1) * (translucent ? 2 : 1));
  free (pdf);

  return n;
}

int
main (void)
{
  /* Fading out: the transparent stop keeps the color it fades from. */
  {
    hb_color_stop_t stops[] = {
      {0.f, 0, HB_COLOR (0, 0, 255, 255)},
      {1.f, 0, HB_COLOR (0, 0, 0, 0)},
    };
    assert (check (stops, 2) == 2);
  }

  /* Through transparent: it has one color on each side. */
  {
    hb_color_stop_t stops[] = {
      {0.f, 0, HB_COLOR (0, 0, 255, 255)},
      {.5f, 0, HB_COLOR (0, 255, 0, 0)},
      {1.f, 0, HB_COLOR (255, 0, 0, 255)},
    };
    assert (check (stops, 3) == 4);
  }

  /* To a translucent color: stops are added along the way. */
  {
    hb_color_stop_t stops[] = {
      {0.f, 0, HB_COLOR (0, 165, 255, 255)},
      {1.f, 0, HB_COLOR (0, 0, 0, 77)},
    };
    assert (check (stops, 2) > 2);
  }
  {
    hb_color_stop_t stops[] = {
      {0.f, 0, HB_COLOR (255, 255, 255, 10)},
      {.3f, 0, HB_COLOR (0, 0, 0, 255)},
      {1.f, 0, HB_COLOR (0, 255, 0, 128)},
    };
    assert (check (stops, 3) > 3);
  }

  /* Nothing to do where alpha does not change. */
  {
    hb_color_stop_t stops[] = {
      {0.f, 0, HB_COLOR (0, 0, 255, 128)},
      {.5f, 0, HB_COLOR (0, 255, 0, 128)},
      {1.f, 0, HB_COLOR (255, 0, 0, 128)},
    };
    assert (check (stops, 3) == 3);
  }
  {
    hb_color_stop_t stops[] = {
      {0.f, 0, HB_COLOR (0, 0, 255, 255)},
      {1.f, 0, HB_COLOR (255, 0, 0, 255)},
    };
    assert (check (stops, 2) == 2);
  }

  return 0;
}
