/*
 * Copyright (C) 2026  Behdad Esfahbod
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

#include "hb-gpu.h"
#include "hb-gpu-draw.hh"
#include "hb-gpu-cu2qu.hh"
#include "hb-machinery.hh"

#include <cmath>

/* ---- Accumulator ---- */

static void
acc_update_extents (hb_gpu_draw_t *g, double x, double y)
{
  g->ext_min_x = hb_min (g->ext_min_x, x);
  g->ext_min_y = hb_min (g->ext_min_y, y);
  g->ext_max_x = hb_max (g->ext_max_x, x);
  g->ext_max_y = hb_max (g->ext_max_y, y);
}

static void
acc_emit (hb_gpu_draw_t *g,
	  double p1x, double p1y,
	  double p2x, double p2y,
	  double p3x, double p3y)
{
  hb_gpu_curve_t c = {p1x, p1y, p2x, p2y, p3x, p3y};
  g->curves.push (c);
  g->num_curves++;
  g->current_x = p3x;
  g->current_y = p3y;

  acc_update_extents (g, p1x, p1y);
  acc_update_extents (g, p2x, p2y);
  acc_update_extents (g, p3x, p3y);
}

static void
acc_emit_conic (hb_gpu_draw_t *g,
		double cx, double cy,
		double x, double y)
{
  if (g->current_x == x && g->current_y == y)
    return;

  if (g->need_moveto) {
    g->start_x = g->current_x;
    g->start_y = g->current_y;
    g->need_moveto = false;
  }

  acc_emit (g, g->current_x, g->current_y, cx, cy, x, y);
}

void
hb_gpu_draw_t::acc_move_to (double x, double y)
{
  need_moveto = true;
  current_x = x;
  current_y = y;
}

void
hb_gpu_draw_t::acc_line_to (double x, double y)
{
  acc_emit_conic (this, current_x, current_y, x, y);
}

void
hb_gpu_draw_t::acc_conic_to (double cx, double cy, double x, double y)
{
  acc_emit_conic (this, cx, cy, x, y);
}

void
hb_gpu_draw_t::acc_close_path ()
{
  if (!need_moveto &&
      (current_x != start_x || current_y != start_y))
    acc_line_to (start_x, start_y);
}


void
hb_gpu_draw_t::acc_cubic_to (double c1x, double c1y,
			       double c2x, double c2y,
			       double x, double y)
{
  double c0x = current_x, c0y = current_y;

  if (c0x == x && c0y == y &&
      c1x == x && c1y == y &&
      c2x == x && c2y == y)
    return;

  /* Degenerate cubic: all control points are collinear.
   * Emit as a line instead of running cu2qu subdivision. */
  double dx = x - c0x, dy = y - c0y;
  double len_sq = dx * dx + dy * dy;
  if (len_sq > 0.)
  {
    double inv = 1.0 / len_sq;
    double cross1 = (c1x - c0x) * dy - (c1y - c0y) * dx;
    double cross2 = (c2x - c0x) * dy - (c2y - c0y) * dx;
    if (cross1 * cross1 * inv <= HB_GPU_CU2QU_TOLERANCE * HB_GPU_CU2QU_TOLERANCE &&
	cross2 * cross2 * inv <= HB_GPU_CU2QU_TOLERANCE * HB_GPU_CU2QU_TOLERANCE)
    {
      acc_line_to (x, y);
      return;
    }
  }

  hb_gpu_cubic_to_quadratics (this,
			      {c0x, c0y}, {c1x, c1y},
			      {c2x, c2y}, {x, y},
			      HB_GPU_CU2QU_TOLERANCE, 0);
}


/* ---- Draw funcs ---- */

static void
hb_gpu_draw_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		     void *data,
		     hb_draw_state_t *st HB_UNUSED,
		     float to_x, float to_y,
		     void *user_data HB_UNUSED)
{
  hb_gpu_draw_t *g = (hb_gpu_draw_t *) data;
  g->acc_close_path ();
  g->acc_move_to ((double) to_x, (double) to_y);
}

static void
hb_gpu_draw_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		     void *data,
		     hb_draw_state_t *st HB_UNUSED,
		     float to_x, float to_y,
		     void *user_data HB_UNUSED)
{
  hb_gpu_draw_t *g = (hb_gpu_draw_t *) data;
  g->acc_line_to ((double) to_x, (double) to_y);
}

static void
hb_gpu_draw_quadratic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
			  void *data,
			  hb_draw_state_t *st HB_UNUSED,
			  float control_x, float control_y,
			  float to_x, float to_y,
			  void *user_data HB_UNUSED)
{
  hb_gpu_draw_t *g = (hb_gpu_draw_t *) data;
  g->acc_conic_to ((double) control_x, (double) control_y,
		   (double) to_x, (double) to_y);
}

static void
hb_gpu_draw_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED,
		      void *data,
		      hb_draw_state_t *st HB_UNUSED,
		      float control1_x, float control1_y,
		      float control2_x, float control2_y,
		      float to_x, float to_y,
		      void *user_data HB_UNUSED)
{
  hb_gpu_draw_t *g = (hb_gpu_draw_t *) data;
  g->acc_cubic_to ((double) control1_x, (double) control1_y,
		   (double) control2_x, (double) control2_y,
		   (double) to_x, (double) to_y);
}

