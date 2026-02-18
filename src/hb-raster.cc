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
#include "hb-machinery.hh"

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#define HB_RASTER_NEON 1
#elif defined(__SSE4_1__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 4) || defined(__AVX__)
#include <smmintrin.h>
#define HB_RASTER_SSE41 1
#endif


/*
 * hb_raster_image_t
 */

hb_raster_image_t *
hb_raster_image_reference (hb_raster_image_t *image)
{
  return hb_object_reference (image);
}

void
hb_raster_image_destroy (hb_raster_image_t *image)
{
  if (!hb_object_destroy (image)) return;
  hb_free (image);
}

hb_bool_t
hb_raster_image_set_user_data (hb_raster_image_t  *image,
			       hb_user_data_key_t *key,
			       void               *data,
			       hb_destroy_func_t   destroy,
			       hb_bool_t           replace)
{
  return hb_object_set_user_data (image, key, data, destroy, replace);
}

void *
hb_raster_image_get_user_data (hb_raster_image_t  *image,
			       hb_user_data_key_t *key)
{
  return hb_object_get_user_data (image, key);
}

const uint8_t *
hb_raster_image_get_buffer (hb_raster_image_t *image)
{
  if (unlikely (!image)) return nullptr;
  return image->buffer;
}

void
hb_raster_image_get_extents (hb_raster_image_t   *image,
			     hb_raster_extents_t *extents)
{
  if (unlikely (!image || !extents)) return;
  *extents = image->extents;
}

hb_raster_format_t
hb_raster_image_get_format (hb_raster_image_t *image)
{
  if (unlikely (!image)) return HB_RASTER_FORMAT_A8;
  return image->format;
}


/*
 * hb_raster_draw_t
 */

hb_raster_draw_t *
hb_raster_draw_create (void)
{
  hb_raster_draw_t *draw = hb_object_create<hb_raster_draw_t> ();
  if (unlikely (!draw)) return nullptr;
  return draw;
}

hb_raster_draw_t *
hb_raster_draw_reference (hb_raster_draw_t *draw)
{
  return hb_object_reference (draw);
}

void
hb_raster_draw_destroy (hb_raster_draw_t *draw)
{
  if (!hb_object_destroy (draw)) return;
  hb_free (draw);
}

hb_bool_t
hb_raster_draw_set_user_data (hb_raster_draw_t   *draw,
			      hb_user_data_key_t *key,
			      void               *data,
			      hb_destroy_func_t   destroy,
			      hb_bool_t           replace)
{
  return hb_object_set_user_data (draw, key, data, destroy, replace);
}

void *
hb_raster_draw_get_user_data (hb_raster_draw_t   *draw,
			      hb_user_data_key_t *key)
{
  return hb_object_get_user_data (draw, key);
}

void
hb_raster_draw_set_format (hb_raster_draw_t  *draw,
			   hb_raster_format_t format)
{
  if (unlikely (!draw)) return;
  draw->format = format;
}

hb_raster_format_t
hb_raster_draw_get_format (hb_raster_draw_t *draw)
{
  if (unlikely (!draw)) return HB_RASTER_FORMAT_A8;
  return draw->format;
}

void
hb_raster_draw_set_transform (hb_raster_draw_t *draw,
			      float xx, float yx,
			      float xy, float yy,
			      float dx, float dy)
{
  if (unlikely (!draw)) return;
  draw->transform = {xx, yx, xy, yy, dx, dy};
}

void
hb_raster_draw_get_transform (hb_raster_draw_t *draw,
			      float *xx, float *yx,
			      float *xy, float *yy,
			      float *dx, float *dy)
{
  if (unlikely (!draw)) return;
  if (xx) *xx = draw->transform.xx;
  if (yx) *yx = draw->transform.yx;
  if (xy) *xy = draw->transform.xy;
  if (yy) *yy = draw->transform.yy;
  if (dx) *dx = draw->transform.x0;
  if (dy) *dy = draw->transform.y0;
}

void
hb_raster_draw_set_extents (hb_raster_draw_t          *draw,
			    const hb_raster_extents_t *extents)
{
  if (unlikely (!draw || !extents)) return;
  draw->fixed_extents     = *extents;
  draw->has_fixed_extents = true;
}

