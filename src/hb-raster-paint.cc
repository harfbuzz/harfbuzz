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

#include "hb-raster-paint.hh"
#include "hb-machinery.hh"

#include <math.h>


/*
 * Pixel helpers
 */

static inline uint8_t
div255 (unsigned a)
{
  return (uint8_t) ((a + 128 + ((a + 128) >> 8)) >> 8);
}

/* SRC_OVER: premultiplied src over premultiplied dst. */
static inline uint32_t
src_over (uint32_t src, uint32_t dst)
{
  uint8_t sa = (uint8_t) (src >> 24);
  if (sa == 255) return src;
  if (sa == 0) return dst;
  unsigned inv_sa = 255 - sa;
  uint8_t rb = div255 ((dst & 0xFF) * inv_sa) + (uint8_t) (src & 0xFF);
  uint8_t rg = div255 (((dst >> 8) & 0xFF) * inv_sa) + (uint8_t) ((src >> 8) & 0xFF);
  uint8_t rr = div255 (((dst >> 16) & 0xFF) * inv_sa) + (uint8_t) ((src >> 16) & 0xFF);
  uint8_t ra = div255 (((dst >> 24) & 0xFF) * inv_sa) + sa;
  return (uint32_t) rb | ((uint32_t) rg << 8) | ((uint32_t) rr << 16) | ((uint32_t) ra << 24);
}

/* Scale a premultiplied pixel by an alpha [0,255]. */
static inline uint32_t
alpha_mul (uint32_t px, unsigned a)
{
  if (a == 255) return px;
  if (a == 0) return 0;
  uint8_t rb = div255 ((px & 0xFF) * a);
  uint8_t rg = div255 (((px >> 8) & 0xFF) * a);
  uint8_t rr = div255 (((px >> 16) & 0xFF) * a);
  uint8_t ra = div255 (((px >> 24) & 0xFF) * a);
  return (uint32_t) rb | ((uint32_t) rg << 8) | ((uint32_t) rr << 16) | ((uint32_t) ra << 24);
}

/* Convert unpremultiplied hb_color_t (BGRA order) to premultiplied BGRA32 pixel. */
static inline uint32_t
color_to_premul_pixel (hb_color_t color)
{
  uint8_t a = hb_color_get_alpha (color);
  uint8_t r = div255 (hb_color_get_red (color) * a);
  uint8_t g = div255 (hb_color_get_green (color) * a);
  uint8_t b = div255 (hb_color_get_blue (color) * a);
  return (uint32_t) b | ((uint32_t) g << 8) | ((uint32_t) r << 16) | ((uint32_t) a << 24);
}


/*
 * Image compositing (operates on hb_raster_image_t)
 */

/* Pack BGRA components into a premultiplied uint32_t. */
static inline uint32_t
pack_pixel (uint8_t b, uint8_t g, uint8_t r, uint8_t a)
{
  return (uint32_t) b | ((uint32_t) g << 8) | ((uint32_t) r << 16) | ((uint32_t) a << 24);
}

/* Unpack premultiplied pixel to float RGBA [0,1]. */
static inline void
unpack_to_float (uint32_t px, float &r, float &g, float &b, float &a)
{
  b = (px & 0xFF) / 255.f;
  g = ((px >> 8) & 0xFF) / 255.f;
  r = ((px >> 16) & 0xFF) / 255.f;
  a = (px >> 24) / 255.f;
}

/* Pack float RGBA [0,1] premultiplied back to uint32_t. */
static inline uint32_t
pack_from_float (float r, float g, float b, float a)
{
  return pack_pixel ((uint8_t) (hb_clamp (b, 0.f, 1.f) * 255.f + 0.5f),
		     (uint8_t) (hb_clamp (g, 0.f, 1.f) * 255.f + 0.5f),
		     (uint8_t) (hb_clamp (r, 0.f, 1.f) * 255.f + 0.5f),
		     (uint8_t) (hb_clamp (a, 0.f, 1.f) * 255.f + 0.5f));
}

