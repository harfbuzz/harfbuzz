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

#include "hb-raster.hh"
#include "hb-geometry.hh"
#include "hb-machinery.hh"

#include <math.h>

/* hb_raster_clip_t — alpha mask for clipping */
struct hb_raster_clip_t
{
  hb_vector_t<uint8_t> alpha;	/* A8 mask, same extents as root surface */
  unsigned width  = 0;
  unsigned height = 0;
  unsigned stride = 0;

  /* Fast path: simple rectangle (no alpha buffer needed) */
  bool is_rect = true;
  int rect_x0 = 0, rect_y0 = 0;
  int rect_x1 = 0, rect_y1 = 0;

  /* Bounding box of non-zero alpha region (valid for both rect and mask) */
  unsigned min_x = 0, min_y = 0;
  unsigned max_x = 0, max_y = 0;

  void init_full (unsigned w, unsigned h)
  {
    width = w;
    height = h;
    stride = (w + 3u) & ~3u;
    is_rect = true;
    rect_x0 = 0;
    rect_y0 = 0;
    rect_x1 = (int) w;
    rect_y1 = (int) h;
    min_x = 0;
    min_y = 0;
    max_x = w;
    max_y = h;
  }

  void update_bounds_from_rect ()
  {
    min_x = (unsigned) hb_max (rect_x0, 0);
    min_y = (unsigned) hb_max (rect_y0, 0);
    max_x = (unsigned) hb_max (hb_min (rect_x1, (int) width), 0);
    max_y = (unsigned) hb_max (hb_min (rect_y1, (int) height), 0);
  }

  uint8_t get_alpha (unsigned x, unsigned y) const
  {
    if (is_rect)
      return ((int) x >= rect_x0 && (int) x < rect_x1 &&
	      (int) y >= rect_y0 && (int) y < rect_y1) ? 255 : 0;
    if (x >= width || y >= height) return 0;
    return alpha[y * stride + x];
  }
};


/* hb_raster_paint_t — color glyph paint context */
struct hb_raster_paint_t
{
  hb_object_header_t header;

  /* Configuration */
  hb_transform_t<>    base_transform     = {1, 0, 0, 1, 0, 0};
  hb_raster_extents_t fixed_extents      = {};
  bool                has_fixed_extents  = false;
  hb_color_t          foreground         = HB_COLOR (0, 0, 0, 255);

  /* Stacks */
  hb_vector_t<hb_transform_t<>>     transform_stack;
  hb_vector_t<hb_raster_clip_t>     clip_stack;
  hb_vector_t<hb_raster_image_t *>  surface_stack;

  /* Cached surface pool (freelist for reuse across push/pop group) */
  hb_vector_t<hb_raster_image_t *>  surface_cache;

  /* Internal rasterizer for clip-to-glyph */
  hb_raster_draw_t *clip_rdr = nullptr;

  /* Recycled output image */
  hb_raster_image_t *recycled_image = nullptr;

  /* Helpers */

  hb_raster_image_t *acquire_surface ()
  {
    hb_raster_image_t *img;
    if (surface_cache.length)
      img = surface_cache.pop ();
    else
    {
      img = hb_object_create<hb_raster_image_t> ();
      if (unlikely (!img)) return nullptr;
    }

    img->format = HB_RASTER_FORMAT_BGRA32;
    img->extents = fixed_extents;
    if (img->extents.stride == 0)
      img->extents.stride = img->extents.width * 4;

    size_t buf_size = (size_t) img->extents.stride * img->extents.height;
    if (unlikely (!img->buffer.resize_dirty (buf_size)))
    {
      hb_raster_image_destroy (img);
      return nullptr;
    }
    memset (img->buffer.arrayZ, 0, buf_size);
    return img;
  }

  void release_surface (hb_raster_image_t *img)
  {
    surface_cache.push (img);
  }

  hb_raster_image_t *current_surface ()
  {
    return surface_stack.length ? surface_stack.tail () : nullptr;
  }

  hb_raster_clip_t &current_clip ()
  {
    return clip_stack.tail ();
  }

  hb_transform_t<> &current_transform ()
  {
    return transform_stack.tail ();
  }
};


/*
 * Pixel helpers (paint-specific)
 */

/* Convert unpremultiplied hb_color_t (BGRA order) to premultiplied BGRA32 pixel. */
static inline uint32_t
color_to_premul_pixel (hb_color_t color)
{
  uint8_t a = hb_color_get_alpha (color);
  uint8_t r = hb_raster_div255 (hb_color_get_red (color) * a);
  uint8_t g = hb_raster_div255 (hb_color_get_green (color) * a);
  uint8_t b = hb_raster_div255 (hb_color_get_blue (color) * a);
  return (uint32_t) b | ((uint32_t) g << 8) | ((uint32_t) r << 16) | ((uint32_t) a << 24);
}


/*
 * Paint callbacks
 */

/* Lazy initialization: set up root surface, initial clip and transform.
 * Called from every paint callback that needs state.
 * hb_font_paint_glyph() does NOT wrap with push/pop_transform,
 * so the first callback could be push_clip_glyph or paint_color. */
static void
ensure_initialized (hb_raster_paint_t *c)
{
  if (c->surface_stack.length) return;

  /* Root surface */
  hb_raster_image_t *root = c->acquire_surface ();
  if (unlikely (!root)) return;
  c->surface_stack.push (root);

  /* Initial transform */
  c->transform_stack.push (c->base_transform);

  /* Initial clip: full coverage rectangle */
  hb_raster_clip_t clip;
  clip.init_full (c->fixed_extents.width, c->fixed_extents.height);
  c->clip_stack.push (clip);
}

static void
hb_raster_paint_push_transform (hb_paint_funcs_t *pfuncs HB_UNUSED,
				void *paint_data,
				float xx, float yx,
				float xy, float yy,
				float dx, float dy,
				void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  hb_transform_t<> t = c->current_transform ();
  t.multiply ({xx, yx, xy, yy, dx, dy});
  c->transform_stack.push (t);
}

static void
hb_raster_paint_pop_transform (hb_paint_funcs_t *pfuncs HB_UNUSED,
			       void *paint_data,
			       void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;
  c->transform_stack.pop ();
}

static hb_bool_t
hb_raster_paint_color_glyph (hb_paint_funcs_t *pfuncs HB_UNUSED,
			     void *paint_data HB_UNUSED,
			     hb_codepoint_t glyph HB_UNUSED,
			     hb_font_t *font HB_UNUSED,
			     void *user_data HB_UNUSED)
{
  return false;
}

