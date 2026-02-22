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

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#define HB_RASTER_NEON 1
#elif defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
#include <emmintrin.h>
#define HB_RASTER_SSE2 1
#endif


/* Fixed-point precision for sub-pixel coordinates.
   8 bits = 24.8: 256 sub-pixel units per pixel. */
#define HB_RASTER_PIXEL_BITS 8
#define HB_RASTER_ONE_PIXEL  (1 << HB_RASTER_PIXEL_BITS)
#define HB_RASTER_PIXEL_MASK (HB_RASTER_ONE_PIXEL - 1)
/* Full-coverage alpha = 2 * ONE_PIXEL^2 */
#define HB_RASTER_FULL_COVERAGE (2 * HB_RASTER_ONE_PIXEL * HB_RASTER_ONE_PIXEL)
/* Flatness threshold for Bézier flattening: max deviation in pixels */
#define HB_RASTER_FLAT_THRESH 0.25f


/* Normalized edge: yH > yL always */
struct hb_raster_edge_t
{
  int32_t xL, yL;   /* lower endpoint (fixed-point) */
  int32_t xH, yH;   /* upper endpoint (fixed-point) */
  int64_t slope;    /* dx/dy in 16.16 fixed point: ((int64_t)dx << 16) / dy */
  int32_t wind;     /* +1 or -1 */
};

/* hb_raster_draw_t — outline rasterizer */
struct hb_raster_draw_t
{
  hb_object_header_t header;

  /* Configuration */
  hb_raster_format_t  format            = HB_RASTER_FORMAT_A8;
  hb_transform_t<>    transform         = {1, 0, 0, 1, 0, 0};
  hb_raster_extents_t fixed_extents     = {};
  bool                has_fixed_extents = false;

  /* Accumulated geometry */
  hb_vector_t<hb_raster_edge_t> edges;

  /* Scratch — reused across render() calls */
  hb_vector_t<int32_t> row_area;
  hb_vector_t<int16_t> row_cover;
  hb_vector_t<hb_vector_t<unsigned>> edge_buckets;
  hb_vector_t<unsigned> active_edges;

  /* Recycled image for zero-malloc render */
  hb_raster_image_t *recycled_image = nullptr;
};


/* hb_raster_draw_t */

/**
 * hb_raster_draw_create:
 *
 * Creates a new rasterizer object.
 *
 * Return value: (transfer full):
 * A newly allocated #hb_raster_draw_t with a reference count of 1. The
 * initial reference count should be released with hb_raster_draw_destroy()
 * when you are done using the #hb_raster_draw_t. This function never
 * returns `NULL`. If memory cannot be allocated, a special singleton
 * #hb_raster_draw_t object will be returned.
 *
 * XSince: REPLACEME
 **/
hb_raster_draw_t *
hb_raster_draw_create (void)
{
  hb_raster_draw_t *draw = hb_object_create<hb_raster_draw_t> ();
  if (unlikely (!draw)) return nullptr;
  return draw;
}

/**
 * hb_raster_draw_reference: (skip)
 * @draw: a rasterizer
 *
 * Increases the reference count on @draw by one.
 *
 * This prevents @draw from being destroyed until a matching
 * call to hb_raster_draw_destroy() is made.
 *
 * Return value: (transfer full):
 * The referenced #hb_raster_draw_t.
 *
 * XSince: REPLACEME
 **/
hb_raster_draw_t *
hb_raster_draw_reference (hb_raster_draw_t *draw)
{
  return hb_object_reference (draw);
}

/**
 * hb_raster_draw_destroy: (skip)
 * @draw: a rasterizer
 *
 * Decreases the reference count on @draw by one. When the
 * reference count reaches zero, the rasterizer is freed.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_draw_destroy (hb_raster_draw_t *draw)
{
  if (!hb_object_destroy (draw)) return;
  hb_raster_image_destroy (draw->recycled_image);
  hb_free (draw);
}

/**
 * hb_raster_draw_set_user_data: (skip)
 * @draw: a rasterizer
 * @key: the user-data key
 * @data: a pointer to the user data
 * @destroy: (nullable): a callback to call when @data is not needed anymore
 * @replace: whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified rasterizer.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_raster_draw_set_user_data (hb_raster_draw_t   *draw,
			      hb_user_data_key_t *key,
			      void               *data,
			      hb_destroy_func_t   destroy,
			      hb_bool_t           replace)
{
  return hb_object_set_user_data (draw, key, data, destroy, replace);
}

/**
 * hb_raster_draw_get_user_data: (skip)
 * @draw: a rasterizer
 * @key: the user-data key
 *
 * Fetches the user-data associated with the specified key,
 * attached to the specified rasterizer.
 *
 * Return value: (transfer none):
 * A pointer to the user data
 *
 * XSince: REPLACEME
 **/
