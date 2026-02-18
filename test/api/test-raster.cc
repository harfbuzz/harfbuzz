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

#include "hb-test.h"
#include "hb-raster.h"

#include <stdio.h>


/* ── Helpers ─────────────────────────────────────────────────────── */

/* Draw a closed rectangle by emitting raw draw callbacks. */
static void
draw_rect (hb_raster_draw_t *rdr,
	   float x0, float y0, float x1, float y1)
{
  hb_draw_funcs_t *df = hb_raster_draw_get_funcs ();
  hb_draw_state_t  st = HB_DRAW_STATE_DEFAULT;

  hb_draw_move_to   (df, rdr, &st, x0, y0);
  hb_draw_line_to   (df, rdr, &st, x1, y0);
  hb_draw_line_to   (df, rdr, &st, x1, y1);
  hb_draw_line_to   (df, rdr, &st, x0, y1);
  hb_draw_close_path (df, rdr, &st);  /* emits closing line_to(x0,y0) */
}

/* Read one A8 pixel value; returns 0 for out-of-bounds coordinates. */
static uint8_t
pixel_at (hb_raster_image_t *img, int x, int y)
{
  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);

  int col = x - ext.x_origin;
  int row = y - ext.y_origin;

  if (col < 0 || row < 0 ||
      (unsigned) col >= ext.width || (unsigned) row >= ext.height)
    return 0;

  return hb_raster_image_get_buffer (img)[row * ext.stride + col];
}


/* ── Test 1: rectangle geometry ──────────────────────────────────── */

static void
test_rectangle (void)
{
  hb_raster_draw_t *rdr = hb_raster_draw_create ();

  draw_rect (rdr, 2.f, 2.f, 30.f, 30.f);

  hb_raster_image_t *img = hb_raster_draw_render (rdr);
  g_assert_nonnull (img);

  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);
  g_assert_cmpint (ext.x_origin, ==, 2);
  g_assert_cmpint (ext.y_origin, ==, 2);
  g_assert_cmpuint (ext.width, ==, 28);
  g_assert_cmpuint (ext.height, ==, 28);

  /* Center: fully inside → full coverage */
  g_assert_cmpint (pixel_at (img, 16, 16), ==, 255);

  /* Well outside the box → zero */
  g_assert_cmpint (pixel_at (img, 0, 0), ==, 0);   /* out of image */
  g_assert_cmpint (pixel_at (img, 31, 31), ==, 0);  /* out of image */

  /* Left-most and right-most pixel columns: still fully inside
     because integer-coordinate box edges align with pixel boundaries */
  g_assert_cmpint (pixel_at (img, 2, 16), ==, 255);
  g_assert_cmpint (pixel_at (img, 29, 16), ==, 255);

  /* One pixel beyond the box right edge: out of image */
  g_assert_cmpint (pixel_at (img, 30, 16), ==, 0);

  hb_raster_image_destroy (img);

  /* render() with no accumulated edges → empty (0×0) image */
  img = hb_raster_draw_render (rdr);
  g_assert_nonnull (img);
  hb_raster_image_get_extents (img, &ext);
  g_assert_cmpuint (ext.width, ==, 0);
  g_assert_cmpuint (ext.height, ==, 0);
  hb_raster_image_destroy (img);

  hb_raster_draw_destroy (rdr);
}


/* ── Test 2: multi-glyph accumulation ────────────────────────────── */

static void
test_accumulate (void)
{
  hb_raster_draw_t *rdr = hb_raster_draw_create ();

  /* Two non-overlapping boxes accumulated before a single render() */
  draw_rect (rdr, 0.f, 0.f, 10.f, 10.f);
  draw_rect (rdr, 20.f, 0.f, 30.f, 10.f);

  hb_raster_image_t *img = hb_raster_draw_render (rdr);
  g_assert_nonnull (img);

  /* Both interiors fully covered */
  g_assert_cmpint (pixel_at (img, 5,  5), ==, 255);
  g_assert_cmpint (pixel_at (img, 25, 5), ==, 255);

  /* Gap between the two boxes is empty */
  g_assert_cmpint (pixel_at (img, 15, 5), ==, 0);

  hb_raster_image_destroy (img);
  hb_raster_draw_destroy (rdr);
}


/* ── Test 3: sub-pixel edge coverage ─────────────────────────────── */

static void
test_subpixel_edge (void)
{
  hb_raster_draw_t *rdr = hb_raster_draw_create ();

  /* Box with a half-pixel offset so the leftmost pixel column is split */
  draw_rect (rdr, 2.5f, 2.f, 30.f, 30.f);

  hb_raster_image_t *img = hb_raster_draw_render (rdr);
  g_assert_nonnull (img);

  /* The pixel at x=2 straddles the left edge → partial coverage */
  int v = pixel_at (img, 2, 16);
  g_assert_cmpint (v, >, 0);
  g_assert_cmpint (v, <, 255);

  /* The pixel at x=3 is fully inside */
  g_assert_cmpint (pixel_at (img, 3, 16), ==, 255);

  hb_raster_image_destroy (img);
  hb_raster_draw_destroy (rdr);
}