void
hb_raster_draw_reset (hb_raster_draw_t *draw)
{
  if (unlikely (!draw)) return;
  draw->format            = HB_RASTER_FORMAT_A8;
  draw->transform         = {1, 0, 0, 1, 0, 0};
  draw->fixed_extents     = {};
  draw->has_fixed_extents = false;
  draw->edges.resize (0);
  draw->tile_offsets.resize (0);
  draw->tile_edge_indices.resize (0);
}


/*
 * Draw callbacks — flatten on the fly into hb_raster_edge_t
 */

static inline void
transform_point (const hb_raster_draw_t *draw,
		 float  x,  float  y,
		 float &tx, float &ty)
{
  tx = x; ty = y;
  draw->transform.transform_point (tx, ty);
}

static void
emit_segment (hb_raster_draw_t *draw,
	      float x0, float y0,
	      float x1, float y1)
{
  int32_t X0 = (int32_t) roundf (x0 * 64.f);
  int32_t Y0 = (int32_t) roundf (y0 * 64.f);
  int32_t X1 = (int32_t) roundf (x1 * 64.f);
  int32_t Y1 = (int32_t) roundf (y1 * 64.f);

  if (Y0 == Y1) return; /* horizontal — skip */

  hb_raster_edge_t e;
  if (Y0 < Y1) {
    e.xL = X0; e.yL = Y0; e.xH = X1; e.yH = Y1; e.wind = +1;
  } else {
    e.xL = X1; e.yL = Y1; e.xH = X0; e.yH = Y0; e.wind = -1;
  }
  e.dx = e.xH - e.xL;
  e.dy = e.yH - e.yL;

  draw->edges.push (e);
}

/* Quadratic Bézier flattener — de Casteljau at t=0.5 */
static void
flatten_quadratic (hb_raster_draw_t *draw,
		   float x0, float y0,
		   float x1, float y1,
		   float x2, float y2,
		   int depth)
{
  /* Flatness: squared distance of midpoint deviation from chord */
  float mx = x0 * 0.25f + x1 * 0.5f + x2 * 0.25f;
  float my = y0 * 0.25f + y1 * 0.5f + y2 * 0.25f;
  float chord_mx = (x0 + x2) * 0.5f;
  float chord_my = (y0 + y2) * 0.5f;
  float dx = mx - chord_mx;
  float dy = my - chord_my;
  /* threshold: (0.25 * 64)^2 = 16^2 = 256 in 26.6; in float space (0.25)^2 */
  static const float flat_thresh = 0.25f * 0.25f;

  if (depth >= 16 || (dx * dx + dy * dy) <= flat_thresh) {
    emit_segment (draw, x0, y0, x2, y2);
    return;
  }

  float x01 = (x0 + x1) * 0.5f, y01 = (y0 + y1) * 0.5f;
  float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
  float xm  = (x01 + x12) * 0.5f, ym  = (y01 + y12) * 0.5f;

  flatten_quadratic (draw, x0, y0, x01, y01, xm,  ym,  depth + 1);
  flatten_quadratic (draw, xm, ym, x12, y12, x2,  y2,  depth + 1);
}

