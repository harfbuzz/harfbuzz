/*
 * Copyright © 2014  Google, Inc.
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

#ifndef HB_OT_CMAP_TABLE_HH
#define HB_OT_CMAP_TABLE_HH

#include "hb-open-type-private.hh"


namespace OT {


/*
 * cmap -- Character To Glyph Index Mapping Table
 */

#define HB_OT_TAG_cmap HB_TAG('c','m','a','p')


struct CmapSubtableFormat4
{
  friend struct CmapSubtable;

  private:
  inline bool get_glyph (hb_codepoint_t codepoint, hb_codepoint_t *glyph) const
  {
    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 c->check_range (this, length) &&
			 16 + 4 * (unsigned int) segCountX2 < length);
  }

  protected:
  USHORT	format;		/* Format number is set to 4. */
  USHORT	length;		/* This is the length in bytes of the
				 * subtable. */
  USHORT	language;	/* Ignore. */
  USHORT	segCountX2;	/* 2 x segCount. */
  USHORT	searchRange;	/* 2 * (2**floor(log2(segCount))) */
  USHORT	entrySelector;	/* log2(searchRange/2) */
  USHORT	rangeShift;	/* 2 x segCount - searchRange */

  USHORT	values[VAR];
#if 0
  USHORT	endCount[segCount];	/* End characterCode for each segment,
					 * last=0xFFFF. */
  USHORT	reservedPad;		/* Set to 0. */
  USHORT	startCount[segCount];	/* Start character code for each segment. */
  SHORT		idDelta[segCount];	/* Delta for all character codes in segment. */
  USHORT	idRangeOffset[segCount];/* Offsets into glyphIdArray or 0 */
  USHORT	glyphIdArray[VAR];	/* Glyph index array (arbitrary length) */
#endif

  public:
  DEFINE_SIZE_ARRAY (14, values);
};

struct CmapSubtable
{
  inline bool get_glyph (hb_codepoint_t codepoint, hb_codepoint_t *glyph) const
  {
    switch (u.format) {
    case 4: return u.format4.get_glyph(codepoint, glyph);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return TRACE_RETURN (false);
    switch (u.format) {
    case 4: return TRACE_RETURN (u.format4.sanitize (c));
    default:return TRACE_RETURN (true);
    }
  }

  protected:
  union {
  USHORT		format;		/* Format identifier */
  CmapSubtableFormat4	format4;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};


struct EncodingRecord
{
  int cmp (const EncodingRecord &other) const
  {
    int ret;
    ret = other.platformID.cmp (platformID);
    if (ret) return ret;
    ret = other.encodingID.cmp (encodingID);
    if (ret) return ret;
    return 0;
  }

  inline bool sanitize (hb_sanitize_context_t *c, void *base) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 subtable.sanitize (c, base));
  }

  USHORT	platformID;	/* Platform ID. */
  USHORT	encodingID;	/* Platform-specific encoding ID. */
  LongOffsetTo<CmapSubtable>
		subtable;	/* Byte offset from beginning of table to the subtable for this encoding. */
  public:
  DEFINE_SIZE_STATIC (8);
};

struct cmap
{
  static const hb_tag_t tableTag	= HB_OT_TAG_cmap;

  inline unsigned int find_subtable (unsigned int platform_id,
				     unsigned int encoding_id) const
  {
    EncodingRecord key;
    key.platformID.set (platform_id);
    key.encodingID.set (encoding_id);

    return encodingRecord.search (key);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE (this);
    return TRACE_RETURN (c->check_struct (this) &&
			 likely (version == 0) &&
			 encodingRecord.sanitize (c, this));
  }

  USHORT			version;	/* Table version number (0). */
  ArrayOf<EncodingRecord>	encodingRecord;	/* Encoding tables. */
  public:
  DEFINE_SIZE_ARRAY (4, encodingRecord);
};


} /* namespace OT */


#endif /* HB_OT_CMAP_TABLE_HH */
