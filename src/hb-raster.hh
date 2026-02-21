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

#ifndef HB_RASTER_HH
#define HB_RASTER_HH

#include "hb.hh"
#include "hb-raster.h"
#include "hb-geometry.hh"
#include "hb-object.hh"
#include "hb-vector.hh"


/* Normalized edge: yH > yL always */
struct hb_raster_edge_t
{
  int32_t xL, yL;   /* lower endpoint (26.6) */
  int32_t xH, yH;   /* upper endpoint (26.6) */
  int64_t slope;    /* dx/dy in 16.16 fixed point: ((int64_t)dx << 16) / dy */
  int32_t wind;     /* +1 or -1 */
};

/* hb_raster_image_t — pixel artifact */
struct hb_raster_image_t
{
  hb_object_header_t  header;

  uint8_t            *buffer      = nullptr;
  size_t              buffer_size = 0;
  hb_raster_extents_t extents     = {};
  hb_raster_format_t  format      = HB_RASTER_FORMAT_A8;

  ~hb_raster_image_t ()
  {
    hb_free (buffer);
  }
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
};


#endif /* HB_RASTER_HH */