/* Cubic Bézier flattener — de Casteljau at t=0.5 */
static void
flatten_cubic (hb_raster_draw_t *draw,
	       float x0, float y0,
	       float x1, float y1,
	       float x2, float y2,
	       float x3, float y3,
	       int depth)
{
  /* Check both control points */
  float chord_x1 = x0 + (x3 - x0) * (1.f / 3.f);
  float chord_y1 = y0 + (y3 - y0) * (1.f / 3.f);
  float chord_x2 = x0 + (x3 - x0) * (2.f / 3.f);
  float chord_y2 = y0 + (y3 - y0) * (2.f / 3.f);
  float dx1 = x1 - chord_x1, dy1 = y1 - chord_y1;
  float dx2 = x2 - chord_x2, dy2 = y2 - chord_y2;

  static const float flat_thresh = 0.25f * 0.25f;

  if (depth >= 16 ||
      ((dx1 * dx1 + dy1 * dy1) <= flat_thresh &&
       (dx2 * dx2 + dy2 * dy2) <= flat_thresh))
  {
    emit_segment (draw, x0, y0, x3, y3);
    return;
  }

  float x01  = (x0 + x1) * 0.5f, y01  = (y0 + y1) * 0.5f;
  float x12  = (x1 + x2) * 0.5f, y12  = (y1 + y2) * 0.5f;
  float x23  = (x2 + x3) * 0.5f, y23  = (y2 + y3) * 0.5f;
  float x012 = (x01 + x12) * 0.5f, y012 = (y01 + y12) * 0.5f;
  float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
  float xm   = (x012 + x123) * 0.5f, ym   = (y012 + y123) * 0.5f;

  flatten_cubic (draw, x0, y0, x01, y01, x012, y012, xm, ym, depth + 1);
  flatten_cubic (draw, xm, ym, x123, y123, x23, y23, x3, y3, depth + 1);
}


/* Draw callback implementations */

static void
hb_raster_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		   void *draw_data,
		   hb_draw_state_t *st HB_UNUSED,
		   float to_x HB_UNUSED, float to_y HB_UNUSED,
		   void *user_data HB_UNUSED)
{
  /* no-op: state tracked by hb_draw_state_t */
}

static void
hb_raster_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		   void *draw_data,
		   hb_draw_state_t *st,
		   float to_x, float to_y,
		   void *user_data HB_UNUSED)
{
  hb_raster_draw_t *draw = (hb_raster_draw_t *) draw_data;

  float tx0, ty0, tx1, ty1;
  transform_point (draw, st->current_x, st->current_y, tx0, ty0);
  transform_point (draw, to_x,          to_y,           tx1, ty1);
  emit_segment (draw, tx0, ty0, tx1, ty1);
}

static void
hb_raster_quadratic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			void *draw_data,
			hb_draw_state_t *st,
			float control_x, float control_y,
			float to_x, float to_y,
			void *user_data HB_UNUSED)
{
  hb_raster_draw_t *draw = (hb_raster_draw_t *) draw_data;

  float tx0, ty0, tx1, ty1, tx2, ty2;
  transform_point (draw, st->current_x, st->current_y, tx0, ty0);
  transform_point (draw, control_x,     control_y,      tx1, ty1);
  transform_point (draw, to_x,          to_y,           tx2, ty2);
  flatten_quadratic (draw, tx0, ty0, tx1, ty1, tx2, ty2, 0);
}

static void
hb_raster_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		    void *draw_data,
		    hb_draw_state_t *st,
		    float control1_x, float control1_y,
		    float control2_x, float control2_y,
		    float to_x, float to_y,
		    void *user_data HB_UNUSED)
{
  hb_raster_draw_t *draw = (hb_raster_draw_t *) draw_data;

  float tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3;
  transform_point (draw, st->current_x, st->current_y, tx0, ty0);
  transform_point (draw, control1_x,    control1_y,     tx1, ty1);
  transform_point (draw, control2_x,    control2_y,     tx2, ty2);
  transform_point (draw, to_x,          to_y,           tx3, ty3);
  flatten_cubic (draw, tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3, 0);
}

static void
hb_raster_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED,
		      void *draw_data HB_UNUSED,
		      hb_draw_state_t *st HB_UNUSED,
		      void *user_data HB_UNUSED)
{
  /* no-op: hb_draw_funcs_t already emits closing line_to before us */
}


/* Lazy-loader singleton for draw funcs */

static inline void free_static_raster_draw_funcs ();

static struct hb_raster_draw_funcs_lazy_loader_t : hb_draw_funcs_lazy_loader_t<hb_raster_draw_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();

    hb_draw_funcs_set_move_to_func      (funcs, hb_raster_move_to,      nullptr, nullptr);
    hb_draw_funcs_set_line_to_func      (funcs, hb_raster_line_to,      nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (funcs, hb_raster_quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func     (funcs, hb_raster_cubic_to,     nullptr, nullptr);
    hb_draw_funcs_set_close_path_func   (funcs, hb_raster_close_path,   nullptr, nullptr);

    hb_draw_funcs_make_immutable (funcs);

    hb_atexit (free_static_raster_draw_funcs);

    return funcs;
  }
} static_raster_draw_funcs;

