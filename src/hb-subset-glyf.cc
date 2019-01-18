/*
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
 * Google Author(s): Garret Rieger, Roderick Sheeter
 */

#include "hb-open-type.hh"
#include "hb-ot-glyf-table.hh"
#include "hb-set.h"
#include "hb-subset-glyf.hh"

struct loca_data_t
{
  bool          is_short;
  void         *data;
  unsigned int  size;

  inline bool
  _write_loca_entry (unsigned int  id,
                     unsigned int  offset)
  {
    unsigned int entry_size = is_short ? sizeof (OT::HBUINT16) : sizeof (OT::HBUINT32);
    if ((id + 1) * entry_size <= size)
    {
      if (is_short) {
        ((OT::HBUINT16*) data) [id].set (offset / 2);
      } else {
        ((OT::HBUINT32*) data) [id].set (offset);
      }
      return true;
    }

    // Offset was not written because the write is out of bounds.
    DEBUG_MSG(SUBSET,
              nullptr,
              "WARNING: Attempted to write an out of bounds loca entry at index %d. Loca size is %d.",
              id,
              size);
    return false;
  }

  inline bool
  _add_empty_glyphs_to_loca (hb_codepoint_t from_gid,
                             hb_codepoint_t to_gid,
                             unsigned int   offset)
  {
    bool success = true;
    while (from_gid < to_gid)
    {
      success = success && _write_loca_entry (from_gid,
                                              offset);
      from_gid++;
    }
    return success;
  }
};

/**
 * If hints are being dropped find the range which in glyf at which
 * the hinting instructions are located. Add them to the instruction_ranges
 * vector.
 */
static bool
_add_instructions_range (const OT::glyf::accelerator_t &glyf,
                         hb_codepoint_t                 glyph_id,
                         unsigned int                   glyph_start_offset,
                         unsigned int                   glyph_end_offset,
                         bool                           drop_hints,
                         hb_vector_t<unsigned int>     *instruction_ranges /* OUT */)
{
  if (!instruction_ranges->resize (instruction_ranges->length + 2))
  {
    DEBUG_MSG(SUBSET, nullptr, "Failed to resize instruction_ranges.");
    return false;
  }
  unsigned int *instruction_start = &(*instruction_ranges)[instruction_ranges->length - 2];
  *instruction_start = 0;
  unsigned int *instruction_end = &(*instruction_ranges)[instruction_ranges->length - 1];
  *instruction_end = 0;

  if (drop_hints)
  {
    if (unlikely (!glyf.get_instruction_offsets (glyph_start_offset, glyph_end_offset,
                                                 instruction_start, instruction_end)))
    {
      DEBUG_MSG(SUBSET, nullptr, "Unable to get instruction offsets for %d", glyph_id);
      return false;
    }
  }

  return true;
}

static bool
_calculate_glyf_and_loca_prime_size (const OT::glyf::accelerator_t &glyf,
                                     const hb_subset_plan_t        *plan,
                                     loca_data_t                   *loca_data, /* OUT */
				     unsigned int                  *glyf_size /* OUT */,
				     hb_vector_t<unsigned int>     *instruction_ranges /* OUT */)
{
  unsigned int total = 0;
  hb_codepoint_t max_new_gid = 0;
  for (unsigned int i = 0; i < plan->glyphs.length; i++)
  {
    hb_codepoint_t next_glyph = plan->glyphs[i];
    hb_codepoint_t new_gid_for_next_glyph;
    if (plan->new_gid_for_old_gid (next_glyph, &new_gid_for_next_glyph))
      max_new_gid = MAX (max_new_gid, new_gid_for_next_glyph);

    unsigned int start_offset, end_offset;
    if (unlikely (!(glyf.get_offsets (next_glyph, &start_offset, &end_offset) &&
		    glyf.remove_padding (start_offset, &end_offset))))
    {
      DEBUG_MSG(SUBSET, nullptr, "Invalid gid %d", next_glyph);
      continue;
    }

    bool is_zero_length = end_offset - start_offset < OT::glyf::GlyphHeader::static_size;
    if (!_add_instructions_range (glyf,
                                  next_glyph,
                                  start_offset,
                                  end_offset,
                                  plan->drop_hints && !is_zero_length,
                                  instruction_ranges))
      return false;

    if (is_zero_length)
      continue; /* 0-length glyph */

    total += end_offset - start_offset
             - ((*instruction_ranges)[instruction_ranges->length - 1]
                - (*instruction_ranges)[instruction_ranges->length - 2]);
    /* round2 so short loca will work */
    total += total % 2;
  }

  *glyf_size = total;
  loca_data->is_short = (total <= 131070);
  loca_data->size = (max_new_gid + 2)
      * (loca_data->is_short ? sizeof (OT::HBUINT16) : sizeof (OT::HBUINT32));

  DEBUG_MSG(SUBSET, nullptr, "preparing to subset glyf: final size %d, loca size %d, using %s loca",
	    total,
	    loca_data->size,
	    loca_data->is_short ? "short" : "long");
  return true;
}

