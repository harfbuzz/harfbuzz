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

#ifndef HB_GPU_DRAW_HH
#define HB_GPU_DRAW_HH

#include "hb.hh"
#include "hb-gpu.h"
#include "hb-object.hh"
#include "hb-vector.hh"

#include <cmath>


#define HB_GPU_UNITS_PER_EM 4


struct hb_gpu_curve_t
{
  double p1x, p1y;
  double p2x, p2y;
  double p3x, p3y;
};

struct hb_gpu_draw_t
{
  hb_object_header_t header;

  /* Accumulator state */
  double start_x = 0, start_y = 0;
  double current_x = 0, current_y = 0;
  bool need_moveto = true;
  unsigned num_curves = 0;
  bool success = true;

  hb_vector_t<hb_gpu_curve_t> curves;

  /* Extents tracking (updated during accumulation) */
  double ext_min_x =  INFINITY;
  double ext_min_y =  INFINITY;
  double ext_max_x = -INFINITY;
  double ext_max_y = -INFINITY;


  /* Internal accumulation methods */
  HB_INTERNAL void acc_move_to (double x, double y);
  HB_INTERNAL void acc_line_to (double x, double y);
  HB_INTERNAL void acc_conic_to (double cx, double cy, double x, double y);
  HB_INTERNAL void acc_cubic_to (double c1x, double c1y,
				 double c2x, double c2y,
				 double x, double y);
  HB_INTERNAL void acc_close_path ();
};


#endif /* HB_GPU_DRAW_HH */
