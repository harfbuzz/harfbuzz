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

#ifndef HB_OT_COLR_TABLE_HH
#define HB_OT_COLR_TABLE_HH

#include "hb-open-type-private.hh"

/*
 * Color Palette
 * http://www.microsoft.com/typography/otspec/colr.htm
 */

#define HB_OT_TAG_COLR HB_TAG('C','O','L','R')

namespace OT {


struct LayerRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  GlyphID gID;			/* Glyph ID of layer glyph */
  HBUINT16 paletteIndex;	/* Index value to use with a selected color palette */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct BaseGlyphRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  GlyphID gID;			/* Glyph ID of reference glyph */
  HBUINT16 firstLayerIndex;	/* Index to the layer record */
  HBUINT16 numLayers;		/* Number of color layers associated with this glyph */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct COLR
{
  static const hb_tag_t tableTag = HB_OT_TAG_COLR;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
      c->check_array ((const void*) &layerRecordsOffset, sizeof (LayerRecord), numLayerRecords) &&
      c->check_array ((const void*) &baseGlyphRecords, sizeof (BaseGlyphRecord), numBaseGlyphRecords));
  }

  protected:
  HBUINT16	version;		/* Table version number */
  HBUINT16	numBaseGlyphRecords;	/* Number of Base Glyph Records */
  LOffsetTo<BaseGlyphRecord>
		baseGlyphRecords;	/* Offset to Base Glyph records. */
  LOffsetTo<LayerRecord>
		layerRecordsOffset;	/* Offset to Layer Records */
  HBUINT16	numLayerRecords;	/* Number of Layer Records */
  public:
  DEFINE_SIZE_STATIC (14);
};

} /* namespace OT */


#endif /* HB_OT_COLR_TABLE_HH */