/* Separable blend mode function: operates on unpremultiplied [0,1] channels. */
static inline float
blend_multiply (float sc, float dc) { return sc * dc; }
static inline float
blend_screen (float sc, float dc) { return sc + dc - sc * dc; }
static inline float
blend_overlay (float sc, float dc)
{ return dc <= 0.5f ? 2.f * sc * dc : 1.f - 2.f * (1.f - sc) * (1.f - dc); }
static inline float
blend_darken (float sc, float dc) { return hb_min (sc, dc); }
static inline float
blend_lighten (float sc, float dc) { return hb_max (sc, dc); }
static inline float
blend_color_dodge (float sc, float dc)
{
  if (dc <= 0.f) return 0.f;
  if (sc >= 1.f) return 1.f;
  return hb_min (1.f, dc / (1.f - sc));
}
static inline float
blend_color_burn (float sc, float dc)
{
  if (dc >= 1.f) return 1.f;
  if (sc <= 0.f) return 0.f;
  return 1.f - hb_min (1.f, (1.f - dc) / sc);
}
static inline float
blend_hard_light (float sc, float dc)
{ return sc <= 0.5f ? 2.f * sc * dc : 1.f - 2.f * (1.f - sc) * (1.f - dc); }
static inline float
blend_soft_light (float sc, float dc)
{
  if (sc <= 0.5f)
    return dc - (1.f - 2.f * sc) * dc * (1.f - dc);
  float d = (dc <= 0.25f) ? ((16.f * dc - 12.f) * dc + 4.f) * dc
			   : sqrtf (dc);
  return dc + (2.f * sc - 1.f) * (d - dc);
}
static inline float
blend_difference (float sc, float dc) { return fabsf (sc - dc); }
static inline float
blend_exclusion (float sc, float dc) { return sc + dc - 2.f * sc * dc; }

/* Apply a separable blend mode per-pixel.
 * Both src and dst are premultiplied BGRA32. */
static inline uint32_t
apply_separable_blend (uint32_t src, uint32_t dst,
		       float (*blend_fn)(float, float))
{
  float sr, sg, sb, sa;
  float dr, dg, db, da;
  unpack_to_float (src, sr, sg, sb, sa);
  unpack_to_float (dst, dr, dg, db, da);

  /* Unpremultiply */
  float usr = sa > 0.f ? sr / sa : 0.f;
  float usg = sa > 0.f ? sg / sa : 0.f;
  float usb = sa > 0.f ? sb / sa : 0.f;
  float udr = da > 0.f ? dr / da : 0.f;
  float udg = da > 0.f ? dg / da : 0.f;
  float udb = da > 0.f ? db / da : 0.f;

  /* Blend (unpremultiplied channels) */
  float br = blend_fn (usr, udr);
  float bg = blend_fn (usg, udg);
  float bb = blend_fn (usb, udb);

  /* Composite formula: result = Sa*Da*B + Sa*(1-Da)*Sc + (1-Sa)*Da*Dc
   * where B is the blended result */
  float ra = sa + da - sa * da;
  float rr = sa * da * br + sa * (1.f - da) * usr + (1.f - sa) * da * udr;
  float rg = sa * da * bg + sa * (1.f - da) * usg + (1.f - sa) * da * udg;
  float rb = sa * da * bb + sa * (1.f - da) * usb + (1.f - sa) * da * udb;

  /* Re-premultiply */
  return pack_from_float (rr, rg, rb, ra);
}

/* HSL helpers */
static inline float
hsl_luminosity (float r, float g, float b)
{ return 0.299f * r + 0.587f * g + 0.114f * b; }

static inline float
hsl_saturation (float r, float g, float b)
{ return hb_max (hb_max (r, g), b) - hb_min (hb_min (r, g), b); }

static inline void
hsl_clip_color (float &r, float &g, float &b)
{
  float l = hsl_luminosity (r, g, b);
  float mn = hb_min (hb_min (r, g), b);
  float mx = hb_max (hb_max (r, g), b);
  if (mn < 0.f)
  {
    float d = l - mn;
    if (d > 0.f) { r = l + (r - l) * l / d; g = l + (g - l) * l / d; b = l + (b - l) * l / d; }
  }
  if (mx > 1.f)
  {
    float d = mx - l;
    if (d > 0.f) { r = l + (r - l) * (1.f - l) / d; g = l + (g - l) * (1.f - l) / d; b = l + (b - l) * (1.f - l) / d; }
  }
}

static inline void
hsl_set_luminosity (float &r, float &g, float &b, float l)
{
  float d = l - hsl_luminosity (r, g, b);
  r += d; g += d; b += d;
  hsl_clip_color (r, g, b);
}

static inline void
hsl_set_saturation_inner (float &mn, float &mid, float &mx, float s)
{
  if (mx > mn)
  {
    mid = (mid - mn) * s / (mx - mn);
    mx = s;
  }
  else
    mid = mx = 0.f;
  mn = 0.f;
}

