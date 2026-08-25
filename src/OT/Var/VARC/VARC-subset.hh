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
  if (unlikely (!records.alloc_exact (old_indices.length)))
    return_trace (fail ());

  if (retain_gids)
  {
    /* Retained glyph IDs need no component-record parsing or rewriting. */
    for (unsigned old_index : old_indices)
      records.push (source_records[old_index]);
  }
  else
  {
    hb_vector_t<unsigned> record_offsets;
    record_data.alloc (data_size);
    if (unlikely (!record_offsets.alloc_exact (old_indices.length + 1)))
      return_trace (fail ());

    for (unsigned old_index : old_indices)
    {
      record_offsets.push (record_data.length);
      hb_ubytes_t remaining = source_records[old_index];
      while (remaining)
      {
	VarComponent::record_t component;
	hb_codepoint_t new_gid;
	if (unlikely (!VarComponent::decompile_record (*this, remaining,
						       nullptr, &component) ||
			    !c->plan->new_gid_for_old_gid (component.gid, &new_gid) ||
			    new_gid > 0xFFFFFFu))
	  return_trace (fail ());

	uint32_t flags = component.flags;
	unsigned gid_size = component.gid_size;
	if (new_gid > 0xFFFFu)
	{
	  flags |= (unsigned) VarComponent::flags_t::GID_IS_24BIT;
	  gid_size = HBGlyphID24::static_size;
	}

	unsigned flags_size = HBUINT32VAR::get_size (flags);
	unsigned tail_offset = component.gid_offset + component.gid_size;
	unsigned tail_size = component.size - tail_offset;
	unsigned component_offset = record_data.length;
	unsigned component_size = flags_size + gid_size + tail_size;
	unsigned new_length = hb_unsigned_add_saturate (component_offset,
						       component_size);
	if (unlikely (new_length == UINT_MAX ||
		      !record_data.resize_dirty (new_length)))
	  return_trace (fail ());

	unsigned char *out = record_data.arrayZ + component_offset;
	HBUINT32VAR::serialize_unsafe (out, flags);
	out += flags_size;
	if (gid_size == HBGlyphID16::static_size)
	{
	  * (HBGlyphID16 *) out = new_gid;
	  out += HBGlyphID16::static_size;
	}
	else
	{
	  * (HBGlyphID24 *) out = new_gid;
	  out += HBGlyphID24::static_size;
	}
	hb_memcpy (out, remaining.arrayZ + tail_offset, tail_size);
	remaining = remaining.sub_array (component.size);
      }
    }
    record_offsets.push (record_data.length);

    if (unlikely (record_data.in_error () || record_offsets.in_error ()))
      return_trace (fail ());
    for (unsigned i = 0; i < old_indices.length; i++)
      records.push (hb_ubytes_t (record_data.arrayZ + record_offsets[i],
				 record_offsets[i + 1] - record_offsets[i]));
    data_size = record_data.length;
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