void *
hb_raster_draw_get_user_data (hb_raster_draw_t   *draw,
			      hb_user_data_key_t *key)
{
  return hb_object_get_user_data (draw, key);
}

/**
 * hb_raster_draw_set_format:
 * @draw: a rasterizer
 * @format: the pixel format to use
 *
 * Sets the output pixel format for subsequent renders.
 * The default is @HB_RASTER_FORMAT_A8.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_draw_set_format (hb_raster_draw_t  *draw,
			   hb_raster_format_t format)
{
  if (unlikely (!draw)) return;
  draw->format = format;
}

/**
 * hb_raster_draw_get_format:
 * @draw: a rasterizer
 *
 * Fetches the output pixel format of the rasterizer.
 *
 * Return value:
 * The #hb_raster_format_t of the rasterizer
 *
 * XSince: REPLACEME
 **/
hb_raster_format_t
hb_raster_draw_get_format (hb_raster_draw_t *draw)
{
  if (unlikely (!draw)) return HB_RASTER_FORMAT_A8;
  return draw->format;
}

/**
 * hb_raster_draw_set_transform:
 * @draw: a rasterizer
 * @xx: xx component of the transform matrix
 * @yx: yx component of the transform matrix
 * @xy: xy component of the transform matrix
 * @yy: yy component of the transform matrix
 * @dx: x translation
 * @dy: y translation
 *
 * Sets a 2×3 affine transform applied to all incoming draw
 * coordinates before rasterization.  The default is the identity.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_draw_set_transform (hb_raster_draw_t *draw,
			      float xx, float yx,
			      float xy, float yy,
			      float dx, float dy)
{
  if (unlikely (!draw)) return;
  draw->transform = {xx, yx, xy, yy, dx, dy};
}

/**
 * hb_raster_draw_get_transform:
 * @draw: a rasterizer
 * @xx: (out) (optional): xx component of the transform matrix
 * @yx: (out) (optional): yx component of the transform matrix
 * @xy: (out) (optional): xy component of the transform matrix
 * @yy: (out) (optional): yy component of the transform matrix
 * @dx: (out) (optional): x translation
 * @dy: (out) (optional): y translation
 *
 * Fetches the current affine transform of the rasterizer.
 *
 * XSince: REPLACEME
 **/
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

/**
 * hb_raster_draw_set_extents:
 * @draw: a rasterizer
 * @extents: the desired output extents
 *
 * Overrides the output image extents for the next render.  When set,
 * hb_raster_draw_render() uses the given extents instead of
 * auto-computing them from the accumulated geometry.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_draw_set_extents (hb_raster_draw_t          *draw,
			    const hb_raster_extents_t *extents)
{
  if (unlikely (!draw || !extents)) return;
  draw->fixed_extents     = *extents;
  draw->has_fixed_extents = true;
}

/**
 * hb_raster_draw_reset:
 * @draw: a rasterizer
 *
 * Resets the rasterizer to its initial state, clearing all accumulated
 * geometry, the transform, format, and fixed extents.  The object can
 * then be reused for a new glyph.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_draw_reset (hb_raster_draw_t *draw)
{
  if (unlikely (!draw)) return;
  draw->format            = HB_RASTER_FORMAT_A8;
  draw->transform         = {1, 0, 0, 1, 0, 0};
  draw->fixed_extents     = {};
  draw->has_fixed_extents = false;
  draw->edges.resize (0);
  draw->row_area.resize (0);
  draw->row_cover.resize (0);
  draw->edge_buckets.resize (0);
  draw->active_edges.resize (0);
  hb_raster_image_destroy (draw->recycled_image);
  draw->recycled_image = nullptr;
}

/**
 * hb_raster_draw_recycle_image:
 * @draw: a rasterizer
 * @image: a raster image to recycle
 *
 * Recycles @image for reuse by a subsequent hb_raster_draw_render()
 * call, avoiding per-render memory allocation.  The caller transfers
 * ownership of @image to @draw and must not use it afterwards.
 *
 * If @draw already holds a recycled image, the previously recycled
 * image is destroyed.
 *
 * XSince: REPLACEME
 **/
void
hb_raster_draw_recycle_image (hb_raster_draw_t  *draw,
			      hb_raster_image_t *image)
{
  if (unlikely (!draw || !image)) return;
  hb_raster_image_destroy (draw->recycled_image);
  draw->recycled_image = image;
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
  int32_t X0 = (int32_t) roundf (x0 * HB_RASTER_ONE_PIXEL);
  int32_t Y0 = (int32_t) roundf (y0 * HB_RASTER_ONE_PIXEL);
  int32_t X1 = (int32_t) roundf (x1 * HB_RASTER_ONE_PIXEL);
  int32_t Y1 = (int32_t) roundf (y1 * HB_RASTER_ONE_PIXEL);

  if (Y0 == Y1) return; /* horizontal — skip */

  hb_raster_edge_t e;
  if (Y0 < Y1) {
    e.xL = X0; e.yL = Y0; e.xH = X1; e.yH = Y1; e.wind = +1;
  } else {
    e.xL = X1; e.yL = Y1; e.xH = X0; e.yH = Y0; e.wind = -1;
  }
  e.slope = ((int64_t) (e.xH - e.xL) << 16) / (e.yH - e.yL);

  draw->edges.push (e);
}