static void
hb_gpu_draw_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED,
			void *data,
			hb_draw_state_t *st HB_UNUSED,
			void *user_data HB_UNUSED)
{
  hb_gpu_draw_t *g = (hb_gpu_draw_t *) data;
  g->acc_close_path ();
}

static inline void free_static_gpu_draw_funcs ();

static struct hb_gpu_draw_funcs_lazy_loader_t : hb_draw_funcs_lazy_loader_t<hb_gpu_draw_funcs_lazy_loader_t>
{
  static hb_draw_funcs_t *create ()
  {
    hb_draw_funcs_t *funcs = hb_draw_funcs_create ();

    hb_draw_funcs_set_move_to_func      (funcs, hb_gpu_draw_move_to,      nullptr, nullptr);
    hb_draw_funcs_set_line_to_func      (funcs, hb_gpu_draw_line_to,      nullptr, nullptr);
    hb_draw_funcs_set_quadratic_to_func (funcs, hb_gpu_draw_quadratic_to, nullptr, nullptr);
    hb_draw_funcs_set_cubic_to_func     (funcs, hb_gpu_draw_cubic_to,     nullptr, nullptr);
    hb_draw_funcs_set_close_path_func   (funcs, hb_gpu_draw_close_path,   nullptr, nullptr);

    hb_draw_funcs_make_immutable (funcs);

    hb_atexit (free_static_gpu_draw_funcs);

    return funcs;
  }
} static_gpu_draw_funcs;

static inline void
free_static_gpu_draw_funcs ()
{
  static_gpu_draw_funcs.free_instance ();
}


/* ---- Encode ---- */

struct hb_gpu_texel_t
{
  int16_t r, g, b, a;
};

struct hb_gpu_blob_data_t
{
  char *buf;
  unsigned capacity;
};

static void
_hb_gpu_blob_data_destroy (void *user_data)
{
  auto *bd = (hb_gpu_blob_data_t *) user_data;
  hb_free (bd->buf);
  hb_free (bd);
}

static int16_t
quantize (double v)
{
  return (int16_t) round (v * HB_GPU_UNITS_PER_EM);
}

static bool
quantize_fits_i16 (double v)
{
  double q = round (v * HB_GPU_UNITS_PER_EM);
  return q >= INT16_MIN &&
	 q <= INT16_MAX;
}

static int16_t
encode_offset (unsigned offset)
{
  return (int16_t) (offset - 32768u);
}

static hb_gpu_encode_curve_info_t
encode_curve_info (const hb_gpu_curve_t *c)
{
  hb_gpu_encode_curve_info_t info;

  info.min_x = hb_min (hb_min (c->p1x, c->p2x), c->p3x);
  info.max_x = hb_max (hb_max (c->p1x, c->p2x), c->p3x);
  info.min_y = hb_min (hb_min (c->p1y, c->p2y), c->p3y);
  info.max_y = hb_max (hb_max (c->p1y, c->p2y), c->p3y);
  info.is_horizontal = c->p1y == c->p2y && c->p2y == c->p3y;
  info.is_vertical   = c->p1x == c->p2x && c->p2x == c->p3x;
  info.hband_lo = 0;
  info.hband_hi = -1;
  info.vband_lo = 0;
  info.vband_hi = -1;

  return info;
}

struct hb_gpu_quantized_curve_t
{
  double p0x, p0y;
  double p1x, p1y;
  double p2x, p2y;
};

struct hb_gpu_monotone_piece_t
{
  double p0x, p0y;
  double p1x, p1y;
  double p2x, p2y;
  double min_primary, max_primary;
  double min_secondary, max_secondary;
  int winding_delta;
};

struct hb_gpu_crossing_t
{
  double pos;
  int delta;
};

static inline double
_hb_gpu_lerp (double a, double b, double t)
{
  return a + (b - a) * t;
}

static hb_gpu_quantized_curve_t
_hb_gpu_quantize_curve (const hb_gpu_curve_t *c)
{
  return {
    (double) quantize (c->p1x), (double) quantize (c->p1y),
    (double) quantize (c->p2x), (double) quantize (c->p2y),
    (double) quantize (c->p3x), (double) quantize (c->p3y),
  };
}

static void
_hb_gpu_split_curve (const hb_gpu_quantized_curve_t &curve,
		     double t,
		     hb_gpu_quantized_curve_t &left,
		     hb_gpu_quantized_curve_t &right)
{
  double p01x = _hb_gpu_lerp (curve.p0x, curve.p1x, t);
  double p01y = _hb_gpu_lerp (curve.p0y, curve.p1y, t);
  double p12x = _hb_gpu_lerp (curve.p1x, curve.p2x, t);
  double p12y = _hb_gpu_lerp (curve.p1y, curve.p2y, t);
  double p0112x = _hb_gpu_lerp (p01x, p12x, t);
  double p0112y = _hb_gpu_lerp (p01y, p12y, t);

  left = {
    curve.p0x, curve.p0y,
    p01x, p01y,
    p0112x, p0112y,
  };

  right = {
    p0112x, p0112y,
    p12x, p12y,
    curve.p2x, curve.p2y,
  };
}

