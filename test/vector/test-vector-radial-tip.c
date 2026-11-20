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

/* A radial gradient paints the circles that have a radius.  A color
 * stop outside of 0 to 1 can lie past the tip of the cone, where the
 * radius would be negative; check that no such circle is written, and
 * what is written instead. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

/* ---- a color line of two or three stops: red, blue, green ---- */

typedef struct
{
  unsigned count;
  float offsets[3];
  hb_paint_extend_t extend;
} line_t;

static unsigned
get_stops (hb_color_line_t *color_line,
	   void *color_line_data,
	   unsigned start,
	   unsigned *count,
	   hb_color_stop_t *stops,
	   void *user_data)
{
  static const hb_color_t colors[3] = {HB_COLOR (0, 0, 255, 255),
				       HB_COLOR (255, 0, 0, 255),
				       HB_COLOR (0, 255, 0, 255)};
  line_t *line = (line_t *) color_line_data;
  (void) color_line;
  (void) user_data;
  if (count)
  {
    unsigned n = 0;
    for (unsigned i = start; i < line->count && n < *count; i++, n++)
    {
      stops[n].offset = line->offsets[i];
      stops[n].is_foreground = 0;
      stops[n].color = colors[i];
    }
    *count = n;
  }
  return line->count;
}

static hb_paint_extend_t
get_extend (hb_color_line_t *color_line,
	    void *color_line_data,
	    void *user_data)
{
  (void) color_line;
  (void) user_data;
  return ((line_t *) color_line_data)->extend;
}

static const char *doc;
static unsigned doc_len;

static int
has (const char *needle)
{
  unsigned n = (unsigned) strlen (needle);
  for (unsigned i = 0; i + n <= doc_len; i++)
    if (!memcmp (doc + i, needle, n))
      return 1;
  return 0;
}

/* Paints circles of radius @r0 where the color line is at 0 and @r1
 * where it is at 1, the one at the origin and the other @x1 to its
 * right, with the @count stops of the color line at @offsets. */
static hb_blob_t *
render_stops (hb_vector_format_t format,
	      float x1,
	      float r0, float r1,
	      unsigned count, const float *offsets,
	      hb_paint_extend_t extend)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (paint);
  hb_vector_extents_t extents = {-100, -100, 200, 200};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  line_t line;
  line.count = count;
  for (unsigned i = 0; i < count; i++)
    line.offsets[i] = offsets[i];
  line.extend = extend;
  hb_color_line_t color_line;
  memset (&color_line, 0, sizeof (color_line));
  color_line.data = &line;
  color_line.get_color_stops = get_stops;
  color_line.get_extend = get_extend;

  hb_paint_push_clip_rectangle (funcs, paint, -100, -100, 100, 100);
  hb_paint_radial_gradient (funcs, paint, &color_line, 0, 0, r0, x1, 0, r1);
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  doc = hb_blob_get_data (blob, &doc_len);
  return blob;
}

/* The same with two stops, at @offset0 and @offset1. */
static hb_blob_t *
render (hb_vector_format_t format,
	float x1,
	float r0, float r1,
	float offset0, float offset1,
	hb_paint_extend_t extend)
{
  float offsets[2];
  offsets[0] = offset0;
  offsets[1] = offset1;
  return render_stops (format, x1, r0, r1, 2, offsets, extend);
}

