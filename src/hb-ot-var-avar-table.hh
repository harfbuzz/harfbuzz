/*
 * Copyright © 2017  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_VAR_AVAR_TABLE_HH
#define HB_OT_VAR_AVAR_TABLE_HH

#include "hb-open-type-private.hh"
#include "hb-ot-var.h"

namespace OT {


struct AxisValueMap
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  F2DOT14	fromCoord;	/* A normalized coordinate value obtained using
				 * default normalization. */
  F2DOT14	toCoord;	/* The modified, normalized coordinate value. */

  public:
  DEFINE_SIZE_STATIC (4);
};

typedef ArrayOf<AxisValueMap> SegmentMaps;

/*
 * avar — Axis Variations Table
 */

struct avar
{
  static const hb_tag_t tableTag	= HB_OT_TAG_avar;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!(version.sanitize (c) &&
		    version.major == 1 &&
		    c->check_struct (this))))
      return_trace (false);

    const SegmentMaps *map = &axisSegmentMapsZ;
    unsigned int count = axisCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (unlikely (!map->sanitize (c)))
        return_trace (false);
      map = &StructAfter<SegmentMaps> (*map);
    }

    return_trace (true);
  }

  protected:
  FixedVersion<>version;	/* Version of the avar table
				 * initially set to 0x00010000u */
  USHORT	reserved;	/* This field is permanently reserved. Set to 0. */
  USHORT	axisCount;	/* The number of variation axes in the font. This
				 * must be the same number as axisCount in the
				 * 'fvar' table. */
  SegmentMaps	axisSegmentMapsZ;

  public:
  DEFINE_SIZE_MIN (8);
};

} /* namespace OT */


#endif /* HB_OT_VAR_AVAR_TABLE_HH */