static bool
_hb_gpu_split_curve_on_primary_extremum (const hb_gpu_quantized_curve_t &curve,
					 bool horizontal,
					 hb_gpu_quantized_curve_t &left,
					 hb_gpu_quantized_curve_t &right)
{
  double p0 = horizontal ? curve.p0y : curve.p0x;
  double p1 = horizontal ? curve.p1y : curve.p1x;
  double p2 = horizontal ? curve.p2y : curve.p2x;
  double denom = p0 - p1 * 2.0 + p2;

  if (denom == 0.0)
    return false;

  double t = (p0 - p1) / denom;
  if (!(t > 1e-9 && t < 1.0 - 1e-9))
    return false;

  _hb_gpu_split_curve (curve, t, left, right);
  return true;
}

static void
_hb_gpu_add_monotone_piece (const hb_gpu_quantized_curve_t &curve,
			    bool horizontal,
			    hb_vector_t<hb_gpu_monotone_piece_t> &pieces)
{
  double start = horizontal ? curve.p0y : curve.p0x;
  double end = horizontal ? curve.p2y : curve.p2x;

  if (start == end)
    return;

  hb_gpu_monotone_piece_t piece = {
    curve.p0x, curve.p0y,
    curve.p1x, curve.p1y,
    curve.p2x, curve.p2y,
    hb_min (start, end),
    hb_max (start, end),
    horizontal ? hb_min (hb_min (curve.p0x, curve.p1x), curve.p2x)
	       : hb_min (hb_min (curve.p0y, curve.p1y), curve.p2y),
    horizontal ? hb_max (hb_max (curve.p0x, curve.p1x), curve.p2x)
	       : hb_max (hb_max (curve.p0y, curve.p1y), curve.p2y),
    start < end ? +1 : -1,
  };

  pieces.push (piece);
}

static bool
_hb_gpu_build_monotone_pieces (const hb_gpu_curve_t *curves,
			       unsigned num_curves,
			       bool horizontal,
			       hb_vector_t<hb_gpu_monotone_piece_t> &pieces)
{
  for (unsigned i = 0; i < num_curves; i++)
  {
    hb_gpu_quantized_curve_t curve = _hb_gpu_quantize_curve (&curves[i]);
    hb_gpu_quantized_curve_t left, right;

    if (_hb_gpu_split_curve_on_primary_extremum (curve, horizontal, left, right))
    {
      _hb_gpu_add_monotone_piece (left, horizontal, pieces);
      _hb_gpu_add_monotone_piece (right, horizontal, pieces);
    }
    else
      _hb_gpu_add_monotone_piece (curve, horizontal, pieces);

    if (unlikely (pieces.in_error ()))
      return false;
  }

  return true;
}

static bool
_hb_gpu_eval_piece_at_primary (const hb_gpu_monotone_piece_t &piece,
			       bool horizontal,
			       double primary,
			       double *secondary)
{
  double p0p = horizontal ? piece.p0y : piece.p0x;
  double p1p = horizontal ? piece.p1y : piece.p1x;
  double p2p = horizontal ? piece.p2y : piece.p2x;

  if (!(primary > piece.min_primary + 1e-9 &&
	primary < piece.max_primary - 1e-9))
    return false;

  double a = p0p - p1p * 2.0 + p2p;
  double b = (p1p - p0p) * 2.0;
  double c = p0p - primary;
  double t = 0.0;

  if (fabs (a) <= 1e-9)
  {
    if (fabs (b) <= 1e-9)
      return false;
    t = -c / b;
  }
  else
  {
    double d = b * b - 4.0 * a * c;
    if (d < -1e-9)
      return false;
    d = hb_max (d, 0.0);
    double sqrt_d = sqrt (d);
    double t1 = (-b - sqrt_d) / (2.0 * a);
    double t2 = (-b + sqrt_d) / (2.0 * a);
    bool ok1 = t1 >= -1e-9 && t1 <= 1.0 + 1e-9;
    bool ok2 = t2 >= -1e-9 && t2 <= 1.0 + 1e-9;

    if (ok1 && ok2)
      t = fabs (t1 - 0.5) < fabs (t2 - 0.5) ? t1 : t2;
    else if (ok1)
      t = t1;
    else if (ok2)
      t = t2;
    else
      return false;
  }

  t = hb_min (hb_max (t, 0.0), 1.0);

  double omt = 1.0 - t;
  double p0s = horizontal ? piece.p0x : piece.p0y;
  double p1s = horizontal ? piece.p1x : piece.p1y;
  double p2s = horizontal ? piece.p2x : piece.p2y;

  *secondary = omt * omt * p0s + 2.0 * omt * t * p1s + t * t * p2s;
  return true;
}

static uint16_t
_hb_gpu_full_band_mask (unsigned num_bands)
{
  return num_bands >= 16 ? 0xFFFFu : (uint16_t) ((1u << num_bands) - 1u);
}

