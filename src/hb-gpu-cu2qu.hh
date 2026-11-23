/*
 * Copyright (C) 2026  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Cubic-to-quadratic Bézier conversion.
 * Ported from fonttools cu2qu:
 * https://github.com/fonttools/fonttools/blob/main/Lib/fontTools/cu2qu/cu2qu.py
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

#ifndef HB_GPU_CU2QU_HH
#define HB_GPU_CU2QU_HH

#include "hb-gpu-draw.hh"

#include <cmath>


/* Maximum approximation error in font units.
 * 0.5 is well below one unit in any reasonable coordinate system. */
#ifndef HB_GPU_CU2QU_TOLERANCE
#define HB_GPU_CU2QU_TOLERANCE 0.5
#endif

/* Most quadratics one cubic is converted to.  Glyphs take up to 10 at
 * this tolerance, and the wildest cubics whose control points the encoder
 * can store about 35.  One that would take more is emitted as lines. */
#ifndef HB_GPU_CU2QU_MAX_N
#define HB_GPU_CU2QU_MAX_N 64
#endif


struct hb_gpu_cu2qu_point_t
{
  double x, y;
};

static inline hb_gpu_cu2qu_point_t
hb_gpu_cu2qu_lerp (hb_gpu_cu2qu_point_t a, hb_gpu_cu2qu_point_t b, double t)
{
  return {a.x + (b.x - a.x) * t,
	  a.y + (b.y - a.y) * t};
}


/*
 * Check whether a cubic error curve stays within `tolerance` of the origin.
 */
static bool
hb_gpu_cubic_farthest_fit_inside (hb_gpu_cu2qu_point_t p0,
				  hb_gpu_cu2qu_point_t p1,
				  hb_gpu_cu2qu_point_t p2,
				  hb_gpu_cu2qu_point_t p3,
				  double tolerance, unsigned depth)
{
  if (hypot (p1.x, p1.y) <= tolerance &&
      hypot (p2.x, p2.y) <= tolerance)
    return true;

  if (depth >= 8)
    return false;

  hb_gpu_cu2qu_point_t mid = {(p0.x + 3 * (p1.x + p2.x) + p3.x) * 0.125,
			      (p0.y + 3 * (p1.y + p2.y) + p3.y) * 0.125};
  if (hypot (mid.x, mid.y) > tolerance)
    return false;

  hb_gpu_cu2qu_point_t d3 = {(p3.x + p2.x - p1.x - p0.x) * 0.125,
			     (p3.y + p2.y - p1.y - p0.y) * 0.125};

  return hb_gpu_cubic_farthest_fit_inside (
    p0,
    hb_gpu_cu2qu_lerp (p0, p1, 0.5),
    {mid.x - d3.x, mid.y - d3.y},
    mid,
    tolerance, depth + 1
  ) && hb_gpu_cubic_farthest_fit_inside (
    mid,
    {mid.x + d3.x, mid.y + d3.y},
    hb_gpu_cu2qu_lerp (p2, p3, 0.5),
    p3,
    tolerance, depth + 1
  );
}


/*
 * Try to approximate a cubic Bézier with a single quadratic.
 */
static bool
hb_gpu_cubic_approx_quadratic (hb_gpu_cu2qu_point_t c0,
			       hb_gpu_cu2qu_point_t c1,
			       hb_gpu_cu2qu_point_t c2,
			       hb_gpu_cu2qu_point_t c3,
			       double tolerance,
			       hb_gpu_cu2qu_point_t *q1)
{
  double ax = c1.x - c0.x, ay = c1.y - c0.y;
  double dx = c3.x - c2.x, dy = c3.y - c2.y;

  /* A control point on its end point leaves that end no tangent to
   * intersect.  The curve leaves the end toward the other control point,
   * so take the direction from that one: the quadratic's control point
   * is then that control point. */
  if (ax == 0 && ay == 0)
  {
    ax = c2.x - c0.x;
    ay = c2.y - c0.y;
  }
  if (dx == 0 && dy == 0)
  {
    dx = c3.x - c1.x;
    dy = c3.y - c1.y;
  }

  double px = -ay, py = ax;

  double denom = px * dx + py * dy;
  if (fabs (denom) < 1e-12)
    return false;

  double h = (px * (c0.x - c2.x) + py * (c0.y - c2.y)) / denom;

  q1->x = c2.x + dx * h;
  q1->y = c2.y + dy * h;

  hb_gpu_cu2qu_point_t err1 = {c0.x + (q1->x - c0.x) * (2.0 / 3.0) - c1.x,
			       c0.y + (q1->y - c0.y) * (2.0 / 3.0) - c1.y};
  hb_gpu_cu2qu_point_t err2 = {c3.x + (q1->x - c3.x) * (2.0 / 3.0) - c2.x,
			       c3.y + (q1->y - c3.y) * (2.0 / 3.0) - c2.y};

  return hb_gpu_cubic_farthest_fit_inside ({0, 0}, err1, err2, {0, 0}, tolerance, 0);
}


