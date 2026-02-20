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
 *
 * Author(s): Behdad Esfahbod
 */

#include <hb.h>
#include "hb-raster.h"

#include <stdio.h>
#include <stdlib.h>


static void
write_pgm (hb_raster_image_t *img, const char *dir, unsigned gid)
{
  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);
  if (!ext.width || !ext.height) return;

  char path[512];
  snprintf (path, sizeof path, "%s/%05u.pgm", dir, gid);
  FILE *f = fopen (path, "wb");
  if (!f) { fprintf (stderr, "cannot write %s\n", path); return; }

  const uint8_t *buf = hb_raster_image_get_buffer (img);
  fprintf (f, "P5\n%u %u\n255\n", ext.width, ext.height);
  for (unsigned row = 0; row < ext.height; row++)
    fwrite (buf + (ext.height - 1 - row) * ext.stride, 1, ext.width, f);
  fclose (f);
}

int
main (int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf (stderr, "Usage: %s font-file [font-size] [output-dir]\n", argv[0]);
    return 1;
  }

  hb_face_t *face = hb_face_create_from_file_or_fail (argv[1], 0);
  if (!face) { fprintf (stderr, "Failed to open font\n"); return 1; }

  unsigned upem = hb_face_get_upem (face);

  int font_size = (argc > 2) ? atoi (argv[2]) : (int) upem;
  if (font_size <= 0) font_size = (int) upem;

  const char *outdir = (argc > 3) ? argv[3] : nullptr;

  hb_font_t *font = hb_font_create (face);
  hb_font_set_scale (font, font_size, font_size);

  hb_raster_draw_t *rdr = hb_raster_draw_create ();
  unsigned glyph_count = hb_face_get_glyph_count (face);

  for (unsigned gid = 0; gid < glyph_count; gid++)
  {
    if (!hb_font_draw_glyph_or_fail (font, gid, hb_raster_draw_get_funcs (), rdr))
      continue;

    hb_raster_image_t *img = hb_raster_draw_render (rdr);
    if (img)
    {
      if (outdir)
	write_pgm (img, outdir, gid);
      hb_raster_image_destroy (img);
    }
  }

  hb_raster_draw_destroy (rdr);
  hb_font_destroy (font);
  hb_face_destroy (face);
  return 0;
}
