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

#ifndef HB_AAT_LAYOUT_ZAPF_TABLE_HH
#define HB_AAT_LAYOUT_ZAPF_TABLE_HH

#include "hb-aat-layout-common.hh"
#include "hb-ot-layout.hh"
#include "hb-open-type.hh"

/*
 * Zapf -- Hermann Zapf -- contains information about the individual glyphs
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6Zapf.html
 */
#define HB_AAT_TAG_Zapf HB_TAG('Z','a','p','f')


namespace AAT {

using namespace OT;

struct ZapfFeatureInfo
{
  enum Context
  {
    LineInitial			= 0x0001,
    LineMedial			= 0x0002,
    LineFinal			= 0x0004,
    WordInitial			= 0x0008,
    WordMedial			= 0x0010,
    WordFinal			= 0x0020,
    AutoFractionNumerator	= 0x0040,
    AutoFractionDenominator	= 0x0080
  };

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  aatFeatures.sanitize (c)));
  }

  protected:
  HBUINT16	context;	/* Bitfield identifying the contexts in which
				 * this glyph appears. Note more than one bit
				 * may be on! A value of zero means context is
				 * irrelevant. */
  ArrayOf<HBUINT32>
		aatFeatures;	/* The <type,selector> pairs. (This type is
				 * defined in SFNTTypes.h, with constants in
				 * SFNTLayoutTypes.h) */
/*LArrayOf<Tag>	otTags; */	/* The array of tags. */
  public:
  DEFINE_SIZE_ARRAY (4, aatFeatures);
};

struct ZapfGlyphSubgroup
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  glyphs.sanitize (c)));
  }

  protected:
  NameID	nameIndex;	/* Index in the 'name' table for this group's
				 * name; a value of zero indicates no name
				 * for this group */
  ArrayOf<GlyphID>
		glyphs;		/* Number of glyph indices in this named group;
				 * this may be zero, in which case no glyphs
				 * will follow and this name is the name for
				 * the whole group (this convention is only
				 * valid for the first name in a group)
				 *
				 * Glyph indices for this group. */
  public:
  DEFINE_SIZE_ARRAY (4, glyphs);
};

struct ZapfGlyphGroup
{
  enum Flags
  {
    isAligned		= 0x8000,
    isSubdivided	= 0x4000,
    reserved		= 0x3FFF
  };

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  ((numGroups & 0x4000) != 0) &&
			  ((numGroups & 0x8000) == 0)));
  }

  protected:
  HBUINT16	numGroups;	/* The low-order 14 bits specify the number of
				 * offsets to GlyphGroups in the array. Bit
				 * 15 should be 0, and bit 14 should be 1. */
/*GlyphSubgroup groups[VAR];*/	/* The GlyphSubgroups for this GlyphGroup. Each
				 * group may be preceded by a 16-bit flag
				 * (depending on the high bit of the numGroups
				 * field) */
  public:
  DEFINE_SIZE_STATIC (2);
};

struct ZapfGlyphGroupOffsetArray
{
  bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  ((numGroups & 0x4000) != 0) &&
			  ((numGroups & 0x8000) == 0) &&
			  groupOffsets.sanitize (c, numGroups & 0x3FFF, base)));
  }

  protected:
  HBUINT16	numGroups;	/* The low-order 14 bits specify the number
				 * of offsets to GlyphGroups in the array.
				 * Bit 15 should be 0, and bit 14 should be 1. */
  HBUINT16	padding;	/* This field is not currently used and
				 * should be set to 0. It is present to
				 * maintain as far as possible 32-bit alignment
				 * within the 'Zapf' table. */
  UnsizedArrayOf<LOffsetTo<ZapfGlyphGroup> >
		groupOffsets;	/* Offsets (relative to extraInfo) to the GlyphGroups
				 * associated with this GlyphGroupOffsetArray. The
				 * first value in this field indicates the group
				 * to be used as the “alternate forms” for the
				 * given glyph, as in the Character Palette on
				 * OS X. In the case where no group containing
				 * this glyph contains the alternate forms for
				 * the glyph, the first groupOffset should be
				 * 0xFFFFFFFF. */
  public:
  DEFINE_SIZE_ARRAY (4, groupOffsets);
};

struct ZapfGlyphIdentifier
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  data.sanitize (c, kind < 64 ? 1 : 2)));
  }

  protected:
  HBUINT8	kind;		/* What kind of identifier this is */
  UnsizedArrayOf<HBUINT8>
		data;		/* Identifier data */
  public:
  DEFINE_SIZE_ARRAY (1, data);
};

struct ZapfGlyphInfo
{
  bool sanitize (hb_sanitize_context_t *c, unsigned int extra_info) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  groupOffset.sanitize (c, this, this) &&
			  featOffset.sanitize (c, this)));
  }

  protected:
  LOffsetTo<ZapfGlyphGroupOffsetArray>
		groupOffset;	/* Byte offset from start of extraInfo to the
				 * GlyphGroup or GlyphGroupOffsetArray for
				 * this glyph, or 0xFFFFFFFF if none */
  LOffsetTo<ZapfFeatureInfo>
		featOffset;	/* Byte offset from start of extraInfo to
				 * FeatureInfo for this glyph, or 0xFFFFFFFF
				 * if none */
  HBUINT8	flags;
  ArrayOf<HBUINT16, HBUINT8>
		unicodes;	/* Unicode code points for this glyph (if any) */
  /*ArrayOf<ZapfGlyphIdentifier, HBUINT16>
		glyphIDs;*/	/* GlyphIdentifiers for this glyph (if any) */
  public:
  DEFINE_SIZE_ARRAY (10, unicodes);
};

struct Zapf
{
  enum { tableTag = HB_AAT_TAG_Zapf };

  void a() const {
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    printf ("%d ", (int) u.glyphInfosV1[0]);
    return_trace (likely (c->check_struct (this) &&
			  ((version == 1 && u.glyphInfosV1.sanitize (c, c->get_num_glyphs (),
								     this, extraInfo)) /*||
			   (version == 2 && u.glyphInfosV2.sanitize (c, this, extraInfo))*/)));
  }

  protected:
  HBUINT16	version;	/* Set to 2 */
  HBUINT16	reserved;	/* 0 */
  LOffsetTo<HBUINT32>
		extraInfo;	/* Offset from start of table to start of
				 * extra info space (added to groupOffset
				 * and featOffset in GlyphInfo) */
  union
  {
    UnsizedArrayOf<LOffsetTo<ZapfGlyphInfo> >
		glyphInfosV1;
    Lookup<LOffsetTo<ZapfGlyphInfo> >
		glyphInfosV2;
  } u;
  public:
  DEFINE_SIZE_MIN (6);
};

} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_ZAPF_TABLE_HH */