static inline void
free_static_raster_draw_funcs ()
{
  static_raster_draw_funcs.free_instance ();
}

hb_draw_funcs_t *
hb_raster_draw_get_funcs (void)
{
  return static_raster_draw_funcs.get_unconst ();
}


/*
 * Tile rasterization
 */

/* Sub-pixel sample offsets in 26.6 units within a pixel cell */
static const unsigned HB_RASTER_SAMPLES = 8;
static const int32_t sx26[HB_RASTER_SAMPLES] = { 10, 22, 35, 51, 13, 29, 45, 54 };
static const int32_t sy26[HB_RASTER_SAMPLES] = { 19, 51, 10, 29, 38, 29, 54, 13 };

/* Rasterize a 16×16 tile at pixel position (px0, py0) using nonzero winding.
   tile_edges/n_edges: the edges that overlap this tile.
   image: output image. */

#if defined (HB_RASTER_NEON)

static void
rasterize_tile_neon (hb_raster_image_t          *image,
		     int                         px0,
		     int                         py0,
		     int                         px1,
		     int                         py1,
		     const hb_raster_edge_t     *tile_edges,
		     unsigned                    n_edges)
{
  uint8_t  *buf    = image->buffer;
  unsigned  stride = image->extents.stride;
  int       x_org  = image->extents.x_origin;
  int       y_org  = image->extents.y_origin;

  for (int row = 0; row < 16; row++)
  {
    int     py       = py0 + row;
    if (py >= py1) break;
    int32_t y_base26 = py << 6;
    uint8_t *row_buf = buf + (unsigned)(py - y_org) * stride;

    /* Process 4 pixels at a time */
    for (int col = 0; col < 16; col += 4)
    {
      int n_write = hb_min (4, px1 - (px0 + col));
      if (n_write <= 0) break;
      /* x_base for 4 adjacent pixels */
      int32_t xb[4] = {
	(px0 + col + 0) << 6,
	(px0 + col + 1) << 6,
	(px0 + col + 2) << 6,
	(px0 + col + 3) << 6,
      };
      int32x4_t x_base = vld1q_s32 (xb);

      int32x4_t inside_count = vdupq_n_s32 (0);

      for (unsigned k = 0; k < HB_RASTER_SAMPLES; k++)
      {
	int32_t ys = y_base26 + sy26[k];
	int32x4_t xs = vaddq_s32 (x_base, vdupq_n_s32 (sx26[k]));

	int32x4_t winding = vdupq_n_s32 (0);

	for (unsigned e = 0; e < n_edges; e++)
	{
	  const hb_raster_edge_t &edge = tile_edges[e];
	  if (ys < edge.yL || ys >= edge.yH) continue;

	  int64_t B = (int64_t)(ys - edge.yL) * edge.dx;
	  int32_t dy = edge.dy;
	  int32_t xL = edge.xL;
	  int32_t wind = edge.wind;

	  /* expr[i] = (xL - xs[i]) * dy + B */
	  /* compute (xL - xs) as int32x4 */
	  int32x4_t xL_minus_xs = vsubq_s32 (vdupq_n_s32 (xL), xs);

	  /* widen to int64 pairs for full precision */
	  int64x2_t expr_lo = vmlal_s32 (vdupq_n_s64 (B),
					  vget_low_s32  (xL_minus_xs),
					  vdup_n_s32 (dy));
	  int64x2_t expr_hi = vmlal_s32 (vdupq_n_s64 (B),
					  vget_high_s32 (xL_minus_xs),
					  vdup_n_s32 (dy));

	  /* mask: expr > 0 */
	  uint64x2_t mask_lo = vcgtq_s64 (expr_lo, vdupq_n_s64 (0));
	  uint64x2_t mask_hi = vcgtq_s64 (expr_hi, vdupq_n_s64 (0));

	  /* narrow masks to 32-bit (top 32 bits of each 64-bit lane hold the value) */
	  uint32x2_t narrow_lo = vmovn_u64 (mask_lo);
	  uint32x2_t narrow_hi = vmovn_u64 (mask_hi);
	  uint32x4_t mask32 = vcombine_u32 (narrow_lo, narrow_hi);

	  /* masked wind add */
	  int32x4_t wind_v = vdupq_n_s32 (wind);
	  winding = vaddq_s32 (winding,
			       vandq_s32 (wind_v, vreinterpretq_s32_u32 (mask32)));
	}

	/* if winding != 0: inside_count++ */
	uint32x4_t nonzero = vtstq_s32 (winding, winding);
	inside_count = vaddq_s32 (inside_count,
				  vreinterpretq_s32_u32 (vshrq_n_u32 (nonzero, 31)));
      }

      /* scale to [0,255] and store (only in-bounds pixels) */
      int32_t ic[4];
      vst1q_s32 (ic, inside_count);
      int bx = px0 + col - x_org;
      for (int i = 0; i < n_write; i++)
	row_buf[bx + i] = (uint8_t)((ic[i] * 255 + HB_RASTER_SAMPLES / 2) / HB_RASTER_SAMPLES);
    }
  }
}