static uint16_t
_hb_gpu_compute_band_complexity_mask (const hb_gpu_curve_t *curves,
				      unsigned num_curves,
				      unsigned num_bands,
				      double primary_min,
				      double primary_max,
				      bool horizontal)
{
  if (!num_curves || !num_bands || primary_max <= primary_min)
    return 0;

  hb_vector_t<hb_gpu_monotone_piece_t> pieces;
  if (!_hb_gpu_build_monotone_pieces (curves, num_curves, horizontal, pieces))
    return _hb_gpu_full_band_mask (num_bands);

  hb_vector_t<unsigned> active;
  hb_vector_t<double> cuts;
  hb_vector_t<hb_gpu_crossing_t> crossings;
  uint16_t mask = 0;
  double band_size = (primary_max - primary_min) / (double) num_bands;

  for (unsigned b = 0; b < num_bands; b++)
  {
    double band_lo = primary_min + band_size * b;
    double band_hi = b + 1 == num_bands ? primary_max : band_lo + band_size;

    active.clear ();
    cuts.clear ();

    cuts.push (band_lo);
    cuts.push (band_hi);

    for (unsigned i = 0; i < pieces.length; i++)
    {
      const hb_gpu_monotone_piece_t &piece = pieces.arrayZ[i];
      if (piece.max_primary <= band_lo || piece.min_primary >= band_hi)
	continue;

      active.push (i);
      cuts.push (hb_max (piece.min_primary, band_lo));
      cuts.push (hb_min (piece.max_primary, band_hi));
    }

    if (unlikely (active.in_error () || cuts.in_error () || crossings.in_error ()))
      return _hb_gpu_full_band_mask (num_bands);

    if (active.length < 2)
      continue;

    cuts.as_array ().qsort ([] (const double &a, const double &b) { return a < b; });

    unsigned num_cuts = 1;
    for (unsigned i = 1; i < cuts.length; i++)
      if (cuts.arrayZ[i] - cuts.arrayZ[num_cuts - 1] > 1e-9)
	cuts.arrayZ[num_cuts++] = cuts.arrayZ[i];

    for (unsigned i = 0; i + 1 < num_cuts; i++)
    {
      double lo = cuts.arrayZ[i];
      double hi = cuts.arrayZ[i + 1];
      if (hi - lo <= 1e-9)
	continue;

      double primary = (lo + hi) * 0.5;
      crossings.clear ();

      for (unsigned j = 0; j < active.length; j++)
      {
	const hb_gpu_monotone_piece_t &piece = pieces.arrayZ[active.arrayZ[j]];
	double secondary;
	if (_hb_gpu_eval_piece_at_primary (piece, horizontal, primary, &secondary))
	{
	  hb_gpu_crossing_t crossing = {secondary, piece.winding_delta};
	  crossings.push (crossing);
	}
      }

      if (unlikely (crossings.in_error ()))
	return _hb_gpu_full_band_mask (num_bands);

      if (crossings.length < 2)
	continue;

      crossings.as_array ().qsort ([] (const hb_gpu_crossing_t &a,
				       const hb_gpu_crossing_t &b)
				   { return a.pos < b.pos; });

      int winding = 0;
      for (unsigned j = 0; j < crossings.length; )
      {
	double pos = crossings.arrayZ[j].pos;
	int delta = 0;
	do
	{
	  delta += crossings.arrayZ[j].delta;
	  j++;
	} while (j < crossings.length && fabs (crossings.arrayZ[j].pos - pos) <= 1e-9);

	winding += delta;
	if (winding > 1 || winding < -1)
	{
	  mask |= (uint16_t) (1u << b);
	  break;
	}
      }

      if (mask & (uint16_t) (1u << b))
	break;
    }
  }

  return mask;
}

/**
 * hb_gpu_draw_encode:
 * @draw: a GPU shape encoder
 *
 * Encodes the accumulated outlines into a compact blob
 * suitable for GPU rendering.  The blob data is an array of
 * RGBA16I texels (8 bytes each) to be uploaded to a texture
 * buffer object.
 *
 * The returned blob owns its own copy of the data.
 *
 * Return value: (transfer full):
 * An #hb_blob_t containing the encoded data, or
 * `NULL` if encoding fails.
 *
 * XSince: REPLACEME
 **/
