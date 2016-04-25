/*
 * Copyright © 2016  Google, Inc.
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
 * Google Author(s): Sascha Brawer
 */

#ifndef HB_OT_CPAL_TABLE_HH
#define HB_OT_CPAL_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {

/*
 * Color Palette
 * http://www.microsoft.com/typography/otspec/cpal.htm
 */

#define HB_OT_TAG_CPAL HB_TAG('C','P','A','L')

struct ColorRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    // We do not enforce alpha != 0 because zero alpha is bogus but harmless.
    TRACE_SANITIZE (this);
    return_trace (true);
  }

  public:
  BYTE blue;
  BYTE green;
  BYTE red;
  BYTE alpha;
  DEFINE_SIZE_STATIC (4);
};

struct CPALV1Tail
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (paletteFlags.sanitize (c, this) &&
    		  paletteLabel.sanitize (c, this) &&
    		  paletteEntryLabel.sanitize (c, this));
  }
  
  public:
  OffsetTo<ULONG, ULONG> paletteFlags;
  OffsetTo<USHORT, ULONG> paletteLabel;
  OffsetTo<USHORT, ULONG> paletteEntryLabel;
  DEFINE_SIZE_STATIC (12);  
};

struct CPAL
{
  static const hb_tag_t tableTag = HB_OT_TAG_CPAL;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!(c->check_struct (this) &&
		    offsetFirstColorRecord.sanitize (c, this)))) {
      return_trace (false);
    }
    for (unsigned int i = 0; i < numPalettes; ++i) {
      if (unlikely (colorRecordIndices[i] + numPaletteEntries > numColorRecords)) {
	return_trace (false);
      }
    }
    if (version > 1) {
      const CPALV1Tail &v1 = StructAfter<CPALV1Tail>(*this);
      return_trace (v1.sanitize (c));
    } else {
      return_trace (true);
    }
  }

  inline unsigned int get_size (void) const {
    return min_size + numPalettes * 2;
  }

  public:
  USHORT	version;

  /* Version 0 */
  USHORT	numPaletteEntries;
  USHORT	numPalettes;
  USHORT	numColorRecords;
  OffsetTo<ColorRecord, ULONG> offsetFirstColorRecord;
  USHORT	colorRecordIndices[VAR];  // VAR=numPalettes

  public:
  DEFINE_SIZE_ARRAY (12, colorRecordIndices);
};

} /* namespace OT */


#endif /* HB_OT_CPAL_TABLE_HH */
