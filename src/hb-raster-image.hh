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

#ifndef HB_RASTER_IMAGE_HH
#define HB_RASTER_IMAGE_HH

#include "hb.hh"
#include "hb-raster.hh"
#include "hb-raster.h"
#include "hb-object.hh"
#include "hb-vector.hh"


/* A box of pixels in an image. */
struct hb_raster_box_t
{
  unsigned x0, y0;
  unsigned x1, y1;

  bool is_empty () const { return x0 >= x1 || y0 >= y1; }

  int64_t get_area () const
  { return is_empty () ? 0 : (int64_t) (x1 - x0) * (y1 - y0); }

  void add (const hb_raster_box_t &o)
  {
    if (o.is_empty ()) return;
    if (is_empty ()) { *this = o; return; }
    x0 = hb_min (x0, o.x0);
    y0 = hb_min (y0, o.y0);
    x1 = hb_max (x1, o.x1);
    y1 = hb_max (y1, o.y1);
  }
};

/* hb_raster_image_t — pixel artifact */
struct hb_raster_image_t
{
  hb_object_header_t  header;

  hb_vector_t<uint8_t> buffer;
  hb_raster_extents_t  extents     = {};
  hb_raster_format_t   format      = HB_RASTER_FORMAT_A8;

  /* Bookkeeping of raster paint, which notes here the box enclosing what
   * it has painted on a surface since clearing it, so as to clear and
   * composite no more than that.  Means nothing on any other image. */
  hb_raster_box_t      painted     = {0, 0, 0, 0};

  HB_INTERNAL static unsigned bytes_per_pixel (hb_raster_format_t format);
  HB_INTERNAL bool configure (hb_raster_format_t format, hb_raster_extents_t extents);
  HB_INTERNAL bool deserialize_from_png (hb_blob_t *png,
					 unsigned max_dimension = (unsigned) INT_MAX);
  HB_INTERNAL hb_blob_t *serialize_to_png_or_fail () const;
  HB_INTERNAL void clear ();
  HB_INTERNAL void clear_painted ();
  HB_INTERNAL const uint8_t *get_buffer () const;
  HB_INTERNAL void composite_from (const hb_raster_image_t *src,
				   hb_paint_composite_mode_t mode,
				   const hb_raster_box_t &box);
};

/* Composite src image onto dst, within box. */
HB_INTERNAL void
hb_raster_image_composite (hb_raster_image_t *dst,
			   const hb_raster_image_t *src,
			   hb_paint_composite_mode_t mode,
			   const hb_raster_box_t &box);

/* Whether compositing a clear source pixel gives back the destination
 * pixel, whatever that holds.  Where it does, only what the source has
 * painted needs compositing; otherwise what either has does, two clear
 * pixels compositing to a clear one in every mode. */
HB_INTERNAL bool
hb_raster_composite_mode_ignores_clear_source (hb_paint_composite_mode_t mode);


#endif /* HB_RASTER_IMAGE_HH */