static void
hb_raster_paint_push_clip_glyph (hb_paint_funcs_t *pfuncs HB_UNUSED,
				 void *paint_data,
				 hb_codepoint_t glyph,
				 hb_font_t *font,
				 void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return;

  unsigned w = surf->extents.width;
  unsigned h = surf->extents.height;

  hb_raster_clip_t new_clip;
  new_clip.width = w;
  new_clip.height = h;
  new_clip.stride = (w + 3u) & ~3u;
  new_clip.is_rect = false;

  /* Rasterize glyph outline as A8 alpha mask using internal rasterizer */
  hb_raster_draw_t *rdr = c->clip_rdr;
  const hb_transform_t<> &t = c->current_transform ();
  hb_raster_draw_set_transform (rdr, t.xx, t.yx, t.xy, t.yy, t.x0, t.y0);
  hb_raster_draw_set_format (rdr, HB_RASTER_FORMAT_A8);
  /* Let draw-render choose tight glyph extents; we map by mask origin below. */

  hb_font_draw_glyph (font, glyph, hb_raster_draw_get_funcs (), rdr);
  hb_raster_image_t *mask_img = hb_raster_draw_render (rdr);

  if (unlikely (!mask_img))
  {
    /* If mask rendering fails, push a fully transparent clip */
    new_clip.init_full (w, h);
    new_clip.is_rect = true;
    new_clip.rect_x0 = new_clip.rect_y0 = 0;
    new_clip.rect_x1 = new_clip.rect_y1 = 0;
    new_clip.min_x = new_clip.min_y = new_clip.max_x = new_clip.max_y = 0;
    c->clip_stack.push (new_clip);
    return;
  }

  /* Allocate alpha buffer and intersect with previous clip */
  if (unlikely (!new_clip.alpha.resize (new_clip.stride * h)))
  {
    hb_raster_draw_recycle_image (rdr, mask_img);
    new_clip.init_full (w, h);
    new_clip.is_rect = true;
    new_clip.rect_x0 = new_clip.rect_y0 = 0;
    new_clip.rect_x1 = new_clip.rect_y1 = 0;
    new_clip.min_x = new_clip.min_y = new_clip.max_x = new_clip.max_y = 0;
    c->clip_stack.push (new_clip);
    return;
  }

  const uint8_t *mask_buf = hb_raster_image_get_buffer (mask_img);
  hb_raster_extents_t mask_ext;
  hb_raster_image_get_extents (mask_img, &mask_ext);
  const hb_raster_clip_t &old_clip = c->current_clip ();

  /* Convert mask extents from surface coordinates to clip-buffer coordinates. */
  int mask_x0 = mask_ext.x_origin - surf->extents.x_origin;
  int mask_y0 = mask_ext.y_origin - surf->extents.y_origin;
  int mask_x1 = mask_x0 + (int) mask_ext.width;
  int mask_y1 = mask_y0 + (int) mask_ext.height;

  int ix0_i = hb_max ((int) old_clip.min_x, hb_max (mask_x0, 0));
  int iy0_i = hb_max ((int) old_clip.min_y, hb_max (mask_y0, 0));
  int ix1_i = hb_min ((int) old_clip.max_x, hb_min (mask_x1, (int) w));
  int iy1_i = hb_min ((int) old_clip.max_y, hb_min (mask_y1, (int) h));

  if (ix0_i >= ix1_i || iy0_i >= iy1_i)
  {
    hb_raster_draw_recycle_image (rdr, mask_img);
    new_clip.init_full (w, h);
    new_clip.is_rect = true;
    new_clip.rect_x0 = new_clip.rect_y0 = 0;
    new_clip.rect_x1 = new_clip.rect_y1 = 0;
    new_clip.min_x = new_clip.min_y = new_clip.max_x = new_clip.max_y = 0;
    c->clip_stack.push (new_clip);
    return;
  }

  unsigned ix0 = (unsigned) ix0_i;
  unsigned iy0 = (unsigned) iy0_i;
  unsigned ix1 = (unsigned) ix1_i;
  unsigned iy1 = (unsigned) iy1_i;

  new_clip.min_x = w; new_clip.min_y = h;
  new_clip.max_x = 0; new_clip.max_y = 0;

  if (old_clip.is_rect)
  {
    for (unsigned y = iy0; y < iy1; y++)
    {
      const uint8_t *mask_row = mask_buf + (unsigned) ((int) y - mask_y0) * mask_ext.stride;
      uint8_t *out_row = new_clip.alpha.arrayZ + y * new_clip.stride;
      unsigned mx0 = (unsigned) ((int) ix0 - mask_x0);
      memcpy (out_row + ix0, mask_row + mx0, ix1 - ix0);

      unsigned row_min = ix1;
      unsigned row_max = ix0;
      for (unsigned x = ix0; x < ix1; x++)
	if (mask_row[(unsigned) ((int) x - mask_x0)])
	{
	  row_min = x;
	  break;
	}
      for (unsigned x = ix1; x > row_min; x--)
	if (mask_row[(unsigned) ((int) (x - 1) - mask_x0)])
	{
	  row_max = x;
	  break;
	}
      if (row_min < row_max)
      {
	new_clip.min_x = hb_min (new_clip.min_x, row_min);
	new_clip.min_y = hb_min (new_clip.min_y, y);
	new_clip.max_x = hb_max (new_clip.max_x, row_max);
	new_clip.max_y = hb_max (new_clip.max_y, y + 1);
      }
    }
  }
  else
  {
    for (unsigned y = iy0; y < iy1; y++)
    {
      const uint8_t *old_row = old_clip.alpha.arrayZ + y * old_clip.stride;
      const uint8_t *mask_row = mask_buf + (unsigned) ((int) y - mask_y0) * mask_ext.stride;
      uint8_t *out_row = new_clip.alpha.arrayZ + y * new_clip.stride;
      unsigned row_min = ix1;
      unsigned row_max = ix0;
      for (unsigned x = ix0; x < ix1; x++)
      {
	unsigned mx = (unsigned) ((int) x - mask_x0);
	uint8_t a = hb_raster_div255 (mask_row[mx] * old_row[x]);
	out_row[x] = a;
	if (a)
	{
	  row_min = hb_min (row_min, x);
	  row_max = x + 1;
	}
      }
      if (row_min < row_max)
      {
	new_clip.min_x = hb_min (new_clip.min_x, row_min);
	new_clip.min_y = hb_min (new_clip.min_y, y);
	new_clip.max_x = hb_max (new_clip.max_x, row_max);
	new_clip.max_y = hb_max (new_clip.max_y, y + 1);
      }
    }
  }

  hb_raster_draw_recycle_image (rdr, mask_img);
  c->clip_stack.push (new_clip);
}