/*
 * A cubic in the power basis, to cut pieces out of.
 */
struct hb_gpu_cu2qu_cubic_t
{
  hb_gpu_cu2qu_cubic_t (hb_gpu_cu2qu_point_t c0_,
			hb_gpu_cu2qu_point_t c1_,
			hb_gpu_cu2qu_point_t c2_,
			hb_gpu_cu2qu_point_t c3_)
    : c0 (c0_), c3 (c3_),
      b1 {3 * (c1_.x - c0_.x),
	  3 * (c1_.y - c0_.y)},
      b2 {3 * (c0_.x - 2 * c1_.x + c2_.x),
	  3 * (c0_.y - 2 * c1_.y + c2_.y)},
      b3 {c3_.x - 3 * c2_.x + 3 * c1_.x - c0_.x,
	  c3_.y - 3 * c2_.y + 3 * c1_.y - c0_.y} {}

  hb_gpu_cu2qu_point_t at (double t) const
  {
    return {c0.x + t * (b1.x + t * (b2.x + t * b3.x)),
	    c0.y + t * (b1.y + t * (b2.y + t * b3.y))};
  }
  hb_gpu_cu2qu_point_t derivative_at (double t) const
  {
    return {b1.x + t * (2 * b2.x + t * 3 * b3.x),
	    b1.y + t * (2 * b2.y + t * 3 * b3.y)};
  }

  /* Control points of the k-th of n equal pieces. */
  void piece (unsigned k, unsigned n, hb_gpu_cu2qu_point_t out[4]) const
  {
    double t0 = (double) k / n, t1 = (double) (k + 1) / n;
    double third = (t1 - t0) / 3;
    hb_gpu_cu2qu_point_t d0 = derivative_at (t0), d1 = derivative_at (t1);
    out[0] = k ? at (t0) : c0;
    out[3] = k + 1 < n ? at (t1) : c3;
    out[1] = {out[0].x + d0.x * third, out[0].y + d0.y * third};
    out[2] = {out[3].x - d1.x * third, out[3].y - d1.y * third};
  }

  /* B(t) = c0 + t (b1 + t (b2 + t b3)); b3 is the third difference. */
  hb_gpu_cu2qu_point_t c0, c3;
  hb_gpu_cu2qu_point_t b1, b2, b3;
};

/*
 * Try to approximate a cubic Bézier with a spline of n > 1 quadratics.
 *
 * The cubic is cut into n equal pieces.  Each gives one off-curve point,
 * between the points its two end tangents suggest, nearer the first one
 * in the first pieces and the second one in the last, and consecutive
 * quadratics meet halfway between their off-curve points.  So the spline
 * is smooth, and it leaves both ends of the cubic along the cubic's own
 * tangents.
 *
 * Fills @off with the n off-curve points and @on with the n + 1 on-curve
 * ones.
 */
