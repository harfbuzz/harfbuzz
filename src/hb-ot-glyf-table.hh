/*
 * Copyright © 2015  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod, Garret Reiger, Roderick Sheeter
 */

#ifndef HB_OT_GLYF_TABLE_HH
#define HB_OT_GLYF_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-head-table.hh"

namespace OT {


/*
 * loca -- Index to Location
 * https://docs.microsoft.com/en-us/typography/opentype/spec/loca
 */
#define HB_OT_TAG_loca HB_TAG('l','o','c','a')


struct loca
{
  friend struct glyf;

  static constexpr hb_tag_t tableTag = HB_OT_TAG_loca;

  bool sanitize (hb_sanitize_context_t *c HB_UNUSED) const
  {
    TRACE_SANITIZE (this);
    return_trace (true);
  }

  protected:
  UnsizedArrayOf<HBUINT8>	dataZ;		/* Location data. */
  public:
  DEFINE_SIZE_MIN (0); /* In reality, this is UNBOUNDED() type; but since we always
			* check the size externally, allow Null() object of it by
			* defining it _MIN instead. */
};


/*
 * glyf -- TrueType Glyph Data
 * https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
 */
#define HB_OT_TAG_glyf HB_TAG('g','l','y','f')


struct glyf
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_glyf;

  bool sanitize (hb_sanitize_context_t *c HB_UNUSED) const
  {
    TRACE_SANITIZE (this);
    /* We don't check for anything specific here.  The users of the
     * struct do all the hard work... */
    return_trace (true);
  }

  template<typename Iterator, typename EntryType>
  static void
  _write_loca (Iterator it, unsigned size_denom, char * dest)
  {
    // write loca[0] through loca[numGlyphs-1]
    EntryType * loca_start = (EntryType *) dest;
    EntryType * loca_current = loca_start;
    unsigned int offset = 0;
    + it
    | hb_apply ([&] (unsigned int padded_size) {
      DEBUG_MSG(SUBSET, nullptr, "loca entry %ld offset %d", loca_current - loca_start, offset);
      *loca_current = offset / size_denom;
      offset += padded_size;
      loca_current++;
    });
    // one bonus element so loca[numGlyphs] - loca[numGlyphs -1] is size of last glyph
    DEBUG_MSG(SUBSET, nullptr, "loca entry %ld offset %d", loca_current - loca_start, offset);
    *loca_current = offset / size_denom;
  }

  // TODO don't pass in plan
  template <typename Iterator>
  bool serialize(hb_serialize_context_t *c,
		 Iterator it,
		 const hb_subset_plan_t *plan)
  {
    TRACE_SERIALIZE (this);

    HBUINT8 pad;
    pad = 0;
    + it
    | hb_apply ( [&] (hb_pair_t<hb_bytes_t, unsigned int> _) {
      const hb_bytes_t& src_glyph = _.first;
      unsigned int padded_size = _.second;
      hb_bytes_t dest_glyph = src_glyph.copy(c);
      unsigned int padding = padded_size - src_glyph.length;
      DEBUG_MSG(SUBSET, nullptr, "serialize %d byte glyph, width %d pad %d", src_glyph.length, padded_size, padding);
      while (padding > 0)
      {
        c->embed(pad);
        padding--;
      }

      _fix_component_gids (plan, dest_glyph);
      if (plan->drop_hints)
      {
        // we copied the glyph w/o instructions, just need to zero instruction length
        _zero_instruction_length (dest_glyph);
      }
    });

    // Things old impl did we now don't:
    // TODO _remove_composite_instruction_flag

    return_trace (true);
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);

    glyf *glyf_prime = c->serializer->start_embed <glyf> ();
    if (unlikely (!glyf_prime)) return_trace (false);

    OT::glyf::accelerator_t glyf;
    glyf.init (c->plan->source);

    // make an iterator of per-glyph hb_bytes_t.
    // unpadded, hints removed if that was requested.

    // TODO log shows we redo a bunch of the work here; should sink this at end?