static inline void
rasterize_tile (hb_raster_image_t      *image,
		int                     px0,
		int                     py0,
		int                     px1,
		int                     py1,
		const hb_raster_edge_t *tile_edges,
		unsigned                n_edges)
{ rasterize_tile_neon (image, px0, py0, px1, py1, tile_edges, n_edges); }

#elif defined (HB_RASTER_SSE41)

static void
rasterize_tile_sse41 (hb_raster_image_t          *image,
		      int                         px0,
		      int                         py0,
		      int                         px1,
		      int                         py1,
		      const hb_raster_edge_t     *tile_edges,
		      unsigned                    n_edges)
{
  uint8_t  *buf    = image->buffer;
  unsigned  stride = image->extents.stride;
  int       x_org  = image->extents.x_origin;
  int       y_org  = image->extents.y_origin;

  for (int row = 0; row < 16; row++)
  {
    int     py       = py0 + row;
    if (py >= py1) break;
    int32_t y_base26 = py << 6;
    uint8_t *row_buf = buf + (unsigned)(py - y_org) * stride;

    for (int col = 0; col < 16; col += 4)
    {
      int n_write = hb_min (4, px1 - (px0 + col));
      if (n_write <= 0) break;
      int32_t xb[4] = {
	(px0 + col + 0) << 6,
	(px0 + col + 1) << 6,
	(px0 + col + 2) << 6,
	(px0 + col + 3) << 6,
      };
      __m128i x_base = _mm_loadu_si128 ((__m128i *) xb);

      __m128i inside_count = _mm_setzero_si128 ();

      for (unsigned k = 0; k < HB_RASTER_SAMPLES; k++)
      {
	int32_t ys = y_base26 + sy26[k];
	__m128i xs = _mm_add_epi32 (x_base, _mm_set1_epi32 (sx26[k]));

	__m128i winding = _mm_setzero_si128 ();

	for (unsigned e = 0; e < n_edges; e++)
	{
	  const hb_raster_edge_t &edge = tile_edges[e];
	  if (ys < edge.yL || ys >= edge.yH) continue;

	  int64_t B    = (int64_t)(ys - edge.yL) * edge.dx;
	  int32_t dy   = edge.dy;
	  int32_t xL   = edge.xL;
	  int32_t wind = edge.wind;

	  /* xL_minus_xs[i] = xL - xs[i] */
	  __m128i xL_minus_xs = _mm_sub_epi32 (_mm_set1_epi32 (xL), xs);

	  /* widen pairs to i64, multiply by dy, add B */
	  __m128i dy_v = _mm_set1_epi32 (dy);
	  __m128i lo32 = _mm_unpacklo_epi32 (xL_minus_xs, _mm_setzero_si128 ());
	  __m128i hi32 = _mm_unpackhi_epi32 (xL_minus_xs, _mm_setzero_si128 ());

	  /* SSE4.1: _mm_cvtepi32_epi64 sign-extends 2 i32 → 2 i64 */
	  __m128i xmx_lo = _mm_cvtepi32_epi64 (_mm_unpacklo_epi32 (xL_minus_xs, xL_minus_xs));
	  __m128i xmx_hi = _mm_cvtepi32_epi64 (_mm_unpackhi_epi32 (xL_minus_xs, xL_minus_xs));
	  /* Actually _mm_cvtepi32_epi64 takes lower 2 ints from a 128-bit register */
	  xmx_lo = _mm_cvtepi32_epi64 (xL_minus_xs);
	  xmx_hi = _mm_cvtepi32_epi64 (_mm_srli_si128 (xL_minus_xs, 8));

	  __m128i dy64   = _mm_set1_epi64x ((int64_t) dy);
	  __m128i B_v    = _mm_set1_epi64x (B);
	  __m128i expr_lo = _mm_add_epi64 (B_v, _mm_mul_epi32 (xmx_lo, dy64));
	  __m128i expr_hi = _mm_add_epi64 (B_v, _mm_mul_epi32 (xmx_hi, dy64));

	  /* mask: expr > 0 */
	  __m128i zero64 = _mm_setzero_si128 ();
	  __m128i mask_lo = _mm_cmpgt_epi64 (expr_lo, zero64);
	  __m128i mask_hi = _mm_cmpgt_epi64 (expr_hi, zero64);

	  /* pack 64-bit masks to 32-bit: take high 32 of each 64-bit lane */
	  __m128i mask32 = _mm_packs_epi32 (
	    _mm_shuffle_epi32 (mask_lo, _MM_SHUFFLE (3,1,2,0)),
	    _mm_shuffle_epi32 (mask_hi, _MM_SHUFFLE (3,1,2,0)));
	  /* mask32 lanes are now 0xFFFFFFFF or 0x00000000 */

	  __m128i wind_v = _mm_set1_epi32 (wind);
	  winding = _mm_add_epi32 (winding, _mm_and_si128 (wind_v, mask32));

	  (void) lo32; (void) hi32; (void) dy_v; /* suppress unused */
	}

	/* if winding != 0: inside_count++ */
	__m128i nonzero = _mm_cmpgt_epi32 (winding, _mm_setzero_si128 ());
	__m128i nonzero2 = _mm_or_si128 (nonzero,
	  _mm_cmpgt_epi32 (_mm_setzero_si128 (), winding));
	inside_count = _mm_sub_epi32 (inside_count,
				      _mm_srli_epi32 (nonzero2, 31));
      }

      int32_t ic[4];
      _mm_storeu_si128 ((__m128i *) ic, inside_count);
      int bx = px0 + col - x_org;
      for (int i = 0; i < n_write; i++)
	row_buf[bx + i] = (uint8_t)((ic[i] * 255 + HB_RASTER_SAMPLES / 2) / HB_RASTER_SAMPLES);
    }
  }
}