static void
hb_raster_paint_push_clip_rectangle (hb_paint_funcs_t *pfuncs HB_UNUSED,
				     void *paint_data,
				     float xmin, float ymin,
				     float xmax, float ymax,
				     void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  if (!c->surface_stack.length) return;

  const hb_transform_t<> &t = c->current_transform ();

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return;

  unsigned w = surf->extents.width;
  unsigned h = surf->extents.height;
  bool is_axis_aligned = (t.xy == 0.f && t.yx == 0.f);

  /* Transform the four corners to pixel space */
  float cx[4], cy[4];
  cx[0] = xmin; cy[0] = ymin;
  cx[1] = xmax; cy[1] = ymin;
  cx[2] = xmax; cy[2] = ymax;
  cx[3] = xmin; cy[3] = ymax;
  for (unsigned i = 0; i < 4; i++)
    t.transform_point (cx[i], cy[i]);

  /* Compute bounding box in pixel coords */
  float fmin_x = cx[0], fmin_y = cy[0], fmax_x = cx[0], fmax_y = cy[0];
  for (unsigned i = 1; i < 4; i++)
  {
    fmin_x = hb_min (fmin_x, cx[i]); fmin_y = hb_min (fmin_y, cy[i]);
    fmax_x = hb_max (fmax_x, cx[i]); fmax_y = hb_max (fmax_y, cy[i]);
  }

  int px0 = (int) floorf (fmin_x) - surf->extents.x_origin;
  int py0 = (int) floorf (fmin_y) - surf->extents.y_origin;
  int px1 = (int) ceilf (fmax_x) - surf->extents.x_origin;
  int py1 = (int) ceilf (fmax_y) - surf->extents.y_origin;

  /* Clamp to surface bounds */
  px0 = hb_max (px0, 0);
  py0 = hb_max (py0, 0);
  px1 = hb_min (px1, (int) w);
  py1 = hb_min (py1, (int) h);

  const hb_raster_clip_t &old_clip = c->current_clip ();

  hb_raster_clip_t new_clip;
  new_clip.width = w;
  new_clip.height = h;
  new_clip.stride = (w + 3u) & ~3u;

  if (is_axis_aligned && old_clip.is_rect)
  {
    /* Fast path: axis-aligned rect-on-rect intersection */
    new_clip.is_rect = true;
    new_clip.rect_x0 = hb_max (px0, old_clip.rect_x0);
    new_clip.rect_y0 = hb_max (py0, old_clip.rect_y0);
    new_clip.rect_x1 = hb_min (px1, old_clip.rect_x1);
    new_clip.rect_y1 = hb_min (py1, old_clip.rect_y1);
    new_clip.update_bounds_from_rect ();
  }
  else
  {
    /* General case: rasterize transformed quad as alpha mask */
    new_clip.is_rect = false;
    if (unlikely (!new_clip.alpha.resize (new_clip.stride * h)))
    {
      new_clip.init_full (w, h);
      new_clip.is_rect = true;
      new_clip.rect_x0 = new_clip.rect_y0 = 0;
      new_clip.rect_x1 = new_clip.rect_y1 = 0;
      new_clip.min_x = new_clip.min_y = new_clip.max_x = new_clip.max_y = 0;
      c->clip_stack.push (new_clip);
      return;
    }
    memset (new_clip.alpha.arrayZ, 0, new_clip.stride * h);

    /* Convert quad corners to pixel-relative coords */
    float qx[4], qy[4];
    int ox = surf->extents.x_origin;
    int oy = surf->extents.y_origin;
    for (unsigned i = 0; i < 4; i++)
    {
      qx[i] = cx[i] - ox;
      qy[i] = cy[i] - oy;
    }

    /* For each pixel in the bounding box, test if inside the quad
     * using cross-product edge tests (winding order). */
    unsigned iy0 = (unsigned) hb_max (py0, (int) old_clip.min_y);
    unsigned iy1 = (unsigned) hb_min (py1, (int) old_clip.max_y);
    unsigned ix0 = (unsigned) hb_max (px0, (int) old_clip.min_x);
    unsigned ix1 = (unsigned) hb_min (px1, (int) old_clip.max_x);
    new_clip.min_x = w; new_clip.min_y = h;
    new_clip.max_x = 0; new_clip.max_y = 0;

    /* Precompute edge normals for point-in-quad test.
     * Edge i goes from corner i to corner (i+1)%4.
     * Normal = (dy, -dx); inside test: dot(normal, p-corner) >= 0 */
    float enx[4], eny[4], ed[4];
    for (unsigned i = 0; i < 4; i++)
    {
      unsigned j = (i + 1) & 3;
      float edx = qx[j] - qx[i], edy = qy[j] - qy[i];
      enx[i] = edy;       /* normal x */
      eny[i] = -edx;      /* normal y */
      ed[i] = enx[i] * qx[i] + eny[i] * qy[i]; /* distance threshold */
    }

    if (old_clip.is_rect)
    {
      for (unsigned y = iy0; y < iy1; y++)
	for (unsigned x = ix0; x < ix1; x++)
	{
	  float px_f = x + 0.5f, py_f = y + 0.5f;
	  /* Test if pixel center is inside the quad */
	  bool inside = true;
	  for (unsigned i = 0; i < 4; i++)
	    if (enx[i] * px_f + eny[i] * py_f < ed[i])
	    {
	      inside = false;
	      break;
	    }
	  uint8_t a = inside ? 255 : 0;
	  new_clip.alpha[y * new_clip.stride + x] = a;
	  if (a)
	  {
	    new_clip.min_x = hb_min (new_clip.min_x, x);
	    new_clip.min_y = hb_min (new_clip.min_y, y);
	    new_clip.max_x = hb_max (new_clip.max_x, x + 1);
	    new_clip.max_y = hb_max (new_clip.max_y, y + 1);
	  }
	}
    }
    else
    {
      for (unsigned y = iy0; y < iy1; y++)
      {
	const uint8_t *old_row = old_clip.alpha.arrayZ + y * old_clip.stride;
	for (unsigned x = ix0; x < ix1; x++)
	{
	  float px_f = x + 0.5f, py_f = y + 0.5f;
	  /* Test if pixel center is inside the quad */
	  bool inside = true;
	  for (unsigned i = 0; i < 4; i++)
	    if (enx[i] * px_f + eny[i] * py_f < ed[i])
	    {
	      inside = false;
	      break;
	    }
	  uint8_t a = inside ? old_row[x] : 0;
	  new_clip.alpha[y * new_clip.stride + x] = a;
	  if (a)
	  {
	    new_clip.min_x = hb_min (new_clip.min_x, x);
	    new_clip.min_y = hb_min (new_clip.min_y, y);
	    new_clip.max_x = hb_max (new_clip.max_x, x + 1);
	    new_clip.max_y = hb_max (new_clip.max_y, y + 1);
	  }
	}
      }
    }
  }

  c->clip_stack.push (new_clip);
}

static void
hb_raster_paint_pop_clip (hb_paint_funcs_t *pfuncs HB_UNUSED,
			  void *paint_data,
			  void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;
  c->clip_stack.pop ();
}

static void
hb_raster_paint_push_group (hb_paint_funcs_t *pfuncs HB_UNUSED,
			    void *paint_data,
			    void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  hb_raster_image_t *new_surf = c->acquire_surface ();
  if (unlikely (!new_surf)) return;
  c->surface_stack.push (new_surf);
}

static void
hb_raster_paint_pop_group (hb_paint_funcs_t *pfuncs HB_UNUSED,
			   void *paint_data,
			   hb_paint_composite_mode_t mode,
			   void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  if (c->surface_stack.length < 2) return;

  hb_raster_image_t *src = c->surface_stack.pop ();
  hb_raster_image_t *dst = c->current_surface ();

  if (dst && src)
    hb_raster_composite_images (dst, src, mode);

  c->release_surface (src);
}

static void
hb_raster_paint_color (hb_paint_funcs_t *pfuncs HB_UNUSED,
		       void *paint_data,
		       hb_bool_t is_foreground,
		       hb_color_t color,
		       void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return;

  if (is_foreground)
  {
    /* Use foreground color, modulating alpha */
    color = HB_COLOR (hb_color_get_blue (c->foreground),
		      hb_color_get_green (c->foreground),
		      hb_color_get_red (c->foreground),
		      hb_raster_div255 (hb_color_get_alpha (c->foreground) *
			      hb_color_get_alpha (color)));
  }

  uint32_t premul = color_to_premul_pixel (color);
  uint8_t premul_a = (uint8_t) (premul >> 24);
  const hb_raster_clip_t &clip = c->current_clip ();

  unsigned stride = surf->extents.stride;
  if (clip.min_x >= clip.max_x || clip.min_y >= clip.max_y) return;
  if (premul_a == 0) return;

  if (likely (!clip.is_rect))
  {
    for (unsigned y = clip.min_y; y < clip.max_y; y++)
    {
      uint32_t *__restrict row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + y * stride);
      const uint8_t *__restrict clip_row = clip.alpha.arrayZ + y * clip.stride;
      for (unsigned x = clip.min_x; x < clip.max_x; x++)
      {
	uint8_t clip_alpha = clip_row[x];
	if (clip_alpha == 0) continue;
	if (clip_alpha == 255)
	{
	  if (premul_a == 255)
	    row[x] = premul;
	  else
	    row[x] = hb_raster_src_over (premul, row[x]);
	}
	else
	{
	  uint32_t src = hb_raster_alpha_mul (premul, clip_alpha);
	  row[x] = hb_raster_src_over (src, row[x]);
	}
      }
    }
  }
  else
  {
    for (unsigned y = clip.min_y; y < clip.max_y; y++)
    {
      uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + y * stride);
      for (unsigned x = clip.min_x; x < clip.max_x; x++)
	row[x] = hb_raster_src_over (premul, row[x]);
    }
  }
}

