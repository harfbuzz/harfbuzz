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

/* A color line that pads goes on before its first stop in that stop's
 * color, and after its last stop in that one's.  Where two stops share
 * the first offset or the last, a PDF shading would go on in the color
 * of the inner one.  Check that the color line is lengthened so that
 * the outer one has a stretch of the shading to itself. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

/* ---- a color line of up to four stops: red, blue, green, red ---- */

typedef struct
{
  unsigned count;
  float offsets[4];
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
  static const hb_color_t colors[4] = {HB_COLOR (0, 0, 255, 255),
				       HB_COLOR (255, 0, 0, 255),
				       HB_COLOR (0, 255, 0, 255),
				       HB_COLOR (0, 0, 255, 255)};
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

/* Paints a gradient whose color line has @count stops at @offsets: along
 * the x axis from 0 to @length if @r1 is negative, and otherwise in
 * circles around the origin of radius @r0 at 0 and @r1 at 1. */
static hb_blob_t *
render (hb_vector_format_t format,
	float length, float r0, float r1,
	unsigned count, const float *offsets,
	hb_paint_extend_t extend)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (paint);
  hb_vector_extents_t extents = {-200, -200, 400, 400};
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

  hb_paint_push_clip_rectangle (funcs, paint, -200, -200, 200, 200);
  if (r1 < 0)
    hb_paint_linear_gradient (funcs, paint, &color_line, 0, 0, length, 0, 0, length);
  else
    hb_paint_radial_gradient (funcs, paint, &color_line, 0, 0, r0, 0, 0, r1);
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  doc = hb_blob_get_data (blob, &doc_len);
  return blob;
}

int
main (void)
{
  static const float at_start[3] = {0, 0, 1};
  static const float at_end[3] = {0, 1, 1};
  static const float at_both[4] = {0, 0, 1, 1};
  hb_blob_t *blob;

  /* Red and blue at 0, green at 1, along 150: red before x = 0.  The
   * shading starts a fifteenth earlier, at -10, and is red up to a
   * sixteenth of the way. */
  blob = render (HB_VECTOR_FORMAT_PDF, 150, 0, -1, 3, at_start, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [-10 0 150 0]"));
  assert (has ("/Bounds [0.0625 0.0625]"));
  assert (has ("/C0 [1 0 0]\n/C1 [1 0 0]"));
  assert (has ("/Extend [true true]"));
  hb_blob_destroy (blob);

  /* Red at 0, blue and green at 1: green after x = 150.  The shading
   * ends a fifteenth later, and is green for its last sixteenth. */
  blob = render (HB_VECTOR_FORMAT_PDF, 150, 0, -1, 3, at_end, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 160 0]"));
  assert (has ("/Bounds [0.9375 0.9375]"));
  assert (has ("/C0 [0 1 0]\n/C1 [0 1 0]"));
  hb_blob_destroy (blob);

  /* Both, along 140: a fourteenth more at either end. */
  blob = render (HB_VECTOR_FORMAT_PDF, 140, 0, -1, 4, at_both, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [-10 0 150 0]"));
  assert (has ("/Bounds [0.0625 0.0625 0.9375 0.9375]"));
  hb_blob_destroy (blob);

  /* A color line that repeats has nothing before it, and gets no
   * stretch of red. */
  blob = render (HB_VECTOR_FORMAT_PDF, 150, 0, -1, 3, at_start, HB_PAINT_EXTEND_REPEAT);
  assert (has ("/C0 [1 0 0]\n/C1 [0 0 1]"));
  assert (!has ("/C0 [1 0 0]\n/C1 [1 0 0]"));
  hb_blob_destroy (blob);

  /* SVG has the color of the first stop before the color line, and
   * that of the last after it: the stops are written as they are. */
  blob = render (HB_VECTOR_FORMAT_SVG, 150, 0, -1, 3, at_start, HB_PAINT_EXTEND_PAD);
  assert (has (" x1=\"0\" y1=\"0\" x2=\"150\" y2=\"0\""));
  assert (has ("<stop offset=\"0\" stop-color=\"rgb(255,0,0)\"/>\n"
	       "<stop offset=\"0\" stop-color=\"rgb(0,0,255)\"/>\n"
	       "<stop offset=\"1\" stop-color=\"rgb(0,255,0)\"/>"));
  hb_blob_destroy (blob);

  /* Circles from a radius of 30 to 180: red within 30.  The shading
   * starts a fifteenth earlier, at a radius of 20. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 30, 180, 3, at_start, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 20 0 0 180]"));
  assert (has ("/Bounds [0.0625 0.0625]"));
  hb_blob_destroy (blob);

  /* From a radius of 6 to 156, a fifteenth earlier would be at a radius
   * of -4.  The shading starts halfway to zero, at 3. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 6, 156, 3, at_start, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 3 0 0 156]"));
  assert (has ("/Bounds [0.0196 0.0196]"));
  hb_blob_destroy (blob);

  /* From a radius of 0 nothing is painted before the color line, and
   * the red stop is not written. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 0, 150, 3, at_start, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 0 0 0 150]"));
  assert (!has ("/Bounds"));
  assert (has ("/C0 [0 0 1]\n/C1 [0 1 0]"));
  assert (!has ("/C0 [1 0 0]"));
  hb_blob_destroy (blob);

  /* Circles that shrink to nothing, with blue and green where they
   * end: nothing is painted after the color line either. */
  blob = render (HB_VECTOR_FORMAT_PDF, 0, 150, 0, 3, at_end, HB_PAINT_EXTEND_PAD);
  assert (has ("/Coords [0 0 150 0 0 0]"));
  assert (!has ("/Bounds"));
  assert (has ("/C0 [1 0 0]\n/C1 [0 0 1]"));
  hb_blob_destroy (blob);

  return 0;
}
