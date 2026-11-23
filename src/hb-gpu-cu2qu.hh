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
 * Try to approximate a cubic Bézier with a single quadratic: the one
 * whose control point q is where the cubic's two end tangents meet.
 *
 * Returns whether the two curves are within `tolerance` of each other as
 * sets of points, not at equal parameters: the cubic runs along that
 * quadratic at another pace, and what counts is how far it leaves it.
 *
 * With a = q - c0 and z = c3 - q, the cubic's control points are at
 * c1 = c0 + l1 a and c2 = c3 - l2 z, and the cubic B is the quadratic Q
 * at another parameter plus an offset along the parabola's axis:
 *
 *   B(t) = Q(tau(t)) + rho(t) (a - z)
 *
 *   tau(t) = 3 t (1-t)^2 l1/2 + 3 t^2 (1-t) (1 - l2/2) + t^3
 *   rho(t) = 3 t^2 (1-t)^2 (l1 + l2 - 4/3 + 3 e(t)^2)
 *   e(t)   = (1-t) (l1/2 - 1/3) + t (1/3 - l2/2)
 *
 * which is an identity of polynomials.  For l1 and l2 from 0 to 2, tau
 * is a Bézier function with control values from 0 to 1, so it takes
 * [0,1] onto [0,1]: every point of the cubic is |rho| |a - z| from a
 * point of the quadratic, and every point of the quadratic that far from
 * one of the cubic.  As t^2 (1-t)^2 is at most 1/16, the two are within
 *
 *   3/16 max |l1 + l2 - 4/3 + 3 e^2| |a - z|
 *
 * So a single quadratic fits when the two handles add up to 4/3 of the
 * way to where the tangents meet, however that is split between them.
 * (At equal parameters it would take 2/3 each.)
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

  /* The two legs of the quadratic, and how far along them the cubic's
   * control points are.  Written so that NaN gives up. */
  hb_gpu_cu2qu_point_t a = {q1->x - c0.x, q1->y - c0.y};
  hb_gpu_cu2qu_point_t z = {c3.x - q1->x, c3.y - q1->y};
  double aa = a.x * a.x + a.y * a.y;
  double zz = z.x * z.x + z.y * z.y;
  if (!(aa > 0 && zz > 0))
    return false;
  double l1 = ((c1.x - c0.x) * a.x + (c1.y - c0.y) * a.y) / aa;
  double l2 = ((c3.x - c2.x) * z.x + (c3.y - c2.y) * z.y) / zz;
  if (!(l1 >= 0 && l1 <= 2 && l2 >= 0 && l2 <= 2))
    return false;

  /* e(t) goes from p to r, so e^2 is between the smaller of their
   * squares, or zero if they differ in sign, and the larger. */
  double p = l1 * .5 - 1. / 3., r = 1. / 3. - l2 * .5;
  double e2_max = hb_max (p * p, r * r);
  double e2_min = p * r <= 0 ? 0. : hb_min (p * p, r * r);
  double g = l1 + l2 - 4. / 3.;
  double m = hb_max (fabs (g + 3 * e2_max), fabs (g + 3 * e2_min));

  return 3. / 16. * m * hypot (a.x - z.x, a.y - z.y) <= tolerance;
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
 * The spline of n > 1 quadratics for a cubic.
 *
 * The cubic is cut into n equal pieces.  Each gives one off-curve point,
 * between the points its two end tangents suggest, nearer the first one
 * in the first pieces and the second one in the last, and consecutive
 * quadratics meet halfway between their off-curve points.  So the spline
 * is smooth, and it leaves both ends of the cubic along the cubic's own
 * tangents.
 */

/* The off-curve point for the k-th of n pieces, @p its control points. */
static inline hb_gpu_cu2qu_point_t
hb_gpu_cubic_spline_control (const hb_gpu_cu2qu_point_t p[4],
			     unsigned k, unsigned n)
{
  hb_gpu_cu2qu_point_t q0 = {p[0].x + (p[1].x - p[0].x) * 1.5,
			     p[0].y + (p[1].y - p[0].y) * 1.5};
  hb_gpu_cu2qu_point_t q1 = {p[3].x + (p[2].x - p[3].x) * 1.5,
			     p[3].y + (p[2].y - p[3].y) * 1.5};
  return hb_gpu_cu2qu_lerp (q0, q1, (double) k / (n - 1));
}

/*
 * How far the spline of n > 1 quadratics gets from its cubic at equal
 * parameters, as a multiple of the cubic's third difference over n^3.
 *
 * It does not depend on the cubic.  With d the third difference over
 * n^3, the off-curve point of piece k is the one of least error for the
 * piece plus d (1 - 2k/(n-1)) / 4, which puts the on-curve point between
 * pieces k-1 and k at d g(k) from the cubic, g(k) = (n-2k) / (4(n-1)).
 * The difference between quadratic k and its piece then is d times
 *
 *   g(k) - u (2g(k) + k/(n-1)) + u^2 (1 + g(k) + g(k+1) + k/(n-1)) - u^3
 *
 * at parameter u, with g(0) = g(n) = 0.  It is largest in the first
 * piece or in the second, and likewise in the last two.
 */
static inline double
hb_gpu_cubic_spline_error (unsigned n)
{
  if (n == 2)
    return 4. / 27.;

  double g = (n - 2) / (4. * (n - 1));
  double first = 4. / 27. * (1 + g) * (1 + g) * (1 + g);

  double w2 = (.25 - .5 / (n - 1)) / 3;
  double second = (n - 3) / (4. * (n - 1)) + 2 * w2 * sqrt (w2);

  return hb_max (first, second);
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
  /* Charge the work budget a unit for the fit test and one for each
   * quadratic computed, and stop once the accumulator has failed or the
   * budget is spent: whatever we would emit from here is dropped by
   * acc_emit() anyway. */
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

  /* The least n whose spline is within tolerance.  Written so that NaN
   * goes past the last. */
  double d3 = hypot (cubic.b3.x, cubic.b3.y);
  unsigned n = 2;
  while (n <= HB_GPU_CU2QU_MAX_N &&
	 !(d3 * hb_gpu_cubic_spline_error (n) <= tolerance * n * n * n))
    n++;

  if (unlikely (n > HB_GPU_CU2QU_MAX_N))
  {
    /* Too large for that many, which is far beyond what the encoder can
     * store, as when a transform puts an outline astronomically far
     * away: hb_gpu_draw_encode() will reject the outline.  Emit the
     * control polygon, which has the cubic's points in it. */
    g->acc_line_to (c1.x, c1.y);
    g->acc_line_to (c2.x, c2.y);
    g->acc_line_to (c3.x, c3.y);
    return;
  }

  if (unlikely (!hb_budget_spend (*budget, HB_BUDGET_1, n)))
    return;

  hb_gpu_cu2qu_point_t piece[4];
  cubic.piece (0, n, piece);
  hb_gpu_cu2qu_point_t off = hb_gpu_cubic_spline_control (piece, 0, n);
  for (unsigned k = 1; k < n; k++)
  {
    cubic.piece (k, n, piece);
    hb_gpu_cu2qu_point_t next_off = hb_gpu_cubic_spline_control (piece, k, n);
    hb_gpu_cu2qu_point_t on = hb_gpu_cu2qu_lerp (off, next_off, 0.5);
    g->acc_conic_to (off.x, off.y, on.x, on.y);
    off = next_off;
  }
  g->acc_conic_to (off.x, off.y, c3.x, c3.y);
}


#endif /* HB_GPU_CU2QU_HH */
