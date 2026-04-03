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
#include "hb-gpu.h"


static void
draw_rect (hb_draw_funcs_t *funcs,
	   void            *data,
	   float            x0,
	   float            y0,
	   float            x1,
	   float            y1)
{
  hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;

  hb_draw_move_to (funcs, data, &st, x0, y0);
  hb_draw_line_to (funcs, data, &st, x1, y0);
  hb_draw_line_to (funcs, data, &st, x1, y1);
  hb_draw_line_to (funcs, data, &st, x0, y1);
  hb_draw_close_path (funcs, data, &st);
}

static void
draw_test_shape_gpu (hb_gpu_draw_t *draw)
{
  hb_draw_funcs_t *funcs = hb_gpu_draw_get_funcs ();

  draw_rect (funcs, draw, 1.f, 1.f, 4.f, 3.f);
  draw_rect (funcs, draw, 6.f, 0.f, 8.f, 2.f);
}

#ifndef HB_NO_RASTER_GPU
static uint8_t
pixel_at (hb_raster_image_t *img,
	  int                x,
	  int                y)
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
#endif

static void
test_render_from_gpu_empty (void)
{
#ifdef HB_NO_RASTER_GPU
  g_assert_null (hb_raster_gpu_render_from_blob_or_fail (hb_blob_get_empty ()));
#else
  hb_raster_image_t *img = hb_raster_gpu_render_from_blob_or_fail (hb_blob_get_empty ());
  g_assert_nonnull (img);

  hb_raster_extents_t ext;
  hb_raster_image_get_extents (img, &ext);
  g_assert_cmpuint (ext.width, ==, 0);
  g_assert_cmpuint (ext.height, ==, 0);

  hb_raster_image_destroy (img);
#endif
}

static void
test_render_from_gpu_invalid (void)
{
  hb_blob_t *blob = hb_blob_create_or_fail ("\0\0\0", 3,
					    HB_MEMORY_MODE_READONLY,
					    nullptr, nullptr);
  g_assert_nonnull (blob);
  g_assert_null (hb_raster_gpu_render_from_blob_or_fail (blob));
  hb_blob_destroy (blob);
}

static void
test_render_from_gpu_roundtrip (void)
{
#ifdef HB_NO_RASTER_GPU
  hb_gpu_draw_t *gpu = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (gpu);
  draw_test_shape_gpu (gpu);

  hb_blob_t *blob = hb_gpu_draw_encode (gpu);
  g_assert_nonnull (blob);
  g_assert_null (hb_raster_gpu_render_from_blob_or_fail (blob));

  hb_blob_destroy (blob);
  hb_gpu_draw_destroy (gpu);
#else
  hb_gpu_draw_t *gpu = hb_gpu_draw_create_or_fail ();
  g_assert_nonnull (gpu);

  draw_test_shape_gpu (gpu);

  hb_blob_t *blob = hb_gpu_draw_encode (gpu);
  g_assert_nonnull (blob);

  hb_raster_image_t *from_gpu = hb_raster_gpu_render_from_blob_or_fail (blob);
  g_assert_nonnull (from_gpu);

  hb_raster_extents_t ext;
  hb_raster_image_get_extents (from_gpu, &ext);
  g_assert_cmpint (ext.x_origin, ==, 0);
  g_assert_cmpint (ext.y_origin, ==, -1);
  g_assert_cmpuint (ext.width, ==, 9);
  g_assert_cmpuint (ext.height, ==, 5);

  static const uint8_t expected[5][9] = {
    {0, 0, 0, 0, 0, 0, 43, 43, 0},
    {0, 43, 43, 43, 0, 43, 191, 191, 42},
    {43, 191, 213, 191, 43, 43, 191, 191, 42},
    {43, 191, 213, 191, 43, 0, 42, 42, 0},
    {0, 42, 42, 42, 0, 0, 0, 0, 0},
  };

  for (unsigned y = 0; y < ext.height; y++)
    for (unsigned x = 0; x < ext.width; x++)
      g_assert_cmpuint (pixel_at (from_gpu,
				  ext.x_origin + x,
				  ext.y_origin + y),
			==,
			expected[y][x]);

  hb_raster_image_destroy (from_gpu);
  hb_blob_destroy (blob);
  hb_gpu_draw_destroy (gpu);
#endif
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_render_from_gpu_empty);
  hb_test_add (test_render_from_gpu_invalid);
  hb_test_add (test_render_from_gpu_roundtrip);

  return hb_test_run ();
}