static inline void
hsl_set_saturation (float &r, float &g, float &b, float s)
{
  /* Sort and apply set_saturation to the sorted triple */
  if (r <= g)
  {
    if (g <= b)      hsl_set_saturation_inner (r, g, b, s);
    else if (r <= b) hsl_set_saturation_inner (r, b, g, s);
    else             hsl_set_saturation_inner (b, r, g, s);
  }
  else
  {
    if (r <= b)      hsl_set_saturation_inner (g, r, b, s);
    else if (g <= b) hsl_set_saturation_inner (g, b, r, s);
    else             hsl_set_saturation_inner (b, g, r, s);
  }
}

/* Apply an HSL blend mode per-pixel. */
static inline uint32_t
apply_hsl_blend (uint32_t src, uint32_t dst,
		 hb_paint_composite_mode_t mode)
{
  float sr, sg, sb, sa;
  float dr, dg, db, da;
  unpack_to_float (src, sr, sg, sb, sa);
  unpack_to_float (dst, dr, dg, db, da);

  /* Unpremultiply */
  float usr = sa > 0.f ? sr / sa : 0.f;
  float usg = sa > 0.f ? sg / sa : 0.f;
  float usb = sa > 0.f ? sb / sa : 0.f;
  float udr = da > 0.f ? dr / da : 0.f;
  float udg = da > 0.f ? dg / da : 0.f;
  float udb = da > 0.f ? db / da : 0.f;

  float br = udr, bg = udg, bb = udb;

  if (mode == HB_PAINT_COMPOSITE_MODE_HSL_HUE)
  {
    br = usr; bg = usg; bb = usb;
    hsl_set_saturation (br, bg, bb, hsl_saturation (udr, udg, udb));
    hsl_set_luminosity (br, bg, bb, hsl_luminosity (udr, udg, udb));
  }
  else if (mode == HB_PAINT_COMPOSITE_MODE_HSL_SATURATION)
  {
    br = udr; bg = udg; bb = udb;
    hsl_set_saturation (br, bg, bb, hsl_saturation (usr, usg, usb));
    hsl_set_luminosity (br, bg, bb, hsl_luminosity (udr, udg, udb));
  }
  else if (mode == HB_PAINT_COMPOSITE_MODE_HSL_COLOR)
  {
    br = usr; bg = usg; bb = usb;
    hsl_set_luminosity (br, bg, bb, hsl_luminosity (udr, udg, udb));
  }
  else /* HSL_LUMINOSITY */
  {
    br = udr; bg = udg; bb = udb;
    hsl_set_luminosity (br, bg, bb, hsl_luminosity (usr, usg, usb));
  }

  float ra = sa + da - sa * da;
  float rr = sa * da * br + sa * (1.f - da) * usr + (1.f - sa) * da * udr;
  float rg = sa * da * bg + sa * (1.f - da) * usg + (1.f - sa) * da * udg;
  float rb = sa * da * bb + sa * (1.f - da) * usb + (1.f - sa) * da * udb;

  return pack_from_float (rr, rg, rb, ra);
}

/* Porter-Duff composite: result = Fa * src + Fb * dst.
 * Operates on premultiplied BGRA32 pixels. */
static inline uint32_t
porter_duff (uint32_t src, uint32_t dst, unsigned fa_num, unsigned fa_den,
	     unsigned fb_num, unsigned fb_den)
{
  auto channel = [&] (unsigned sc, unsigned dc) -> uint8_t
  {
    return (uint8_t) hb_clamp ((int) (sc * fa_num / fa_den + dc * fb_num / fb_den), 0, 255);
  };
  uint8_t rb = channel (src & 0xFF, dst & 0xFF);
  uint8_t rg = channel ((src >> 8) & 0xFF, (dst >> 8) & 0xFF);
  uint8_t rr = channel ((src >> 16) & 0xFF, (dst >> 16) & 0xFF);
  uint8_t ra = channel (src >> 24, dst >> 24);
  return pack_pixel (rb, rg, rr, ra);
}

