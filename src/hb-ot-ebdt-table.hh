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
 * Google Author(s): Seigo Nonaka
 */

#ifndef HB_OT_EBDT_TABLE_HH
#define HB_OT_EBDT_TABLE_HH

#include "hb-open-type-private.hh"

namespace OT {

struct SmallGlyphMetrics
{
  BYTE height;
  BYTE width;
  int8_t bearingX;
  int8_t bearingY;
  BYTE advance;

  DEFINE_SIZE_STATIC(5);
};

struct SBitLineMetrics {
  int8_t ascender;
  int8_t decender;
  BYTE widthMax;
  int8_t caretSlopeNumerator;
  int8_t caretSlopeDenominator;
  int8_t caretOffset;
  int8_t minOriginSB;
  int8_t minAdvanceSB;
  int8_t maxBeforeBL;
  int8_t minAfterBL;
  int8_t padding1;
  int8_t padding2;

  DEFINE_SIZE_STATIC(12);
};

struct BitmapSizeTable
{
  ULONG indexSubtableArrayOffset;
  ULONG indexTablesSize;
  ULONG numberOfIndexSubtables;
  ULONG colorRef;
  SBitLineMetrics horizontal;
  SBitLineMetrics vertical;
  USHORT startGlyphIndex;
  USHORT endGlyphIndex;
  BYTE ppemX;
  BYTE ppemY;
  BYTE bitDepth;
  int8_t flags;

  DEFINE_SIZE_STATIC(48);
};

/*
 * Index Subtables.
 */
struct IndexSubtable
{
  USHORT firstGlyphIndex;
  USHORT lastGlyphIndex;
  ULONG offsetToSubtable;

  DEFINE_SIZE_STATIC(8);
};

struct IndexSubHeader
{
  USHORT indexFormat;
  USHORT imageFormat;
  ULONG imageDataOffset;
};

struct IndexSubtableFormat1
{
  IndexSubHeader header;
  ULONG offsetArray[VAR];
};

/*
 * Glyph Bitmap Data Formats.
 */
struct GlyphBitmapDataFormat17
{
  SmallGlyphMetrics glyphMetrics;
  ULONG dataLen;
  BYTE data[VAR];
};

struct IndexSubtableArray
{
  public:
  const IndexSubtable* find_table(hb_codepoint_t glyph, unsigned int numTables) const
  {
    for (unsigned int i = 0; i < numTables; ++i) {
      unsigned int firstGlyphIndex = indexSubtables[i].firstGlyphIndex;
      unsigned int lastGlyphIndex = indexSubtables[i].lastGlyphIndex;
      if (firstGlyphIndex <= glyph && glyph <= lastGlyphIndex) {
        return &indexSubtables[i];
      }
    }
    return NULL;
  }

  protected:
  IndexSubtable indexSubtables[VAR];
};

/*
 * CBLC -- Color Bitmap Location Table
 */

#define HB_OT_TAG_CBLC HB_TAG('C','B','L','C')

struct CBLC
{
  static const hb_tag_t tableTag = HB_OT_TAG_CBLC;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (true);
  }

  public:
  const BitmapSizeTable* find_table(hb_codepoint_t glyph) const
  {
    // TODO: Make it possible to select strike.
    const uint32_t tableSize = numSizes;
    for (uint32_t i = 0; i < tableSize; ++i) {
      unsigned int startGlyphIndex = sizeTables[i].startGlyphIndex;
      unsigned int endGlyphIndex = sizeTables[i].endGlyphIndex;
      if (startGlyphIndex <= glyph && glyph <= endGlyphIndex) {
        return &sizeTables[i];
      }
    }
    return NULL;
  }

  protected:
  ULONG version;
  ULONG numSizes;

  BitmapSizeTable sizeTables[VAR];
};

/*
 * CBDT -- Color Bitmap Data Table
 */
#define HB_OT_TAG_CBDT HB_TAG('C','B','D','T')

struct CBDT
{
  static const hb_tag_t tableTag = HB_OT_TAG_CBDT;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (true);
  }

  protected:
  BYTE data[VAR];
};

} /* namespace OT */

#endif /* HB_OT_EBDT_TABLE_HH */