static bool
hb_gpu_cubic_approx_spline (const hb_gpu_cu2qu_cubic_t &cubic,
			    unsigned n,
			    double tolerance,
			    hb_gpu_cu2qu_point_t *off,
			    hb_gpu_cu2qu_point_t *on)
{
  auto control = [n] (unsigned k, const hb_gpu_cu2qu_point_t p[4])
  {
    hb_gpu_cu2qu_point_t q0 = {p[0].x + (p[1].x - p[0].x) * 1.5,
			       p[0].y + (p[1].y - p[0].y) * 1.5};
    hb_gpu_cu2qu_point_t q1 = {p[3].x + (p[2].x - p[3].x) * 1.5,
			       p[3].y + (p[2].y - p[3].y) * 1.5};
    return hb_gpu_cu2qu_lerp (q0, q1, (double) k / (n - 1));
  };

  hb_gpu_cu2qu_point_t cur[4], next[4];
  cubic.piece (0, n, next);
  off[0] = control (0, next);
  on[0] = cubic.c0;

  for (unsigned k = 0; k < n; k++)
  {
    hb_memcpy (cur, next, sizeof (cur));
    if (k + 1 < n)
    {
      cubic.piece (k + 1, n, next);
      off[k + 1] = control (k + 1, next);
      on[k + 1] = hb_gpu_cu2qu_lerp (off[k], off[k + 1], 0.5);
    }
    else
      on[k + 1] = cubic.c3;

    /* The quadratic from on[k] to on[k + 1] against its piece of the
     * cubic.  Their ends differ as well, except at the cubic's own. */
    hb_gpu_cu2qu_point_t err0 = {on[k].x - cur[0].x,
				 on[k].y - cur[0].y};
    hb_gpu_cu2qu_point_t err1 = {on[k].x + (off[k].x - on[k].x) * (2.0 / 3.0) - cur[1].x,
				 on[k].y + (off[k].y - on[k].y) * (2.0 / 3.0) - cur[1].y};
    hb_gpu_cu2qu_point_t err2 = {on[k + 1].x + (off[k].x - on[k + 1].x) * (2.0 / 3.0) - cur[2].x,
				 on[k + 1].y + (off[k].y - on[k + 1].y) * (2.0 / 3.0) - cur[2].y};
    hb_gpu_cu2qu_point_t err3 = {on[k + 1].x - cur[3].x,
				 on[k + 1].y - cur[3].y};

    if (!(hypot (err3.x, err3.y) <= tolerance) ||
	!hb_gpu_cubic_farthest_fit_inside (err0, err1, err2, err3, tolerance, 0))
      return false;
  }

  return true;
}


/*
 * Convert a cubic into quadratic segments.
 */
static void
hb_gpu_cubic_to_quadratics (hb_gpu_draw_t *g,
			    hb_gpu_cu2qu_point_t c0,
			    hb_gpu_cu2qu_point_t c1,
			    hb_gpu_cu2qu_point_t c2,
			    hb_gpu_cu2qu_point_t c3,
			    double tolerance)
{
  /* Charge the fit tests to the work budget, a unit for each quadratic
   * tried, and stop once the accumulator has failed or the budget is
   * spent: whatever we would emit from here is dropped by acc_emit()
   * anyway. */
  int64_t *budget = g->get_budget_remaining ();
  if (unlikely (!g->success ||
		!hb_budget_spend (*budget, HB_BUDGET_1)))
    return;

  hb_gpu_cu2qu_point_t q1;
  if (hb_gpu_cubic_approx_quadratic (c0, c1, c2, c3, tolerance, &q1))
  {
    g->acc_conic_to (q1.x, q1.y, c3.x, c3.y);
    return;
  }

  hb_gpu_cu2qu_cubic_t cubic (c0, c1, c2, c3);

  /* At equal parameters no quadratic comes closer to a cubic than 1/32
   * of the cubic's third difference, and that of one of n pieces is the
   * whole cubic's over n^3.  So no fewer pieces than this can do.
   * Written so that NaN goes past the last. */
  double d3 = hypot (cubic.b3.x, cubic.b3.y);
  unsigned n = 2;
  while (n <= HB_GPU_CU2QU_MAX_N &&
	 !((double) n * n * n * (32 * tolerance) >= d3))
    n++;

  hb_gpu_cu2qu_point_t off[HB_GPU_CU2QU_MAX_N];
  hb_gpu_cu2qu_point_t on[HB_GPU_CU2QU_MAX_N + 1];
  for (; n <= HB_GPU_CU2QU_MAX_N; n++)
  {
    if (unlikely (!hb_budget_spend (*budget, HB_BUDGET_1, n)))
      return;

    if (!hb_gpu_cubic_approx_spline (cubic, n, tolerance, off, on))
      continue;

    for (unsigned k = 0; k < n; k++)
      g->acc_conic_to (off[k].x, off[k].y, on[k + 1].x, on[k + 1].y);
    return;
  }

  /* Too large for that many, which is far beyond what the encoder can
   * store, as when a transform puts an outline astronomically far away:
   * hb_gpu_draw_encode() will reject the outline.  Emit the control
   * polygon, which has the cubic's points in it. */
  g->acc_line_to (c1.x, c1.y);
  g->acc_line_to (c2.x, c2.y);
  g->acc_line_to (c3.x, c3.y);
}


#endif /* HB_GPU_CU2QU_HH */
