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

/* A clip rectangle is given by two corners, and is the same rectangle
 * whichever way round they come: those of the clip box of a variable
 * font can cross.  SVG does not render a rect of negative width or
 * height, and a clip path of nothing lets nothing through. */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-vector.h>

static const char *
find (const char *data, unsigned len, const char *needle)
{
  unsigned n = (unsigned) strlen (needle);
  for (unsigned i = 0; i + n <= len; i++)
    if (!memcmp (data + i, needle, n))
      return data + i;
  return NULL;
}

/* Paints a color under a clip rectangle from one corner to another. */
static hb_blob_t *
render (float x0, float y0, float x1, float y1)
{
  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_SVG);
  assert (paint);
  hb_vector_extents_t extents = {0, 0, 100, 100};
  hb_vector_paint_set_extents (paint, &extents);
  hb_paint_funcs_t *funcs = hb_vector_paint_get_funcs (paint);

  hb_paint_push_clip_rectangle (funcs, paint, x0, y0, x1, y1);
  hb_paint_color (funcs, paint, 0, HB_COLOR (0, 0, 255, 255));
  hb_paint_pop_clip (funcs, paint);

  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);
  hb_vector_paint_destroy (paint);
  return blob;
}

int
main (void)
{
  /* The same rectangle, from each of its corners to the opposite one. */
  static const float corners[4][4] = {{20, 30, 80, 70},
				      {80, 30, 20, 70},
				      {20, 70, 80, 30},
				      {80, 70, 20, 30}};

  hb_blob_t *first = render (corners[0][0], corners[0][1], corners[0][2], corners[0][3]);
  unsigned first_len;
  const char *first_data = hb_blob_get_data (first, &first_len);
  assert (find (first_data, first_len,
		"<rect x=\"20\" y=\"30\" width=\"60\" height=\"40\"/></clipPath>"));

  for (unsigned i = 1; i < 4; i++)
  {
    hb_blob_t *blob = render (corners[i][0], corners[i][1], corners[i][2], corners[i][3]);
    unsigned len;
    const char *data = hb_blob_get_data (blob, &len);
    assert (!find (data, len, "width=\"-"));
    assert (!find (data, len, "height=\"-"));
    assert (len == first_len && !memcmp (data, first_data, len));
    hb_blob_destroy (blob);
  }

  hb_blob_destroy (first);
  return 0;
}
