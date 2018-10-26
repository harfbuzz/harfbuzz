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

#include "hb-open-type.hh"

/*
 * sbix -- Standard Bitmap Graphics
 * https://docs.microsoft.com/en-us/typography/opentype/spec/sbix
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6sbix.html
 */
#define HB_OT_TAG_sbix HB_TAG('s','b','i','x')


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
  UnsizedArrayOf<HBUINT8>
		data;		/* The actual embedded graphic data. The total
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
		  imageOffsetsZ.sanitize_shallow (c, c->get_num_glyphs () + 1));
  }

  inline unsigned int get_ppem () const
  { return ppem; }

  inline unsigned int get_resolution () const
  { return resolution; }

  inline unsigned int blob_size (unsigned int glyph_id) const
  {
    return imageOffsetsZ[glyph_id + 1] - imageOffsetsZ[glyph_id] - SBIXGlyph::min_size;
  }

  inline hb_blob_t *get_glyph_blob (unsigned int  glyph_id,
				    hb_blob_t    *sbix_blob,
				    unsigned int  strike_offset,
				    unsigned int *x_offset,
				    unsigned int *y_offset,
				    hb_tag_t      requested_file_type,
				    unsigned int  num_glyphs) const
  {
    if (imageOffsetsZ[glyph_id + 1] - imageOffsetsZ[glyph_id] == 0)
      return hb_blob_get_empty ();

    const SBIXGlyph *glyph = &(this+imageOffsetsZ[glyph_id]);
    if (unlikely (glyph->graphicType == HB_TAG ('d','u','p','e') &&
		  blob_size (glyph_id) >= 2))
    {
      unsigned int new_glyph_id = *((HBUINT16 *) &glyph->data);
      if (new_glyph_id < num_glyphs)
      {
	glyph = &(this+imageOffsetsZ[new_glyph_id]);
	glyph_id = new_glyph_id;
      }
    }
    if (unlikely (requested_file_type != glyph->graphicType))
      return hb_blob_get_empty ();
    if (likely (x_offset)) *x_offset = glyph->xOffset;
    if (likely (y_offset)) *y_offset = glyph->yOffset;
    unsigned int offset = strike_offset + SBIXGlyph::min_size;
    offset += imageOffsetsZ[glyph_id];
    return hb_blob_create_sub_blob (sbix_blob, offset, blob_size (glyph_id));
  }

  protected:
  HBUINT16	ppem;		/* The PPEM size for which this strike was designed. */
  HBUINT16	resolution;	/* The device pixel density (in PPI) for which this
				 * strike was designed. (E.g., 96 PPI, 192 PPI.) */
  UnsizedArrayOf<LOffsetTo<SBIXGlyph> >
		imageOffsetsZ;	/* Offset from the beginning of the strike data header
				 * to bitmap data for an individual glyph ID. */
  public:
  DEFINE_SIZE_STATIC (8);
};

struct sbix
{
  static const hb_tag_t tableTag = HB_OT_TAG_sbix;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) && strikes.sanitize (c, this)));
  }

  struct accelerator_t
  {
    inline void init (hb_face_t *face)
    {
      sbix_blob = hb_sanitize_context_t().reference_table<sbix> (face);
      sbix_len = hb_blob_get_length (sbix_blob);
      sbix_table = sbix_blob->as<sbix> ();
      num_glyphs = face->get_num_glyphs ();
    }

    inline void fini (void)
    {
      hb_blob_destroy (sbix_blob);
    }

    inline void dump (void (*callback) (hb_blob_t *data,
					unsigned int group, unsigned int gid)) const
    {
      for (unsigned group = 0; group < sbix_table->strikes.len; group++)
      {
	const SBIXStrike &strike = sbix_table+sbix_table->strikes[group];
	for (unsigned int glyph_id = 0; glyph_id < num_glyphs; glyph_id++)
	{
	  unsigned int x_offset, y_offset;
	  hb_tag_t tag;
	  hb_blob_t *blob;
	  blob = strike.get_glyph_blob (glyph_id, sbix_blob, sbix_table->strikes[group],
					&x_offset, &x_offset,
					HB_TAG('p','n','g',' '), num_glyphs);
	  if (hb_blob_get_length (blob)) callback (blob, group, glyph_id);
	}
      }
    }

    inline hb_blob_t* reference_blob_for_glyph (hb_codepoint_t  glyph_id,
						unsigned int    ptem HB_UNUSED,
						unsigned int    requested_ppem,
						unsigned int    requested_file_type,
						unsigned int   *available_x_ppem,
						unsigned int   *available_y_ppem) const
    {
      if (unlikely (sbix_len == 0 || sbix_table->strikes.len == 0))
        return hb_blob_get_empty ();

      /* TODO: Does spec guarantee strikes are ascended sorted? */
      unsigned int group = sbix_table->strikes.len - 1;
      if (requested_ppem != 0)
	/* TODO: Use bsearch maybe or doesn't worth it? */
        for (group = 0; group < sbix_table->strikes.len; group++)
	  if ((sbix_table+sbix_table->strikes[group]).get_ppem () >= requested_ppem)
	    break;

      const SBIXStrike &strike = sbix_table+sbix_table->strikes[group];
      if (available_x_ppem) *available_x_ppem = strike.get_ppem ();
      if (available_y_ppem) *available_y_ppem = strike.get_ppem ();
      return strike.get_glyph_blob (glyph_id, sbix_blob, sbix_table->strikes[group],
				    nullptr, nullptr, requested_file_type, num_glyphs);
    }

    inline bool has_data () const
    { return sbix_len; }

    private:
    hb_blob_t *sbix_blob;
    const sbix *sbix_table;

    unsigned int sbix_len;
    unsigned int num_glyphs;
    hb_vector_t<hb_vector_t<unsigned int> > data_offsets;
  };

  protected:
  HBUINT16	version;	/* Table version number — set to 1 */
  HBUINT16	flags;		/* Bit 0: Set to 1. Bit 1: Draw outlines.
				 * Bits 2 to 15: reserved (set to 0). */
  LArrayOf<LOffsetTo<SBIXStrike> >
		strikes;	/* Offsets from the beginning of the 'sbix'
				 * table to data for each individual bitmap strike. */
  public:
  DEFINE_SIZE_ARRAY (8, strikes);
};

struct sbix_accelerator_t : sbix::accelerator_t {};

} /* namespace OT */

#endif /* HB_OT_COLOR_SBIX_TABLE_HH */
