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

#ifndef HB_OT_CPAL_TABLE_HH
#define HB_OT_CPAL_TABLE_HH

#include "hb-open-type-private.hh"

/*
 * Color Palette
 * http://www.microsoft.com/typography/otspec/cpal.htm
 */

#define HB_OT_TAG_CPAL HB_TAG('C','P','A','L')

namespace OT {


struct ColorRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (true);
  }

  HBUINT8 blue;
  HBUINT8 green;
  HBUINT8 red;
  HBUINT8 alpha;
  public:
  DEFINE_SIZE_STATIC (4);
};

struct CPALV1Tail
{
  friend struct CPAL;

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int palettes) const
  {
    TRACE_SANITIZE (this);
    return_trace (
      c->check_struct (this) &&
      c->check_array ((const void*) &paletteFlags, sizeof (HBUINT32), palettes) &&
      c->check_array ((const void*) &paletteLabel, sizeof (HBUINT16), palettes) &&
      c->check_array ((const void*) &paletteEntryLabel, sizeof (HBUINT16), palettes));
  }

  private:
  inline hb_ot_color_palette_flags_t
  get_palette_flags (const void *base, unsigned int palette) const
  {
    const HBUINT32* flags = (const HBUINT32*) (const void*) &paletteFlags (base);
    return (hb_ot_color_palette_flags_t) (uint32_t) flags[palette];
  }

  inline unsigned int
  get_palette_name_id (const void *base, unsigned int palette) const
  {
    const HBUINT16* name_ids = (const HBUINT16*) (const void*) &paletteLabel (base);
    return name_ids[palette];
  }

  protected:
  LOffsetTo<HBUINT32> paletteFlags;
  LOffsetTo<HBUINT16> paletteLabel;
  LOffsetTo<HBUINT16> paletteEntryLabel;

  public:
  DEFINE_SIZE_STATIC (12);
};

struct CPAL
{
  static const hb_tag_t tableTag = HB_OT_TAG_CPAL;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!(c->check_struct (this) &&
		    colorRecords.sanitize (c)))
      return_trace (false);

    unsigned int palettes = numPalettes;
    if (!c->check_array (colorRecordIndices, sizeof (HBUINT16), palettes))
      return_trace (false);

    for (unsigned int i = 0; i < palettes; ++i)
      if (colorRecordIndices[i] + numPaletteEntries > colorRecords.get_size ())
        return_trace (false);

    if (version > 1)
    {
      const CPALV1Tail &v1 = StructAfter<CPALV1Tail> (*this);
      return_trace (v1.sanitize (c, palettes));
    }
    else
      return_trace (true);
  }

  inline unsigned int get_size (void) const
  {
    return min_size + numPalettes * 2;
  }

  inline hb_ot_color_palette_flags_t get_palette_flags (unsigned int palette) const
  {
    if (version == 0 || palette >= numPalettes)
      return HB_OT_COLOR_PALETTE_FLAG_DEFAULT;

    const CPALV1Tail& cpal1 = StructAfter<CPALV1Tail> (*this);
    return cpal1.get_palette_flags (this, palette);
  }

  inline unsigned int get_palette_name_id (unsigned int palette) const
  {
    if (version == 0 || palette >= numPalettes)
      return 0xFFFF;

    const CPALV1Tail& cpal1 = StructAfter<CPALV1Tail> (*this);
    return cpal1.get_palette_name_id (this, palette);
  }

  inline unsigned int get_palette_count () const
  {
    return numPalettes;
  }

  protected:
  HBUINT16	version;

  /* Version 0 */
  HBUINT16	numPaletteEntries;
  HBUINT16	numPalettes;
  ArrayOf<ColorRecord>	colorRecords;
  HBUINT16	colorRecordIndices[VAR];  // VAR=numPalettes

  public:
  DEFINE_SIZE_ARRAY (12, colorRecordIndices);
};

} /* namespace OT */


#endif /* HB_OT_CPAL_TABLE_HH */