/* ── Test 4: transform ───────────────────────────────────────────── */

static void
test_transform (void)
{
  hb_raster_draw_t *rdr = hb_raster_draw_create ();

  /* Scale by 2 — same unit-square box maps to (0,0)→(20,20) */
  hb_raster_draw_set_transform (rdr, 2.f, 0.f, 0.f, 2.f, 0.f, 0.f);
  draw_rect (rdr, 0.f, 0.f, 10.f, 10.f);

  hb_raster_image_t *img = hb_raster_draw_render (rdr);
  g_assert_nonnull (img);

  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);
  g_assert_cmpuint (ext.width, ==, 20);
  g_assert_cmpuint (ext.height, ==, 20);
  g_assert_cmpint (pixel_at (img, 10, 10), ==, 255); /* center of scaled box */

  hb_raster_image_destroy (img);
  hb_raster_draw_destroy (rdr);
}


/* ── Test 5: glyph rendering ─────────────────────────────────────── */

static const char *glyph_font_path = nullptr; /* set from argv[1] if provided */

/* Write a PGM file (y-flipped so text reads correctly in viewers). */
static void
write_pgm (hb_raster_image_t *img, const char *path)
{
  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);

  if (!ext.width || !ext.height) return;

  FILE *f = fopen (path, "wb");
  if (!f) { fprintf (stderr, "cannot write %s\n", path); return; }

  fprintf (f, "P5\n%u %u\n255\n", ext.width, ext.height);
  const uint8_t *buf = hb_raster_image_get_buffer (img);
  for (unsigned row = 0; row < ext.height; row++)
    fwrite (buf + (ext.height - 1 - row) * ext.stride, 1, ext.width, f);
  fclose (f);

  g_test_message ("wrote %s (%ux%u at %d,%d)",
		  path, ext.width, ext.height, ext.x_origin, ext.y_origin);
}

static void
test_glyph (void)
{
  if (!glyph_font_path) { g_test_skip ("no font specified"); return; }

  hb_blob_t *blob = hb_blob_create_from_file_or_fail (glyph_font_path);
  if (!blob) { g_test_skip ("font not found"); return; }
  hb_face_t *face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  hb_font_t *font = hb_font_create (face);
  unsigned upem    = hb_face_get_upem (face);
  unsigned nglyphs = hb_face_get_glyph_count (face);
  hb_face_destroy (face);
  hb_font_set_scale (font, (int) upem, (int) upem);

  hb_raster_draw_t *rdr = hb_raster_draw_create ();
  unsigned rendered = 0;
  for (hb_codepoint_t gid = 0; gid < (nglyphs < 20u ? nglyphs : 20u); gid++)
  {
    if (!hb_font_draw_glyph_or_fail (font, gid, hb_raster_draw_get_funcs (), rdr))
      continue;

    hb_raster_image_t *img = hb_raster_draw_render (rdr);
    g_assert_nonnull (img);

    hb_raster_extents_t ext;
    hb_raster_image_get_extents (img, &ext);

    if (ext.width && ext.height)
    {
      /* Verify the image has some non-zero pixels */
      const uint8_t *buf = hb_raster_image_get_buffer (img);
      unsigned nz = 0;
      for (unsigned i = 0; i < ext.stride * ext.height; i++)
	if (buf[i]) nz++;
      g_assert_cmpuint (nz, >, 0);
      rendered++;

      char path[64];
      snprintf (path, sizeof path, "/tmp/hb-raster-gid%u.pgm", gid);
      write_pgm (img, path);
    }

    hb_raster_image_destroy (img);
  }

  g_assert_cmpuint (rendered, >, 0);

  hb_raster_draw_destroy (rdr);
  hb_font_destroy (font);
}


/* ── main ────────────────────────────────────────────────────────── */

int
main (int argc, char **argv)
{
  /* Extract a trailing non-flag argument as the font path before
     hb_test_init() can swallow it as a test-path filter. */
  if (argc > 1 && argv[argc - 1][0] != '-')
  {
    glyph_font_path = argv[argc - 1];
    argv[--argc] = nullptr;
  }

  hb_test_init (&argc, &argv);

  hb_test_add (test_rectangle);
  hb_test_add (test_accumulate);
  hb_test_add (test_subpixel_edge);
  hb_test_add (test_transform);
  hb_test_add (test_glyph);

  return hb_test_run ();
}
