/*
 * Copyright © 2018  Ebrahim Byagowi
 * Copyright © 2018  Google, Inc.
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

#ifndef HB_AAT_LAYOUT_TRAK_TABLE_HH
#define HB_AAT_LAYOUT_TRAK_TABLE_HH

#include "hb-aat-layout-common-private.hh"
#include "hb-open-type-private.hh"

#define HB_AAT_TAG_trak HB_TAG('t','r','a','k')


namespace AAT {


struct TrackTableEntry
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  inline float get_value (const void *base, unsigned int index) const
  {
    return (base+values)[index];
  }

  protected:
  Fixed			track;		/* Track value for this record. */
  HBUINT16		trackNameID;	/* The 'name' table index for this track */
  OffsetTo<UnsizedArrayOf<HBINT16> >
			values;		/* Offset from start of tracking table to
					 * per-size tracking values for this track. */

  public:
  DEFINE_SIZE_STATIC (8);
};

struct TrackData
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (c->check_struct (this));
  }

  inline float get_tracking (const void *base, float ptem) const
  {
    /* CoreText points are CSS pixels (96 per inch),
     * NOT typographic points (72 per inch).
     *
     * https://developer.apple.com/library/content/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Explained/Explained.html
     */
    float csspx = ptem * 96.f / 72.f;
    Fixed fixed_size;
    fixed_size.set_float (csspx);

    // TODO: Make indexing work and use only an entry with zero track
    const TrackTableEntry trackTableEntry = trackTable[0];

    unsigned int size_index;
    for (size_index = 0; size_index < nSizes; ++size_index)
      if ((base+sizeTable)[size_index] >= fixed_size)
        break;

    // We don't attempt to extrapolate to larger or smaller values
    if (size_index == nSizes)
      return trackTableEntry.get_value (base, nSizes - 1);
    if (size_index == 0 || (base+sizeTable)[size_index] == fixed_size)
      return trackTableEntry.get_value (base, size_index);

    float s0 = (base+sizeTable)[size_index - 1].to_float ();
    float s1 = (base+sizeTable)[size_index].to_float ();
    float t = (csspx - s0) / (s1 - s0);
    return t * trackTableEntry.get_value (base, size_index) +
      (1.0 - t) * trackTableEntry.get_value (base, size_index - 1);
  }

  protected:
  HBUINT16		nTracks;	/* Number of separate tracks included in this table. */
  HBUINT16		nSizes;		/* Number of point sizes included in this table. */
  LOffsetTo<UnsizedArrayOf<Fixed> >
			sizeTable;
  UnsizedArrayOf<TrackTableEntry>	trackTable;/* Array[nSizes] of size values. */

  public:
  DEFINE_SIZE_ARRAY (8, trackTable);
};

struct trak
{
  static const hb_tag_t tableTag = HB_AAT_TAG_trak;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    /* TODO */
    return_trace (c->check_struct (this));
  }

  inline bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    const float ptem = c->font->ptem;
    if (ptem > 0.f)
    {
      hb_buffer_t *buffer = c->buffer;
      if (HB_DIRECTION_IS_HORIZONTAL (c->buffer->props.direction))
      {
        const TrackData trackData = this+horizOffset;
        float tracking = trackData.get_tracking (this, ptem);
        hb_position_t advance_to_add = c->font->em_scalef_x (tracking / 2);
        foreach_grapheme (buffer, start, end)
        {
          buffer->pos[start].x_advance += advance_to_add;
          buffer->pos[end].x_advance += advance_to_add;
        }
      }
      else
      {
        const TrackData trackData = this+vertOffset;
        float tracking = trackData.get_tracking (this, ptem);
        hb_position_t advance_to_add = c->font->em_scalef_y (tracking / 2);
        foreach_grapheme (buffer, start, end)
        {
          buffer->pos[start].y_advance += advance_to_add;
          buffer->pos[end].y_advance += advance_to_add;
        }
      }
    }
    return_trace (false);
  }

  protected:
  FixedVersion<>	version;	/* Version of the tracking table--currently
					 * 0x00010000u for version 1.0. */
  HBUINT16		format; 	/* Format of the tracking table */
  OffsetTo<TrackData>	horizOffset;	/* TrackData for horizontal text */
  OffsetTo<TrackData>	vertOffset;	/* TrackData for vertical text */
  HBUINT16		reserved;	/* Reserved. Set to 0. */

  public:
  DEFINE_SIZE_MIN (12);
};

} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_TRAK_TABLE_HH */