hb_blob_t *
hb_gpu_draw_encode (hb_gpu_draw_t *draw)
{
  const hb_gpu_curve_t *curves = draw->curves.arrayZ;
  unsigned num_curves = draw->curves.length;

  if (num_curves == 0)
    return hb_blob_get_empty ();

  hb_gpu_encode_scratch_t &s = draw->scratch;

  /* Compute per-curve info and extents */
  s.curve_infos.resize (num_curves);

  double min_x = draw->ext_min_x;
  double min_y = draw->ext_min_y;
  double max_x = draw->ext_max_x;
  double max_y = draw->ext_max_y;

  for (unsigned i = 0; i < num_curves; i++)
    s.curve_infos.arrayZ[i] = encode_curve_info (&curves[i]);

  /* Choose number of bands (capped at 16 per Slug paper) */
  unsigned num_hbands = hb_min (num_curves, 16u);
  unsigned num_vbands = hb_min (num_curves, 16u);
  num_hbands = hb_max (num_hbands, 1u);
  num_vbands = hb_max (num_vbands, 1u);

  double height = max_y - min_y;
  double width  = max_x - min_x;

  if (height <= 0) num_hbands = 1;
  if (width  <= 0) num_vbands = 1;

  double hband_size = height / num_hbands;
  double vband_size = width  / num_vbands;

  s.hband_curve_counts.resize (num_hbands);
  s.vband_curve_counts.resize (num_vbands);
  for (unsigned b = 0; b < num_hbands; b++) s.hband_curve_counts.arrayZ[b] = 0;
  for (unsigned b = 0; b < num_vbands; b++) s.vband_curve_counts.arrayZ[b] = 0;

  for (unsigned i = 0; i < num_curves; i++)
  {
    hb_gpu_encode_curve_info_t &info = s.curve_infos.arrayZ[i];

    if (!info.is_horizontal)
    {
      if (height > 0) {
	info.hband_lo = (int) floor ((info.min_y - min_y) / hband_size);
	info.hband_hi = (int) floor ((info.max_y - min_y) / hband_size);
	info.hband_lo = hb_max (info.hband_lo, 0);
	info.hband_hi = hb_min (info.hband_hi, (int) num_hbands - 1);
	for (int b = info.hband_lo; b <= info.hband_hi; b++)
	  s.hband_curve_counts.arrayZ[b]++;
      } else {
	info.hband_lo = 0;
	info.hband_hi = 0;
	s.hband_curve_counts.arrayZ[0]++;
      }
    }

    if (!info.is_vertical)
    {
      if (width > 0) {
	info.vband_lo = (int) floor ((info.min_x - min_x) / vband_size);
	info.vband_hi = (int) floor ((info.max_x - min_x) / vband_size);
	info.vband_lo = hb_max (info.vband_lo, 0);
	info.vband_hi = hb_min (info.vband_hi, (int) num_vbands - 1);
	for (int b = info.vband_lo; b <= info.vband_hi; b++)
	  s.vband_curve_counts.arrayZ[b]++;
      } else {
	info.vband_lo = 0;
	info.vband_hi = 0;
	s.vband_curve_counts.arrayZ[0]++;
      }
    }
  }

  s.hband_offsets.resize (num_hbands);
  s.vband_offsets.resize (num_vbands);
  unsigned total_hband_indices = 0;
  unsigned total_vband_indices = 0;

  for (unsigned b = 0; b < num_hbands; b++) {
    s.hband_offsets.arrayZ[b] = total_hband_indices;
    total_hband_indices += s.hband_curve_counts.arrayZ[b];
  }

  for (unsigned b = 0; b < num_vbands; b++) {
    s.vband_offsets.arrayZ[b] = total_vband_indices;
    total_vband_indices += s.vband_curve_counts.arrayZ[b];
  }

  /* Assign curves to bands */
  s.hband_curves.resize (total_hband_indices);
  s.hband_curves_asc.resize (total_hband_indices);
  s.vband_curves.resize (total_vband_indices);
  s.vband_curves_asc.resize (total_vband_indices);
  s.hband_cursors.resize (num_hbands);
  s.vband_cursors.resize (num_vbands);
  for (unsigned b = 0; b < num_hbands; b++) s.hband_cursors.arrayZ[b] = s.hband_offsets.arrayZ[b];
  for (unsigned b = 0; b < num_vbands; b++) s.vband_cursors.arrayZ[b] = s.vband_offsets.arrayZ[b];

  for (unsigned i = 0; i < num_curves; i++)
  {
    const hb_gpu_encode_curve_info_t &info = s.curve_infos.arrayZ[i];

    for (int b = info.hband_lo; b <= info.hband_hi; b++) {
      unsigned idx = s.hband_cursors.arrayZ[b]++;
      s.hband_curves.arrayZ[idx] = i;
      s.hband_curves_asc.arrayZ[idx] = i;
    }

    for (int b = info.vband_lo; b <= info.vband_hi; b++) {
      unsigned idx = s.vband_cursors.arrayZ[b]++;
      s.vband_curves.arrayZ[idx] = i;
      s.vband_curves_asc.arrayZ[idx] = i;
    }
  }

  /* Sort: descending by max, ascending by min */
  const hb_gpu_encode_curve_info_t *infos = s.curve_infos.arrayZ;

  for (unsigned b = 0; b < num_hbands; b++)
  {
    unsigned off = s.hband_offsets.arrayZ[b];
    unsigned count = s.hband_curve_counts.arrayZ[b];
    s.hband_curves.as_array ().sub_array (off, count)
      .qsort ([infos] (const unsigned &a, const unsigned &b) {
	return infos[a].max_x > infos[b].max_x;
      });
    s.hband_curves_asc.as_array ().sub_array (off, count)
      .qsort ([infos] (const unsigned &a, const unsigned &b) {
	return infos[a].min_x < infos[b].min_x;
      });
  }

  for (unsigned b = 0; b < num_vbands; b++)
  {
    unsigned off = s.vband_offsets.arrayZ[b];
    unsigned count = s.vband_curve_counts.arrayZ[b];
    s.vband_curves.as_array ().sub_array (off, count)
      .qsort ([infos] (const unsigned &a, const unsigned &b) {
	return infos[a].max_y > infos[b].max_y;
      });
    s.vband_curves_asc.as_array ().sub_array (off, count)
      .qsort ([infos] (const unsigned &a, const unsigned &b) {
	return infos[a].min_y < infos[b].min_y;
      });
  }

  /* Compute sizes */
  unsigned total_curve_indices = (total_hband_indices + total_vband_indices) * 2;

  unsigned header_len = 2;

  unsigned num_contour_breaks = 0;
  for (unsigned i = 0; i + 1 < num_curves; i++)
    if (curves[i].p3x != curves[i + 1].p1x ||
	curves[i].p3y != curves[i + 1].p1y)
      num_contour_breaks++;

  unsigned curve_data_len = num_curves + num_contour_breaks + 1;
  unsigned band_headers_len = num_hbands + num_vbands;
  unsigned total_len = header_len + band_headers_len + total_curve_indices + curve_data_len;

  /* Validate fits in uint16 offsets (stored as int16 with bias) */
  if (total_len - 1 > (unsigned) UINT16_MAX)
    return nullptr;

  if (!quantize_fits_i16 (min_x) ||
      !quantize_fits_i16 (min_y) ||
      !quantize_fits_i16 (max_x) ||
      !quantize_fits_i16 (max_y))
    return nullptr;

  int16_t qmin_x = quantize (min_x);
  int16_t qmin_y = quantize (min_y);
  int16_t qmax_x = quantize (max_x);
  int16_t qmax_y = quantize (max_y);

  uint16_t hband_complexity_mask =
    _hb_gpu_compute_band_complexity_mask (curves, num_curves,
					  num_hbands,
					  (double) qmin_y,
					  (double) qmax_y,
					  true);
  uint16_t vband_complexity_mask =
    _hb_gpu_compute_band_complexity_mask (curves, num_curves,
					  num_vbands,
					  (double) qmin_x,
					  (double) qmax_x,
					  false);

  /* Allocate or reuse encode buffer */
  unsigned needed_bytes = total_len * sizeof (hb_gpu_texel_t);
  hb_gpu_texel_t *buf = nullptr;
  unsigned buf_capacity = 0;

  if (draw->recycled_blob &&
      draw->recycled_blob->destroy == _hb_gpu_blob_data_destroy)
  {
    auto *bd = (hb_gpu_blob_data_t *) draw->recycled_blob->user_data;
    if (bd->capacity >= needed_bytes)
    {
      buf = (hb_gpu_texel_t *) (void *) bd->buf;
      buf_capacity = bd->capacity;
    }
    else
    {
      unsigned alloc_bytes = needed_bytes + needed_bytes / 2;
      char *new_buf = (char *) hb_realloc (bd->buf, alloc_bytes);
      if (new_buf)
      {
	bd->buf = new_buf;
	bd->capacity = alloc_bytes;
	buf = (hb_gpu_texel_t *) (void *) new_buf;
	buf_capacity = alloc_bytes;
      }
    }
  }

  if (!buf)
  {
    buf_capacity = needed_bytes;
    buf = (hb_gpu_texel_t *) hb_malloc (needed_bytes);
    if (unlikely (!buf))
      return nullptr;
  }

  unsigned curve_data_offset = header_len + band_headers_len + total_curve_indices;

  /* Pack header */
  buf[0].r = qmin_x;
  buf[0].g = qmin_y;
  buf[0].b = qmax_x;
  buf[0].a = qmax_y;
  buf[1].r = (int16_t) num_hbands;
  buf[1].g = (int16_t) num_vbands;
  buf[1].b = (int16_t) hband_complexity_mask;
  buf[1].a = (int16_t) vband_complexity_mask;

  /* Pack curve data with shared endpoints */
  s.curve_texel_offset.resize (num_curves);
  unsigned texel = curve_data_offset;

  for (unsigned i = 0; i < num_curves; i++)
  {
    bool contour_start = (i == 0 ||
			  curves[i - 1].p3x != curves[i].p1x ||
			  curves[i - 1].p3y != curves[i].p1y);

    if (contour_start) {
      s.curve_texel_offset.arrayZ[i] = texel;
      buf[texel].r = quantize (curves[i].p1x);
      buf[texel].g = quantize (curves[i].p1y);
      buf[texel].b = quantize (curves[i].p2x);
      buf[texel].a = quantize (curves[i].p2y);
      texel++;
    } else {
      s.curve_texel_offset.arrayZ[i] = texel - 1;
    }

    bool has_next = (i + 1 < num_curves &&
		     curves[i].p3x == curves[i + 1].p1x &&
		     curves[i].p3y == curves[i + 1].p1y);

    buf[texel].r = quantize (curves[i].p3x);
    buf[texel].g = quantize (curves[i].p3y);
    if (has_next) {
      buf[texel].b = quantize (curves[i + 1].p2x);
      buf[texel].a = quantize (curves[i + 1].p2y);
    } else {
      buf[texel].b = 0;
      buf[texel].a = 0;
    }
    texel++;
  }

  /* Pack band headers and curve indices */
  unsigned index_offset = header_len + band_headers_len;

  for (unsigned b = 0; b < num_hbands; b++)
  {
    int16_t hband_split;
    {
      unsigned off = s.hband_offsets.arrayZ[b];
      unsigned n = s.hband_curve_counts.arrayZ[b];
      unsigned best_worst = n;
      double best_split = (min_x + max_x) * 0.5;
      unsigned left_count = n;
      for (unsigned ci = 0; ci < n; ci++) {
	double split = s.curve_infos.arrayZ[s.hband_curves.arrayZ[off + ci]].max_x;
	unsigned right_count = ci + 1;
	while (left_count &&
	       s.curve_infos.arrayZ[s.hband_curves_asc.arrayZ[off + left_count - 1]].min_x > split)
	  left_count--;
	unsigned worst = hb_max (right_count, left_count);
	if (worst < best_worst) {
	  best_worst = worst;
	  best_split = split;
	}
      }
      hband_split = quantize (best_split);
    }
    unsigned hdr = header_len + b;
    unsigned desc_off = index_offset;

    for (unsigned ci = 0; ci < s.hband_curve_counts.arrayZ[b]; ci++) {
      buf[index_offset].r = encode_offset (s.curve_texel_offset.arrayZ[s.hband_curves.arrayZ[s.hband_offsets.arrayZ[b] + ci]]);
      buf[index_offset].g = 0;
      buf[index_offset].b = 0;
      buf[index_offset].a = 0;
      index_offset++;
    }

    unsigned asc_off = index_offset;

    for (unsigned ci = 0; ci < s.hband_curve_counts.arrayZ[b]; ci++) {
      buf[index_offset].r = encode_offset (s.curve_texel_offset.arrayZ[s.hband_curves_asc.arrayZ[s.hband_offsets.arrayZ[b] + ci]]);
      buf[index_offset].g = 0;
      buf[index_offset].b = 0;
      buf[index_offset].a = 0;
      index_offset++;
    }

    buf[hdr].r = (int16_t) s.hband_curve_counts.arrayZ[b];
    buf[hdr].g = encode_offset (desc_off);
    buf[hdr].b = encode_offset (asc_off);
    buf[hdr].a = hband_split;
  }

  for (unsigned b = 0; b < num_vbands; b++)
  {
    int16_t vband_split;
    {
      unsigned off = s.vband_offsets.arrayZ[b];
      unsigned n = s.vband_curve_counts.arrayZ[b];
      unsigned best_worst = n;
      double best_split = (min_y + max_y) * 0.5;
      unsigned left_count = n;
      for (unsigned ci = 0; ci < n; ci++) {
	double split = s.curve_infos.arrayZ[s.vband_curves.arrayZ[off + ci]].max_y;
	unsigned right_count = ci + 1;
	while (left_count &&
	       s.curve_infos.arrayZ[s.vband_curves_asc.arrayZ[off + left_count - 1]].min_y > split)
	  left_count--;
	unsigned worst = hb_max (right_count, left_count);
	if (worst < best_worst) {
	  best_worst = worst;
	  best_split = split;
	}
      }
      vband_split = quantize (best_split);
    }

    unsigned hdr = header_len + num_hbands + b;
    unsigned desc_off = index_offset;

    for (unsigned ci = 0; ci < s.vband_curve_counts.arrayZ[b]; ci++) {
      buf[index_offset].r = encode_offset (s.curve_texel_offset.arrayZ[s.vband_curves.arrayZ[s.vband_offsets.arrayZ[b] + ci]]);
      buf[index_offset].g = 0;
      buf[index_offset].b = 0;
      buf[index_offset].a = 0;
      index_offset++;
    }

    unsigned asc_off = index_offset;

    for (unsigned ci = 0; ci < s.vband_curve_counts.arrayZ[b]; ci++) {
      buf[index_offset].r = encode_offset (s.curve_texel_offset.arrayZ[s.vband_curves_asc.arrayZ[s.vband_offsets.arrayZ[b] + ci]]);
      buf[index_offset].g = 0;
      buf[index_offset].b = 0;
      buf[index_offset].a = 0;
      index_offset++;
    }

    buf[hdr].r = (int16_t) s.vband_curve_counts.arrayZ[b];
    buf[hdr].g = encode_offset (desc_off);
    buf[hdr].b = encode_offset (asc_off);
    buf[hdr].a = vband_split;
  }

  if (draw->recycled_blob)
  {
    hb_blob_t *blob = draw->recycled_blob;
    draw->recycled_blob = nullptr;

    if (blob->destroy == _hb_gpu_blob_data_destroy)
    {
      /* Our blob — update the closure's buf pointer (may have been realloc'd). */
      auto *bd = (hb_gpu_blob_data_t *) blob->user_data;
      bd->buf = (char *) buf;
      bd->capacity = buf_capacity;
      blob->data = (const char *) buf;
      blob->length = needed_bytes;
      return blob;
    }

    /* Foreign blob — can't reuse buffer, replace entirely. */
    blob->replace_buffer ((const char *) buf, needed_bytes,
			  HB_MEMORY_MODE_WRITABLE,
			  buf, free);
    return blob;
  }

  /* No recycled blob — create new one with closure. */
  hb_gpu_blob_data_t *bd = (hb_gpu_blob_data_t *) hb_malloc (sizeof (*bd));
  if (unlikely (!bd))
  {
    hb_free (buf);
    return nullptr;
  }
  bd->buf = (char *) buf;
  bd->capacity = buf_capacity;

  return hb_blob_create ((const char *) buf, needed_bytes,
			 HB_MEMORY_MODE_WRITABLE,
			 bd, _hb_gpu_blob_data_destroy);
}