    auto glyphs =
    + hb_range (c->plan->num_output_glyphs ())
    | hb_map ([&] (hb_codepoint_t new_gid) {
      hb_codepoint_t old_gid;

      // should never fail, ALL old gids should be mapped
      if (!c->plan->old_gid_for_new_gid (new_gid, &old_gid)) return hb_bytes_t ();

      unsigned int start_offset, end_offset;
      if (unlikely (!(glyf.get_offsets (old_gid, &start_offset, &end_offset) &&
	glyf.remove_padding (start_offset, &end_offset))))
      {
	// TODO signal fatal error
	DEBUG_MSG(SUBSET, nullptr, "Unable to get offset or remove padding for new_gid %d", new_gid);
	return hb_bytes_t ();
      }
      hb_bytes_t glyph (((const char *) this) + start_offset, end_offset - start_offset);

      // if dropping hints, find hints region and chop it off the end
      if (c->plan->drop_hints) {
	unsigned int instruction_length = 0;
	if (!glyf.get_instruction_length (glyph, &instruction_length))
	{
	  // TODO signal fatal error
	  DEBUG_MSG(SUBSET, nullptr, "Unable to read instruction length for new_gid %d", new_gid);
	  return hb_bytes_t ();
	}
	DEBUG_MSG(SUBSET, nullptr, "new_gid %d drop %d instruction bytes from %d byte glyph", new_gid, instruction_length, glyph.length);
	glyph = hb_bytes_t (&glyph, glyph.length - instruction_length);
      }

      return glyph;
    });

    auto padded_offsets =
    + glyphs
    | hb_map ([&] (hb_bytes_t _) { return _.length + _.length % 2; });

    glyf_prime->serialize (c->serializer, hb_zip (glyphs, padded_offsets), c->plan);

    // TODO whats the right way to serialize loca?
    // _subset2 will think these bytes are part of glyf if we write to serializer
    unsigned int max_offset = + padded_offsets | hb_reduce (hb_max, 0);
    bool use_short_loca = max_offset <= 131070;
    unsigned int loca_prime_size = (c->plan->num_output_glyphs () + 1) * (use_short_loca ? 2 : 4);
    char *loca_prime_data = (char *) calloc(1, loca_prime_size);

    if (use_short_loca)
      _write_loca <decltype (padded_offsets), HBUINT16> (padded_offsets, 2, loca_prime_data);
    else
      _write_loca <decltype (padded_offsets), HBUINT32> (padded_offsets, 1, loca_prime_data);

    hb_blob_t * loca_blob = hb_blob_create (loca_prime_data,
                                            loca_prime_size,
                                            HB_MEMORY_MODE_READONLY,
                                            loca_prime_data,
                                            free);
    if (unlikely (! (c->plan->add_table (HB_OT_TAG_loca, loca_blob)
                  && _add_head_and_set_loca_version(c->plan, use_short_loca))))
    {
      // TODO signal fatal error
      hb_blob_destroy (loca_blob);
      return false;
    }