static hb_bool_t
hb_raster_paint_image (hb_paint_funcs_t *pfuncs HB_UNUSED,
		       void *paint_data,
		       hb_blob_t *blob,
		       unsigned width,
		       unsigned height,
		       hb_tag_t format,
		       float slant HB_UNUSED,
		       hb_glyph_extents_t *extents,
		       void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  /* Only handle raw BGRA32 for now */
  if (format != HB_TAG ('B','G','R','A'))
    return false;

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return false;
  if (!extents) return false;

  unsigned data_len;
  const uint8_t *data = (const uint8_t *) hb_blob_get_data (blob, &data_len);
  if (!data || data_len < width * height * 4) return false;

  const hb_raster_clip_t &clip = c->current_clip ();
  const hb_transform_t<> &t = c->current_transform ();

  /* Compute inverse transform for sampling */
  float det = t.xx * t.yy - t.xy * t.yx;
  if (fabsf (det) < 1e-10f) return false;
  float inv_det = 1.f / det;
  float inv_xx =  t.yy * inv_det;
  float inv_xy = -t.xy * inv_det;
  float inv_yx = -t.yx * inv_det;
  float inv_yy =  t.xx * inv_det;
  float inv_x0 = (t.xy * t.y0 - t.yy * t.x0) * inv_det;
  float inv_y0 = (t.yx * t.x0 - t.xx * t.y0) * inv_det;

  unsigned surf_stride = surf->extents.stride;
  int ox = surf->extents.x_origin;
  int oy = surf->extents.y_origin;

  /* Image source rectangle in glyph space */
  float img_x = extents->x_bearing;
  float img_y = extents->y_bearing + extents->height; /* bottom-left in glyph space */
  float img_sx = (float) extents->width / width;
  float img_sy = (float) -extents->height / height; /* flip Y */

  if (clip.is_rect)
  {
    for (unsigned py = clip.min_y; py < clip.max_y; py++)
    {
      uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * surf_stride);
      float gx = inv_xx * (clip.min_x + ox) + inv_xy * (py + oy) + inv_x0;
      float gy = inv_yx * (clip.min_x + ox) + inv_yy * (py + oy) + inv_y0;
      for (unsigned px = clip.min_x; px < clip.max_x; px++)
      {
	/* Map glyph space to image texel */
	int ix = (int) floorf ((gx - img_x) / img_sx);
	int iy = (int) floorf ((gy - img_y) / img_sy);

	if (ix < 0 || ix >= (int) width || iy < 0 || iy >= (int) height)
	{
	  gx += inv_xx;
	  gy += inv_yx;
	  continue;
	}

	uint32_t src_px = reinterpret_cast<const uint32_t *> (data)[iy * width + ix];
	row[px] = hb_raster_src_over (src_px, row[px]);
	gx += inv_xx;
	gy += inv_yx;
      }
    }
  }
  else
  {
    for (unsigned py = clip.min_y; py < clip.max_y; py++)
    {
      uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * surf_stride);
      const uint8_t *clip_row = clip.alpha.arrayZ + py * clip.stride;
      float gx = inv_xx * (clip.min_x + ox) + inv_xy * (py + oy) + inv_x0;
      float gy = inv_yx * (clip.min_x + ox) + inv_yy * (py + oy) + inv_y0;
      for (unsigned px = clip.min_x; px < clip.max_x; px++)
      {
	uint8_t clip_alpha = clip_row[px];
	if (clip_alpha == 0)
	{
	  gx += inv_xx;
	  gy += inv_yx;
	  continue;
	}

	/* Map glyph space to image texel */
	int ix = (int) floorf ((gx - img_x) / img_sx);
	int iy = (int) floorf ((gy - img_y) / img_sy);

	if (ix < 0 || ix >= (int) width || iy < 0 || iy >= (int) height)
	{
	  gx += inv_xx;
	  gy += inv_yx;
	  continue;
	}

	uint32_t src_px = reinterpret_cast<const uint32_t *> (data)[iy * width + ix];
	src_px = hb_raster_alpha_mul (src_px, clip_alpha);
	row[px] = hb_raster_src_over (src_px, row[px]);
	gx += inv_xx;
	gy += inv_yx;
      }
    }
  }

  return true;
}


/*
 * Gradient helpers
 */

#define PREALLOCATED_COLOR_STOPS 16

static int
cmp_color_stop (const void *p1, const void *p2)
{
  const hb_color_stop_t *c1 = (const hb_color_stop_t *) p1;
  const hb_color_stop_t *c2 = (const hb_color_stop_t *) p2;
  if (c1->offset < c2->offset) return -1;
  if (c1->offset > c2->offset) return 1;
  return 0;
}

static bool
get_color_stops (hb_raster_paint_t *c,
		 hb_color_line_t *color_line,
		 unsigned *count,
		 hb_color_stop_t **stops)
{
  unsigned len = hb_color_line_get_color_stops (color_line, 0, nullptr, nullptr);
  if (len > *count)
  {
    *stops = (hb_color_stop_t *) hb_malloc (len * sizeof (hb_color_stop_t));
    if (unlikely (!*stops))
      return false;
  }
  hb_color_line_get_color_stops (color_line, 0, &len, *stops);
  for (unsigned i = 0; i < len; i++)
    if ((*stops)[i].is_foreground)
      (*stops)[i].color = HB_COLOR (hb_color_get_blue (c->foreground),
				    hb_color_get_green (c->foreground),
				    hb_color_get_red (c->foreground),
				    hb_raster_div255 (hb_color_get_alpha (c->foreground) *
					    hb_color_get_alpha ((*stops)[i].color)));

  *count = len;
  return true;
}

static void
normalize_color_line (hb_color_stop_t *stops,
		      unsigned len,
		      float *omin, float *omax)
{
  hb_qsort (stops, len, sizeof (hb_color_stop_t), cmp_color_stop);

  float mn = stops[0].offset, mx = stops[0].offset;
  for (unsigned i = 1; i < len; i++)
  {
    mn = hb_min (mn, stops[i].offset);
    mx = hb_max (mx, stops[i].offset);
  }
  if (mn != mx)
    for (unsigned i = 0; i < len; i++)
      stops[i].offset = (stops[i].offset - mn) / (mx - mn);

  *omin = mn;
  *omax = mx;
}

