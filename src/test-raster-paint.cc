/*
 * Copyright © 2026  Behdad Esfahbod
 *
 * This is part of HarfBuzz, a text shaping library.
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

#include "hb-raster-paint.hh"

/* Clips to a box given as a path, which takes the mask route rather than
 * the rectangle fast path. */
static void
push_box_clip (hb_paint_funcs_t *funcs, hb_raster_paint_t *paint,
	       float x0, float y0, float x1, float y1)
{
  void *draw_data = nullptr;
  hb_draw_funcs_t *draw_funcs = hb_paint_push_clip_path_start (funcs, paint, &draw_data);
  hb_always_assert (draw_funcs);
  hb_draw_state_t state = HB_DRAW_STATE_DEFAULT;
  hb_draw_move_to (draw_funcs, draw_data, &state, x0, y0);
  hb_draw_line_to (draw_funcs, draw_data, &state, x1, y0);
  hb_draw_line_to (draw_funcs, draw_data, &state, x1, y1);
  hb_draw_line_to (draw_funcs, draw_data, &state, x0, y1);
  hb_draw_close_path (draw_funcs, draw_data, &state);
  hb_paint_push_clip_path_end (funcs, paint);
}

/* A color line of one opaque color. */
static unsigned int
get_color_stops (hb_color_line_t *color_line HB_UNUSED,
		 void *color_line_data HB_UNUSED,
		 unsigned int start,
		 unsigned int *count,
		 hb_color_stop_t *color_stops,
		 void *user_data HB_UNUSED)
{
  if (count)
  {
    unsigned n = 0;
    for (unsigned i = start; i < 2 && n < *count; i++, n++)
    {
      color_stops[n].offset = (float) i;
      color_stops[n].is_foreground = false;
      color_stops[n].color = HB_COLOR (255, 255, 255, 255);
    }
    *count = n;
  }
  return 2;
}

static hb_paint_extend_t
get_extend (hb_color_line_t *color_line HB_UNUSED,
	    void *color_line_data HB_UNUSED,
	    void *user_data HB_UNUSED)
{
  return HB_PAINT_EXTEND_PAD;
}

