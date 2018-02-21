/*
 * Copyright © 2018  Google, Inc.
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

  protected:
  Fixed			track;		/* Track value for this record. */
  HBUINT16		trackNameID;	/* The 'name' table index for this track */
  OffsetTo<UnsizedArrayOf<Fixed> >
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
    return_trace (c->check_struct (this));
  }

  protected:
  HBUINT16		nTracks;	/* Number of separate tracks included in this table. */
  HBUINT16		nSizes;		/* Number of point sizes included in this table. */
  LOffsetTo<UnsizedArrayOf<Fixed> >
			sizeTable;
  TrackTableEntry	trackTable[VAR];/* Array[nSizes] of size values. */

  public:
  DEFINE_SIZE_ARRAY (8, trackTable);
};

struct trak
{
  static const hb_tag_t tableTag = HB_AAT_TAG_trak;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
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