/* ---- Public API ---- */

/**
 * hb_gpu_draw_create_or_fail:
 *
 * Creates a new GPU shape encoder.
 *
 * Return value: (transfer full):
 * A newly allocated #hb_gpu_draw_t, or `NULL` on allocation failure.
 *
 * XSince: REPLACEME
 **/
hb_gpu_draw_t *
hb_gpu_draw_create_or_fail (void)
{
  return hb_object_create<hb_gpu_draw_t> ();
}

/**
 * hb_gpu_draw_reference: (skip)
 * @draw: a GPU shape encoder
 *
 * Increases the reference count on @draw by one.
 *
 * Return value: (transfer full):
 * The referenced #hb_gpu_draw_t.
 *
 * XSince: REPLACEME
 **/
hb_gpu_draw_t *
hb_gpu_draw_reference (hb_gpu_draw_t *draw)
{
  return hb_object_reference (draw);
}

/**
 * hb_gpu_draw_destroy: (skip)
 * @draw: a GPU shape encoder
 *
 * Decreases the reference count on @draw by one. When the
 * reference count reaches zero, the encoder is freed.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_draw_destroy (hb_gpu_draw_t *draw)
{
  if (!hb_object_destroy (draw)) return;
  hb_free (draw);
}

/**
 * hb_gpu_draw_set_user_data: (skip)
 * @draw: a GPU shape encoder
 * @key: the user-data key
 * @data: a pointer to the user data
 * @destroy: (nullable): a callback to call when @data is not needed anymore
 * @replace: whether to replace an existing data with the same key
 *
 * Attaches user data to @draw.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * XSince: REPLACEME
 **/