int
main ()
{
  hb_raster_paint_t *paint = hb_raster_paint_create_or_fail ();
  hb_always_assert (paint);
  hb_paint_funcs_t *funcs = hb_raster_paint_get_funcs (paint);
  hb_raster_extents_t root = {0, 0, 64, 64, 0};
  hb_raster_extents_t small_extents = {0, 0, 4, 4, 0};
  hb_raster_extents_t large = {0, 0, 8192, 8192, 0};

  hb_raster_paint_set_extents (paint, &root);
  hb_paint_push_transform (funcs, paint, 1, 0, 0, 1, 0, 0);
  hb_paint_pop_transform (funcs, paint);
  hb_always_assert (paint->surface_stack.length == 1);
  int64_t initial_budget = paint->pixel_remaining;

  hb_raster_paint_set_extents (paint, &small_extents);
  int64_t before;
  for (unsigned i = 0; i < 2; i++)
  {
    /* A group is laid out as the active root is, whatever extents have
     * been set since.  A new surface is cleared whole; one from the
     * cache costs what was painted on it, here nothing. */
    before = paint->pixel_remaining;
    hb_paint_push_group (funcs, paint);
    hb_always_assert (paint->surface_stack.length == 2);
    hb_always_assert (paint->surface_stack.arrayZ[1]->extents.width == root.width &&
		      paint->surface_stack.arrayZ[1]->extents.height == root.height);
    hb_always_assert (paint->pixel_remaining ==
		      before - (i ? 0 : (int64_t) root.width * root.height));
    hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
  }

  /* A group costs what is painted on it: to composite it, and to clear
   * its surface for the next group to take it. */
  const uint8_t *root_pixels = hb_raster_image_get_buffer (paint->surface_stack.arrayZ[0]);
  unsigned root_stride = paint->surface_stack.arrayZ[0]->extents.stride;
  hb_paint_push_group (funcs, paint);
  hb_paint_push_clip_rectangle (funcs, paint, 8.f, 8.f, 16.f, 16.f);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  before = paint->pixel_remaining;
  hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
  hb_always_assert (before - paint->pixel_remaining == 8 * 8);
  for (unsigned y = 0; y < root.height; y++)
    for (unsigned x = 0; x < root.width; x++)
      hb_always_assert (root_pixels[y * root_stride + x * 4 + 3] ==
			(x >= 8 && x < 16 && y >= 8 && y < 16 ? 255 : 0));
  before = paint->pixel_remaining;
  hb_paint_push_group (funcs, paint);
  hb_always_assert (before - paint->pixel_remaining == 8 * 8);
  hb_always_assert (paint->surface_stack.arrayZ[1]->painted.is_empty ());

  /* Where the mode clears what is under a clear source, what is under
   * the group counts too: all of it is composited, and here cleared. */
  hb_paint_push_clip_rectangle (funcs, paint, 32.f, 32.f, 40.f, 48.f);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  before = paint->pixel_remaining;
  hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_IN);
  hb_always_assert (before - paint->pixel_remaining == (40 - 8) * (48 - 8));
  for (unsigned y = 0; y < root.height; y++)
    for (unsigned x = 0; x < root.width * 4; x++)
      hb_always_assert (!root_pixels[y * root_stride + x]);

  /* A group nothing was painted on leaves what is under it as it was, or
   * clears it, according to the mode; either way, all of it. */
  for (unsigned mode = HB_PAINT_COMPOSITE_MODE_CLEAR;
       mode <= HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY; mode++)
  {
    hb_paint_push_clip_rectangle (funcs, paint, 4.f, 4.f, 60.f, 60.f);
    hb_paint_color (funcs, paint, false, HB_COLOR (10, 200, 90, 128));
    hb_paint_pop_clip (funcs, paint);
    hb_paint_push_clip_rectangle (funcs, paint, 20.f, 10.f, 50.f, 30.f);
    hb_paint_color (funcs, paint, false, HB_COLOR (250, 20, 60, 77));
    hb_paint_pop_clip (funcs, paint);
    uint8_t under[64 * 64 * 4];
    for (unsigned y = 0; y < root.height; y++)
      hb_memcpy (under + y * root.width * 4, root_pixels + y * root_stride, root.width * 4);

    hb_paint_push_group (funcs, paint);
    hb_paint_pop_group (funcs, paint, (hb_paint_composite_mode_t) mode);
    bool clears = mode == HB_PAINT_COMPOSITE_MODE_CLEAR ||
		  mode == HB_PAINT_COMPOSITE_MODE_SRC ||
		  mode == HB_PAINT_COMPOSITE_MODE_SRC_IN ||
		  mode == HB_PAINT_COMPOSITE_MODE_DEST_IN ||
		  mode == HB_PAINT_COMPOSITE_MODE_SRC_OUT ||
		  mode == HB_PAINT_COMPOSITE_MODE_DEST_ATOP;
    for (unsigned y = 0; y < root.height; y++)
      for (unsigned x = 0; x < root.width * 4; x++)
	hb_always_assert (root_pixels[y * root_stride + x] ==
			  (clears ? 0 : under[y * root.width * 4 + x]));

    hb_paint_push_group (funcs, paint);
    hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_CLEAR);
  }

  /* Recharging must use the active root, without allocating the new size. */
  hb_raster_paint_set_extents (paint, &large);
  hb_always_assert (hb_paint_set_budget (funcs, paint, HB_BUDGET_DEFAULT));
  hb_always_assert (paint->pixel_remaining == initial_budget);

  /* An outline can surround the small visible surface while extending far
   * past the automatic dimension cap.  Rasterize its visible part: a mask
   * starting at -10000 and truncated to 4096 pixels misses the surface. */
  hb_raster_paint_clear (paint);
  hb_raster_paint_set_extents (paint, &root);
  push_box_clip (funcs, paint, -10000.f, -10000.f, 10000.f, 10000.f);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  hb_raster_image_t *image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  hb_raster_extents_t extents;
  hb_raster_image_get_extents (image, &extents);
  hb_always_assert (extents.width == root.width && extents.height == root.height);
  const uint8_t *pixels = hb_raster_image_get_buffer (image);
  for (unsigned y = 0; y < root.height; y++)
    for (unsigned x = 0; x < root.width * 4; x++)
      hb_always_assert (pixels[y * extents.stride + x] == 255);
  hb_always_assert (paint->pixel_remaining >= 0);
  hb_raster_image_destroy (image);

  /* A clip mask is built, and charged, over the box it can cover rather
   * than over the whole surface, so that many small clipped layers do not
   * each cost a pass over it. */
  hb_raster_extents_t wide = {0, 0, 1024, 1024, 0};
  hb_raster_paint_set_extents (paint, &wide);
  hb_paint_push_transform (funcs, paint, 1, 0, 0, 1, 0, 0);
  before = paint->pixel_remaining;
  push_box_clip (funcs, paint, 100.f, 100.f, 108.f, 108.f);
  hb_always_assert (before - paint->pixel_remaining < 16 * 8 * 8);

  /* An axis-aligned rectangle copies only what it shares with a mask... */
  before = paint->pixel_remaining;
  hb_paint_push_clip_rectangle (funcs, paint, 0.f, 0.f, 1024.f, 1024.f);
  hb_always_assert (before - paint->pixel_remaining == 8 * 8);

  /* ...and a transformed one tests only those pixels of its bounding box. */
  hb_paint_push_transform (funcs, paint, .6f, .8f, -.8f, .6f, 0, 0);
  before = paint->pixel_remaining;
  hb_paint_push_clip_rectangle (funcs, paint, -2048.f, -2048.f, 2048.f, 2048.f);
  hb_always_assert (before - paint->pixel_remaining == 8 * 8);
  hb_paint_pop_transform (funcs, paint);

  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  for (unsigned i = 0; i < 3; i++)
    hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_transform (funcs, paint);
  image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  hb_raster_image_get_extents (image, &extents);
  pixels = hb_raster_image_get_buffer (image);
  for (unsigned y = 0; y < wide.height; y++)
    for (unsigned x = 0; x < wide.width; x++)
      hb_always_assert (pixels[y * extents.stride + x * 4 + 3] ==
			(x >= 100 && x < 108 && y >= 100 && y < 108 ? 255 : 0));
  hb_raster_image_destroy (image);

  /* Cached masks are zero again, so a mask built in one, here for a
   * surface of another size, starts out empty. */
  hb_always_assert (paint->clip_cache.length == 3);
  for (const hb_raster_clip_t &clip : paint->clip_cache)
    for (uint8_t alpha : clip.alpha)
      hb_always_assert (!alpha);
  hb_raster_paint_set_extents (paint, &root);
  push_box_clip (funcs, paint, 8.f, 8.f, 24.f, 24.f);
  hb_always_assert (paint->clip_cache.length == 2);
  hb_always_assert (paint->clip_stack.arrayZ[paint->clip_stack.length - 1].alpha.length >
		    root.width * root.height);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  hb_raster_image_get_extents (image, &extents);
  pixels = hb_raster_image_get_buffer (image);
  for (unsigned y = 0; y < root.height; y++)
    for (unsigned x = 0; x < root.width; x++)
      hb_always_assert (pixels[y * extents.stride + x * 4 + 3] ==
			(x >= 8 && x < 24 && y >= 8 && y < 24 ? 255 : 0));
  hb_raster_image_destroy (image);

  /* A mask is rendered only as far as the clip it is used within can
   * show it, so a layer costs what it paints, however large its outline... */
  hb_raster_paint_set_extents (paint, &wide);
  push_box_clip (funcs, paint, 100.f, 100.f, 108.f, 108.f);
  before = paint->pixel_remaining;
  push_box_clip (funcs, paint, -4096.f, -4096.f, 4096.f, 4096.f);
  hb_always_assert (before - paint->pixel_remaining < 16 * 8 * 8);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_clip (funcs, paint);

  /* ...and next to nothing within a clip that shows nothing. */
  push_box_clip (funcs, paint, 2000.f, 2000.f, 2008.f, 2008.f);
  before = paint->pixel_remaining;
  push_box_clip (funcs, paint, -4096.f, -4096.f, 4096.f, 4096.f);
  hb_always_assert (before - paint->pixel_remaining < 8 * 8);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_clip (funcs, paint);

  image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  hb_raster_image_get_extents (image, &extents);
  pixels = hb_raster_image_get_buffer (image);
  for (unsigned y = 0; y < wide.height; y++)
    for (unsigned x = 0; x < wide.width; x++)
      hb_always_assert (pixels[y * extents.stride + x * 4 + 3] ==
			(x >= 100 && x < 108 && y >= 100 && y < 108 ? 255 : 0));
  hb_raster_image_destroy (image);

  /* Every kind of paint notes what it paints, so that a group shows all
   * of it; and the surface of a group is clear again when the next takes
   * it, here to paint elsewhere. */
  static const uint8_t texels[2 * 2 * 4] = {255, 255, 255, 255, 255, 255, 255, 255,
					     255, 255, 255, 255, 255, 255, 255, 255};
  hb_blob_t *texels_blob = hb_blob_create ((const char *) texels, sizeof (texels),
					   HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_glyph_extents_t texels_extents = {0, 0, 128, 128};
  hb_color_line_t line;
  hb_memset (&line, 0, sizeof (line));
  line.get_color_stops = get_color_stops;
  line.get_extend = get_extend;
  for (unsigned kind = 0; kind < 5; kind++)
  {
    unsigned x0 = 8 + 4 * kind, y0 = 4 + 8 * kind;
    hb_raster_paint_set_extents (paint, &root);
    hb_paint_push_group (funcs, paint);
    hb_paint_push_clip_rectangle (funcs, paint, (float) x0, (float) y0,
				  (float) (x0 + 16), (float) (y0 + 16));
    switch (kind)
    {
    case 0: hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255)); break;
    case 1: hb_paint_linear_gradient (funcs, paint, &line, 0.f, 0.f, 64.f, 0.f, 0.f, 64.f); break;
    case 2: hb_paint_radial_gradient (funcs, paint, &line, 16.f, 16.f, 0.f, 16.f, 16.f, 32.f); break;
    case 3: hb_paint_sweep_gradient (funcs, paint, &line, 16.f, 16.f, 0.f, 6.f); break;
    case 4: hb_paint_image (funcs, paint, texels_blob, 2, 2, HB_PAINT_IMAGE_FORMAT_BGRA,
			    0.f, &texels_extents); break;
    }
    hb_paint_pop_clip (funcs, paint);
    before = paint->pixel_remaining;
    hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
    hb_always_assert (before - paint->pixel_remaining == 16 * 16);
    image = hb_raster_paint_render (paint);
    hb_always_assert (image);
    hb_raster_image_get_extents (image, &extents);
    pixels = hb_raster_image_get_buffer (image);
    for (unsigned y = 0; y < root.height; y++)
      for (unsigned x = 0; x < root.width; x++)
	hb_always_assert (!pixels[y * extents.stride + x * 4 + 3] ==
			  !(x >= x0 && x < x0 + 16 && y >= y0 && y < y0 + 16));
    hb_raster_paint_recycle_image (paint, image);
  }
  hb_blob_destroy (texels_blob);

  /* A background is painted all over the root, so a group that clears
   * what is under a clear source clears all of it. */
  hb_raster_paint_set_background (paint, HB_COLOR (255, 255, 255, 255));
  hb_raster_paint_set_extents (paint, &root);
  hb_paint_push_group (funcs, paint);
  hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_IN);
  hb_raster_paint_set_background (paint, HB_COLOR (0, 0, 0, 0));
  image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  for (uint8_t byte : image->buffer)
    hb_always_assert (!byte);
  hb_raster_image_destroy (image);

  /* What an image holds when it is recycled is not ours to know: the next
   * to take it clears all of it, not just what was painted on it here. */
  hb_raster_paint_set_extents (paint, &root);
  hb_paint_push_clip_rectangle (funcs, paint, 8.f, 8.f, 16.f, 16.f);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  hb_memset (image->buffer.arrayZ, 0xFF, image->buffer.length);
  hb_raster_paint_recycle_image (paint, image);
  hb_raster_paint_set_extents (paint, &root);
  image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  for (uint8_t byte : image->buffer)
    hb_always_assert (!byte);
  hb_raster_image_destroy (image);

  /* Nor is what a surface holds once it is laid out differently, here
   * with other row padding: what was painted on it is somewhere else. */
  hb_raster_paint_set_extents (paint, &root);
  hb_paint_push_group (funcs, paint);
  hb_paint_push_clip_rectangle (funcs, paint, 8.f, 8.f, 16.f, 16.f);
  hb_paint_color (funcs, paint, false, HB_COLOR (255, 255, 255, 255));
  hb_paint_pop_clip (funcs, paint);
  hb_paint_pop_group (funcs, paint, HB_PAINT_COMPOSITE_MODE_SRC_OVER);
  hb_raster_image_destroy (hb_raster_paint_render (paint));
  hb_raster_extents_t padded = {0, 0, 64, 64, 64 * 4 + 8};
  hb_raster_paint_set_extents (paint, &padded);
  image = hb_raster_paint_render (paint);
  hb_always_assert (image);
  hb_always_assert (image->extents.stride == padded.stride);
  for (uint8_t byte : image->buffer)
    hb_always_assert (!byte);
  hb_raster_image_destroy (image);

  hb_raster_paint_destroy (paint);
  return 0;
}