/* Composite per-pixel with full blend mode support. */
static inline uint32_t
composite_pixel (uint32_t src, uint32_t dst,
		 hb_paint_composite_mode_t mode)
{
  uint8_t sa = (uint8_t) (src >> 24);
  uint8_t da = (uint8_t) (dst >> 24);

  switch (mode)
  {
  case HB_PAINT_COMPOSITE_MODE_CLEAR:
    return 0;
  case HB_PAINT_COMPOSITE_MODE_SRC:
    return src;
  case HB_PAINT_COMPOSITE_MODE_DEST:
    return dst;
  case HB_PAINT_COMPOSITE_MODE_SRC_OVER:
    return src_over (src, dst);
  case HB_PAINT_COMPOSITE_MODE_DEST_OVER:
    return src_over (dst, src);
  case HB_PAINT_COMPOSITE_MODE_SRC_IN:
    return alpha_mul (src, da);
  case HB_PAINT_COMPOSITE_MODE_DEST_IN:
    return alpha_mul (dst, sa);
  case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
    return alpha_mul (src, 255 - da);
  case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
    return alpha_mul (dst, 255 - sa);
  case HB_PAINT_COMPOSITE_MODE_SRC_ATOP:
    return porter_duff (src, dst, da, 255, 255 - sa, 255);
  case HB_PAINT_COMPOSITE_MODE_DEST_ATOP:
    return porter_duff (dst, src, sa, 255, 255 - da, 255);
  case HB_PAINT_COMPOSITE_MODE_XOR:
    return porter_duff (src, dst, 255 - da, 255, 255 - sa, 255);
  case HB_PAINT_COMPOSITE_MODE_PLUS:
  {
    uint8_t rb = (uint8_t) hb_min (255u, (unsigned) (src & 0xFF) + (dst & 0xFF));
    uint8_t rg = (uint8_t) hb_min (255u, (unsigned) ((src >> 8) & 0xFF) + ((dst >> 8) & 0xFF));
    uint8_t rr = (uint8_t) hb_min (255u, (unsigned) ((src >> 16) & 0xFF) + ((dst >> 16) & 0xFF));
    uint8_t ra = (uint8_t) hb_min (255u, (unsigned) (src >> 24) + (dst >> 24));
    return pack_pixel (rb, rg, rr, ra);
  }

  /* Separable blend modes */
  case HB_PAINT_COMPOSITE_MODE_MULTIPLY:  return apply_separable_blend (src, dst, blend_multiply);
  case HB_PAINT_COMPOSITE_MODE_SCREEN:    return apply_separable_blend (src, dst, blend_screen);
  case HB_PAINT_COMPOSITE_MODE_OVERLAY:   return apply_separable_blend (src, dst, blend_overlay);
  case HB_PAINT_COMPOSITE_MODE_DARKEN:    return apply_separable_blend (src, dst, blend_darken);
  case HB_PAINT_COMPOSITE_MODE_LIGHTEN:   return apply_separable_blend (src, dst, blend_lighten);
  case HB_PAINT_COMPOSITE_MODE_COLOR_DODGE: return apply_separable_blend (src, dst, blend_color_dodge);
  case HB_PAINT_COMPOSITE_MODE_COLOR_BURN:  return apply_separable_blend (src, dst, blend_color_burn);
  case HB_PAINT_COMPOSITE_MODE_HARD_LIGHT:  return apply_separable_blend (src, dst, blend_hard_light);
  case HB_PAINT_COMPOSITE_MODE_SOFT_LIGHT:  return apply_separable_blend (src, dst, blend_soft_light);
  case HB_PAINT_COMPOSITE_MODE_DIFFERENCE:  return apply_separable_blend (src, dst, blend_difference);
  case HB_PAINT_COMPOSITE_MODE_EXCLUSION:   return apply_separable_blend (src, dst, blend_exclusion);

  /* HSL blend modes */
  case HB_PAINT_COMPOSITE_MODE_HSL_HUE:
  case HB_PAINT_COMPOSITE_MODE_HSL_SATURATION:
  case HB_PAINT_COMPOSITE_MODE_HSL_COLOR:
  case HB_PAINT_COMPOSITE_MODE_HSL_LUMINOSITY:
    return apply_hsl_blend (src, dst, mode);

  default:
    return src_over (src, dst);
  }
}

/* Composite src image onto dst image.
 * Both images must have the same extents and BGRA32 format. */
