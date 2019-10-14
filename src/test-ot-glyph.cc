/*
 * Copyright © 2019  Ebrahim Byagowi
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

#include "hb-ot.h"

#ifdef HB_NO_OPEN
#define hb_blob_create_from_file(x)  hb_blob_get_empty ()
#endif

#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf (stderr, "usage: %s font-file.ttf\n", argv[0]);
    exit (1);
  }

  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  unsigned int num_faces = hb_face_count (blob);
  if (num_faces == 0)
  {
    fprintf (stderr, "error: The file (%s) was corrupted, empty or not found", argv[1]);
    exit (1);
  }

  for (unsigned int face_index = 0; face_index < hb_face_count (blob); face_index++)
  {
    hb_face_t *face = hb_face_create (blob, face_index);
    hb_font_t *font = hb_font_create (face);
    unsigned int glyph_count = hb_face_get_glyph_count (face);
    for (unsigned int gid = 0; gid < glyph_count; ++gid)
    {
      hb_ot_glyph_path_point_t points[200];
      unsigned int points_len = 200;
      hb_ot_glyph_get_outline_path (font, gid, 0, nullptr, nullptr); /* just to test it */
      printf ("gid %d, points count: %d\n", gid, hb_ot_glyph_get_outline_path (font, gid, 0, &points_len, points));
      hb_glyph_extents_t extents = {0};
      hb_font_get_glyph_extents (font, gid, &extents);
      char name[100];
      sprintf (name, "%d.svg", gid);
      FILE *f = fopen (name, "wb");
      int factor = 1;
      if (extents.height < 0) factor = -1;
      fprintf (f, "<svg xmlns=\"http://www.w3.org/2000/svg\""
		  " viewBox=\"0 0 %d %d\"><path d=\"", extents.width, extents.height * factor);
      for (unsigned i = 0; i < points_len; ++i)
      {
	if (points[i].cmd == 'Z') fprintf (f, "Z");
	else fprintf (f, "%c%d,%d", points[i].cmd, points[i].x, (points[i].y + extents.height) * factor);
      }
      fprintf (f, "\"/></svg>");
      fclose (f);
    }
    hb_font_destroy (font);
    hb_face_destroy (face);
  }

  hb_blob_destroy (blob);

  return 0;
}
