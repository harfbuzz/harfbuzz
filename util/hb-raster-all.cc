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
#include <hb-ot.h>
#include "hb-raster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


static void
write_ppm (hb_raster_image_t *img, const char *dir, unsigned gid)
{
  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);
  if (!ext.width || !ext.height) return;

  char path[512];
  snprintf (path, sizeof path, "%s/%05u.ppm", dir, gid);
  FILE *f = fopen (path, "wb");
  if (!f) { fprintf (stderr, "cannot write %s\n", path); return; }

  const uint8_t *buf = hb_raster_image_get_buffer (img);
  hb_raster_format_t fmt = hb_raster_image_get_format (img);

  fprintf (f, "P6\n%u %u\n255\n", ext.width, ext.height);
  uint8_t rgb[3];
  for (unsigned row = 0; row < ext.height; row++)
  {
    const uint8_t *src = buf + (ext.height - 1 - row) * ext.stride;
    for (unsigned x = 0; x < ext.width; x++)
    {
      if (fmt == HB_RASTER_FORMAT_A8)
      {
	rgb[0] = rgb[1] = rgb[2] = (uint8_t) (255 - src[x]);
      }
      else /* BGRA32 — composite over white */
      {
	uint32_t px;
	memcpy (&px, src + x * 4, 4);
	uint8_t b = (uint8_t) (px & 0xFF);
	uint8_t g = (uint8_t) ((px >> 8) & 0xFF);
	uint8_t r = (uint8_t) ((px >> 16) & 0xFF);
	uint8_t a = (uint8_t) (px >> 24);
	unsigned inv_a = 255 - a;
	rgb[0] = (uint8_t) (r + ((255 * inv_a + 128 + ((255 * inv_a + 128) >> 8)) >> 8));
	rgb[1] = (uint8_t) (g + ((255 * inv_a + 128 + ((255 * inv_a + 128) >> 8)) >> 8));
	rgb[2] = (uint8_t) (b + ((255 * inv_a + 128 + ((255 * inv_a + 128) >> 8)) >> 8));
      }
      fwrite (rgb, 1, 3, f);
    }
  }
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

  bool has_color = hb_ot_color_has_paint (face) ||
		   hb_ot_color_has_layers (face) ||
		   hb_ot_color_has_png (face);

  hb_raster_draw_t  *rdr = hb_raster_draw_create ();
  hb_raster_paint_t *pnt = has_color ? hb_raster_paint_create () : nullptr;
  unsigned glyph_count = hb_face_get_glyph_count (face);

  for (unsigned gid = 0; gid < glyph_count; gid++)
  {
    hb_raster_image_t *img = nullptr;

    if (pnt)
    {
      hb_glyph_extents_t gext;
      hb_raster_extents_t ext;
      if (hb_font_get_glyph_extents (font, gid, &gext) &&
	  hb_raster_extents_from_glyph_extents (&gext, &ext))
      {
	hb_raster_paint_set_extents (pnt, &ext);

	hb_bool_t painted = hb_font_paint_glyph_or_fail (font, gid,
							  hb_raster_paint_get_funcs (), pnt,
							  0, HB_COLOR (0, 0, 0, 255));
	img = hb_raster_paint_render (pnt);
	if (!painted && img)
	{
	  hb_raster_paint_recycle_image (pnt, img);
	  img = nullptr;
	}
      }

      if (img)
      {
	if (outdir)
	  write_ppm (img, outdir, gid);
	hb_raster_paint_recycle_image (pnt, img);
	continue;
      }
    }

    if (!hb_font_draw_glyph_or_fail (font, gid, hb_raster_draw_get_funcs (), rdr))
      continue;

    img = hb_raster_draw_render (rdr);
    if (img)
    {
      if (outdir)
	write_ppm (img, outdir, gid);
      hb_raster_draw_recycle_image (rdr, img);
    }
  }

  hb_raster_paint_destroy (pnt);
  hb_raster_draw_destroy (rdr);
  hb_font_destroy (font);
  hb_face_destroy (face);
  return 0;
}
