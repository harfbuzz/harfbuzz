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


/* A PDF sweep gradient is a mesh.  Check that the mesh is sized to
 * the glyph it paints, and so puts the sweep's center where the font
 * says, instead of spanning the whole 16-bit coordinate range. */

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

static const char *
find (const char *data, unsigned len, const char *needle)
{
  unsigned n = (unsigned) strlen (needle);
  for (unsigned i = 0; i + n <= len; i++)
    if (!memcmp (data + i, needle, n))
      return data + i;
  return NULL;
}

int
main (int argc, char **argv)
{
  assert (argc > 1);
  hb_blob_t *font_blob = hb_blob_create_from_file_or_fail (argv[1]);
  assert (font_blob);
  hb_face_t *face = hb_face_create (font_blob, 0);
  hb_font_t *font = hb_font_create (face);

  /* sweep_0_360_pad_narrow: centered at (500, 600), clipped by a
   * circle of radius 350 around the same point. */
  hb_codepoint_t glyph;
  assert (hb_font_get_nominal_glyph (font, 0xF0200, &glyph));

  hb_vector_paint_t *paint = hb_vector_paint_create_or_fail (HB_VECTOR_FORMAT_PDF);
  assert (paint);
  hb_vector_paint_glyph (paint, font, glyph, HB_VECTOR_EXTENTS_MODE_EXPAND);
  hb_blob_t *blob = hb_vector_paint_render (paint);
  assert (blob);

  unsigned len;
  const char *data = hb_blob_get_data (blob, &len);
  const char *shading = find (data, len, "/ShadingType 6");
  assert (shading);
  unsigned rest = len - (unsigned) (shading - data);

  const char *decode = find (shading, rest, "/Decode [");
  assert (decode);
  float xlo, xhi, ylo, yhi;
  assert (4 == sscanf (decode, "/Decode [%f %f %f %f", &xlo, &xhi, &ylo, &yhi));

  /* Reaches the corners of the clip's bounding box, about 495 units
   * away, and not much further. */
  assert (xhi - xlo > 2 * 495 && xhi - xlo < 2 * 510);
  assert (yhi - ylo > 2 * 495 && yhi - ylo < 2 * 510);

  /* The first point of the first patch is the sweep's center. */
  const char *stream = find (shading, rest, "stream\n");
  assert (stream);
  const unsigned char *p = (const unsigned char *) stream + strlen ("stream\n");
  assert ((const char *) p + 5 <= data + len);
  assert (p[0] == 0); /* Patch flag. */
  float x = xlo + (float) ((p[1] << 8) | p[2]) / 65535.f * (xhi - xlo);
  float y = ylo + (float) ((p[3] << 8) | p[4]) / 65535.f * (yhi - ylo);
  assert (fabsf (x - 500.f) < 0.1f);
  assert (fabsf (y - 600.f) < 0.1f);

  hb_blob_destroy (blob);
  hb_vector_paint_destroy (paint);
  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (font_blob);
  return 0;
}