/* Evaluate color at normalized position t, interpolating in premultiplied space. */
static uint32_t
evaluate_color_line (const hb_color_stop_t *stops, unsigned len, float t,
		     hb_paint_extend_t extend)
{
  /* Apply extend mode */
  if (extend == HB_PAINT_EXTEND_PAD)
  {
    t = hb_clamp (t, 0.f, 1.f);
  }
  else if (extend == HB_PAINT_EXTEND_REPEAT)
  {
    t = t - floorf (t);
  }
  else /* REFLECT */
  {
    if (t < 0) t = -t;
    int period = (int) floorf (t);
    float frac = t - (float) period;
    t = (period & 1) ? 1.f - frac : frac;
  }

  /* Find bounding stops */
  if (t <= stops[0].offset)
    return color_to_premul_pixel (stops[0].color);
  if (t >= stops[len - 1].offset)
    return color_to_premul_pixel (stops[len - 1].color);

  unsigned i;
  for (i = 0; i < len - 1; i++)
    if (t < stops[i + 1].offset)
      break;

  float range = stops[i + 1].offset - stops[i].offset;
  float k = range > 0.f ? (t - stops[i].offset) / range : 0.f;

  /* Interpolate in premultiplied [0,255] space */
  hb_color_t c0 = stops[i].color;
  hb_color_t c1 = stops[i + 1].color;

  float a0 = hb_color_get_alpha (c0) / 255.f;
  float r0 = hb_color_get_red   (c0) / 255.f * a0;
  float g0 = hb_color_get_green (c0) / 255.f * a0;
  float b0 = hb_color_get_blue  (c0) / 255.f * a0;

  float a1 = hb_color_get_alpha (c1) / 255.f;
  float r1 = hb_color_get_red   (c1) / 255.f * a1;
  float g1 = hb_color_get_green (c1) / 255.f * a1;
  float b1 = hb_color_get_blue  (c1) / 255.f * a1;

  float a = a0 + k * (a1 - a0);
  float r = r0 + k * (r1 - r0);
  float g = g0 + k * (g1 - g0);
  float b = b0 + k * (b1 - b0);

  uint8_t pa = (uint8_t) (a * 255.f + 0.5f);
  uint8_t pr = (uint8_t) (r * 255.f + 0.5f);
  uint8_t pg = (uint8_t) (g * 255.f + 0.5f);
  uint8_t pb = (uint8_t) (b * 255.f + 0.5f);

  return (uint32_t) pb | ((uint32_t) pg << 8) | ((uint32_t) pr << 16) | ((uint32_t) pa << 24);
}

static void
reduce_anchors (float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		float *xx0, float *yy0,
		float *xx1, float *yy1)
{
  float q2x = x2 - x0, q2y = y2 - y0;
  float q1x = x1 - x0, q1y = y1 - y0;
  float s = q2x * q2x + q2y * q2y;
  if (s < 0.000001f)
  {
    *xx0 = x0; *yy0 = y0;
    *xx1 = x1; *yy1 = y1;
    return;
  }
  float k = (q2x * q1x + q2y * q1y) / s;
  *xx0 = x0;
  *yy0 = y0;
  *xx1 = x1 - k * q2x;
  *yy1 = y1 - k * q2y;
}


/*
 * Gradient paint callbacks
 */