int
main (void)
{
  hb_blob_t *blob;

  /* The circles grow from 10 to 50, and the color line starts as far
   * before the first as it ends after it: at a radius of -30.  It is
   * cut where the radius is zero, three eighths along, and starts
   * there with the color it has there. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 10, 50, -1, 1, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 0 0 0 50]"));
  assert (has ("/C0 [0.6235 0 0.3765]"));
  assert (has ("/C1 [0 0 1]"));
  hb_blob_destroy (blob);

  blob = render (HB_VECTOR_FORMAT_SVG, 0, 10, 50, -1, 1, HB_PAINT_EXTEND_PAD);
  assert (has (" r=\"50\" fx=\"0\" fy=\"0\" spreadMethod"));
  assert (has ("<stop offset=\"0\" stop-color=\"rgb(159,0,96)\"/>"));
  assert (has ("<stop offset=\"1\" stop-color=\"rgb(0,0,255)\"/>"));
  hb_blob_destroy (blob);

  /* The same from the other end: the circles shrink from 50 to 10,
   * and the color line goes on to where the radius would be -30. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 50, 10, 0, 2, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 50 0 0 0]"));
  assert (has ("/C0 [1 0 0]"));
  assert (has ("/C1 [0.3765 0 0.6235]"));
  hb_blob_destroy (blob);

  blob = render (HB_VECTOR_FORMAT_SVG, 0, 50, 10, 0, 2, HB_PAINT_EXTEND_PAD);
  assert (has (" r=\"0\" fx=\"0\" fy=\"0\" fr=\"50\" spreadMethod"));
  assert (!has ("r=\"-"));
  hb_blob_destroy (blob);

  /* Red and blue both at 0, and green where the radius would be -30:
   * the end that is not cut keeps its two stops, so that red still
   * goes on before it. */
  {
    static const float offsets[3] = {0, 0, 2};
    blob = render_stops (HB_VECTOR_FORMAT_SVG, 0, 50, 10, 3, offsets, HB_PAINT_EXTEND_PAD);
    assert (has ("<stop offset=\"0\" stop-color=\"rgb(255,0,0)\"/>\n"
		 "<stop offset=\"0\" stop-color=\"rgb(0,0,255)\"/>\n"
		 "<stop offset=\"1\" stop-color=\"rgb(0,159,96)\"/>"));
    hb_blob_destroy (blob);
  }

  /* All of the color line is past the tip.  Its last color goes on
   * from its end, so that color is painted from the tip on. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 10, 30, -3, -1, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 0 0 0 40]"));
  assert (has ("/C0 [0 0 1]"));
  assert (has ("/C1 [0 0 1]"));
  hb_blob_destroy (blob);

  blob = render (HB_VECTOR_FORMAT_SVG, 0, 10, 30, -3, -1, HB_PAINT_EXTEND_PAD);
  assert (has (" r=\"40\" fx=\"0\" fy=\"0\" spreadMethod"));
  assert (has ("<stop offset=\"0\" stop-color=\"rgb(0,0,255)\"/>"));
  hb_blob_destroy (blob);

  /* A color line that repeats is not cut: its period from a radius of
   * -10 to 30 paints the same as the next, from 30 to 70.  One that
   * reflects is taken two periods further. */
  blob = render (HB_VECTOR_FORMAT_SVG, 0, 10, 30, -1, 1, HB_PAINT_EXTEND_REPEAT);
  assert (has (" r=\"70\" fx=\"0\" fy=\"0\" fr=\"30\" spreadMethod=\"repeat\""));
  assert (has ("<stop offset=\"0\" stop-color=\"rgb(255,0,0)\"/>"));
  hb_blob_destroy (blob);

  blob = render (HB_VECTOR_FORMAT_SVG, 0, 10, 30, -1, 1, HB_PAINT_EXTEND_REFLECT);
  assert (has (" r=\"110\" fx=\"0\" fy=\"0\" fr=\"70\" spreadMethod=\"reflect\""));
  hb_blob_destroy (blob);

  /* A cone that starts at its tip, and whose first color goes on
   * before it, is started a hair after the tip in PDF, for pdfium. */
  blob = render (HB_VECTOR_FORMAT_PDF, 100, 0, 50, 0, 1, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0.39 0 0.2 100 0 50]"));
  assert (has ("/C0 [0.9961 0 0.0039]"));
  hb_blob_destroy (blob);

  blob = render (HB_VECTOR_FORMAT_SVG, 100, 0, 50, 0, 1, HB_PAINT_EXTEND_PAD);
  assert (has (" cx=\"100\" cy=\"0\" r=\"50\" fx=\"0\" fy=\"0\" spreadMethod"));
  hb_blob_destroy (blob);

  /* Circles inside one another make no cone. */
  blob = render (HB_VECTOR_FORMAT_PDF, 10, 0, 50, 0, 1, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 0 10 0 50]"));
  hb_blob_destroy (blob);

  /* Circles that all have a radius are written as they are. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 10, 50, 0, 1, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 10 0 0 50]"));
  assert (has ("/C0 [1 0 0]"));
  hb_blob_destroy (blob);

  /* And nothing is painted if none has. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, -10, -10, 0, 1, HB_PAINT_EXTEND_PAD);
  assert (!has ("/ShadingType"));
  hb_blob_destroy (blob);

  blob = render (HB_VECTOR_FORMAT_SVG, 0, -10, -10, 0, 1, HB_PAINT_EXTEND_PAD);
  assert (!has ("<radialGradient"));
  hb_blob_destroy (blob);

  return 0;
}