hb_bool_t
hb_gpu_draw_set_user_data (hb_gpu_draw_t     *draw,
			     hb_user_data_key_t *key,
			     void               *data,
			     hb_destroy_func_t   destroy,
			     hb_bool_t           replace)
{
  return hb_object_set_user_data (draw, key, data, destroy, replace);
}

/**
 * hb_gpu_draw_get_user_data: (skip)
 * @draw: a GPU shape encoder
 * @key: the user-data key
 *
 * Fetches the user-data associated with the specified key.
 *
 * Return value: (transfer none):
 * A pointer to the user data
 *
 * XSince: REPLACEME
 **/
void *
hb_gpu_draw_get_user_data (hb_gpu_draw_t     *draw,
			     hb_user_data_key_t *key)
{
  return hb_object_get_user_data (draw, key);
}

/**
 * hb_gpu_draw_get_funcs:
 *
 * Fetches the singleton #hb_draw_funcs_t that feeds outline data
 * into an #hb_gpu_draw_t.  Pass the #hb_gpu_draw_t as the
 * @draw_data argument when calling the draw functions.
 *
 * Return value: (transfer none):
 * The GPU draw functions
 *
 * XSince: REPLACEME
 **/
hb_draw_funcs_t *
hb_gpu_draw_get_funcs (void)
{
  return static_gpu_draw_funcs.get_unconst ();
}