static void
composite_images (hb_raster_image_t *dst,
		  const hb_raster_image_t *src,
		  hb_paint_composite_mode_t mode)
{
  unsigned w = dst->extents.width;
  unsigned h = dst->extents.height;
  unsigned stride = dst->extents.stride;

  for (unsigned y = 0; y < h; y++)
  {
    uint32_t *dp = reinterpret_cast<uint32_t *> (dst->buffer.arrayZ + y * stride);
    const uint32_t *sp = reinterpret_cast<const uint32_t *> (src->buffer.arrayZ + y * stride);
    for (unsigned x = 0; x < w; x++)
      dp[x] = composite_pixel (sp[x], dp[x], mode);
  }
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
  hb_raster_extents_t clip_ext = {
    surf->extents.x_origin, surf->extents.y_origin,
    w, h, 0
  };
  hb_raster_draw_set_extents (rdr, &clip_ext);

  hb_font_draw_glyph (font, glyph, hb_raster_draw_get_funcs (), rdr);
  hb_raster_image_t *mask_img = hb_raster_draw_render (rdr);
  if (unlikely (!mask_img))
  {
    /* If mask rendering fails, push a fully transparent clip */
    new_clip.init_full (w, h);
    new_clip.is_rect = true;
    new_clip.rect_x0 = new_clip.rect_y0 = 0;
    new_clip.rect_x1 = new_clip.rect_y1 = 0;
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
    c->clip_stack.push (new_clip);
    return;
  }

  const uint8_t *mask_buf = hb_raster_image_get_buffer (mask_img);
  hb_raster_extents_t mask_ext;
  hb_raster_image_get_extents (mask_img, &mask_ext);
  const hb_raster_clip_t &old_clip = c->current_clip ();

  for (unsigned y = 0; y < h; y++)
    for (unsigned x = 0; x < w; x++)
    {
      uint8_t glyph_alpha = (x < mask_ext.width && y < mask_ext.height)
			    ? mask_buf[y * mask_ext.stride + x] : 0;
      uint8_t old_alpha = old_clip.get_alpha (x, y);
      new_clip.alpha[y * new_clip.stride + x] = div255 (glyph_alpha * old_alpha);
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

  hb_raster_image_t *surf = c->current_surface ();
  if (unlikely (!surf)) return;

  unsigned w = surf->extents.width;
  unsigned h = surf->extents.height;

  const hb_transform_t<> &t = c->current_transform ();

  /* Transform rectangle corners to pixel space */
  hb_extents_t<> rect_ext {xmin, ymin, xmax, ymax};
  t.transform_extents (rect_ext);

  /* Convert to pixel coordinates relative to surface origin */
  int px0 = (int) floorf (rect_ext.xmin) - surf->extents.x_origin;
  int py0 = (int) floorf (rect_ext.ymin) - surf->extents.y_origin;
  int px1 = (int) ceilf (rect_ext.xmax) - surf->extents.x_origin;
  int py1 = (int) ceilf (rect_ext.ymax) - surf->extents.y_origin;

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

  if (old_clip.is_rect)
  {
    /* Fast path: rect-on-rect intersection */
    new_clip.is_rect = true;
    new_clip.rect_x0 = hb_max (px0, old_clip.rect_x0);
    new_clip.rect_y0 = hb_max (py0, old_clip.rect_y0);
    new_clip.rect_x1 = hb_min (px1, old_clip.rect_x1);
    new_clip.rect_y1 = hb_min (py1, old_clip.rect_y1);
  }
  else
  {
    /* General case: intersect rectangle with existing alpha mask */
    new_clip.is_rect = false;
    if (unlikely (!new_clip.alpha.resize (new_clip.stride * h)))
    {
      new_clip.init_full (w, h);
      new_clip.is_rect = true;
      new_clip.rect_x0 = new_clip.rect_y0 = 0;
      new_clip.rect_x1 = new_clip.rect_y1 = 0;
      c->clip_stack.push (new_clip);
      return;
    }
    memset (new_clip.alpha.arrayZ, 0, new_clip.stride * h);
    for (int y = py0; y < py1; y++)
      for (int x = px0; x < px1; x++)
	new_clip.alpha[y * new_clip.stride + x] = old_clip.get_alpha (x, y);
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
    composite_images (dst, src, mode);

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
		      div255 (hb_color_get_alpha (c->foreground) *
			      hb_color_get_alpha (color)));
  }

  uint32_t premul = color_to_premul_pixel (color);
  const hb_raster_clip_t &clip = c->current_clip ();

  unsigned w = surf->extents.width;
  unsigned h = surf->extents.height;
  unsigned stride = surf->extents.stride;

  for (unsigned y = 0; y < h; y++)
  {
    uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + y * stride);
    for (unsigned x = 0; x < w; x++)
    {
      uint8_t clip_alpha = clip.get_alpha (x, y);
      if (clip_alpha == 0) continue;
      uint32_t src = alpha_mul (premul, clip_alpha);
      row[x] = src_over (src, row[x]);
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

  unsigned surf_w = surf->extents.width;
  unsigned surf_h = surf->extents.height;
  unsigned surf_stride = surf->extents.stride;
  int ox = surf->extents.x_origin;
  int oy = surf->extents.y_origin;

  /* Image source rectangle in glyph space */
  float img_x = extents->x_bearing;
  float img_y = extents->y_bearing + extents->height; /* bottom-left in glyph space */
  float img_sx = (float) extents->width / width;
  float img_sy = (float) -extents->height / height; /* flip Y */

  for (unsigned py = 0; py < surf_h; py++)
  {
    uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * surf_stride);
    for (unsigned px = 0; px < surf_w; px++)
    {
      uint8_t clip_alpha = clip.get_alpha (px, py);
      if (clip_alpha == 0) continue;

      /* Map pixel to glyph space */
      float gx = inv_xx * (px + ox) + inv_xy * (py + oy) + inv_x0;
      float gy = inv_yx * (px + ox) + inv_yy * (py + oy) + inv_y0;

      /* Map glyph space to image texel */
      int ix = (int) floorf ((gx - img_x) / img_sx);
      int iy = (int) floorf ((gy - img_y) / img_sy);

      if (ix < 0 || ix >= (int) width || iy < 0 || iy >= (int) height)
	continue;

      uint32_t src_px = reinterpret_cast<const uint32_t *> (data)[iy * width + ix];
      src_px = alpha_mul (src_px, clip_alpha);
      row[px] = src_over (src_px, row[px]);
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
				    div255 (hb_color_get_alpha (c->foreground) *
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
    t = t - floorf (t);
    int period = (int) floorf (t);
    float frac = t - floorf (t);
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
    unsigned w = surf->extents.width;
    unsigned h = surf->extents.height;
    unsigned stride = surf->extents.stride;
    int ox = surf->extents.x_origin;
    int oy = surf->extents.y_origin;

    for (unsigned py = 0; py < h; py++)
    {
      uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
      for (unsigned px = 0; px < w; px++)
      {
	uint8_t clip_alpha = clip.get_alpha (px, py);
	if (clip_alpha == 0) continue;

	/* Pixel center → glyph space */
	float gx = inv_xx * (px + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (px + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;

	/* Project onto gradient axis: t = dot(p - p0, dir) / dot(dir, dir) */
	float proj_t = ((gx - gx0) * dx + (gy - gy0) * dy) * inv_denom;

	uint32_t src = evaluate_color_line (stops, len, proj_t, extend);
	src = alpha_mul (src, clip_alpha);
	row[px] = src_over (src, row[px]);
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
    unsigned w = surf->extents.width;
    unsigned h = surf->extents.height;
    unsigned stride = surf->extents.stride;
    int ox = surf->extents.x_origin;
    int oy = surf->extents.y_origin;

    for (unsigned py = 0; py < h; py++)
    {
      uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
      for (unsigned px = 0; px < w; px++)
      {
	uint8_t clip_alpha = clip.get_alpha (px, py);
	if (clip_alpha == 0) continue;

	float gx = inv_xx * (px + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (px + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;

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
	src = alpha_mul (src, clip_alpha);
	row[px] = src_over (src, row[px]);
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
    unsigned w = surf->extents.width;
    unsigned h = surf->extents.height;
    unsigned stride = surf->extents.stride;
    int ox = surf->extents.x_origin;
    int oy = surf->extents.y_origin;

    for (unsigned py = 0; py < h; py++)
    {
      uint32_t *row = reinterpret_cast<uint32_t *> (surf->buffer.arrayZ + py * stride);
      for (unsigned px = 0; px < w; px++)
      {
	uint8_t clip_alpha = clip.get_alpha (px, py);
	if (clip_alpha == 0) continue;

	float gx = inv_xx * (px + ox + 0.5f) + inv_xy * (py + oy + 0.5f) + inv_x0;
	float gy = inv_yx * (px + ox + 0.5f) + inv_yy * (py + oy + 0.5f) + inv_y0;

	float angle = atan2f (gy - cy, gx - cx);

	/* Map angle to normalized gradient parameter */
	float grad_t = (angle - a0) * inv_angle_range;

	uint32_t src = evaluate_color_line (stops, len, grad_t, extend);
	src = alpha_mul (src, clip_alpha);
	row[px] = src_over (src, row[px]);
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
 * The paint context's stacks are initialized before paint and
 * cleaned up here.
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

  /* Clean up stacks */
  paint->transform_stack.resize (0);
  paint->clip_stack.resize (0);

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
