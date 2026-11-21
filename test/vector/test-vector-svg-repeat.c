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

/* An SVG gradient that repeats or reflects goes on in periods as long
 * as it is written, so what its length is rounded by is multiplied by
 * the number of the period.  Check that its ends are written more
 * finely than to the two decimals of everything else, and that a
 * gradient that pads is written as everything else is. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

/* ---- a color line from red to blue ---- */

static unsigned
get_stops (hb_color_line_t *color_line,
	   void *color_line_data,
	   unsigned start,
	   unsigned *count,
	   hb_color_stop_t *stops,
	   void *user_data)
{
  static const hb_color_t colors[2] = {HB_COLOR (0, 0, 255, 255),
				       HB_COLOR (255, 0, 0, 255)};
  (void) color_line;
  (void) color_line_data;
  (void) user_data;
  if (count)
  {
    unsigned n = 0;
    for (unsigned i = start; i < 2 && n < *count; i++, n++)
    {
      stops[n].offset = (float) i;
      stops[n].is_foreground = 0;
      stops[n].color = colors[i];
    }
    *count = n;
  }
  return 2;
}

static hb_paint_extend_t
get_extend (hb_color_line_t *color_line,
	    void *color_line_data,
	    void *user_data)
{
  (void) color_line;
  (void) user_data;
  return *(hb_paint_extend_t *) color_line_data;
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

/* Paints a gradient in font units, written at a 64th of them as the
 * hb-vector tool has it: a linear one from (480, 512) to (511, 519),
 * or a radial one around (1000, 0) from a radius of 50 to one of 87. */
static hb_blob_t *
render (int radial, hb_paint_extend_t extend)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);
  assert (paint);
  hb_vector_paint_set_scale_factor (paint, 64.f, 64.f);
  hb_vector_extents_t extents = {0, 0, 16, 16};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  hb_color_line_t color_line;
  memset (&color_line, 0, sizeof (color_line));
  color_line.data = &extend;
  color_line.get_color_stops = get_stops;
  color_line.get_extend = get_extend;

  hb_paint_push_clip_rectangle (funcs, paint, 0, 0, 1000, 1000);
  if (radial)
    hb_paint_radial_gradient (funcs, paint, &color_line, 1000, 0, 50, 1000, 0, 87);
  else
    hb_paint_linear_gradient (funcs, paint, &color_line,
			      480, 512, 511, 519, 480 - 7, 512 + 31);
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
  hb_blob_t *blob;

  /* 31 font units along x are 0.484375 as written, and 7 along y are
   * 0.109375.  Rounded to 0.48 and 0.11, the gradient would be shorter
   * by 1 part in 120: a whole period out of place 120 periods on. */
  blob = render (0, HB_PAINT_EXTEND_REPEAT);
  assert (has (" x1=\"7.5\" y1=\"8\" x2=\"7.984375\" y2=\"8.109375\" spreadMethod=\"repeat\""));
  hb_blob_destroy (blob);

  blob = render (0, HB_PAINT_EXTEND_REFLECT);
  assert (has (" x1=\"7.5\" y1=\"8\" x2=\"7.984375\" y2=\"8.109375\" spreadMethod=\"reflect\""));
  hb_blob_destroy (blob);

  /* One that pads has no periods. */
  blob = render (0, HB_PAINT_EXTEND_PAD);
  assert (has (" x1=\"7.5\" y1=\"8\" x2=\"7.98\" y2=\"8.11\" spreadMethod=\"pad\""));
  hb_blob_destroy (blob);

  /* Radii of 50 and 87 font units are 0.78125 and 1.359375. */
  blob = render (1, HB_PAINT_EXTEND_REPEAT);
  assert (has (" cx=\"15.625\" cy=\"0\" r=\"1.359375\" fx=\"15.625\" fy=\"0\" fr=\"0.78125\" spreadMethod=\"repeat\""));
  hb_blob_destroy (blob);

  blob = render (1, HB_PAINT_EXTEND_REFLECT);
  assert (has (" cx=\"15.625\" cy=\"0\" r=\"1.359375\" fx=\"15.625\" fy=\"0\" fr=\"0.78125\" spreadMethod=\"reflect\""));
  hb_blob_destroy (blob);

  blob = render (1, HB_PAINT_EXTEND_PAD);
  assert (has (" cx=\"15.62\" cy=\"0\" r=\"1.36\" fx=\"15.62\" fy=\"0\" fr=\"0.78\" spreadMethod=\"pad\""));
  hb_blob_destroy (blob);

  return 0;
}
