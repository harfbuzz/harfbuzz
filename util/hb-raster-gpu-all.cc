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
#include <hb-gpu.h>
#include <hb-raster.h>

#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void
write_ppm (hb_raster_image_t *img, const char *dir, unsigned gid)
{
  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);
  if (!ext.width || !ext.height) return;

  char path[512];
  snprintf (path, sizeof path, "%s/%05u.ppm", dir, gid);
  FILE *f = fopen (path, "wb");
  if (!f)
  {
    fprintf (stderr, "cannot write %s\n", path);
    return;
  }

  const uint8_t *buf = hb_raster_image_get_buffer (img);
  fprintf (f, "P6\n%u %u\n255\n", ext.width, ext.height);

  uint8_t rgb[3];
  for (unsigned row = 0; row < ext.height; row++)
  {
    const uint8_t *src = buf + (ext.height - 1 - row) * ext.stride;
    for (unsigned x = 0; x < ext.width; x++)
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
#ifdef HB_NO_RASTER_GPU
  fprintf (stderr, "%s was built with HB_NO_RASTER_GPU\n", argv[0]);
  return 1;
#else
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

  hb_face_t *face = hb_face_create_from_file_or_fail (argv[argi], 0);
  if (!face)
  {
    fprintf (stderr, "Failed to open font\n");
    return 1;
  }

  unsigned upem = hb_face_get_upem (face);
  int font_size = (argc - argi > 1) ? atoi (argv[argi + 1]) : (int) upem;
  if (font_size <= 0) font_size = (int) upem;
  const char *outdir = (argc - argi > 2) ? argv[argi + 2] : nullptr;

  hb_font_t *font = hb_font_create (face);
  hb_font_set_scale (font, font_size, font_size);

  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  if (!draw)
  {
    fprintf (stderr, "Failed to create GPU draw.\n");
    hb_font_destroy (font);
    hb_face_destroy (face);
    return 1;
  }

  unsigned glyph_count = hb_face_get_glyph_count (face);
  unsigned num_rendered = 0;
  unsigned num_empty = 0;
  unsigned num_failed = 0;
  uint64_t total_blob_bytes = 0;
  uint64_t total_pixels = 0;
  unsigned max_blob_bytes = 0;
  unsigned max_blob_gid = 0;
  uint64_t outline_ns = 0;
  uint64_t encode_ns = 0;
  uint64_t render_ns = 0;
  bool failed = false;

  typedef std::chrono::steady_clock clock;
  clock::time_point wall_start = clock::now ();

  for (unsigned iter = 0; iter < num_iterations; iter++)
    for (unsigned gid = 0; gid < glyph_count; gid++)
    {
      hb_gpu_draw_reset (draw);

      clock::time_point t0 = clock::now ();
      hb_gpu_draw_glyph (draw, font, gid);
      clock::time_point t1 = clock::now ();

      hb_blob_t *blob = hb_gpu_draw_encode (draw);
      clock::time_point t2 = clock::now ();

      outline_ns += std::chrono::duration_cast<std::chrono::nanoseconds> (t1 - t0).count ();
      encode_ns  += std::chrono::duration_cast<std::chrono::nanoseconds> (t2 - t1).count ();

      if (!blob)
      {
	if (iter == 0)
	{
	  fprintf (stderr, "gid %u: encode failed\n", gid);
	  num_failed++;
	}
	failed = true;
	continue;
      }

      unsigned blob_length = hb_blob_get_length (blob);
      if (iter == 0)
      {
	total_blob_bytes += blob_length;
	if (blob_length == 0)
	  num_empty++;
	else
	{
	  num_rendered++;
	  if (blob_length > max_blob_bytes)
	  {
	    max_blob_bytes = blob_length;
	    max_blob_gid = gid;
	  }
	}
      }

      clock::time_point t3 = clock::now ();
      hb_raster_image_t *img = hb_raster_gpu_render_from_blob_or_fail (blob);
      clock::time_point t4 = clock::now ();
      render_ns += std::chrono::duration_cast<std::chrono::nanoseconds> (t4 - t3).count ();

      hb_gpu_draw_recycle_blob (draw, blob);

      if (!img)
      {
	if (iter == 0)
	{
	  fprintf (stderr, "gid %u: raster render failed\n", gid);
	  num_failed++;
	}
	failed = true;
	continue;
      }

      if (iter == 0)
      {
	hb_raster_extents_t ext;
	hb_raster_image_get_extents (img, &ext);
	total_pixels += (uint64_t) ext.width * ext.height;
      }

      if (outdir && iter + 1 == num_iterations)
	write_ppm (img, outdir, gid);

      hb_raster_image_destroy (img);
    }

  uint64_t wall_ns =
    std::chrono::duration_cast<std::chrono::nanoseconds> (clock::now () - wall_start).count ();
  uint64_t total_glyphs = (uint64_t) glyph_count * num_iterations;

  printf ("font:     %s\n", argv[argi]);
  printf ("glyphs:   %u\n", glyph_count);
  printf ("rendered: %u\n", num_rendered);
  printf ("empty:    %u\n", num_empty);
  if (num_failed)
    printf ("FAILED:   %u\n", num_failed);
  printf ("blob:     %.2f KiB\n", total_blob_bytes / 1024.);
  if (num_rendered)
  {
    printf ("avg:      %.2f KiB/glyph (non-empty)\n",
	    total_blob_bytes / 1024. / num_rendered);
    printf ("max:      %.2f KiB (gid %u)\n",
	    max_blob_bytes / 1024., max_blob_gid);
  }
  printf ("pixels:   %.2f Mpx\n", total_pixels / 1000000.);
  printf ("outline:  %.3fms (%.1fus/glyph)\n",
	  outline_ns / 1e6 / num_iterations,
	  total_glyphs ? outline_ns / 1e3 / total_glyphs : 0.);
  printf ("encode:   %.3fms (%.1fus/glyph)\n",
	  encode_ns / 1e6 / num_iterations,
	  total_glyphs ? encode_ns / 1e3 / total_glyphs : 0.);
  printf ("render:   %.3fms (%.1fus/glyph)\n",
	  render_ns / 1e6 / num_iterations,
	  total_glyphs ? render_ns / 1e3 / total_glyphs : 0.);
  printf ("wall:     %.3fms (%.0f glyphs/s)\n",
	  wall_ns / 1e6 / num_iterations,
	  wall_ns ? total_glyphs * 1e9 / wall_ns : 0.);

  hb_gpu_draw_destroy (draw);
  hb_font_destroy (font);
  hb_face_destroy (face);

  return failed ? 1 : 0;
#endif
}