static inline void
rasterize_tile (hb_raster_image_t      *image,
		int                     px0,
		int                     py0,
		int                     px1,
		int                     py1,
		const hb_raster_edge_t *tile_edges,
		unsigned                n_edges)
{ rasterize_tile_sse41 (image, px0, py0, px1, py1, tile_edges, n_edges); }

#else /* scalar */

static inline void
rasterize_tile (hb_raster_image_t      *image,
		int                     px0,
		int                     py0,
		int                     px1,
		int                     py1,
		const hb_raster_edge_t *tile_edges,
		unsigned                n_edges)
{
  uint8_t  *buf    = image->buffer;
  unsigned  stride = image->extents.stride;
  int       x_org  = image->extents.x_origin;
  int       y_org  = image->extents.y_origin;

  for (int row = 0; row < 16; row++)
  {
    int     py       = py0 + row;
    if (py >= py1) break;
    int32_t y_base26 = py << 6;
    uint8_t *row_buf = buf + (unsigned)(py - y_org) * stride;

    for (int col = 0; col < 16; col++)
    {
      int     px       = px0 + col;
      if (px >= px1) break;
      int32_t x_base26 = px << 6;

      int inside_count = 0;

      for (unsigned k = 0; k < HB_RASTER_SAMPLES; k++)
      {
	int32_t ys = y_base26 + sy26[k];
	int32_t xs = x_base26 + sx26[k];

	int winding = 0;
	for (unsigned e = 0; e < n_edges; e++)
	{
	  const hb_raster_edge_t &edge = tile_edges[e];
	  if (ys < edge.yL || ys >= edge.yH) continue;

	  int64_t B    = (int64_t)(ys - edge.yL) * edge.dx;
	  int64_t expr = (int64_t)(edge.xL - xs) * edge.dy + B;
	  if (expr > 0) winding += edge.wind;
	}
	if (winding != 0) inside_count++;
      }

      row_buf[(unsigned)(px - x_org)] = (uint8_t)((inside_count * 255 + HB_RASTER_SAMPLES / 2) / HB_RASTER_SAMPLES);
    }
  }
}

