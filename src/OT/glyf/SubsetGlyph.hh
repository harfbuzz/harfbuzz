#ifndef OT_GLYF_SUBSETGLYPH_HH
#define OT_GLYF_SUBSETGLYPH_HH


#include "../../hb-open-type.hh"


namespace OT {

struct glyf_accelerator_t;

namespace glyf_impl {


struct SubsetGlyph
{
  hb_codepoint_t old_gid;
  Glyph source_glyph;
  hb_bytes_t dest_start;  /* region of source_glyph to copy first */
  hb_bytes_t dest_end;    /* region of source_glyph to copy second */
  bool allocated;

  bool serialize (hb_serialize_context_t *c,
		  bool use_short_loca,
		  const hb_subset_plan_t *plan) const
  {
    TRACE_SERIALIZE (this);

    bool do_copy = !plan->should_omit_glyf_bytes ();
    hb_bytes_t dest_glyph;

    if (do_copy)
    {
      dest_glyph = dest_start.copy (c);
      hb_bytes_t end_copy = dest_end.copy (c);
      if (!end_copy.arrayZ || !dest_glyph.arrayZ) {
        return false;
      }

      dest_glyph = hb_bytes_t (&dest_glyph, dest_glyph.length + end_copy.length);
    } else {
      // TODO(garretrieger): remove assert.
      assert(!dest_end); // shouldn't be set if hint dropping isn't on.
      dest_glyph = dest_start;
    }

    unsigned int pad_length = use_short_loca ? padding () : 0;
    DEBUG_MSG (SUBSET, nullptr, "serialize %u byte glyph, width %u pad %u", dest_glyph.length, dest_glyph.length + pad_length, pad_length);

    HBUINT8 pad;
    pad = 0;
    if (do_copy)
    {
      while (pad_length > 0)
      {
        (void) c->embed (pad);
        pad_length--;
      }
    }

    if (unlikely (!dest_glyph.length)) return_trace (true);

    /* update components gids */
    if (!(plan->flags & HB_SUBSET_FLAGS_RETAIN_GIDS) && do_copy)
    {
      for (auto &_ : Glyph (dest_glyph).get_composite_iterator ())
       {
        hb_codepoint_t new_gid;
        if (plan->new_gid_for_old_gid (_.get_gid(), &new_gid))
          const_cast<CompositeGlyphRecord &> (_).set_gid (new_gid);
      }
    }
#ifndef HB_NO_VAR_COMPOSITES
    // TODO(grieger): skip if retain gids or not copying.
    for (auto &_ : Glyph (dest_glyph).get_var_composite_iterator ())
    {
      hb_codepoint_t new_gid;
      if (plan->new_gid_for_old_gid (_.get_gid(), &new_gid))
	const_cast<VarCompositeGlyphRecord &> (_).set_gid (new_gid);
    }
#endif

#ifndef HB_NO_BEYOND_64K
    // TODO(grieger): not allowed in omit glyf mode.
    auto it = Glyph (dest_glyph).get_composite_iterator ();
    if (it)
    {
      /* lower GID24 to GID16 in components if possible.
       *
       * TODO: VarComposite. Not as critical, since VarComposite supports
       * gid24 from the first version. */
      char *p = it ? (char *) &*it : nullptr;
      char *q = p;
      const char *end = dest_glyph.arrayZ + dest_glyph.length;
      while (it)
      {
	auto &rec = const_cast<CompositeGlyphRecord &> (*it);
	++it;

	q += rec.get_size ();

	rec.lower_gid_24_to_16 ();

	unsigned size = rec.get_size ();

	memmove (p, &rec, size);

	p += size;
      }
      memmove (p, q, end - q);
      p += end - q;

      /* We want to shorten the glyph, but we can't do that without
       * updating the length in the loca table, which is already
       * written out :-(.  So we just fill the rest of the glyph with
       * harmless instructions, since that's what they will be
       * interpreted as.
       *
       * Should move the lowering to _populate_subset_glyphs() to
       * fix this issue. */

      hb_memset (p, 0x7A /* TrueType instruction ROFF; harmless */, end - p);
      p += end - p;
      dest_glyph = hb_bytes_t (dest_glyph.arrayZ, p - (char *) dest_glyph.arrayZ);

      // TODO: Padding; & trim serialized bytes.
      // TODO: Update length in loca. Ugh.
    }
#endif

    if (plan->flags & HB_SUBSET_FLAGS_NO_HINTING)
      Glyph (dest_glyph).drop_hints ();

    if (plan->flags & HB_SUBSET_FLAGS_SET_OVERLAPS_FLAG)
      Glyph (dest_glyph).set_overlaps_flag ();

    return_trace (true);
  }

  bool compile_bytes_with_deltas (const hb_subset_plan_t *plan,
                                  hb_font_t *font,
                                  const glyf_accelerator_t &glyf)
  {
    allocated = source_glyph.compile_bytes_with_deltas (plan, font, glyf, dest_start, dest_end);
    return allocated;
  }

  void free_compiled_bytes ()
  {
    if (likely (allocated)) {
      allocated = false;
      dest_start.fini ();
      dest_end.fini ();
    }
  }

  void drop_hints_bytes ()
  { source_glyph.drop_hints_bytes (dest_start, dest_end); }

  unsigned int      length () const { return dest_start.length + dest_end.length; }
  /* pad to 2 to ensure 2-byte loca will be ok */
  unsigned int     padding () const { return length () % 2; }
  unsigned int padded_size () const { return length () + padding (); }

  uint32_t checksum (bool use_short_loca,
                     uint8_t   remainder[4],
                     unsigned *remainder_length) const
  {
    // TODO(garretrieger): remove assertion.
    assert (!dest_end); // we don't handle hint removal.
    assert (*remainder_length <= 4);

    const uint8_t* input = (const uint8_t*) dest_start.arrayZ;
    unsigned input_length = dest_start.length;
    while (*remainder_length < 4 && input_length > 0)
    {
      remainder[(*remainder_length)++] = *(input++);
      input_length--;
    }

    if (*remainder_length < 4) {
      // Remainder not yet filled.
      return 0;
    }

    // commit remainder.
    HBUINT32 checksum = *((HBUINT32*) remainder);
    *remainder_length = 0;
    *((HBUINT32*) remainder) = 0;

    if (input_length >= 4)
    {
      unsigned aligned_length = (input_length / 4) * 4;
      CheckSum block_checksum;
      block_checksum.set_for_data (input, aligned_length);
      checksum += block_checksum;
      input += aligned_length;
      input_length -= aligned_length;
    }

    unsigned pad_length = use_short_loca ? padding() : 0;
    if (input_length + pad_length >= 4)
    {
      unsigned i = 0;
      while (input_length > 0) {
        remainder[i++] = *(input++);
        input_length--;
      }
      checksum += *((HBUINT32*) remainder);
      pad_length -= (4 - i);
      *((HBUINT32*) remainder) = 0;
    }

    *remainder_length = input_length + pad_length;
    for (unsigned i = 0; i < input_length; i++)
      remainder[i] = input[i];

    return checksum;
  }
};


} /* namespace glyf_impl */
} /* namespace OT */


#endif /* OT_GLYF_SUBSETGLYPH_HH */
