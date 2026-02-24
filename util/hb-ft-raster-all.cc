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

#include "hb.hh"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_BBOX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>


static inline int
floor_div_64 (FT_Pos v)
{
  return v >= 0 ? (int) (v / 64) : -(int) ((-v + 63) / 64);
}

static inline int
ceil_div_64 (FT_Pos v)
{
  return v >= 0 ? (int) ((v + 63) / 64) : -(int) ((-v) / 64);
}

static void
write_ppm (const std::vector<uint8_t> &pixels,
	   unsigned width, unsigned height, unsigned stride,
	   const char *dir, unsigned gid)
{
  if (!width || !height) return;

  char path[512];
  snprintf (path, sizeof path, "%s/%05u.ppm", dir, gid);
  FILE *f = fopen (path, "wb");
  if (!f) { fprintf (stderr, "cannot write %s\n", path); return; }

  fprintf (f, "P6\n%u %u\n255\n", width, height);
  uint8_t rgb[3];
  for (unsigned row = 0; row < height; row++)
  {
    const uint8_t *src = pixels.data () + row * stride;
    for (unsigned x = 0; x < width; x++)
    {
      rgb[0] = rgb[1] = rgb[2] = (uint8_t) (255 - src[x]);
      fwrite (rgb, 1, 3, f);
    }
  }
  fclose (f);
}

int
main (int argc, char **argv)
{
  unsigned num_iterations = 1;
  int argi = 1;
  for (; argi < argc; argi++)
  {
    const char *arg = argv[argi];
    if (strcmp (arg, "--") == 0)
    {
      argi++;
      break;
    }
    if (arg[0] != '-')
      break;
    if (strcmp (arg, "-h") == 0 || strcmp (arg, "--help") == 0)
    {
      fprintf (stderr, "Usage: %s [-n N|--num-iterations N] font-file [font-size] [output-dir]\n", argv[0]);
      return 0;
    }
    if (strcmp (arg, "-n") == 0 || strcmp (arg, "--num-iterations") == 0)
    {
      if (++argi >= argc)
      {
	fprintf (stderr, "Missing value for %s\n", arg);
	return 1;
      }
      char *end = nullptr;
      long n = strtol (argv[argi], &end, 10);
      if (!argv[argi][0] || end == argv[argi] || *end || n <= 0)
      {
	fprintf (stderr, "Invalid --num-iterations: %s\n", argv[argi]);
	return 1;
      }
      num_iterations = (unsigned) n;
      continue;
    }

    fprintf (stderr, "Unknown option: %s\n", arg);
    return 1;
  }

  if (argc - argi < 1 || argc - argi > 3)
  {
    fprintf (stderr, "Usage: %s [-n N|--num-iterations N] font-file [font-size] [output-dir]\n", argv[0]);
    return 1;
  }

  hb_face_t *hb_face = hb_face_create_from_file_or_fail (argv[argi], 0);
  if (!hb_face) { fprintf (stderr, "Failed to open font\n"); return 1; }

  unsigned upem = hb_face_get_upem (hb_face);
  int font_size = (argc - argi > 1) ? atoi (argv[argi + 1]) : (int) upem;
  if (font_size <= 0) font_size = (int) upem;
  const char *outdir = (argc - argi > 2) ? argv[argi + 2] : nullptr;

  FT_Library ft_lib = nullptr;
  FT_Face ft_face = nullptr;
  if (FT_Init_FreeType (&ft_lib) != 0 ||
      FT_New_Face (ft_lib, argv[argi], 0, &ft_face) != 0)
  {
    fprintf (stderr, "Failed to initialize FreeType face\n");
    hb_face_destroy (hb_face);
    if (ft_face) FT_Done_Face (ft_face);
    if (ft_lib) FT_Done_FreeType (ft_lib);
    return 1;
  }

  FT_Set_Char_Size (ft_face, (FT_F26Dot6) font_size * 64, (FT_F26Dot6) font_size * 64, 72, 72);

  unsigned glyph_count = hb_face_get_glyph_count (hb_face);
  for (unsigned gid = 0; gid < glyph_count; gid++)
  {
    if (FT_Load_Glyph (ft_face, gid, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) != 0)
      continue;
    if (ft_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
      continue;

    FT_Outline *outline = &ft_face->glyph->outline;
    FT_BBox bbox;
    FT_Outline_Get_BBox (outline, &bbox);

    int x0 = floor_div_64 (bbox.xMin);
    int y0 = floor_div_64 (bbox.yMin);
    int x1 = ceil_div_64 (bbox.xMax);
    int y1 = ceil_div_64 (bbox.yMax);
    unsigned width  = (unsigned) (x1 > x0 ? x1 - x0 : 0);
    unsigned height = (unsigned) (y1 > y0 ? y1 - y0 : 0);
    if (!width || !height)
      continue;

    unsigned stride = (width + 3u) & ~3u;
    std::vector<uint8_t> pixels (stride * height, 0);

    for (unsigned iter = 0; iter < num_iterations; iter++)
    {
      std::fill (pixels.begin (), pixels.end (), 0);
      if (FT_Load_Glyph (ft_face, gid, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) != 0)
	break;
      if (ft_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
	break;

      outline = &ft_face->glyph->outline;
      FT_Outline_Translate (outline, -(FT_Pos) x0 * 64, -(FT_Pos) y0 * 64);

      FT_Bitmap bitmap;
      FT_Bitmap_Init (&bitmap);
      bitmap.rows       = height;
      bitmap.width      = width;
      bitmap.pitch      = (int) stride;
      bitmap.buffer     = pixels.data ();
      bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
      bitmap.num_grays  = 256;

      FT_Raster_Params params = {};
      params.target = &bitmap;
      params.flags  = FT_RASTER_FLAG_AA;
      FT_Outline_Render (ft_lib, outline, &params);

      if (outdir && iter + 1 == num_iterations)
	write_ppm (pixels, width, height, stride, outdir, gid);
    }
  }

  FT_Done_Face (ft_face);
  FT_Done_FreeType (ft_lib);
  hb_face_destroy (hb_face);
  return 0;
}