/* Quadratic Bézier flattener — de Casteljau recursive at t=0.5 */
static inline void
flatten_quadratic_recursive (hb_raster_draw_t *draw,
			     float x0, float y0,
			     float x1, float y1,
			     float x2, float y2,
			     int depth = 0)
{
  bool is_flat;
  if (false)
  {
    /* Old behavior: midpoint deviation from chord midpoint. */
    float mx = x0 * 0.25f + x1 * 0.5f + x2 * 0.25f;
    float my = y0 * 0.25f + y1 * 0.5f + y2 * 0.25f;
    float chord_mx = (x0 + x2) * 0.5f;
    float chord_my = (y0 + y2) * 0.5f;
    float dx = mx - chord_mx;
    float dy = my - chord_my;
    static const float flat_thresh = HB_RASTER_FLAT_THRESH * HB_RASTER_FLAT_THRESH;
    is_flat = (dx * dx + dy * dy) <= flat_thresh;
  }
  else
  {
    /* FreeType behavior: control-point deviation from chord center. */
    const float flat_thresh = 0.25f;
    float dx = x0 + x2 - 2.f * x1;
    float dy = y0 + y2 - 2.f * y1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    is_flat = dx <= flat_thresh && dy <= flat_thresh;
  }

  if (depth >= 16 || is_flat) {
    emit_segment (draw, x0, y0, x2, y2);
    return;
  }

  float x01 = (x0 + x1) * 0.5f, y01 = (y0 + y1) * 0.5f;
  float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
  float xm  = (x01 + x12) * 0.5f, ym  = (y01 + y12) * 0.5f;

  flatten_quadratic_recursive (draw, x0, y0, x01, y01, xm,  ym,  depth + 1);
  flatten_quadratic_recursive (draw, xm, ym, x12, y12, x2,  y2,  depth + 1);
}

/* Quadratic Bézier flattener using forward differencing.
   The error (midpoint deviation) shrinks exactly 4× per de Casteljau
   subdivision, so we compute the subdivision count upfront and iterate
   with constant-cost additions instead of recursive branching. */
static inline void
flatten_quadratic_fd (hb_raster_draw_t *draw,
		      float x0, float y0,
		      float x1, float y1,
		      float x2, float y2)
{
  /* Deviation of curve midpoint from chord midpoint (squared). */
  float devx = (x0 - 2 * x1 + x2) * 0.25f;
  float devy = (y0 - 2 * y1 + y2) * 0.25f;
  float err2 = devx * devx + devy * devy;

  static const float flat_thresh = HB_RASTER_FLAT_THRESH * HB_RASTER_FLAT_THRESH;

  if (err2 <= flat_thresh)
  {
    emit_segment (draw, x0, y0, x2, y2);
    return;
  }

  /* err² shrinks 16× per subdivision level.  Find n such that
     err2 / 16^n <= flat_thresh, i.e. n = ceil(log₁₆(err2/flat_thresh)). */
  unsigned n = 1;
  {
    float ratio = err2 / flat_thresh;
    while (ratio > 16.f) { ratio *= (1.f / 16.f); n++; }
    if (n > 16) n = 16;
  }
  unsigned N = 1u << n;   /* number of line segments */
  float h = 1.f / N;

  /* Quadratic: B(t) = a·t² + b·t + c
     Forward differences with step h:
       d²f = 2·a·h²   (constant)
       df₀ = a·h² + b·h
       f₀  = c = P₀                     */
  float ax = x0 - 2 * x1 + x2;
  float ay = y0 - 2 * y1 + y2;
  float bx = 2 * (x1 - x0);
  float by = 2 * (y1 - y0);

  float d2fx = 2 * ax * h * h;
  float d2fy = 2 * ay * h * h;
  float dfx  = ax * h * h + bx * h;
  float dfy  = ay * h * h + by * h;
  float fx   = x0;
  float fy   = y0;

  for (unsigned i = 1; i < N; i++)
  {
    float nx = fx + dfx;
    float ny = fy + dfy;
    emit_segment (draw, fx, fy, nx, ny);
    fx = nx;
    fy = ny;
    dfx += d2fx;
    dfy += d2fy;
  }
  /* Last segment uses exact endpoint to avoid drift. */
  emit_segment (draw, fx, fy, x2, y2);
}

static void
flatten_quadratic (hb_raster_draw_t *draw,
		   float x0, float y0,
		   float x1, float y1,
		   float x2, float y2)
{
  if (false)
    flatten_quadratic_fd (draw, x0, y0, x1, y1, x2, y2);
  else
    flatten_quadratic_recursive (draw, x0, y0, x1, y1, x2, y2);
}