static void
_update_components (const hb_subset_plan_t *plan,
		    char                   *glyph_start,
		    unsigned int            length)
{
  OT::glyf::CompositeGlyphHeader::Iterator iterator;
  if (OT::glyf::CompositeGlyphHeader::get_iterator (glyph_start,
						    length,
						    &iterator))
  {
    do
    {
      hb_codepoint_t new_gid;
      if (!plan->new_gid_for_old_gid (iterator.current->glyphIndex,
				      &new_gid))
	continue;

      ((OT::glyf::CompositeGlyphHeader *) iterator.current)->glyphIndex.set (new_gid);
    } while (iterator.move_to_next ());
  }
}

static bool _remove_composite_instruction_flag (char *glyf_prime, unsigned int length)
{
  /* remove WE_HAVE_INSTRUCTIONS from flags in dest */
  OT::glyf::CompositeGlyphHeader::Iterator composite_it;
  if (unlikely (!OT::glyf::CompositeGlyphHeader::get_iterator (glyf_prime, length, &composite_it))) return false;
  const OT::glyf::CompositeGlyphHeader *glyph;
  do {
    glyph = composite_it.current;
    OT::HBUINT16 *flags = const_cast<OT::HBUINT16 *> (&glyph->flags);
    flags->set ( (uint16_t) *flags & ~OT::glyf::CompositeGlyphHeader::WE_HAVE_INSTRUCTIONS);
  } while (composite_it.move_to_next ());
  return true;
}

static bool
_write_glyf_and_loca_prime (const hb_subset_plan_t        *plan,
			    const OT::glyf::accelerator_t &glyf,
			    const char                    *glyf_data,
			    hb_vector_t<unsigned int>     &instruction_ranges,
			    unsigned int                   glyf_prime_size,
			    char                          *glyf_prime_data /* OUT */,
			    loca_data_t                   *loca_prime /* OUT */)
{
  const hb_vector_t<hb_codepoint_t> &glyph_ids = plan->glyphs;
  char *glyf_prime_data_next = glyf_prime_data;

  bool success = true;
  unsigned int current_gid = 0;
  for (unsigned int i = 0; i < glyph_ids.length; i++)
  {
    hb_codepoint_t next_gid = current_gid;
    success = success && plan->new_gid_for_old_gid (glyph_ids[i], &next_gid);

    // If we are retaining existing gids then there will potentially be gaps
    // in the loca table between glyphs. Fill in this gap with glyphs that have
    // no outlines.
    success = success && loca_prime->_add_empty_glyphs_to_loca (current_gid,
                                                                next_gid,
                                                                glyf_prime_data_next - glyf_prime_data);
    current_gid = next_gid;

    unsigned int start_offset, end_offset;
    if (unlikely (!(glyf.get_offsets (glyph_ids[i], &start_offset, &end_offset) &&
		    glyf.remove_padding (start_offset, &end_offset))))
      end_offset = start_offset = 0;

    unsigned int instruction_start = instruction_ranges[i * 2];
    unsigned int instruction_end = instruction_ranges[i * 2 + 1];

    int length = end_offset - start_offset - (instruction_end - instruction_start);

    if (glyf_prime_data_next + length > glyf_prime_data + glyf_prime_size)
    {
      DEBUG_MSG(SUBSET,
		 nullptr,
		 "WARNING: Attempted to write an out of bounds glyph entry for gid %d (length %d)",
		 i, length);
      return false;
    }

    if (instruction_start == instruction_end)
      memcpy (glyf_prime_data_next, glyf_data + start_offset, length);
    else
    {
      memcpy (glyf_prime_data_next, glyf_data + start_offset, instruction_start - start_offset);
      memcpy (glyf_prime_data_next + instruction_start - start_offset, glyf_data + instruction_end, end_offset - instruction_end);
      /* if the instructions end at the end this was a composite glyph, else simple */
      if (instruction_end == end_offset)
      {
	if (unlikely (!_remove_composite_instruction_flag (glyf_prime_data_next, length))) return false;
      }
      else
	/* zero instruction length, which is just before instruction_start */
	memset (glyf_prime_data_next + instruction_start - start_offset - 2, 0, 2);
    }

    success = success && loca_prime->_write_loca_entry (current_gid,
                                                        glyf_prime_data_next - glyf_prime_data);
    _update_components (plan, glyf_prime_data_next, length);

    // TODO: don't align to two bytes if using long loca.
    glyf_prime_data_next += length + (length % 2); // Align to 2 bytes for short loca.
    current_gid++;
  }

  // loca table has n+1 entries where the last entry signifies the end location of the last
  // glyph.
  success = success && loca_prime->_write_loca_entry (current_gid,
                                                      glyf_prime_data_next - glyf_prime_data);
  return success;
}

