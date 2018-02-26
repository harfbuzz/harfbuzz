/*
 * Copyright © 2018  Ebrahim Byagowi
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
 */

#ifndef HB_AAT_LAYOUT_ANKR_TABLE_HH
#define HB_AAT_LAYOUT_ANKR_TABLE_HH

#include "hb-aat-layout-common-private.hh"

#define HB_AAT_TAG_ankr HB_TAG('a','n','k','r')


namespace AAT {


/*
 * ankr -- Anchor point
 */

struct GlyphDataTable
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      (numPoints == 0) &&
      (anchorPoints.sanitize (c, numPoints)));
  }

  protected:
  HBUINT32			numPoints;	/* Number of anchor points for this glyph */
  UnsizedArrayOf<HBUINT32>	anchorPoints;	/* Individual anchor points. Each anchor point is a two-byte
						 * signed x-coordinate followed by a two-byte signed y-coordinate */

  public:
  DEFINE_SIZE_ARRAY (4, anchorPoints);
};

struct ankr
{
  static const hb_tag_t tableTag = HB_AAT_TAG_ankr;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      (version == 0) &&
      (flags == 0) &&
      (lookupTableOffset == 0x0000000C) &&
      (glyphDataTableOffset.sanitize (c, this)));
  }

  protected:
  HBUINT16			version; 		/* Version number (set to zero) */
  HBUINT16			flags;			/* Flags (currently unused; set to zero) */
  LOffsetTo<const void*>	lookupTableOffset;	/* Offset to the table's lookup table */
  LOffsetTo<GlyphDataTable>	glyphDataTableOffset;	/* Offset to the glyph data table */

  public:
  DEFINE_SIZE_MIN (12);
};

} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_ANKR_TABLE_HH */