    hb_blob_destroy (loca_blob);
    return_trace (true);
  }

  static void
  _fix_component_gids (const hb_subset_plan_t *plan,
		       hb_bytes_t glyph)
  {
    OT::glyf::CompositeGlyphHeader::Iterator iterator;
    if (OT::glyf::CompositeGlyphHeader::get_iterator (&glyph,
						      glyph.length,
						      &iterator))
    {
      do
      {
        hb_codepoint_t new_gid;
        if (!plan->new_gid_for_old_gid (iterator.current->glyphIndex,
				        &new_gid))
	  continue;
        ((OT::glyf::CompositeGlyphHeader *) iterator.current)->glyphIndex = new_gid;
      } while (iterator.move_to_next ());
    }
  }

  static void
  _zero_instruction_length (hb_bytes_t glyph)
  {
    const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (&glyph, 0);
    int16_t num_contours = (int16_t) glyph_header.numberOfContours;
    if (num_contours > 0)
    {
      const HBUINT16 &instruction_length = StructAtOffset<HBUINT16> (&glyph, GlyphHeader::static_size + 2 * num_contours);
      (HBUINT16 &) instruction_length = 0;
    }
  }

  static bool
  _add_head_and_set_loca_version (hb_subset_plan_t *plan, bool use_short_loca)
  {
    hb_blob_t *head_blob = hb_sanitize_context_t ().reference_table<head> (plan->source);
    hb_blob_t *head_prime_blob = hb_blob_copy_writable_or_fail (head_blob);
    hb_blob_destroy (head_blob);

    if (unlikely (!head_prime_blob))
      return false;

    head *head_prime = (head *) hb_blob_get_data_writable (head_prime_blob, nullptr);
    head_prime->indexToLocFormat = use_short_loca ? 0 : 1;
    bool success = plan->add_table (HB_OT_TAG_head, head_prime_blob);

    hb_blob_destroy (head_prime_blob);
    return success;
  }

  struct GlyphHeader
  {
    HBINT16		numberOfContours;	/* If the number of contours is
						 * greater than or equal to zero,
						 * this is a simple glyph; if negative,
						 * this is a composite glyph. */
    FWORD		xMin;			/* Minimum x for coordinate data. */
    FWORD		yMin;			/* Minimum y for coordinate data. */
    FWORD		xMax;			/* Maximum x for coordinate data. */
    FWORD		yMax;			/* Maximum y for coordinate data. */

    DEFINE_SIZE_STATIC (10);
  };

  struct CompositeGlyphHeader
  {
    enum composite_glyph_flag_t {
      ARG_1_AND_2_ARE_WORDS =      0x0001,
      ARGS_ARE_XY_VALUES =         0x0002,
      ROUND_XY_TO_GRID =           0x0004,
      WE_HAVE_A_SCALE =            0x0008,
      MORE_COMPONENTS =            0x0020,
      WE_HAVE_AN_X_AND_Y_SCALE =   0x0040,
      WE_HAVE_A_TWO_BY_TWO =       0x0080,
      WE_HAVE_INSTRUCTIONS =       0x0100,
      USE_MY_METRICS =             0x0200,
      OVERLAP_COMPOUND =           0x0400,
      SCALED_COMPONENT_OFFSET =    0x0800,
      UNSCALED_COMPONENT_OFFSET =  0x1000
    };

    HBUINT16 flags;
    GlyphID  glyphIndex;

    unsigned int get_size () const
    {
      unsigned int size = min_size;
      // arg1 and 2 are int16
      if (flags & ARG_1_AND_2_ARE_WORDS) size += 4;
      // arg1 and 2 are int8
      else size += 2;

      // One x 16 bit (scale)
      if (flags & WE_HAVE_A_SCALE) size += 2;
      // Two x 16 bit (xscale, yscale)
      else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) size += 4;
      // Four x 16 bit (xscale, scale01, scale10, yscale)
      else if (flags & WE_HAVE_A_TWO_BY_TWO) size += 8;

      return size;
    }

    struct Iterator
    {
      const char *glyph_start;
      const char *glyph_end;
      const CompositeGlyphHeader *current;

      bool move_to_next ()
      {
	if (current->flags & CompositeGlyphHeader::MORE_COMPONENTS)
	{
	  const CompositeGlyphHeader *possible =
	    &StructAfter<CompositeGlyphHeader, CompositeGlyphHeader> (*current);
	  if (!in_range (possible))
	    return false;
	  current = possible;
	  return true;
	}
	return false;
      }

      bool in_range (const CompositeGlyphHeader *composite) const
      {
	return (const char *) composite >= glyph_start
	  && ((const char *) composite + CompositeGlyphHeader::min_size) <= glyph_end
	  && ((const char *) composite + composite->get_size ()) <= glyph_end;
      }
    };

    static bool get_iterator (const char * glyph_data,
			      unsigned int length,
			      CompositeGlyphHeader::Iterator *iterator /* OUT */)
    {
      if (length < GlyphHeader::static_size)
	return false; /* Empty glyph; zero extents. */

      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyph_data, 0);
      if (glyph_header.numberOfContours < 0)
      {
	const CompositeGlyphHeader *possible =
	  &StructAfter<CompositeGlyphHeader, GlyphHeader> (glyph_header);

	iterator->glyph_start = glyph_data;
	iterator->glyph_end = (const char *) glyph_data + length;
	if (!iterator->in_range (possible))
	  return false;
	iterator->current = possible;
	return true;
      }

      return false;
    }

    DEFINE_SIZE_MIN (4);
  };

  struct accelerator_t
  {
    void init (hb_face_t *face)
    {
      memset (this, 0, sizeof (accelerator_t));

      const OT::head &head = *face->table.head;
      if (head.indexToLocFormat > 1 || head.glyphDataFormat != 0)
	/* Unknown format.  Leave num_glyphs=0, that takes care of disabling us. */
	return;
      short_offset = 0 == head.indexToLocFormat;

      loca_table = hb_sanitize_context_t ().reference_table<loca> (face);
      glyf_table = hb_sanitize_context_t ().reference_table<glyf> (face);

      num_glyphs = hb_max (1u, loca_table.get_length () / (short_offset ? 2 : 4)) - 1;
    }

    void fini ()
    {
      loca_table.destroy ();
      glyf_table.destroy ();
    }

    /*
     * Returns true if the referenced glyph is a valid glyph and a composite glyph.
     * If true is returned a pointer to the composite glyph will be written into
     * composite.
     */
    bool get_composite (hb_codepoint_t glyph,
			CompositeGlyphHeader::Iterator *composite /* OUT */) const
    {
      if (unlikely (!num_glyphs))
	return false;

      unsigned int start_offset, end_offset;
      if (!get_offsets (glyph, &start_offset, &end_offset))
	return false; /* glyph not found */

      return CompositeGlyphHeader::get_iterator ((const char *) this->glyf_table + start_offset,
						 end_offset - start_offset,
						 composite);
    }

    enum simple_glyph_flag_t {
      FLAG_ON_CURVE = 0x01,
      FLAG_X_SHORT = 0x02,
      FLAG_Y_SHORT = 0x04,
      FLAG_REPEAT = 0x08,
      FLAG_X_SAME = 0x10,
      FLAG_Y_SAME = 0x20,
      FLAG_RESERVED1 = 0x40,
      FLAG_RESERVED2 = 0x80
    };

    /* based on FontTools _g_l_y_f.py::trim */
    bool remove_padding (unsigned int start_offset,
			 unsigned int *end_offset) const
    {
      if (*end_offset - start_offset < GlyphHeader::static_size) return true;

      const char *glyph = ((const char *) glyf_table) + start_offset;
      const char * const glyph_end = glyph + (*end_offset - start_offset);
      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyph, 0);
      int16_t num_contours = (int16_t) glyph_header.numberOfContours;

      if (num_contours < 0)
	/* Trimming for composites not implemented.
	 * If removing hints it falls out of that. */
	return true;
      else if (num_contours > 0)
      {
	/* simple glyph w/contours, possibly trimmable */
	glyph += GlyphHeader::static_size + 2 * num_contours;

	if (unlikely (glyph + 2 >= glyph_end)) return false;
	uint16_t nCoordinates = (uint16_t) StructAtOffset<HBUINT16> (glyph - 2, 0) + 1;
	uint16_t nInstructions = (uint16_t) StructAtOffset<HBUINT16> (glyph, 0);

	glyph += 2 + nInstructions;
	if (unlikely (glyph + 2 >= glyph_end)) return false;

	unsigned int coordBytes = 0;
	unsigned int coordsWithFlags = 0;
	while (glyph < glyph_end)
	{
	  uint8_t flag = (uint8_t) *glyph;
	  glyph++;

	  unsigned int repeat = 1;
	  if (flag & FLAG_REPEAT)
	  {
	    if (glyph >= glyph_end)
	    {
	      DEBUG_MSG(SUBSET, nullptr, "Bad flag");
	      return false;
	    }
	    repeat = ((uint8_t) *glyph) + 1;
	    glyph++;
	  }

	  unsigned int xBytes, yBytes;
	  xBytes = yBytes = 0;
	  if (flag & FLAG_X_SHORT) xBytes = 1;
	  else if ((flag & FLAG_X_SAME) == 0) xBytes = 2;

	  if (flag & FLAG_Y_SHORT) yBytes = 1;
	  else if ((flag & FLAG_Y_SAME) == 0) yBytes = 2;

	  coordBytes += (xBytes + yBytes) * repeat;
	  coordsWithFlags += repeat;
	  if (coordsWithFlags >= nCoordinates)
	    break;
	}

	if (coordsWithFlags != nCoordinates)
	{
	  DEBUG_MSG(SUBSET, nullptr, "Expect %d coords to have flags, got flags for %d", nCoordinates, coordsWithFlags);
	  return false;
	}
	glyph += coordBytes;

	if (glyph < glyph_end)
	  *end_offset -= glyph_end - glyph;
      }
      return true;
    }

    bool get_offsets (hb_codepoint_t  glyph,
		      unsigned int   *start_offset /* OUT */,
		      unsigned int   *end_offset   /* OUT */) const
    {
      if (unlikely (glyph >= num_glyphs))
	return false;

      if (short_offset)
      {
	const HBUINT16 *offsets = (const HBUINT16 *) loca_table->dataZ.arrayZ;
	*start_offset = 2 * offsets[glyph];
	*end_offset   = 2 * offsets[glyph + 1];
      }
      else
      {
	const HBUINT32 *offsets = (const HBUINT32 *) loca_table->dataZ.arrayZ;

	*start_offset = offsets[glyph];
	*end_offset   = offsets[glyph + 1];
      }

      if (*start_offset > *end_offset || *end_offset > glyf_table.get_length ())
	return false;

      return true;
    }

    bool get_instruction_length (hb_bytes_t glyph,
				 unsigned int * length /* OUT */) const
    {
      /* Empty glyph; no instructions. */
      if (glyph.length < GlyphHeader::static_size)
      {
	*length = 0;
	// only 0 byte glyphs are healthy when missing GlyphHeader
	return glyph.length == 0;
      }
      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (&glyph, 0);
      int16_t num_contours = (int16_t) glyph_header.numberOfContours;
      if (num_contours < 0)
      {
	unsigned int start = glyph.length;
	unsigned int end = glyph.length;
	CompositeGlyphHeader::Iterator composite_it;
	if (unlikely (!CompositeGlyphHeader::get_iterator (&glyph, glyph.length, &composite_it))) return false;
	const CompositeGlyphHeader *last;
	do {
	  last = composite_it.current;
	} while (composite_it.move_to_next ());

	if ((uint16_t) last->flags & CompositeGlyphHeader::WE_HAVE_INSTRUCTIONS)
	  start = ((char *) last - (char *) glyf_table->dataZ.arrayZ) + last->get_size ();
	if (unlikely (start > end))
	{
	  DEBUG_MSG(SUBSET, nullptr, "Invalid instruction offset, %d is outside %d byte buffer", start, glyph.length);
	  return false;
	}
	*length = end - start;
      }
      else
      {
	unsigned int instruction_length_offset = GlyphHeader::static_size + 2 * num_contours;
	if (unlikely (instruction_length_offset + 2 > glyph.length))
	{
	  DEBUG_MSG(SUBSET, nullptr, "Glyph size is too short, missing field instructionLength.");
	  return false;
	}

	const HBUINT16 &instruction_length = StructAtOffset<HBUINT16> (&glyph, instruction_length_offset);
	if (unlikely (instruction_length_offset + instruction_length > glyph.length)) // Out of bounds of the current glyph
	{
	  DEBUG_MSG(SUBSET, nullptr, "The instructions array overruns the glyph's boundaries.");
	  return false;
	}
	*length = (uint16_t) instruction_length;
      }
      return true;
    }

    bool get_extents (hb_codepoint_t glyph, hb_glyph_extents_t *extents) const
    {
      unsigned int start_offset, end_offset;
      if (!get_offsets (glyph, &start_offset, &end_offset))
	return false;

      if (end_offset - start_offset < GlyphHeader::static_size)
	return true; /* Empty glyph; zero extents. */

      const GlyphHeader &glyph_header = StructAtOffset<GlyphHeader> (glyf_table, start_offset);

      extents->x_bearing = hb_min (glyph_header.xMin, glyph_header.xMax);
      extents->y_bearing = hb_max (glyph_header.yMin, glyph_header.yMax);
      extents->width     = hb_max (glyph_header.xMin, glyph_header.xMax) - extents->x_bearing;
      extents->height    = hb_min (glyph_header.yMin, glyph_header.yMax) - extents->y_bearing;

      return true;
    }

    private:
    bool short_offset;
    unsigned int num_glyphs;
    hb_blob_ptr_t<loca> loca_table;
    hb_blob_ptr_t<glyf> glyf_table;
  };

  protected:
  UnsizedArrayOf<HBUINT8>	dataZ;		/* Glyphs data. */
  public:
  DEFINE_SIZE_MIN (0); /* In reality, this is UNBOUNDED() type; but since we always
			* check the size externally, allow Null() object of it by
			* defining it _MIN instead. */
};

struct glyf_accelerator_t : glyf::accelerator_t {};

} /* namespace OT */


#endif /* HB_OT_GLYF_TABLE_HH */