static bool
_hb_subset_glyf_and_loca (const OT::glyf::accelerator_t  &glyf,
			  const char                     *glyf_data,
			  hb_subset_plan_t               *plan,
			  bool                           *use_short_loca,
			  hb_blob_t                     **glyf_prime_blob /* OUT */,
			  hb_blob_t                     **loca_prime_blob /* OUT */)
{
  // TODO(grieger): Sanity check allocation size for the new table.
  loca_data_t loca_prime;
  unsigned int glyf_prime_size;
  hb_vector_t<unsigned int> instruction_ranges;
  instruction_ranges.init ();

  if (unlikely (!_calculate_glyf_and_loca_prime_size (glyf,
                                                      plan,
                                                      &loca_prime,
						      &glyf_prime_size,
						      &instruction_ranges))) {
    instruction_ranges.fini ();
    return false;
  }
  *use_short_loca = loca_prime.is_short;

  char *glyf_prime_data = (char *) calloc (1, glyf_prime_size);
  loca_prime.data = (void *) calloc (1, loca_prime.size);
  if (unlikely (!_write_glyf_and_loca_prime (plan, glyf, glyf_data,
					     instruction_ranges,
					     glyf_prime_size, glyf_prime_data,
					     &loca_prime))) {
    free (glyf_prime_data);
    free (loca_prime.data);
    instruction_ranges.fini ();
    return false;
  }
  instruction_ranges.fini ();

  *glyf_prime_blob = hb_blob_create (glyf_prime_data,
                                     glyf_prime_size,
                                     HB_MEMORY_MODE_READONLY,
                                     glyf_prime_data,
                                     free);
  *loca_prime_blob = hb_blob_create ((char *) loca_prime.data,
                                     loca_prime.size,
                                     HB_MEMORY_MODE_READONLY,
                                     loca_prime.data,
                                     free);
  return true;
}

/**
 * hb_subset_glyf:
 * Subsets the glyph table according to a provided plan.
 *
 * Return value: subsetted glyf table.
 *
 * Since: 1.7.5
 **/
bool
hb_subset_glyf_and_loca (hb_subset_plan_t *plan,
			 bool             *use_short_loca, /* OUT */
			 hb_blob_t       **glyf_prime, /* OUT */
			 hb_blob_t       **loca_prime /* OUT */)
{
  hb_blob_t *glyf_blob = hb_sanitize_context_t ().reference_table<OT::glyf> (plan->source);
  const char *glyf_data = hb_blob_get_data (glyf_blob, nullptr);

  OT::glyf::accelerator_t glyf;
  glyf.init (plan->source);
  bool result = _hb_subset_glyf_and_loca (glyf,
					  glyf_data,
					  plan,
					  use_short_loca,
					  glyf_prime,
					  loca_prime);

  hb_blob_destroy (glyf_blob);
  glyf.fini ();

  return result;
}