/* For cubic B(t), the max deviation from its chord on [0,1] is bounded by:
     max||B(t)-L(t)|| <= max_t ||B''(t)|| / 8.
   B''(t) is linear, so max norm is attained at t=0 or t=1:
     B''(0)=6*(P0-2P1+P2), B''(1)=6*(P1-2P2+P3). */
static inline float
cubic_chord_error_bound2 (float x0, float y0,
			  float x1, float y1,
			  float x2, float y2,
			  float x3, float y3)
{
  float d20x = x0 - 2 * x1 + x2;
  float d20y = y0 - 2 * y1 + y2;
  float d21x = x1 - 2 * x2 + x3;
  float d21y = y1 - 2 * y2 + y3;
  float m0 = d20x * d20x + d20y * d20y;
  float m1 = d21x * d21x + d21y * d21y;
  float m = m0 > m1 ? m0 : m1;
  /* (max||B''||/8)^2 = (6/8)^2 * max||d2||^2 = (3/4)^2 * m. */
  return m * (9.f / 16.f);
}

/* Cubic Bézier flattener — de Casteljau recursive at t=0.5 */
static inline void
flatten_cubic_recursive (hb_raster_draw_t *draw,
			 float x0, float y0,
			 float x1, float y1,
			 float x2, float y2,
			 float x3, float y3,
			 int depth = 0)
{
  bool is_flat;
  if (false)
  {
    /* Old behavior: curvature/chord-error bound. */
    float err2 = cubic_chord_error_bound2 (x0, y0, x1, y1, x2, y2, x3, y3);
    static const float flat_thresh = HB_RASTER_FLAT_THRESH * HB_RASTER_FLAT_THRESH;
    is_flat = err2 <= flat_thresh;
  }
  else
  {
    /* FreeType behavior: chord-trisection distance test. */
    const float flat_thresh = 0.5f;

    float d10x = 2.f * x0 - 3.f * x1 + x3;
    float d10y = 2.f * y0 - 3.f * y1 + y3;
    float d20x = x0 - 3.f * x2 + 2.f * x3;
    float d20y = y0 - 3.f * y2 + 2.f * y3;

    if (d10x < 0) d10x = -d10x;
    if (d10y < 0) d10y = -d10y;
    if (d20x < 0) d20x = -d20x;
    if (d20y < 0) d20y = -d20y;

    is_flat = d10x <= flat_thresh &&
              d10y <= flat_thresh &&
              d20x <= flat_thresh &&
              d20y <= flat_thresh;
  }

  if (depth >= 16 || is_flat)
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

  flatten_cubic_recursive (draw, x0, y0, x01, y01, x012, y012, xm, ym, depth + 1);
  flatten_cubic_recursive (draw, xm, ym, x123, y123, x23, y23, x3, y3, depth + 1);
}

/* Cubic Bézier flattener using forward differencing.
   Use a curvature-based chord-error bound (max||B''||/8), then choose a
   uniform subdivision count n such that the bound drops below threshold.
   The cubic adds a constant third difference d³f = 6·a·h³. */
static inline void
flatten_cubic_fd (hb_raster_draw_t *draw,
		  float x0, float y0,
		  float x1, float y1,
		  float x2, float y2,
		  float x3, float y3)
{
  float err2 = cubic_chord_error_bound2 (x0, y0, x1, y1, x2, y2, x3, y3);

  static const float flat_thresh = HB_RASTER_FLAT_THRESH * HB_RASTER_FLAT_THRESH;

  if (err2 <= flat_thresh)
  {
    emit_segment (draw, x0, y0, x3, y3);
    return;
  }

  /* The bound scales with h², so err² shrinks 16× per subdivision level. */
  unsigned n = 1;
  {
    float ratio = err2 / flat_thresh;
    while (ratio > 16.f) { ratio *= (1.f / 16.f); n++; }
    if (n > 16) n = 16;
  }
  unsigned N = 1u << n;
  float h = 1.f / N;

  /* Cubic: B(t) = a·t³ + b·t² + c·t + d
     a = -P₀ + 3P₁ - 3P₂ + P₃
     b =  3P₀ - 6P₁ + 3P₂
     c =  3(P₁ - P₀)
     d =  P₀
     Forward differences with step h:
       d³f  = 6·a·h³              (constant)
       d²f₀ = 6·a·h³ + 2·b·h²
       d¹f₀ = a·h³ + b·h² + c·h
       f₀   = d = P₀              */
  float ax = -x0 + 3*x1 - 3*x2 + x3;
  float ay = -y0 + 3*y1 - 3*y2 + y3;
  float bx =  3*x0 - 6*x1 + 3*x2;
  float by =  3*y0 - 6*y1 + 3*y2;
  float cx =  3*(x1 - x0);
  float cy =  3*(y1 - y0);

  float h2 = h * h, h3 = h2 * h;
  float d3fx = 6 * ax * h3;
  float d3fy = 6 * ay * h3;
  float d2fx = d3fx + 2 * bx * h2;
  float d2fy = d3fy + 2 * by * h2;
  float dfx  = ax * h3 + bx * h2 + cx * h;
  float dfy  = ay * h3 + by * h2 + cy * h;
  float fx   = x0;
  float fy   = y0;

  for (unsigned i = 1; i < N; i++)
  {
    float nx = fx + dfx;
    float ny = fy + dfy;
    emit_segment (draw, fx, fy, nx, ny);
    fx = nx;  fy = ny;
    dfx += d2fx;  dfy += d2fy;
    d2fx += d3fx; d2fy += d3fy;
  }
  /* Last segment uses exact endpoint to avoid drift. */
  emit_segment (draw, fx, fy, x3, y3);
}

