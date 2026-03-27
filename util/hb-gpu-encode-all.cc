/*
 * Copyright (C) 2026  Behdad Esfahbod
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
#include <hb-gpu.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>

int
main (int argc, char **argv)
{
  if (argc < 2 || !strcmp (argv[1], "-h") || !strcmp (argv[1], "--help"))
  {
    fprintf (stderr, "Usage: %s FONTFILE\n\n"
		     "Encode all glyphs and report failures and sizes.\n",
	     argv[0]);
    return argc < 2 ? 1 : 0;
  }

  const char *font_path = argv[1];

  hb_blob_t *blob = hb_blob_create_from_file_or_fail (font_path);
  if (!blob)
  {
    fprintf (stderr, "Failed to open font file: %s\n", font_path);
    return 1;
  }

  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);
  unsigned glyph_count = hb_face_get_glyph_count (face);

  if (!glyph_count)
  {
    fprintf (stderr, "Font has no glyphs.\n");
    hb_font_destroy (font);
    hb_face_destroy (face);
    hb_blob_destroy (blob);
    return 1;
  }

  hb_gpu_draw_t *draw = hb_gpu_draw_create_or_fail ();
  if (!draw)
  {
    fprintf (stderr, "Failed to create GPU draw.\n");
    hb_font_destroy (font);
    hb_face_destroy (face);
    hb_blob_destroy (blob);
    return 1;
  }

  unsigned num_encoded = 0;
  unsigned num_empty = 0;
  unsigned num_failed = 0;
  uint64_t total_bytes = 0;

  for (unsigned gid = 0; gid < glyph_count; gid++)
  {
    hb_gpu_draw_reset (draw);
    hb_gpu_draw_glyph (draw, font, gid);

    hb_blob_t *encoded = hb_gpu_draw_encode (draw);
    if (!encoded)
    {
      fprintf (stderr, "gid %u: encode failed\n", gid);
      num_failed++;
      continue;
    }

    unsigned len = hb_blob_get_length (encoded);
    if (len == 0)
      num_empty++;
    else
      num_encoded++;

    total_bytes += len;
    hb_blob_destroy (encoded);
  }

  printf ("font:     %s\n", font_path);
  printf ("glyphs:   %u\n", glyph_count);
  printf ("encoded:  %u\n", num_encoded);
  printf ("empty:    %u\n", num_empty);
  if (num_failed)
    printf ("FAILED:   %u\n", num_failed);
  printf ("total:    %.2f KiB\n", total_bytes / 1024.);
  if (num_encoded)
    printf ("avg:      %.2f KiB/glyph (non-empty)\n",
	    total_bytes / 1024. / num_encoded);

  hb_gpu_draw_destroy (draw);
  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return num_failed ? 1 : 0;
}
