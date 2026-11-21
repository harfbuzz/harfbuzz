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

/* A color line whose stops all share one offset has no length.  If it
 * pads, it paints the first stop's color before that offset and the
 * last stop's from it on; if it repeats or reflects, nothing.  Nor
 * does a gradient along no line, or between two circles that are one,
 * paint anything.  Check what is written for each. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

static hb_font_t *font;
static hb_blob_t *blob;
static const char *doc;
static unsigned doc_len;

static unsigned
count (const char *needle)
{
  unsigned n = (unsigned) strlen (needle), found = 0;
  for (unsigned i = 0; i + n <= doc_len; i++)
    if (!memcmp (doc + i, needle, n))
      found++;
  return found;
}

/* Writes the glyph of @unicode in @format. */
static void
render (hb_vector_format_t format, hb_codepoint_t unicode)
{
  hb_codepoint_t glyph;
  assert (hb_font_get_nominal_glyph (font, unicode, &glyph));

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (format);
  assert (paint);
  hb_vector_paint_glyph (paint, font, glyph, HB_VECTOR_EXTENTS_MODE_EXPAND);
  hb_blob_destroy (blob);
  blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  doc = hb_blob_get_data (blob, &doc_len);
}

int
main (int argc, char **argv)
{
  assert (argc > 1);
  hb_blob_t *font_blob = hb_blob_create_from_file_or_fail (argv[1]);
  assert (font_blob);
  hb_face_t *face = hb_face_create (font_blob, 0);
  font = hb_font_create (face);

  /* a: a linear gradient of one stop is that color. */
  render (HB_VECTOR_FORMAT_SVG, 'a');
  assert (count ("<linearGradient ") == 1);
  assert (count ("<stop ") == 2);
  assert (count ("stop-color=\"rgb(255,0,0)\"") == 2);
  render (HB_VECTOR_FORMAT_PDF, 'a');
  assert (count ("/ShadingType 2") == 1);
  assert (count ("/C0 [1 0 0]") == 1 && count ("/C1 [1 0 0]") == 1);
  assert (count ("/Extend [true true]") == 1);

  /* b: two stops at 0.25 of a line from x = 200 to 800: red up to x =
   * 350, blue from there on.  That is written as a color line as long
   * as the font's, around x = 350, whose colors meet halfway along. */
  render (HB_VECTOR_FORMAT_SVG, 'b');
  assert (count (" x1=\"50\" y1=\"500\" x2=\"650\" y2=\"500\" spreadMethod=\"pad\"") == 1);
  assert (count ("<stop offset=\"0\" stop-color=\"rgb(255,0,0)\"/>\n"
		 "<stop offset=\"0.5\" stop-color=\"rgb(255,0,0)\"/>\n"
		 "<stop offset=\"0.5\" stop-color=\"rgb(0,76,255)\"/>\n"
		 "<stop offset=\"1\" stop-color=\"rgb(0,76,255)\"/>") == 1);
  render (HB_VECTOR_FORMAT_PDF, 'b');
  assert (count ("/Coords [50 500 650 500]") == 1);
  assert (count ("/Bounds [0.5 0.5]") == 1);
  assert (count ("/C0 [1 0 0]") == 2 && count ("/C1 [0 0.298 1]") == 2);
  assert (count ("/Extend [true true]") == 1);

  /* c: the same at 0.125 of circles growing from 100 to 500: red
   * within a radius of 150, blue outside.  A color line as long as the
   * font's around that would start where the radius is -50; it starts
   * at the center instead, and its colors meet 150 of its 350 along. */
  render (HB_VECTOR_FORMAT_SVG, 'c');
  assert (count (" cx=\"500\" cy=\"500\" r=\"350\" fx=\"500\" fy=\"500\" spreadMethod=\"pad\"") == 1);
  assert (count ("<stop offset=\"0\" stop-color=\"rgb(255,0,0)\"/>\n"
		 "<stop offset=\"0.4286\" stop-color=\"rgb(255,0,0)\"/>\n"
		 "<stop offset=\"0.4286\" stop-color=\"rgb(0,76,255)\"/>\n"
		 "<stop offset=\"1\" stop-color=\"rgb(0,76,255)\"/>") == 1);
  render (HB_VECTOR_FORMAT_PDF, 'c');
  assert (count ("/Coords [500 500 0 500 500 350]") == 1);
  assert (count ("/Bounds [0.4286 0.4286]") == 1);
  assert (count ("/Extend [true true]") == 1);

  /* d, e: nothing if the color line repeats or reflects. */
  render (HB_VECTOR_FORMAT_SVG, 'd');
  assert (count ("Gradient") == 0);
  render (HB_VECTOR_FORMAT_PDF, 'd');
  assert (count ("/ShadingType") == 0);
  render (HB_VECTOR_FORMAT_SVG, 'e');
  assert (count ("Gradient") == 0);
  render (HB_VECTOR_FORMAT_PDF, 'e');
  assert (count ("/ShadingType") == 0);

  /* f: a radial gradient of one stop is that color. */
  render (HB_VECTOR_FORMAT_SVG, 'f');
  assert (count ("<radialGradient ") == 1);
  assert (count ("<stop ") == 2);
  assert (count ("stop-color=\"rgb(255,0,0)\"") == 2);
  render (HB_VECTOR_FORMAT_PDF, 'f');
  assert (count ("/ShadingType 3") == 1);
  assert (count ("/C0 [1 0 0]") == 1 && count ("/C1 [1 0 0]") == 1);

  /* g: a gradient along no line is ill-formed; h: so is one between
   * two circles that are one.  Viewers would paint what was written
   * for them. */
  render (HB_VECTOR_FORMAT_SVG, 'g');
  assert (count ("Gradient") == 0);
  render (HB_VECTOR_FORMAT_PDF, 'g');
  assert (count ("/ShadingType") == 0);
  render (HB_VECTOR_FORMAT_SVG, 'h');
  assert (count ("Gradient") == 0);
  render (HB_VECTOR_FORMAT_PDF, 'h');
  assert (count ("/ShadingType") == 0);

  /* i: as b, with a green stop at the same offset between the red and
   * the blue: only the first and the last of them are used. */
  render (HB_VECTOR_FORMAT_SVG, 'i');
  assert (count ("<stop offset=\"0\" stop-color=\"rgb(255,0,0)\"/>\n"
		 "<stop offset=\"0.5\" stop-color=\"rgb(255,0,0)\"/>\n"
		 "<stop offset=\"0.5\" stop-color=\"rgb(0,76,255)\"/>\n"
		 "<stop offset=\"1\" stop-color=\"rgb(0,76,255)\"/>") == 1);
  assert (count ("<stop ") == 4);
  render (HB_VECTOR_FORMAT_PDF, 'i');
  assert (count ("/Bounds [0.5 0.5]") == 1);
  assert (count ("/C0 [1 0 0]") == 2 && count ("/C1 [0 0.298 1]") == 2);
  assert (count ("/C0 [0 1 0]") == 0 && count ("/C1 [0 1 0]") == 0);

  hb_blob_destroy (blob);
  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (font_blob);
  return 0;
}