#endif /* HB_RASTER_NEON / HB_RASTER_SSE41 / scalar */


/*
 * hb_raster_draw_render
 */

hb_raster_image_t *
hb_raster_draw_render (hb_raster_draw_t *draw)
{
  if (unlikely (!draw)) return nullptr;

  /* ── 1. Compute result extents ─────────────────────────────────── */
  hb_raster_extents_t ext;

  if (draw->has_fixed_extents)
  {
    ext = draw->fixed_extents;
  }
  else
  {
    /* Auto-size from edge bounding box */
    if (draw->edges.length == 0)
    {
      /* No edges: produce 0×0 image */
      ext = { 0, 0, 0, 0, 0 };
    }
    else
    {
      int32_t xmin = draw->edges[0].xL, xmax = draw->edges[0].xL;
      int32_t ymin = draw->edges[0].yL, ymax = draw->edges[0].yH;

      for (const auto &e : draw->edges)
      {
	xmin = hb_min (xmin, hb_min (e.xL, e.xH));
	xmax = hb_max (xmax, hb_max (e.xL, e.xH));
	ymin = hb_min (ymin, e.yL);
	ymax = hb_max (ymax, e.yH);
      }

      /* Convert 26.6 → pixels (floor for min, ceil for max) */
      int x0 = xmin >> 6;
      int y0 = ymin >> 6;
      int x1 = (xmax + 63) >> 6;
      int y1 = (ymax + 63) >> 6;

      ext.x_origin = x0;
      ext.y_origin = y0;
      ext.width    = (unsigned) hb_max (0, x1 - x0);
      ext.height   = (unsigned) hb_max (0, y1 - y0);
      ext.stride   = 0; /* filled below */
    }
  }

  /* ── 2. Compute stride ─────────────────────────────────────────── */
  if (ext.stride == 0)
  {
    if (draw->format == HB_RASTER_FORMAT_BGRA32)
      ext.stride = ext.width * 4;
    else
      ext.stride = (ext.width + 3u) & ~3u;
  }

  /* ── 3. Allocate image ─────────────────────────────────────────── */
  hb_raster_image_t *image = hb_object_create<hb_raster_image_t> ();
  if (unlikely (!image)) goto done;

  if (ext.width && ext.height)
  {
    size_t buf_size = (size_t) ext.stride * ext.height;
    image->buffer = (uint8_t *) hb_malloc (buf_size);
    if (unlikely (!image->buffer))
    {
      hb_raster_image_destroy (image);
      image = nullptr;
      goto done;
    }
    image->buffer_size = buf_size;
    memset (image->buffer, 0, buf_size);
  }

  image->extents = ext;
  image->format  = draw->format;

  /* ── 4. Tile assignment ────────────────────────────────────────── */
  if (draw->edges.length && ext.width && ext.height)
  {
    unsigned ntx = (ext.width  + 15) / 16;
    unsigned nty = (ext.height + 15) / 16;
    unsigned n_tiles = ntx * nty;

    /* Pass 1: count edges per tile */
    if (unlikely (!draw->tile_offsets.resize (n_tiles + 1)))
      goto done;
    memset (draw->tile_offsets.arrayZ, 0, (n_tiles + 1) * sizeof (uint32_t));

    for (const auto &e : draw->edges)
    {
      /* Tile y/x range of edge (pixel coords relative to x_origin/y_origin).
	 x: edges are assigned from tx=0 to the edge's rightmost tile because
	 the winding test needs edges to the right of each sample point.  */
      int ex1 = (hb_max (e.xL, e.xH) + 63) >> 6;  ex1 -= ext.x_origin;
      int ey0 = (e.yL >> 6) - ext.y_origin;
      int ey1 = (e.yH + 63) >> 6;                  ey1 -= ext.y_origin;

      int tx0 = 0;
      int tx1 = hb_min ((int) ntx - 1, (ex1 - 1) / 16);
      int ty0 = hb_max (0, ey0 / 16);
      int ty1 = hb_min ((int) nty - 1, (ey1 - 1) / 16);

      for (int ty = ty0; ty <= ty1; ty++)
	for (int tx = tx0; tx <= tx1; tx++)
	  draw->tile_offsets[ty * ntx + tx]++;
    }

    /* Prefix-sum */
    uint32_t total = 0;
    for (unsigned i = 0; i < n_tiles; i++)
    {
      uint32_t cnt = draw->tile_offsets[i];
      draw->tile_offsets[i] = total;
      total += cnt;
    }
    draw->tile_offsets[n_tiles] = total;

    /* Pass 2: fill tile_edge_indices */
    if (unlikely (!draw->tile_edge_indices.resize (total)))
      goto done;

    hb_vector_t<uint32_t> tile_fill;
    if (unlikely (!tile_fill.resize (n_tiles)))
      goto done;
    memcpy (tile_fill.arrayZ, draw->tile_offsets.arrayZ, n_tiles * sizeof (uint32_t));

    for (unsigned ei = 0; ei < draw->edges.length; ei++)
    {
      const auto &e = draw->edges[ei];

      int ex1 = (hb_max (e.xL, e.xH) + 63) >> 6;  ex1 -= ext.x_origin;
      int ey0 = (e.yL >> 6) - ext.y_origin;
      int ey1 = (e.yH + 63) >> 6;                  ey1 -= ext.y_origin;

      int tx0 = 0;
      int tx1 = hb_min ((int) ntx - 1, (ex1 - 1) / 16);
      int ty0 = hb_max (0, ey0 / 16);
      int ty1 = hb_min ((int) nty - 1, (ey1 - 1) / 16);

      for (int ty = ty0; ty <= ty1; ty++)
	for (int tx = tx0; tx <= tx1; tx++)
	{
	  unsigned tile_id = ty * ntx + tx;
	  draw->tile_edge_indices[tile_fill[tile_id]++] = ei;
	}
    }

    /* ── 5. Rasterize each tile ──────────────────────────────────── */
    hb_vector_t<hb_raster_edge_t> tile_edge_buf;
    for (unsigned ty = 0; ty < nty; ty++)
    {
      for (unsigned tx = 0; tx < ntx; tx++)
      {
	unsigned tile_id = ty * ntx + tx;
	unsigned start   = draw->tile_offsets[tile_id];
	unsigned end     = draw->tile_offsets[tile_id + 1];
	unsigned n       = end - start;

	if (!n) continue;

	/* Build local edge list */
	if (unlikely (!tile_edge_buf.resize (n)))
	  goto done;
	for (unsigned i = 0; i < n; i++)
	  tile_edge_buf[i] = draw->edges[draw->tile_edge_indices[start + i]];

	/* Pixel origin of tile */
	int px0 = ext.x_origin + (int)(tx * 16);
	int py0 = ext.y_origin + (int)(ty * 16);

	/* Clip tile to image bounds */
	int px1 = hb_min (px0 + 16, (int)(ext.x_origin + (int) ext.width));
	int py1 = hb_min (py0 + 16, (int)(ext.y_origin + (int) ext.height));
	if (px0 >= px1 || py0 >= py1) continue;

	rasterize_tile (image, px0, py0, px1, py1, tile_edge_buf.arrayZ, n);
      }
    }
  }

done:
  /* ── 6. Reset one-shot state ────────────────────────────────────── */
  draw->edges.resize (0);
  draw->has_fixed_extents = false;
  draw->fixed_extents     = {};

  return image;
}
