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

/* ── Test 5: transformed glyph extents helper ───────────────────── */

static void
test_set_glyph_extents_with_transform (void)
{
  hb_raster_draw_t *rdr = hb_raster_draw_create ();

  hb_raster_draw_set_transform (rdr, 2.f, 0.f, 0.f, 3.f, 5.f, 7.f);

  hb_glyph_extents_t gext = {
    1,   /* x_bearing */
    4,   /* y_bearing */
    10,  /* width */
    -6   /* height */
  };

  g_assert_true (hb_raster_draw_set_glyph_extents (rdr, &gext));

  draw_rect (rdr, (float) gext.x_bearing,
	     (float) gext.y_bearing + gext.height,
	     (float) gext.x_bearing + gext.width,
	     (float) gext.y_bearing);

  hb_raster_image_t *img = hb_raster_draw_render (rdr);
  g_assert_nonnull (img);

  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);
  g_assert_cmpint (ext.x_origin, ==, 7);   /* 2*1 + 5 */
  g_assert_cmpint (ext.y_origin, ==, 1);   /* 3*(-2) + 7 */
  g_assert_cmpuint (ext.width, ==, 20);    /* 2*10 */
  g_assert_cmpuint (ext.height, ==, 18);   /* 3*6 */

  hb_raster_image_destroy (img);
  hb_raster_draw_destroy (rdr);
}


/* ── main ────────────────────────────────────────────────────────── */

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_rectangle);
  hb_test_add (test_accumulate);
  hb_test_add (test_subpixel_edge);
  hb_test_add (test_transform);
  hb_test_add (test_set_glyph_extents_with_transform);

  return hb_test_run ();
}
