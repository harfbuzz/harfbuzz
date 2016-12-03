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

#ifndef HB_OT_CBDT_TABLE_HH
#define HB_OT_CBDT_TABLE_HH

#include "hb-open-type-private.hh"

namespace OT {

struct SmallGlyphMetrics
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  BYTE height;
  BYTE width;
  CHAR bearingX;
  CHAR bearingY;
  BYTE advance;

  DEFINE_SIZE_STATIC(5);
};

struct SBitLineMetrics
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  CHAR ascender;
  CHAR decender;
  BYTE widthMax;
  CHAR caretSlopeNumerator;
  CHAR caretSlopeDenominator;
  CHAR caretOffset;
  CHAR minOriginSB;
  CHAR minAdvanceSB;
  CHAR maxBeforeBL;
  CHAR minAfterBL;
  CHAR padding1;
  CHAR padding2;

  DEFINE_SIZE_STATIC(12);
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

  DEFINE_SIZE_STATIC(8);
};

struct IndexSubtableFormat1
{
  IndexSubHeader header;
  ULONG offsetArrayZ[VAR];

  DEFINE_SIZE_ARRAY(8, offsetArrayZ);
};

/*
 * Glyph Bitmap Data Formats.
 */

struct GlyphBitmapDataFormat17
{
  SmallGlyphMetrics glyphMetrics;
  ULONG dataLen;
  BYTE dataZ[VAR];

  DEFINE_SIZE_ARRAY(9, dataZ);
};

struct IndexSubtableArray
{
  inline bool sanitize (hb_sanitize_context_t *c, unsigned int count) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this)); // XXX
  }

  public:
  const IndexSubtable* find_table (hb_codepoint_t glyph, unsigned int numTables) const
  {
    for (unsigned int i = 0; i < numTables; ++i)
    {
      unsigned int firstGlyphIndex = indexSubtablesZ[i].firstGlyphIndex;
      unsigned int lastGlyphIndex = indexSubtablesZ[i].lastGlyphIndex;
      if (firstGlyphIndex <= glyph && glyph <= lastGlyphIndex) {
        return &indexSubtablesZ[i];
      }
    }
    return NULL;
  }

  protected:
  IndexSubtable indexSubtablesZ[VAR];

  public:
  DEFINE_SIZE_ARRAY(0, indexSubtablesZ);
};

struct BitmapSizeTable
{
  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  indexSubtableArrayOffset.sanitize (c, base, numberOfIndexSubtables) &&
		  c->check_range (&(base+indexSubtableArrayOffset), indexTablesSize) &&
		  horizontal.sanitize (c) &&
		  vertical.sanitize (c));
  }

  OffsetTo<IndexSubtableArray, ULONG> indexSubtableArrayOffset;
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
  CHAR flags;

  DEFINE_SIZE_STATIC(48);
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
    return_trace (c->check_struct (this) &&
		  likely (version.major == 2 || version.major == 3) &&
		  sizeTables.sanitize (c, this));
  }

  public:
  const BitmapSizeTable* find_table (hb_codepoint_t glyph) const
  {
    // TODO: Make it possible to select strike.
    unsigned int count = sizeTables.len;
    for (uint32_t i = 0; i < count; ++i) {
      unsigned int startGlyphIndex = sizeTables.array[i].startGlyphIndex;
      unsigned int endGlyphIndex = sizeTables.array[i].endGlyphIndex;
      if (startGlyphIndex <= glyph && glyph <= endGlyphIndex) {
        return &sizeTables[i];
      }
    }
    return NULL;
  }

  protected:
  FixedVersion<>version;
  ArrayOf<BitmapSizeTable, ULONG> sizeTables;

  public:
  DEFINE_SIZE_ARRAY(8, sizeTables);
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
    return_trace (c->check_struct (this) &&
		  likely (version.major == 2 || version.major == 3));
  }

  protected:
  FixedVersion<>version;
  BYTE dataZ[VAR];

  public:
  DEFINE_SIZE_ARRAY(4, dataZ);
};

} /* namespace OT */

#endif /* HB_OT_CBDT_TABLE_HH */