/**
 * hb_gpu_draw_glyph:
 * @draw: a GPU shape encoder
 * @font: font to draw from
 * @codepoint: glyph ID to draw
 *
 * Convenience wrapper that draws a single glyph outline into the
 * encoder using hb_font_draw_glyph().
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_draw_glyph (hb_gpu_draw_t *draw,
			  hb_font_t      *font,
			  hb_codepoint_t  codepoint)
{
  hb_font_draw_glyph (font, codepoint,
		       hb_gpu_draw_get_funcs (),
		       draw);
}

/**
 * hb_gpu_draw_get_extents:
 * @draw: a GPU shape encoder
 * @extents: (out): glyph extents
 *
 * Fetches the extents of the accumulated outlines.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_draw_get_extents (hb_gpu_draw_t     *draw,
			   hb_glyph_extents_t *extents)
{
  if (draw->num_curves == 0 || draw->ext_min_x == HUGE_VAL)
  {
    extents->x_bearing = 0;
    extents->y_bearing = 0;
    extents->width = 0;
    extents->height = 0;
    return;
  }

  extents->x_bearing = (hb_position_t) floor (draw->ext_min_x);
  extents->y_bearing = (hb_position_t) ceil  (draw->ext_max_y);
  extents->width     = (hb_position_t) ceil  (draw->ext_max_x) -
		       (hb_position_t) floor (draw->ext_min_x);
  extents->height    = (hb_position_t) floor (draw->ext_min_y) -
		       (hb_position_t) ceil  (draw->ext_max_y);
}

/**
 * hb_gpu_draw_reset:
 * @draw: a GPU shape encoder
 *
 * Resets the encoder, discarding all accumulated outlines.
 * The internal encode buffer is kept for reuse.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_draw_reset (hb_gpu_draw_t *draw)
{
  draw->start_x = draw->start_y = 0;
  draw->current_x = draw->current_y = 0;
  draw->need_moveto = true;
  draw->num_curves = 0;
  draw->success = true;
  draw->curves.clear ();

  draw->ext_min_x =  HUGE_VAL;
  draw->ext_min_y =  HUGE_VAL;
  draw->ext_max_x = -HUGE_VAL;
  draw->ext_max_y = -HUGE_VAL;
}

/**
 * hb_gpu_draw_recycle_blob:
 * @draw: a GPU shape encoder
 * @blob: (transfer full): a blob previously returned by hb_gpu_draw_encode()
 *
 * Returns a blob to the encoder for potential reuse.
 * The caller transfers ownership of @blob.
 *
 * Currently this simply destroys the blob.  A future version
 * may reclaim the underlying buffer to avoid allocation on the
 * next hb_gpu_draw_encode() call.
 *
 * XSince: REPLACEME
 **/
void
hb_gpu_draw_recycle_blob (hb_gpu_draw_t *draw,
			    hb_blob_t      *blob)
{
  hb_blob_destroy (draw->recycled_blob);
  draw->recycled_blob = nullptr;
  if (!blob || blob == hb_blob_get_empty ())
    return;
  draw->recycled_blob = blob;
}
