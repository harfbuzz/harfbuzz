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

#include "hb-aat-layout-common.hh"
#include "hb-ot-layout.hh"
#include "hb-open-type.hh"

/*
 * trak -- Tracking
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6trak.html
 */
#define HB_AAT_TAG_trak HB_TAG('t','r','a','k')


namespace AAT {


struct TrackTableEntry
{
  friend struct TrackData;

  float get_track_value () const { return track.to_float (); }

  float interpolate_at (unsigned int idx,
			float ptem,
			const void *base,
			hb_array_t<const F16DOT16> size_table) const
  {
    const FWORD *values = (base+valuesZ).arrayZ;

    float s0 = size_table[idx].to_float ();
    float s1 = size_table[idx + 1].to_float ();
    int v0 = values[idx];
    int v1 = values[idx + 1];

    // Deal with font bugs.
    if (unlikely (s1 < s0))
    { hb_swap (s0, s1); hb_swap (v0, v1); }
    if (unlikely (ptem < s0)) return v0;
    if (unlikely (ptem > s1)) return v1;
    if (unlikely (s0 == s1)) return (v0 + v1) * 0.5f;

    float t = (ptem - s0) / (s1 - s0);
    return v0 + t * (v1 - v0);
  }

  float get_value (float ptem,
		   const void *base,
		   hb_array_t<const F16DOT16> size_table) const
  {
    const FWORD *values = (base+valuesZ).arrayZ;

    unsigned int n_sizes = size_table.length;

    /*
     * Choose size.
     */
    if (!n_sizes) return 0.f;
    if (n_sizes == 1) return values[0];

    // At least two entries.

    unsigned i;
    for (i = 0; i < n_sizes; i++)
      if (size_table[i].to_float () >= ptem)
	break;

    // Boundary conditions.
    if (i == 0)       return values[0];
    if (i == n_sizes) return values[n_sizes - 1];

    // Exact match.
    if (size_table[i].to_float () == ptem) return values[i];

    // Interpolate.
    return interpolate_at (i - 1, ptem, base, size_table);
  }

  public:
  bool sanitize (hb_sanitize_context_t *c,
		 const void *base,
		 unsigned int n_sizes) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  (valuesZ.sanitize (c, base, n_sizes))));
  }

  protected:
  F16DOT16	track;		/* Track value for this record. */
  NameID	trackNameID;	/* The 'name' table index for this track.
				 * (a short word or phrase like "loose"
				 * or "very tight") */
  NNOffset16To<UnsizedArrayOf<FWORD>>
		valuesZ;	/* Offset from start of tracking table to
				 * per-size tracking values for this track. */

  public:
  DEFINE_SIZE_STATIC (8);
};

struct TrackData
{
  float get_tracking (const void *base, float ptem) const
  {
    /*
     * Choose track.
     */
    const TrackTableEntry *trackTableEntry = nullptr;
    unsigned int count = nTracks;
    float last_trak = 1e5;
    for (unsigned int i = 0; i < count; i++)
    {
      /* Note: Seems like the track entries are sorted by values.  But the
       * spec doesn't explicitly say that.  It just mentions it in the example. */

      /* Not sure what CoreText does, but it looks to apply a trak=1.0 by default
       * if there is no 0.0 trak. So, just pick the one closest to 0.0. */

      float trak = trackTable[i].get_track_value ();
      if (fabsf (trak) < fabsf (last_trak))
      {
	trackTableEntry = &trackTable[i];
	break;
      }
    }
    if (!trackTableEntry) return 0;

    return trackTableEntry->get_value (ptem,
				       base,
				       (base+sizeTable).as_array (nSizes));
  }

  bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (c->check_struct (this) &&
			  hb_barrier () &&
			  sizeTable.sanitize (c, base, nSizes) &&
			  trackTable.sanitize (c, nTracks, base, nSizes)));
  }

  protected:
  HBUINT16	nTracks;	/* Number of separate tracks included in this table. */
  HBUINT16	nSizes;		/* Number of point sizes included in this table. */
  NNOffset32To<UnsizedArrayOf<F16DOT16>>
		sizeTable;	/* Offset from start of the tracking table to
				 * Array[nSizes] of size values.. */
  UnsizedArrayOf<TrackTableEntry>
		trackTable;	/* Array[nTracks] of TrackTableEntry records. */

  public:
  DEFINE_SIZE_ARRAY (8, trackTable);
};

struct trak
{
  static constexpr hb_tag_t tableTag = HB_AAT_TAG_trak;

  bool has_data () const { return version.to_int (); }

  bool apply (hb_aat_apply_context_t *c) const
  {
    TRACE_APPLY (this);

    hb_mask_t trak_mask = c->plan->trak_mask;

    float ptem = c->font->ptem;
    if (unlikely (ptem <= 0.f))
    {
      /* https://developer.apple.com/documentation/coretext/1508745-ctfontcreatewithgraphicsfont */
      ptem = HB_CORETEXT_DEFAULT_FONT_SIZE;
    }

    hb_buffer_t *buffer = c->buffer;
    if (HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction))
    {
      const TrackData &trackData = this+horizData;
      float tracking = trackData.get_tracking (this, ptem);
      hb_position_t offset_to_add = c->font->em_scalef_x (tracking / 2);
      hb_position_t advance_to_add = c->font->em_scalef_x (tracking);
      foreach_grapheme (buffer, start, end)
      {
	if (!(buffer->info[start].mask & trak_mask)) continue;
	buffer->pos[start].x_advance += advance_to_add;
	buffer->pos[start].x_offset += offset_to_add;
      }
    }
    else
    {
      const TrackData &trackData = this+vertData;
      float tracking = trackData.get_tracking (this, ptem);
      hb_position_t offset_to_add = c->font->em_scalef_y (tracking / 2);
      hb_position_t advance_to_add = c->font->em_scalef_y (tracking);
      foreach_grapheme (buffer, start, end)
      {
	if (!(buffer->info[start].mask & trak_mask)) continue;
	buffer->pos[start].y_advance += advance_to_add;
	buffer->pos[start].y_offset += offset_to_add;
      }
    }

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    return_trace (likely (c->check_struct (this) &&
			  hb_barrier () &&
			  version.major == 1 &&
			  horizData.sanitize (c, this, this) &&
			  vertData.sanitize (c, this, this)));
  }

  protected:
  FixedVersion<>version;	/* Version of the tracking table
				 * (0x00010000u for version 1.0). */
  HBUINT16	format;		/* Format of the tracking table (set to 0). */
  Offset16To<TrackData>
		horizData;	/* Offset from start of tracking table to TrackData
				 * for horizontal text (or 0 if none). */
  Offset16To<TrackData>
		vertData;	/* Offset from start of tracking table to TrackData
				 * for vertical text (or 0 if none). */
  HBUINT16	reserved;	/* Reserved. Set to 0. */

  public:
  DEFINE_SIZE_STATIC (12);
};

} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_TRAK_TABLE_HH */