static void
flatten_cubic (hb_raster_draw_t *draw,
	       float x0, float y0,
	       float x1, float y1,
	       float x2, float y2,
	       float x3, float y3)
{
  if (false)
    flatten_cubic_fd (draw, x0, y0, x1, y1, x2, y2, x3, y3);
  else
    flatten_cubic_recursive (draw, x0, y0, x1, y1, x2, y2, x3, y3);
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
  flatten_quadratic (draw, tx0, ty0, tx1, ty1, tx2, ty2);
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
  flatten_cubic (draw, tx0, ty0, tx1, ty1, tx2, ty2, tx3, ty3);
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

/**
 * hb_raster_draw_get_funcs:
 *
 * Fetches the singleton #hb_draw_funcs_t that feeds outline data
 * into an #hb_raster_draw_t.  Pass the #hb_raster_draw_t as the
 * @draw_data argument when calling the draw functions.
 *
 * Return value: (transfer none):
 * The rasterizer draw functions
 *
 * XSince: REPLACEME
 **/
hb_draw_funcs_t *
hb_raster_draw_get_funcs (void)
{
  return static_raster_draw_funcs.get_unconst ();
}


/*
 * Analytic coverage rasterizer
 *
 * For each line-segment edge and each pixel row it crosses, we compute
 * exact area/cover contributions per pixel cell.  A left-to-right sweep
 * then converts accumulated (area, cover) into alpha values.
 *
 * Coordinates are fixed-point.
 *
 *   cover[x] = Σ dy · wind        — signed vertical extent per cell
 *   area[x]  = Σ (fx₀+fx₁)·dy·wind — twice the signed trapezoidal area
 *
 * Sweep:
 *   cover_accum += cover[x]
 *   α = min(|cover_accum·128 − area[x]|, 8192) · 255 / 8192
 */

/* Add one edge piece's area/cover into a single cell. */
static HB_ALWAYS_INLINE void
cell_add (int32_t *area, int16_t *cover, unsigned width, int col,
	  int32_t fx0, int32_t fy0, int32_t fx1, int32_t fy1, int32_t wind,
	  unsigned &x_min, unsigned &x_max)
{
  if ((unsigned) col >= width) return;
  int32_t dy = fy1 - fy0;
  area[col]  += (fx0 + fx1) * dy * wind;
  cover[col] += (int16_t) (dy * wind);
  x_min = hb_min (x_min, (unsigned) col);
  x_max = hb_max (x_max, (unsigned) col);
}

/* Walk one edge through the pixel cells of a single pixel row,
   accumulating area/cover.  py is the integer pixel-row index. */
static HB_ALWAYS_INLINE void
edge_sweep_row (int32_t                *area,
		int16_t                *cover,
		unsigned                width,
		int                     x_org,
		int32_t                 y_top,
		const hb_raster_edge_t &edge,
		unsigned               &x_min,
		unsigned               &x_max)
{
  int32_t y_bot = y_top + HB_RASTER_ONE_PIXEL;

  int32_t ey0 = hb_max (edge.yL, y_top);
  int32_t ey1 = hb_min (edge.yH, y_bot);
  if (ey0 >= ey1) return;

  /* X at clipped endpoints (fixed-point) */
  int32_t x0 = edge.xL + (int32_t) ((int64_t) (ey0 - edge.yL) * edge.slope >> 16);
  int32_t x1 = edge.xL + (int32_t) ((int64_t) (ey1 - edge.yL) * edge.slope >> 16);

  /* Fractional y within this pixel row [0, ONE_PIXEL] */
  int32_t fy0 = ey0 - y_top;
  int32_t fy1 = ey1 - y_top;

  int32_t cx0 = x0 >> HB_RASTER_PIXEL_BITS;
  int32_t fx0 = x0 & HB_RASTER_PIXEL_MASK;
  int32_t cx1 = x1 >> HB_RASTER_PIXEL_BITS;
  int32_t fx1 = x1 & HB_RASTER_PIXEL_MASK;
  int32_t wind = edge.wind;

  /* Fast path: both endpoints in the same pixel column. */
  if (cx0 == cx1)
  {
    cell_add (area, cover, width, cx0 - x_org, fx0, fy0, fx1, fy1, wind, x_min, x_max);
    return;
  }

  int32_t total_dx = x1 - x0;
  int32_t total_dy = fy1 - fy0;

  /* fy increment per pixel column (constant since x_b advances by ONE_PIXEL). */
  int32_t delta_fy = (int32_t) ((int64_t) HB_RASTER_ONE_PIXEL * total_dy / total_dx);

  if (total_dx > 0)
  {
    /* Left-to-right edge. */
    int32_t x_b  = (cx0 + 1) << HB_RASTER_PIXEL_BITS;
    int32_t fy_b = fy0 + (int32_t) ((int64_t) (x_b - x0) * total_dy / total_dx);
    cell_add (area, cover, width, cx0 - x_org, fx0, fy0, HB_RASTER_ONE_PIXEL, fy_b, wind, x_min, x_max);

    int32_t fy_prev = fy_b;
    for (int32_t cx = cx0 + 1; cx < cx1; cx++)
    {
      fy_b = fy_prev + delta_fy;
      cell_add (area, cover, width, cx - x_org, 0, fy_prev, HB_RASTER_ONE_PIXEL, fy_b, wind, x_min, x_max);
      fy_prev = fy_b;
    }

    cell_add (area, cover, width, cx1 - x_org, 0, fy_prev, fx1, fy1, wind, x_min, x_max);
  }
  else
  {
    /* Right-to-left edge. */
    int32_t x_b  = cx0 << HB_RASTER_PIXEL_BITS;
    int32_t fy_b = fy0 + (int32_t) ((int64_t) (x_b - x0) * total_dy / total_dx);
    cell_add (area, cover, width, cx0 - x_org, fx0, fy0, 0, fy_b, wind, x_min, x_max);

    int32_t fy_prev = fy_b;
    for (int32_t cx = cx0 - 1; cx > cx1; cx--)
    {
      fy_b = fy_prev - delta_fy;
      cell_add (area, cover, width, cx - x_org, HB_RASTER_ONE_PIXEL, fy_prev, 0, fy_b, wind, x_min, x_max);
      fy_prev = fy_b;
    }

    cell_add (area, cover, width, cx1 - x_org, HB_RASTER_ONE_PIXEL, fy_prev, fx1, fy1, wind, x_min, x_max);
  }
}

/* Prefix-sum cover in-place. Returns final accumulator. */
static int32_t
prefix_sum_cover (int16_t *cover, unsigned x_min, unsigned x_max)
{
  int32_t cover_accum = 0;
  for (unsigned x = x_min; x <= x_max; x++)
  {
    cover_accum += cover[x];
    cover[x] = (int16_t) cover_accum;
  }
  return cover_accum;
}

/* Convert prefix-summed cover + area to alpha bytes, then clear. */
static void
sweep_row_to_alpha (uint8_t *row_buf,
		    int32_t *area,
		    int16_t *cover,
		    unsigned x_min,
		    unsigned x_max)
{
  unsigned x = x_min;

#ifdef HB_RASTER_NEON
  int32x4_t clamp_v = vdupq_n_s32 (HB_RASTER_FULL_COVERAGE);
  int32x4_t bias_v  = vdupq_n_s32 (HB_RASTER_FULL_COVERAGE / 2);
  int32x4_t zero32  = vdupq_n_s32 (0);
  int16x8_t zero16  = vdupq_n_s16 (0);
  for (; x + 7 <= x_max; x += 8)
  {
    int16x8_t c16 = vld1q_s16 (cover + x);
    int32x4_t c0  = vshlq_n_s32 (vmovl_s16 (vget_low_s16 (c16)), HB_RASTER_PIXEL_BITS + 1);
    int32x4_t c1  = vshlq_n_s32 (vmovl_s16 (vget_high_s16 (c16)), HB_RASTER_PIXEL_BITS + 1);
    int32x4_t a0  = vld1q_s32 (area + x);
    int32x4_t a1  = vld1q_s32 (area + x + 4);

    int32x4_t v0 = vabsq_s32 (vsubq_s32 (c0, a0));
    int32x4_t v1 = vabsq_s32 (vsubq_s32 (c1, a1));

    v0 = vminq_s32 (v0, clamp_v);
    v1 = vminq_s32 (v1, clamp_v);

    int32x4_t r0 = vshrq_n_s32 (vmlaq_n_s32 (bias_v, v0, 255), 2 * HB_RASTER_PIXEL_BITS + 1);
    int32x4_t r1 = vshrq_n_s32 (vmlaq_n_s32 (bias_v, v1, 255), 2 * HB_RASTER_PIXEL_BITS + 1);

    int16x4_t h0 = vmovn_s32 (r0);
    int16x4_t h1 = vmovn_s32 (r1);
    int16x8_t h  = vcombine_s16 (h0, h1);
    uint8x8_t b  = vqmovun_s16 (h);
    vst1_u8 (row_buf + x, b);

    vst1q_s32 (area + x,     zero32);
    vst1q_s32 (area + x + 4, zero32);
    vst1q_s16 (cover + x,    zero16);
  }
#elif defined(HB_RASTER_SSE2)
  __m128i clamp_v = _mm_set1_epi32 (HB_RASTER_FULL_COVERAGE);
  __m128i bias_v  = _mm_set1_epi32 (HB_RASTER_FULL_COVERAGE / 2);
  __m128i zero_v  = _mm_setzero_si128 ();
  for (; x + 7 <= x_max; x += 8)
  {
    __m128i c16 = _mm_loadu_si128 ((__m128i *) (void *) (cover + x));
    __m128i c0  = _mm_slli_epi32 (_mm_srai_epi32 (_mm_unpacklo_epi16 (c16, c16), 16), HB_RASTER_PIXEL_BITS + 1);
    __m128i c1  = _mm_slli_epi32 (_mm_srai_epi32 (_mm_unpackhi_epi16 (c16, c16), 16), HB_RASTER_PIXEL_BITS + 1);
    __m128i a0  = _mm_loadu_si128 ((__m128i *) (void *) (area + x));
    __m128i a1  = _mm_loadu_si128 ((__m128i *) (void *) (area + x + 4));

    __m128i v0 = _mm_sub_epi32 (c0, a0);
    __m128i v1 = _mm_sub_epi32 (c1, a1);

    __m128i s0 = _mm_srai_epi32 (v0, 31);
    __m128i s1 = _mm_srai_epi32 (v1, 31);
    v0 = _mm_sub_epi32 (_mm_xor_si128 (v0, s0), s0);
    v1 = _mm_sub_epi32 (_mm_xor_si128 (v1, s1), s1);

    __m128i lt0 = _mm_cmplt_epi32 (v0, clamp_v);
    __m128i lt1 = _mm_cmplt_epi32 (v1, clamp_v);
    v0 = _mm_or_si128 (_mm_and_si128 (lt0, v0), _mm_andnot_si128 (lt0, clamp_v));
    v1 = _mm_or_si128 (_mm_and_si128 (lt1, v1), _mm_andnot_si128 (lt1, clamp_v));

    __m128i r0 = _mm_srai_epi32 (_mm_add_epi32 (_mm_sub_epi32 (_mm_slli_epi32 (v0, 8), v0), bias_v), 2 * HB_RASTER_PIXEL_BITS + 1);
    __m128i r1 = _mm_srai_epi32 (_mm_add_epi32 (_mm_sub_epi32 (_mm_slli_epi32 (v1, 8), v1), bias_v), 2 * HB_RASTER_PIXEL_BITS + 1);

    __m128i h = _mm_packs_epi32 (r0, r1);
    __m128i b = _mm_packus_epi16 (h, h);
    _mm_storel_epi64 ((__m128i *) (void *) (row_buf + x), b);

    _mm_storeu_si128 ((__m128i *) (void *) (area + x),     zero_v);
    _mm_storeu_si128 ((__m128i *) (void *) (area + x + 4), zero_v);
    _mm_storeu_si128 ((__m128i *) (void *) (cover + x),    zero_v);
  }
#endif

  for (; x <= x_max; x++)
  {
    int32_t val   = (int32_t) cover[x] * (2 * HB_RASTER_ONE_PIXEL) - area[x];
    int32_t alpha = val < 0 ? -val : val;
    if (alpha > HB_RASTER_FULL_COVERAGE) alpha = HB_RASTER_FULL_COVERAGE;
    row_buf[x] = (uint8_t) (((unsigned) alpha * 255 + HB_RASTER_FULL_COVERAGE / 2) >> (2 * HB_RASTER_PIXEL_BITS + 1));
    area[x]  = 0;
    cover[x] = 0;
  }
}


/**
 * hb_raster_draw_render:
 * @draw: a rasterizer
 *
 * Rasterizes the accumulated outline geometry into a new
 * #hb_raster_image_t.  After rendering, the accumulated edges are
 * cleared so the rasterizer can be reused.
 *
 * Return value: (transfer full):
 * A newly allocated #hb_raster_image_t, or `NULL` on allocation failure
 *
 * XSince: REPLACEME
 **/
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
      int32_t xmin = draw->edges.arrayZ[0].xL, xmax = draw->edges.arrayZ[0].xL;
      int32_t ymin = draw->edges.arrayZ[0].yL, ymax = draw->edges.arrayZ[0].yH;

      for (const auto &e : draw->edges)
      {
	xmin = hb_min (xmin, hb_min (e.xL, e.xH));
	xmax = hb_max (xmax, hb_max (e.xL, e.xH));
	ymin = hb_min (ymin, e.yL);
	ymax = hb_max (ymax, e.yH);
      }

      /* Convert fixed-point → pixels (floor for min, ceil for max) */
      int x0 = xmin >> HB_RASTER_PIXEL_BITS;
      int y0 = ymin >> HB_RASTER_PIXEL_BITS;
      int x1 = (xmax + HB_RASTER_PIXEL_MASK) >> HB_RASTER_PIXEL_BITS;
      int y1 = (ymax + HB_RASTER_PIXEL_MASK) >> HB_RASTER_PIXEL_BITS;

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

  /* ── 3. Allocate or reuse image ─────────────────────────────────── */
  hb_raster_image_t *image;
  if (draw->recycled_image)
  {
    image = draw->recycled_image;
    draw->recycled_image = nullptr;
  }
  else
  {
    image = hb_object_create<hb_raster_image_t> ();
    if (unlikely (!image)) goto done;
  }

  image->extents = ext;
  image->format  = draw->format;

  if (ext.width && ext.height)
  {
    size_t buf_size = (size_t) ext.stride * ext.height;
    if (unlikely (!image->buffer.resize_dirty (buf_size)))
    {
      hb_raster_image_destroy (image);
      image = nullptr;
      goto done;
    }
    memset (image->buffer.arrayZ, 0, buf_size);
  }
  else
    image->buffer.resize (0);

  /* ── 4. Bucket edges by starting row and rasterize scanlines ──── */
  if (draw->edges.length && ext.width && ext.height)
  {
    if (unlikely (!draw->row_area.resize_dirty (ext.width) ||
		  !draw->row_cover.resize_dirty (ext.width)))
      goto done;
    memset (draw->row_area.arrayZ,  0, ext.width * sizeof (int32_t));
    memset (draw->row_cover.arrayZ, 0, ext.width * sizeof (int16_t));

    /* Bucket edges by their starting pixel row.
       Only grow the outer vector; clear inner vectors without freeing. */
    unsigned old_buckets = draw->edge_buckets.length;
    if (ext.height > old_buckets)
    {
      if (unlikely (!draw->edge_buckets.resize (ext.height)))
	goto done;
    }
    for (unsigned i = 0; i < hb_min (ext.height, old_buckets); i++)
      draw->edge_buckets.arrayZ[i].resize (0);
    /* New buckets (if any) are already empty from resize's zero-init. */

    for (unsigned i = 0; i < draw->edges.length; i++)
    {
      int row = (draw->edges.arrayZ[i].yL >> HB_RASTER_PIXEL_BITS) - ext.y_origin;
      if (row < 0) row = 0;
      if ((unsigned) row >= ext.height) continue;
      draw->edge_buckets.arrayZ[row].push (i);
    }

    /* Scanline loop with active edge list. */
    draw->active_edges.resize (0);

    for (unsigned row = 0; row < ext.height; row++)
    {
      int32_t y_top = (ext.y_origin + (int) row) << HB_RASTER_PIXEL_BITS;

      /* Add new edges from this row's bucket. */
      draw->active_edges.extend (draw->edge_buckets.arrayZ[row]);

      /* Process active edges, removing expired ones. */
      unsigned x_min = ext.width, x_max = 0;
      for (unsigned j = 0; j < draw->active_edges.length; )
      {
	const auto &e = draw->edges.arrayZ[draw->active_edges.arrayZ[j]];
	if (e.yH <= y_top)
	{
	  draw->active_edges.remove_unordered (j);
	  continue;
	}

	edge_sweep_row (draw->row_area.arrayZ, draw->row_cover.arrayZ,
			ext.width, ext.x_origin, y_top, e, x_min, x_max);
	j++;
      }

      if (x_min <= x_max)
      {
	int32_t cover_accum = prefix_sum_cover (draw->row_cover.arrayZ, x_min, x_max);

	/* If cover doesn't cancel, memset the constant-alpha tail. */
	if (cover_accum != 0)
	{
	  int32_t alpha = cover_accum * (2 * HB_RASTER_ONE_PIXEL);
	  alpha = alpha < 0 ? -alpha : alpha;
	  if (alpha > HB_RASTER_FULL_COVERAGE) alpha = HB_RASTER_FULL_COVERAGE;
	  uint8_t byte = (uint8_t) (((unsigned) alpha * 255 + HB_RASTER_FULL_COVERAGE / 2) >> (2 * HB_RASTER_PIXEL_BITS + 1));

	  uint8_t *row_buf = image->buffer.arrayZ + row * ext.stride;
	  memset (row_buf + x_max + 1, byte, ext.width - 1 - x_max);
	}

	sweep_row_to_alpha (image->buffer.arrayZ + row * ext.stride,
			    draw->row_area.arrayZ, draw->row_cover.arrayZ,
			    x_min, x_max);
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
