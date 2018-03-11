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

#ifndef HB_OT_COLOR_SBIX_TABLE_HH
#define HB_OT_COLOR_SBIX_TABLE_HH

#include "hb-open-type-private.hh"

#define HB_OT_TAG_SBIX HB_TAG('s','b','i','x')

namespace OT {


struct SBIXGlyph
{
  HBINT16	xOffset;	/* The horizontal (x-axis) offset from the left
				 * edge of the graphic to the glyph’s origin.
				 * That is, the x-coordinate of the point on the
				 * baseline at the left edge of the glyph. */
  HBINT16	yOffset;	/* The vertical (y-axis) offset from the bottom
				 * edge of the graphic to the glyph’s origin.
				 * That is, the y-coordinate of the point on the
				 * baseline at the left edge of the glyph. */
  Tag		graphicType;	/* Indicates the format of the embedded graphic
				 * data: one of 'jpg ', 'png ' or 'tiff', or the
				 * special format 'dupe'. */
  HBUINT8	data[VAR];	/* The actual embedded graphic data. The total
				 * length is inferred from sequential entries in
				 * the glyphDataOffsets array and the fixed size
				 * (8 bytes) of the preceding fields. */
  public:
  DEFINE_SIZE_ARRAY (8, data);
};

struct SBIXStrike
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  c->check_array (imageOffsetsZ,
				  sizeof (HBUINT32),
				  1 + c->num_glyphs));
  }

  HBUINT16		ppem;		/* The PPEM size for which this strike was designed. */
  HBUINT16		resolution;	/* The device pixel density (in PPI) for which this
					 * strike was designed. (E.g., 96 PPI, 192 PPI.) */
  protected:
  LOffsetTo<SBIXGlyph>	imageOffsetsZ[VAR]; // VAR=maxp.numGlyphs + 1
					/* Offset from the beginning of the strike data header
					 * to bitmap data for an individual glyph ID. */
  public:
  DEFINE_SIZE_STATIC (8);
};

/*
 * sbix -- Standard Bitmap Graphics Table
 */
// It should be called with something like this so it can have
// access to num_glyph while sanitizing.
//
//   static inline const OT::sbix*
//   _get_sbix (hb_face_t *face)
//   {
//     OT::Sanitizer<OT::sbix> sanitizer;
//     sanitizer.set_num_glyphs (face->get_num_glyphs ());
//     hb_blob_t *sbix_blob = sanitizer.sanitize (face->reference_table (HB_OT_TAG_SBIX));
//     return OT::Sanitizer<OT::sbix>::lock_instance (sbix_blob);
//   }
//
struct sbix
{
  static const hb_tag_t tableTag = HB_OT_TAG_SBIX;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && strikes.sanitize (c, this));
  }

  // inline void dump (unsigned int num_glyphs, unsigned int group) const
  // {
  //   const SBIXStrike &strike = strikes[group](this);
  //   for (unsigned int i = 0; i < num_glyphs; ++i)
  //     if (strike.imageOffsetsZ[i + 1] - strike.imageOffsetsZ[i] > 0)
  //     {
  //       const SBIXGlyph &sbixGlyph = strike.imageOffsetsZ[i]((const void *) &strike);
  //       char outName[255];
  //       sprintf (outName, "out/%d-%d.png", group, i);
  //       FILE *f = fopen (outName, "wb");
  //       fwrite (sbixGlyph.data, 1,
  //         strike.imageOffsetsZ[i + 1] - strike.imageOffsetsZ[i] - 8, f);
  //       fclose (f);
  //     }
  // }

  protected:
  HBUINT16	version;	/* Table version number — set to 1 */
  HBUINT16	flags;		/* Bit 0: Set to 1. Bit 1: Draw outlines.
				 * Bits 2 to 15: reserved (set to 0). */
  ArrayOf<LOffsetTo<SBIXStrike>, HBUINT32>
		strikes;	/* Offsets from the beginning of the 'sbix'
				 * table to data for each individual bitmap strike. */
  public:
  DEFINE_SIZE_ARRAY (8, strikes);
};

} /* namespace OT */

#endif /* HB_OT_COLOR_SBIX_TABLE_HH */