static void
hb_raster_paint_linear_gradient (hb_paint_funcs_t *pfuncs HB_UNUSED,
				 void *paint_data,
				 hb_color_line_t *color_line,
				 float x0, float y0,
				 float x1, float y1,
				 float x2, float y2,
				 void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return;

  unsigned len = PREALLOCATED_COLOR_STOPS;
  hb_color_stop_t stops_[PREALLOCATED_COLOR_STOPS];
  hb_color_stop_t *stops = stops_;

  if (unlikely (!get_color_stops (c, color_line, &len, &stops)))
    return;
  float mn, mx;
  normalize_color_line (stops, len, &mn, &mx);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  /* Reduce 3-point anchor to 2-point gradient axis */
  float lx0, ly0, lx1, ly1;
  reduce_anchors (x0, y0, x1, y1, x2, y2, &lx0, &ly0, &lx1, &ly1);

  /* Apply normalization to endpoints */
  float gx0 = lx0 + mn * (lx1 - lx0);
  float gy0 = ly0 + mn * (ly1 - ly0);
  float gx1 = lx0 + mx * (lx1 - lx0);
  float gy1 = ly0 + mx * (ly1 - ly0);

  /* Inverse transform: pixel → glyph space */
  const hb_transform_t<> &t = c->current_transform ();
  float det = t.xx * t.yy - t.xy * t.yx;
  if (fabsf (det) < 1e-10f) goto done;

  {
    float inv_det = 1.f / det;
    float inv_xx =  t.yy * inv_det;
    float inv_xy = -t.xy * inv_det;
    float inv_yx = -t.yx * inv_det;
    float inv_yy =  t.xx * inv_det;
    float inv_x0 = (t.xy * t.y0 - t.yy * t.x0) * inv_det;
    float inv_y0 = (t.yx * t.x0 - t.xx * t.y0) * inv_det;

    /* Gradient direction vector and denominator for projection */
    float dx = gx1 - gx0, dy = gy1 - gy0;
    float denom = dx * dx + dy * dy;
    if (denom < 1e-10f) goto done;
    float inv_denom = 1.f / denom;

    const hb_raster_clip_t &clip = c->current_clip ();
    unsigned stride = surf->extents.stride;
    int ox = surf->extents.x_origin;
    int oy = surf->extents.y_origin;

    if (clip.is_rect)
    {
      for (unsigned py = clip.min_y; py < clip.max_y; py++)
      {
	uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
	float gx = inv_xx * (clip.min_x + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (clip.min_x + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;
	for (unsigned px = clip.min_x; px < clip.max_x; px++)
	{
	  /* Project onto gradient axis: t = dot(p - p0, dir) / dot(dir, dir) */
	  float proj_t = ((gx - gx0) * dx + (gy - gy0) * dy) * inv_denom;

	  uint32_t src = evaluate_color_line (stops, len, proj_t, extend);
	  row[px] = hb_raster_src_over (src, row[px]);
	  gx += inv_xx;
	  gy += inv_yx;
	}
      }
    }
    else
    {
      for (unsigned py = clip.min_y; py < clip.max_y; py++)
      {
	uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
	const uint8_t *clip_row = clip.alpha.arrayZ + py * clip.stride;
	float gx = inv_xx * (clip.min_x + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (clip.min_x + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;
	for (unsigned px = clip.min_x; px < clip.max_x; px++)
	{
	  uint8_t clip_alpha = clip_row[px];
	  if (clip_alpha == 0)
	  {
	    gx += inv_xx;
	    gy += inv_yx;
	    continue;
	  }

	  /* Project onto gradient axis: t = dot(p - p0, dir) / dot(dir, dir) */
	  float proj_t = ((gx - gx0) * dx + (gy - gy0) * dy) * inv_denom;

	  uint32_t src = evaluate_color_line (stops, len, proj_t, extend);
	  src = hb_raster_alpha_mul (src, clip_alpha);
	  row[px] = hb_raster_src_over (src, row[px]);
	  gx += inv_xx;
	  gy += inv_yx;
	}
      }
    }
  }

done:
  if (stops != stops_)
    hb_free (stops);
}

static void
hb_raster_paint_radial_gradient (hb_paint_funcs_t *pfuncs HB_UNUSED,
				 void *paint_data,
				 hb_color_line_t *color_line,
				 float x0, float y0, float r0,
				 float x1, float y1, float r1,
				 void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return;

  unsigned len = PREALLOCATED_COLOR_STOPS;
  hb_color_stop_t stops_[PREALLOCATED_COLOR_STOPS];
  hb_color_stop_t *stops = stops_;

  if (unlikely (!get_color_stops (c, color_line, &len, &stops)))
    return;
  float mn, mx;
  normalize_color_line (stops, len, &mn, &mx);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  /* Apply normalization to circle parameters */
  float cx0 = x0 + mn * (x1 - x0);
  float cy0 = y0 + mn * (y1 - y0);
  float cr0 = r0 + mn * (r1 - r0);
  float cx1 = x0 + mx * (x1 - x0);
  float cy1 = y0 + mx * (y1 - y0);
  float cr1 = r0 + mx * (r1 - r0);

  /* Inverse transform */
  const hb_transform_t<> &t = c->current_transform ();
  float det = t.xx * t.yy - t.xy * t.yx;
  if (fabsf (det) < 1e-10f) goto done;

  {
    float inv_det = 1.f / det;
    float inv_xx =  t.yy * inv_det;
    float inv_xy = -t.xy * inv_det;
    float inv_yx = -t.yx * inv_det;
    float inv_yy =  t.xx * inv_det;
    float inv_x0 = (t.xy * t.y0 - t.yy * t.x0) * inv_det;
    float inv_y0 = (t.yx * t.x0 - t.xx * t.y0) * inv_det;

    /* Precompute quadratic coefficients for radial gradient:
     * |p - c0 - t*(c1-c0)|^2 = (r0 + t*(r1-r0))^2
     *
     * Expanding gives At^2 + Bt + C = 0 where:
     *   cdx = c1.x - c0.x, cdy = c1.y - c0.y, dr = r1 - r0
     *   A = cdx^2 + cdy^2 - dr^2
     *   B = -2*(px-c0.x)*cdx - 2*(py-c0.y)*cdy - 2*r0*dr
     *   C = (px-c0.x)^2 + (py-c0.y)^2 - r0^2
     */
    float cdx = cx1 - cx0, cdy = cy1 - cy0;
    float dr = cr1 - cr0;
    float A = cdx * cdx + cdy * cdy - dr * dr;

    const hb_raster_clip_t &clip = c->current_clip ();
    unsigned stride = surf->extents.stride;
    int ox = surf->extents.x_origin;
    int oy = surf->extents.y_origin;

    if (clip.is_rect)
    {
      for (unsigned py = clip.min_y; py < clip.max_y; py++)
      {
	uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
	float gx = inv_xx * (clip.min_x + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (clip.min_x + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;
	for (unsigned px = clip.min_x; px < clip.max_x; px++)
	{
	  float dpx = gx - cx0, dpy = gy - cy0;
	  float B = -2.f * (dpx * cdx + dpy * cdy + cr0 * dr);
	  float C = dpx * dpx + dpy * dpy - cr0 * cr0;

	  float grad_t;
	  if (fabsf (A) > 1e-10f)
	  {
	    float disc = B * B - 4.f * A * C;
	    if (disc < 0.f) continue;
	    float sq = sqrtf (disc);
	    /* Pick the larger root (t closer to 1 = outer circle) */
	    float t1 = (-B + sq) / (2.f * A);
	    float t2 = (-B - sq) / (2.f * A);
	    /* Choose the root that gives a positive radius */
	    if (cr0 + t1 * dr >= 0.f)
	      grad_t = t1;
	    else
	      grad_t = t2;
	  }
	  else
	  {
	    /* Linear case: Bt + C = 0 */
	    if (fabsf (B) < 1e-10f) continue;
	    grad_t = -C / B;
	  }

	  uint32_t src = evaluate_color_line (stops, len, grad_t, extend);
	  row[px] = hb_raster_src_over (src, row[px]);
	  gx += inv_xx;
	  gy += inv_yx;
	}
      }
    }
    else
    {
      for (unsigned py = clip.min_y; py < clip.max_y; py++)
      {
	uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
	const uint8_t *clip_row = clip.alpha.arrayZ + py * clip.stride;
	float gx = inv_xx * (clip.min_x + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (clip.min_x + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;
	for (unsigned px = clip.min_x; px < clip.max_x; px++)
	{
	  uint8_t clip_alpha = clip_row[px];
	  if (clip_alpha == 0)
	  {
	    gx += inv_xx;
	    gy += inv_yx;
	    continue;
	  }

	  float dpx = gx - cx0, dpy = gy - cy0;
	  float B = -2.f * (dpx * cdx + dpy * cdy + cr0 * dr);
	  float C = dpx * dpx + dpy * dpy - cr0 * cr0;

	  float grad_t;
	  if (fabsf (A) > 1e-10f)
	  {
	    float disc = B * B - 4.f * A * C;
	    if (disc < 0.f) continue;
	    float sq = sqrtf (disc);
	    /* Pick the larger root (t closer to 1 = outer circle) */
	    float t1 = (-B + sq) / (2.f * A);
	    float t2 = (-B - sq) / (2.f * A);
	    /* Choose the root that gives a positive radius */
	    if (cr0 + t1 * dr >= 0.f)
	      grad_t = t1;
	    else
	      grad_t = t2;
	  }
	  else
	  {
	    /* Linear case: Bt + C = 0 */
	    if (fabsf (B) < 1e-10f) continue;
	    grad_t = -C / B;
	  }

	  uint32_t src = evaluate_color_line (stops, len, grad_t, extend);
	  src = hb_raster_alpha_mul (src, clip_alpha);
	  row[px] = hb_raster_src_over (src, row[px]);
	  gx += inv_xx;
	  gy += inv_yx;
	}
      }
    }
  }

done:
  if (stops != stops_)
    hb_free (stops);
}

static void
hb_raster_paint_sweep_gradient (hb_paint_funcs_t *pfuncs HB_UNUSED,
				void *paint_data,
				hb_color_line_t *color_line,
				float cx, float cy,
				float start_angle,
				float end_angle,
				void *user_data HB_UNUSED)
{
  hb_raster_paint_t *c = (hb_raster_paint_t *) paint_data;

  ensure_initialized (c);

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return;

  unsigned len = PREALLOCATED_COLOR_STOPS;
  hb_color_stop_t stops_[PREALLOCATED_COLOR_STOPS];
  hb_color_stop_t *stops = stops_;

  if (unlikely (!get_color_stops (c, color_line, &len, &stops)))
    return;
  float mn, mx;
  normalize_color_line (stops, len, &mn, &mx);

  hb_paint_extend_t extend = hb_color_line_get_extend (color_line);

  /* Apply normalization to angle range */
  float a0 = start_angle + mn * (end_angle - start_angle);
  float a1 = start_angle + mx * (end_angle - start_angle);
  float angle_range = a1 - a0;

  /* Inverse transform */
  const hb_transform_t<> &t = c->current_transform ();
  float det = t.xx * t.yy - t.xy * t.yx;
  if (fabsf (det) < 1e-10f || fabsf (angle_range) < 1e-10f) goto done;

  {
    float inv_det = 1.f / det;
    float inv_xx =  t.yy * inv_det;
    float inv_xy = -t.xy * inv_det;
    float inv_yx = -t.yx * inv_det;
    float inv_yy =  t.xx * inv_det;
    float inv_x0 = (t.xy * t.y0 - t.yy * t.x0) * inv_det;
    float inv_y0 = (t.yx * t.x0 - t.xx * t.y0) * inv_det;

    float inv_angle_range = 1.f / angle_range;

    const hb_raster_clip_t &clip = c->current_clip ();
    unsigned stride = surf->extents.stride;
    int ox = surf->extents.x_origin;
    int oy = surf->extents.y_origin;

    if (clip.is_rect)
    {
      for (unsigned py = clip.min_y; py < clip.max_y; py++)
      {
	uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
	float gx = inv_xx * (clip.min_x + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (clip.min_x + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;
	for (unsigned px = clip.min_x; px < clip.max_x; px++)
	{
	  float angle = atan2f (gy - cy, gx - cx);
	  /* Normalize to [0, 2*pi) — matches OT spec seam at angle 0. */
	  if (angle < 0) angle += (float) HB_2_PI;

	  float grad_t = (angle - a0) * inv_angle_range;

	  uint32_t src = evaluate_color_line (stops, len, grad_t, extend);
	  row[px] = hb_raster_src_over (src, row[px]);
	  gx += inv_xx;
	  gy += inv_yx;
	}
      }
    }
    else
    {
      for (unsigned py = clip.min_y; py < clip.max_y; py++)
      {
	uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
	const uint8_t *clip_row = clip.alpha.arrayZ + py * clip.stride;
	float gx = inv_xx * (clip.min_x + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (clip.min_x + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;
	for (unsigned px = clip.min_x; px < clip.max_x; px++)
	{
	  uint8_t clip_alpha = clip_row[px];
	  if (clip_alpha == 0)
	  {
	    gx += inv_xx;
	    gy += inv_yx;
	    continue;
	  }

	  float angle = atan2f (gy - cy, gx - cx);
	  /* Normalize to [0, 2*pi) — matches OT spec seam at angle 0. */
	  if (angle < 0) angle += (float) HB_2_PI;

	  float grad_t = (angle - a0) * inv_angle_range;

	  uint32_t src = evaluate_color_line (stops, len, grad_t, extend);
	  src = hb_raster_alpha_mul (src, clip_alpha);
	  row[px] = hb_raster_src_over (src, row[px]);
	  gx += inv_xx;
	  gy += inv_yx;
	}
      }
    }
  }

done:
  if (stops != stops_)
    hb_free (stops);
}

static hb_bool_t
hb_raster_paint_custom_palette_color (hb_paint_funcs_t *funcs HB_UNUSED,
				      void *paint_data HB_UNUSED,
				      unsigned int color_index HB_UNUSED,
				      hb_color_t *color HB_UNUSED,
				      void *user_data HB_UNUSED)
{
  return false;
}


/*
 * Lazy-loader singleton for paint funcs
 */

static inline void free_static_raster_paint_funcs ();

static struct hb_raster_paint_funcs_lazy_loader_t : hb_paint_funcs_lazy_loader_t<hb_raster_paint_funcs_lazy_loader_t>
{
  static hb_paint_funcs_t *create ()
  {
    hb_paint_funcs_t *funcs = hb_paint_funcs_create ();

    hb_paint_funcs_set_push_transform_func (funcs, hb_raster_paint_push_transform, nullptr, nullptr);
    hb_paint_funcs_set_pop_transform_func (funcs, hb_raster_paint_pop_transform, nullptr, nullptr);
    hb_paint_funcs_set_color_glyph_func (funcs, hb_raster_paint_color_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_glyph_func (funcs, hb_raster_paint_push_clip_glyph, nullptr, nullptr);
    hb_paint_funcs_set_push_clip_rectangle_func (funcs, hb_raster_paint_push_clip_rectangle, nullptr, nullptr);
    hb_paint_funcs_set_pop_clip_func (funcs, hb_raster_paint_pop_clip, nullptr, nullptr);
    hb_paint_funcs_set_push_group_func (funcs, hb_raster_paint_push_group, nullptr, nullptr);
    hb_paint_funcs_set_pop_group_func (funcs, hb_raster_paint_pop_group, nullptr, nullptr);
    hb_paint_funcs_set_color_func (funcs, hb_raster_paint_color, nullptr, nullptr);
    hb_paint_funcs_set_image_func (funcs, hb_raster_paint_image, nullptr, nullptr);
    hb_paint_funcs_set_linear_gradient_func (funcs, hb_raster_paint_linear_gradient, nullptr, nullptr);
    hb_paint_funcs_set_radial_gradient_func (funcs, hb_raster_paint_radial_gradient, nullptr, nullptr);
    hb_paint_funcs_set_sweep_gradient_func (funcs, hb_raster_paint_sweep_gradient, nullptr, nullptr);
    hb_paint_funcs_set_custom_palette_color_func (funcs, hb_raster_paint_custom_palette_color, nullptr, nullptr);

    hb_paint_funcs_make_immutable (funcs);

    hb_atexit (free_static_raster_paint_funcs);

    return funcs;
  }
} static_raster_paint_funcs;

static inline void
free_static_raster_paint_funcs ()
{
  static_raster_paint_funcs.free_instance ();
}


/*
 * Public API
 */

/**
 * hb_raster_paint_create:
 *
 * Creates a new color-glyph paint context.
 *
 * Return value: (transfer full):
 * A newly allocated #hb_raster_paint_t.
 *
 * XSince: REPLACEME
 **/
hb_raster_paint_t *
hb_raster_paint_create (void)
{
  hb_raster_paint_t *paint = hb_object_create<hb_raster_paint_t> ();
  if (unlikely (!paint)) return nullptr;

  paint->clip_rdr = hb_raster_draw_create ();

  return paint;
}

/**
 * hb_raster_paint_reference: (skip)
 * @paint: a paint context
 *
 * Increases the reference count on @paint by one.
 *
 * Return value: (transfer full):
 * The referenced #hb_raster_paint_t.
 *
 * XSince: REPLACEME
 **/
hb_raster_paint_t *
hb_raster_paint_reference (hb_raster_paint_t *paint)
{
  return hb_object_reference (paint);
}

/**
 * hb_raster_paint_destroy: (skip)
 * @paint: a paint context
 *
 * Decreases the reference count on @paint by one. When the
 * reference count reaches zero, the paint context is freed.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_paint_destroy (hb_raster_paint_t *paint)
{
  if (!hb_object_destroy (paint)) return;
  hb_raster_draw_destroy (paint->clip_rdr);
  for (auto *s : paint->surface_stack)
    hb_raster_image_destroy (s);
  for (auto *s : paint->surface_cache)
    hb_raster_image_destroy (s);
  hb_raster_image_destroy (paint->recycled_image);
  hb_free (paint);
}

/**
 * hb_raster_paint_set_user_data: (skip)
 * @paint: a paint context
 * @key: the user-data key
 * @data: a pointer to the user data
 * @destroy: (nullable): a callback to call when @data is not needed anymore
 * @replace: whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified paint context.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_raster_paint_set_user_data (hb_raster_paint_t  *paint,
			       hb_user_data_key_t *key,
			       void               *data,
			       hb_destroy_func_t   destroy,
			       hb_bool_t           replace)
{
  return hb_object_set_user_data (paint, key, data, destroy, replace);
}

/**
 * hb_raster_paint_get_user_data: (skip)
 * @paint: a paint context
 * @key: the user-data key
 *
 * Fetches the user-data associated with the specified key,
 * attached to the specified paint context.
 *
 * Return value: (transfer none):
 * A pointer to the user data
 *
 * XSince: REPLACEME
 **/
void *
hb_raster_paint_get_user_data (hb_raster_paint_t  *paint,
			       hb_user_data_key_t *key)
{
  return hb_object_get_user_data (paint, key);
}

/**
 * hb_raster_paint_set_transform:
 * @paint: a paint context
 * @xx: xx component of the transform matrix
 * @yx: yx component of the transform matrix
 * @xy: xy component of the transform matrix
 * @yy: yy component of the transform matrix
 * @dx: x translation
 * @dy: y translation
 *
 * Sets the base 2×3 affine transform that maps from glyph-space
 * coordinates to pixel-space coordinates.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_paint_set_transform (hb_raster_paint_t *paint,
			       float xx, float yx,
			       float xy, float yy,
			       float dx, float dy)
{
  if (unlikely (!paint)) return;
  paint->base_transform = {xx, yx, xy, yy, dx, dy};
}

/**
 * hb_raster_paint_set_extents:
 * @paint: a paint context
 * @extents: the desired output extents
 *
 * Sets the output image extents (pixel rectangle).
 *
 * Call this before hb_font_paint_glyph() for each render.
 * A common pattern is:
 * ```
 * hb_glyph_extents_t gext;
 * if (hb_font_get_glyph_extents (font, gid, &gext))
 *   hb_raster_paint_set_glyph_extents (paint, &gext);
 * ```
 *
 * XSince: REPLACEME
 **/
void
hb_raster_paint_set_extents (hb_raster_paint_t         *paint,
			     const hb_raster_extents_t *extents)
{
  if (unlikely (!paint || !extents)) return;
  paint->fixed_extents = *extents;
  paint->has_fixed_extents = true;
  if (paint->fixed_extents.stride == 0)
    paint->fixed_extents.stride = paint->fixed_extents.width * 4;
}

/**
 * hb_raster_paint_set_glyph_extents:
 * @paint: a paint context
 * @glyph_extents: glyph extents from hb_font_get_glyph_extents()
 *
 * Transforms @glyph_extents with the paint context's base transform and
 * sets the resulting output image extents.
 *
 * This is equivalent to computing a transformed bounding box in pixel
 * space and calling hb_raster_paint_set_extents().
 *
 * Return value: `true` if transformed extents are non-empty and set;
 * `false` otherwise.
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_raster_paint_set_glyph_extents (hb_raster_paint_t        *paint,
				   const hb_glyph_extents_t *glyph_extents)
{
  if (unlikely (!paint || !glyph_extents))
    return false;

  float x0 = (float) glyph_extents->x_bearing;
  float y0 = (float) glyph_extents->y_bearing;
  float x1 = (float) glyph_extents->x_bearing + glyph_extents->width;
  float y1 = (float) glyph_extents->y_bearing + glyph_extents->height;

  float xmin = hb_min (x0, x1);
  float xmax = hb_max (x0, x1);
  float ymin = hb_min (y0, y1);
  float ymax = hb_max (y0, y1);

  float px[4] = {xmin, xmin, xmax, xmax};
  float py[4] = {ymin, ymax, ymin, ymax};

  float tx, ty;
  paint->base_transform.transform_point (px[0], py[0]);
  tx = px[0]; ty = py[0];
  float tx_min = tx, tx_max = tx;
  float ty_min = ty, ty_max = ty;

  for (unsigned i = 1; i < 4; i++)
  {
    paint->base_transform.transform_point (px[i], py[i]);
    tx_min = hb_min (tx_min, px[i]);
    tx_max = hb_max (tx_max, px[i]);
    ty_min = hb_min (ty_min, py[i]);
    ty_max = hb_max (ty_max, py[i]);
  }

  int ex0 = (int) floorf (tx_min);
  int ey0 = (int) floorf (ty_min);
  int ex1 = (int) ceilf  (tx_max);
  int ey1 = (int) ceilf  (ty_max);

  if (ex1 <= ex0 || ey1 <= ey0)
  {
    paint->fixed_extents = {};
    paint->has_fixed_extents = false;
    return false;
  }

  paint->fixed_extents = {
    ex0, ey0,
    (unsigned) (ex1 - ex0),
    (unsigned) (ey1 - ey0),
    0
  };
  paint->has_fixed_extents = true;
  if (paint->fixed_extents.stride == 0)
    paint->fixed_extents.stride = paint->fixed_extents.width * 4;
  return true;
}

/**
 * hb_raster_paint_set_foreground:
 * @paint: a paint context
 * @foreground: the foreground color
 *
 * Sets the foreground color used when paint callbacks request it
 * (e.g. `is_foreground` in color stops or solid fills).
 *
 * XSince: REPLACEME
 **/
void
hb_raster_paint_set_foreground (hb_raster_paint_t *paint,
				hb_color_t         foreground)
{
  if (unlikely (!paint)) return;
  paint->foreground = foreground;
}

/**
 * hb_raster_paint_get_funcs:
 *
 * Fetches the singleton #hb_paint_funcs_t that renders color glyphs
 * into an #hb_raster_paint_t.  Pass the #hb_raster_paint_t as the
 * @paint_data argument when calling hb_font_paint_glyph().
 *
 * Return value: (transfer none):
 * The rasterizer paint functions
 *
 * XSince: REPLACEME
 **/
hb_paint_funcs_t *
hb_raster_paint_get_funcs (void)
{
  return static_raster_paint_funcs.get_unconst ();
}

/**
 * hb_raster_paint_render:
 * @paint: a paint context
 *
 * Extracts the rendered image after hb_font_paint_glyph() has
 * completed.  The paint context's surface stack is consumed and
 * the result returned as a new #hb_raster_image_t.
 *
 * Call hb_font_paint_glyph() before calling this function.
 * hb_raster_paint_set_extents() or hb_raster_paint_set_glyph_extents()
 * must be called before painting.
 * Internal drawing state is cleared here so the same object can
 * be reused without client-side clearing.
 *
 * Return value: (transfer full):
 * A newly allocated #hb_raster_image_t, or `NULL` on failure
 *
 * XSince: REPLACEME
 **/
hb_raster_image_t *
hb_raster_paint_render (hb_raster_paint_t *paint)
{
  if (unlikely (!paint)) return nullptr;

  if (unlikely (!paint->has_fixed_extents))
  {
    for (auto *s : paint->surface_stack)
      paint->release_surface (s);
    paint->surface_stack.resize (0);
    paint->transform_stack.resize (0);
    paint->clip_stack.resize (0);
    hb_raster_draw_reset (paint->clip_rdr);
    return nullptr;
  }

  hb_raster_image_t *result = nullptr;

  if (paint->surface_stack.length)
  {
    result = paint->surface_stack[0];
    /* Release any remaining group surfaces (shouldn't happen with
     * well-formed paint calls, but be safe). */
    for (unsigned i = 1; i < paint->surface_stack.length; i++)
      paint->release_surface (paint->surface_stack[i]);
    paint->surface_stack.resize (0);
  }

  /* Clean up stacks and reset auto-extents for next glyph. */
  paint->transform_stack.resize (0);
  paint->clip_stack.resize (0);
  hb_raster_draw_reset (paint->clip_rdr);
  paint->has_fixed_extents = false;
  paint->fixed_extents = {};

  return result;
}

/**
 * hb_raster_paint_reset:
 * @paint: a paint context
 *
 * Resets the paint context to its initial state, clearing all
 * configuration and cached surfaces.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_paint_reset (hb_raster_paint_t *paint)
{
  if (unlikely (!paint)) return;
  paint->base_transform = {1, 0, 0, 1, 0, 0};
  paint->fixed_extents = {};
  paint->has_fixed_extents = false;
  paint->foreground = HB_COLOR (0, 0, 0, 255);
  paint->transform_stack.resize (0);
  paint->clip_stack.resize (0);
  for (auto *s : paint->surface_stack)
    hb_raster_image_destroy (s);
  paint->surface_stack.resize (0);
  for (auto *s : paint->surface_cache)
    hb_raster_image_destroy (s);
  paint->surface_cache.resize (0);
  hb_raster_image_destroy (paint->recycled_image);
  paint->recycled_image = nullptr;
}

/**
 * hb_raster_paint_recycle_image:
 * @paint: a paint context
 * @image: a raster image to recycle
 *
 * Recycles @image for reuse by subsequent render calls.
 * The caller transfers ownership of @image to @paint.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_paint_recycle_image (hb_raster_paint_t  *paint,
			       hb_raster_image_t  *image)
{
  if (unlikely (!paint || !image)) return;
  hb_raster_image_destroy (paint->recycled_image);
  paint->recycled_image = image;
}
