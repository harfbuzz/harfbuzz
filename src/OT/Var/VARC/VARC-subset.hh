#ifndef OT_VAR_VARC_VARC_SUBSET_HH
#define OT_VAR_VARC_VARC_SUBSET_HH

#include "VARC.hh"

namespace OT {

bool
VARC::subset (hb_subset_context_t *c) const
{
  TRACE_SUBSET (this);
  auto fail = [&] ()
  {
    c->serializer->err (HB_SERIALIZE_ERROR_OTHER);
    return false;
  };

  /* VARC instancing is not supported yet.  In particular, copying the
   * auxiliary lists while fvar axes are removed would leave stale axis
   * indices in the table. */
  if (unlikely (!c->plan->user_axes_location.is_empty ()))
    return_trace (fail ());

  const Coverage &source_coverage = this+coverage;
  const CFF2Index &source_records = this+glyphRecords;
  const hb_set_t &glyphset = c->plan->_glyphset_varced;

  hb_vector_t<unsigned> old_indices;
  hb_sorted_vector_t<hb_codepoint_t> new_gids;
  unsigned index = 0;
  unsigned data_size = 0;
  for (hb_codepoint_t old_gid : source_coverage.iter ())
  {
    if (index >= source_records.count) break;
    if (glyphset.has (old_gid))
    {
      hb_codepoint_t new_gid;
      if (unlikely (!c->plan->new_gid_for_old_gid (old_gid, &new_gid)))
	return_trace (fail ());
      old_indices.push (index);
      new_gids.push (new_gid);
      data_size = hb_unsigned_add_saturate (data_size,
					    source_records[index].length);
    }
    index++;
  }
  if (unlikely (old_indices.in_error () || new_gids.in_error () ||
		data_size == UINT_MAX))
    return_trace (fail ());
  if (!old_indices)
    return_trace (false);

  hb_vector_t<unsigned char> record_data;
  hb_vector_t<hb_ubytes_t> records;
  bool retain_gids = c->plan->flags & HB_SUBSET_FLAGS_RETAIN_GIDS;
  if (unlikely ((!retain_gids && !record_data.resize_dirty (data_size)) ||
		!records.alloc_exact (old_indices.length)))
    return_trace (fail ());

  unsigned data_offset = 0;
  for (unsigned old_index : old_indices)
  {
    hb_ubytes_t source_record = source_records[old_index];

    /* Retained glyph IDs need no component-record parsing or rewriting. */
    if (retain_gids)
    {
      records.push (source_record);
      continue;
    }

    unsigned char *record_start = record_data.arrayZ + data_offset;
    hb_memcpy (record_start, source_record.arrayZ, source_record.length);

    hb_ubytes_t remaining (record_start, source_record.length);
    while (remaining)
    {
	VarComponent::record_t component;
	hb_codepoint_t new_gid;
	if (unlikely (!VarComponent::decompile_record (*this, remaining,
						       nullptr, &component) ||
			    !c->plan->new_gid_for_old_gid (component.gid, &new_gid)))
	return_trace (fail ());

      unsigned char *gid = const_cast<unsigned char *> (remaining.arrayZ) + component.gid_offset;
      if (component.gid_size == HBGlyphID16::static_size)
      {
        if (unlikely (new_gid > 0xFFFFu)) return_trace (fail ());
        * (HBGlyphID16 *) gid = new_gid;
      }
      else
      {
        if (unlikely (new_gid > 0xFFFFFFu)) return_trace (fail ());
	* (HBGlyphID24 *) gid = new_gid;
      }
      remaining = remaining.sub_array (component.size);
    }

    records.push (hb_ubytes_t (record_start, source_record.length));
    data_offset += source_record.length;
  }
  if (unlikely (records.in_error ()))
    return_trace (fail ());

  /* Keep the auxiliary structures byte-for-byte for now.  Copying the full
   * source table preserves all of their internal offsets; the filtered
   * coverage and glyph-record index are appended and their offsets replaced. */
  unsigned source_length = c->source_blob->length;
  if (unlikely (source_length < min_size))
    return_trace (fail ());
  VARC *out = c->serializer->start_embed<VARC> ();
  if (unlikely (!c->serializer->extend_size (out, source_length, false)))
    return_trace (false);
  hb_memcpy (out, this, source_length);

  if (unlikely (!out->coverage.serialize_serialize (c->serializer,
						     new_gids.iter ()) ||
		!out->glyphRecords.serialize_serialize (c->serializer,
						 records.iter (), &data_size)))
    return_trace (false);

  return_trace (true);
}

} /* namespace OT */

#endif /* OT_VAR_VARC_VARC_SUBSET_HH */
