/*
 * Copyright © 2016  Google, Inc.
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
 *
 * Google Author(s): Sascha Brawer
 */

#ifndef HB_OT_COLOR_CPAL_TABLE_HH
#define HB_OT_COLOR_CPAL_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-color.h"
#include "hb-ot-name.h"


/*
 * CPAL -- Color Palette
 * https://docs.microsoft.com/en-us/typography/opentype/spec/cpal
 */
#define HB_OT_TAG_CPAL HB_TAG('C','P','A','L')


namespace OT {


struct CPALV1Tail
{
  friend struct CPAL;

  inline bool
  sanitize (hb_sanitize_context_t *c, const void *base,
	    unsigned int palettes, unsigned int paletteEntries) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  (!paletteFlagsZ || (base+paletteFlagsZ).sanitize (c, palettes)) &&
		  (!paletteLabelZ || (base+paletteLabelZ).sanitize (c, palettes)) &&
		  (!paletteEntryLabelZ || (base+paletteEntryLabelZ).sanitize (c, paletteEntries)));
  }

  private:
  inline hb_ot_color_palette_flags_t
  get_palette_flags (const void *base, unsigned int palette,
		     unsigned int palettes_count) const
  {
    if (unlikely (palette >= palettes_count || !paletteFlagsZ))
      return HB_OT_COLOR_PALETTE_FLAG_DEFAULT;

    return (hb_ot_color_palette_flags_t) (uint32_t) (base+paletteFlagsZ)[palette];
  }

  inline unsigned int
  get_palette_name_id (const void *base, unsigned int palette,
		       unsigned int palettes_count) const
  {
    if (unlikely (palette >= palettes_count || !paletteLabelZ))
      return HB_NAME_ID_INVALID;

    return (base+paletteLabelZ)[palette];
  }

  inline unsigned int
  get_palette_entry_name_id (const void *base, unsigned int palette_entry,
			     unsigned int palettes_entries_count) const
  {
    if (unlikely (palette_entry >= palettes_entries_count || !paletteEntryLabelZ))
      return HB_NAME_ID_INVALID;

    return (base+paletteEntryLabelZ)[palette_entry];
  }

  protected:
  LOffsetTo<UnsizedArrayOf<HBUINT32>, false>
		paletteFlagsZ;		/* Offset from the beginning of CPAL table to
					 * the Palette Type Array. Set to 0 if no array
					 * is provided. */
  LOffsetTo<UnsizedArrayOf<HBUINT16>, false>
		paletteLabelZ;		/* Offset from the beginning of CPAL table to
					 * the Palette Labels Array. Set to 0 if no
					 * array is provided. */
  LOffsetTo<UnsizedArrayOf<HBUINT16>, false>
		paletteEntryLabelZ;	/* Offset from the beginning of CPAL table to
					 * the Palette Entry Label Array. Set to 0
					 * if no array is provided. */
  public:
  DEFINE_SIZE_STATIC (12);
};

typedef HBUINT32 BGRAColor;

struct CPAL
{
  static const hb_tag_t tableTag = HB_OT_TAG_CPAL;

  inline bool has_data (void) const { return numPalettes; }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!(c->check_struct (this) &&  /* it checks colorRecordIndices also
					        * See #get_size */
		    (this+colorRecordsZ).sanitize (c, numColorRecords))))
      return_trace (false);

    /* Check for indices sanity so no need for doing it runtime */
    for (unsigned int i = 0; i < numPalettes; ++i)
      if (unlikely (colorRecordIndicesZ[i] + numPaletteEntries > numColorRecords))
	return_trace (false);

    /* If version is zero, we are done here; otherwise we need to check tail also */
    if (version == 0)
      return_trace (true);

    const CPALV1Tail &v1 = StructAfter<CPALV1Tail> (*this);
    return_trace (likely (v1.sanitize (c, this, numPalettes, numPaletteEntries)));
  }

  inline unsigned int get_size (void) const
  {
    return min_size + numPalettes * sizeof (HBUINT16);
  }

  inline hb_ot_color_palette_flags_t get_palette_flags (unsigned int palette) const
  {
    if (unlikely (version == 0))
      return HB_OT_COLOR_PALETTE_FLAG_DEFAULT;

    const CPALV1Tail& cpal1 = StructAfter<CPALV1Tail> (*this);
    return cpal1.get_palette_flags (this, palette, numPalettes);
  }

  inline unsigned int get_palette_name_id (unsigned int palette) const
  {
    if (unlikely (version == 0))
      return HB_NAME_ID_INVALID;

    const CPALV1Tail& cpal1 = StructAfter<CPALV1Tail> (*this);
    return cpal1.get_palette_name_id (this, palette, numPalettes);
  }

  inline unsigned int get_palette_entry_name_id (unsigned int palette_entry) const
  {
    if (unlikely (version == 0))
      return HB_NAME_ID_INVALID;

    const CPALV1Tail& cpal1 = StructAfter<CPALV1Tail> (*this);
    return cpal1.get_palette_entry_name_id (this, palette_entry, numPaletteEntries);
  }

  inline unsigned int get_palette_count () const
  { return numPalettes; }

  inline unsigned int get_palette_entries_count () const
  { return numPaletteEntries; }

  bool
  get_color_record_argb (unsigned int color_index, unsigned int palette, hb_color_t* color) const
  {
    if (unlikely (color_index >= numPaletteEntries || palette >= numPalettes))
      return false;

    /* No need for more range check as it is already done on #sanitize */
    const UnsizedArrayOf<BGRAColor>& color_records = this+colorRecordsZ;
    if (color)
      *color = color_records[colorRecordIndicesZ[palette] + color_index];
    return true;
  }

  protected:
  HBUINT16	version;		/* Table version number */
  /* Version 0 */
  HBUINT16	numPaletteEntries;	/* Number of palette entries in each palette. */
  HBUINT16	numPalettes;		/* Number of palettes in the table. */
  HBUINT16	numColorRecords;	/* Total number of color records, combined for
					 * all palettes. */
  LOffsetTo<UnsizedArrayOf<BGRAColor>, false>
		colorRecordsZ;		/* Offset from the beginning of CPAL table to
					 * the first ColorRecord. */
  UnsizedArrayOf<HBUINT16>
		colorRecordIndicesZ;	/* Index of each palette’s first color record in
					 * the combined color record array. */
/*CPALV1Tail	v1;*/
  public:
  DEFINE_SIZE_ARRAY (12, colorRecordIndicesZ);
};

} /* namespace OT */


#endif /* HB_OT_COLOR_CPAL_TABLE_HH */
