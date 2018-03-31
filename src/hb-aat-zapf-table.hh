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

#ifndef HB_AAT_LCAR_TABLE_HH
#define HB_AAT_LCAR_TABLE_HH

#include "hb-aat-layout-common-private.hh"

#define HB_AAT_TAG_Zapf HB_TAG('Z','a','p','f')


namespace AAT {

struct ZapfGlyphIdentifier
{
  HBUINT8	kind;	/* What kind of identifier this is */
  HBUINT8	data[var];	/* Identifier data */
};

struct ZapfGlyphInfo
{
  Offset<HBUINT32>
	groupOffset;	/* Byte offset from start of extraInfo to the GlyphGroup or
			 * GlyphGroupOffsetArray for this glyph, or 0xFFFFFFFF if none */
  Offset<HBUINT32>
	featOffset;	/* Byte offset from start of extraInfo to FeatureInfo
			 * for this glyph, or 0xFFFFFFFF if none */
  HBUINT8	flags;
  ArrayOf<HBUINT16, HBUINT8>
	unicodes;	/* Unicode code points for this glyph (if any) */
  ArrayOf<ZapfGlyphIdentifier, HBUINT16>
	glyphIDs;	/* GlyphIdentifiers for this glyph (if any) */
  //(UInt8	padding2[1..3]	If needed, to pad to 32-bit alignment before next GlyphInfo in the array)
};

/*
 * Zapf -- Hermann Zapf -- contains information about the individual glyphs in the font
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6Zapf.html
 */

struct Zapf
{
  static const hb_tag_t tableTag = HB_AAT_TAG_Zapf;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && lookup.sanitize (c, this));
  }

  protected:
  HBUINT16	version;	/* Set to 2 */
  HBUINT16	reserved;	/* 0 */
  Lookup<LOffsetTo<ZapfGlyphInfo> >
		lookup;		/* Offset from start of table to start of extra info space
				 * (added to groupOffset and featOffset in GlyphInfo) */
  public:
  DEFINE_SIZE_MIN (8);
};

} /* namespace AAT */


#endif /* HB_AAT_LCAR_TABLE_HH */
