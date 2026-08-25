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
  const ConditionList &source_conditions = this+conditionList;
  const TupleList &source_axis_indices = this+axisIndicesList;
  const CFF2Index &source_axis_indices_index = source_axis_indices;
  const MultiItemVariationStore &source_var_store = this+varStore;
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

  hb_set_t condition_indices;
  hb_set_t axis_indices;
  hb_set_t var_indices;
  for (unsigned old_index : old_indices)
  {
    hb_ubytes_t remaining = source_records[old_index];
    while (remaining)
    {
      VarComponent::record_t component;
      if (unlikely (!VarComponent::decompile_record (*this, remaining,
						     nullptr, &component)))
	return_trace (fail ());

      if (component.flags & (unsigned) VarComponent::flags_t::HAVE_CONDITION)
      {
	if (unlikely (component.condition_index >= source_conditions.get_count ()))
	  return_trace (fail ());
	condition_indices.add (component.condition_index);
      }
      if (component.flags & (unsigned) VarComponent::flags_t::HAVE_AXES)
      {
	if (unlikely (component.axis_indices_index >= source_axis_indices_index.count))
	  return_trace (fail ());
	axis_indices.add (component.axis_indices_index);
      }
      if (component.flags & (unsigned) VarComponent::flags_t::AXIS_VALUES_HAVE_VARIATION)
	var_indices.add (component.axis_values_var_idx);
      if (component.flags & (unsigned) VarComponent::flags_t::TRANSFORM_HAS_VARIATION)
	var_indices.add (component.transform_var_idx);

      remaining = remaining.sub_array (component.size);
    }
  }

  if (unlikely (condition_indices.in_error () || axis_indices.in_error () ||
		var_indices.in_error ()))
    return_trace (fail ());

  for (hb_codepoint_t condition_index : condition_indices)
    if (unlikely (!source_conditions[condition_index].collect_var_indices (&var_indices)))
      return_trace (fail ());

  hb_inc_bimap_t condition_map;
  hb_inc_bimap_t axis_indices_map;
  condition_map.add_set (&condition_indices);
  axis_indices_map.add_set (&axis_indices);
  hb_vector_t<hb_inc_bimap_t> var_inner_maps;
  hb_map_t varidx_map;
  if (unlikely (condition_map.in_error () || axis_indices_map.in_error () ||
		!source_var_store.create_subset_plan (var_indices,
						      &var_inner_maps,
						      &varidx_map)))
    return_trace (fail ());

  hb_vector_t<unsigned char> record_data;
  hb_vector_t<hb_ubytes_t> records;
  bool retain_gids = c->plan->flags & HB_SUBSET_FLAGS_RETAIN_GIDS;
  if (unlikely (!records.alloc_exact (old_indices.length)))
    return_trace (fail ());

  bool auxiliary_indices_unchanged = true;
  for (hb_codepoint_t old_index : condition_indices)
    auxiliary_indices_unchanged &= condition_map.get (old_index) == old_index;
  for (hb_codepoint_t old_index : axis_indices)
    auxiliary_indices_unchanged &= axis_indices_map.get (old_index) == old_index;
  for (hb_codepoint_t old_index : var_indices)
    auxiliary_indices_unchanged &= varidx_map.get (old_index) == old_index;

  if (retain_gids && auxiliary_indices_unchanged)
  {
    /* The records reference the same indices, so retain them byte-for-byte. */
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
	if (unlikely (!VarComponent::decompile_record (*this, remaining,
						       nullptr, &component)))
	  return_trace (fail ());
	hb_codepoint_t new_gid = component.gid;
	if (unlikely ((!retain_gids &&
		       !c->plan->new_gid_for_old_gid (component.gid, &new_gid)) ||
		      new_gid > 0xFFFFFFu))
	  return_trace (fail ());

	uint32_t flags = component.flags;
	unsigned gid_size = component.gid_size;
	if (new_gid > 0xFFFFu)
	{
	  flags |= (unsigned) VarComponent::flags_t::GID_IS_24BIT;
	  gid_size = HBGlyphID24::static_size;
	}

	auto append_varint = [&] (uint32_t value)
	{
	  unsigned size = HBUINT32VAR::get_size (value);
	  unsigned offset = record_data.length;
	  unsigned new_length = hb_unsigned_add_saturate (offset, size);
	  if (unlikely (new_length == UINT_MAX ||
			!record_data.resize_dirty (new_length)))
	    return false;
	  HBUINT32VAR::serialize_unsafe (record_data.arrayZ + offset, value);
	  return true;
	};
	auto append_bytes = [&] (unsigned start, unsigned end)
	{
	  if (unlikely (start > end || end > component.size)) return false;
	  unsigned offset = record_data.length;
	  unsigned size = end - start;
	  unsigned new_length = hb_unsigned_add_saturate (offset, size);
	  if (unlikely (new_length == UINT_MAX ||
			!record_data.resize_dirty (new_length)))
	    return false;
	  hb_memcpy (record_data.arrayZ + offset, remaining.arrayZ + start, size);
	  return true;
	};

	if (unlikely (!append_varint (flags))) return_trace (fail ());
	unsigned gid_offset = record_data.length;
	unsigned gid_end = hb_unsigned_add_saturate (gid_offset, gid_size);
	if (unlikely (gid_end == UINT_MAX ||
		      !record_data.resize_dirty (gid_end)))
	  return_trace (fail ());
	unsigned char *out = record_data.arrayZ + gid_offset;
	if (gid_size == HBGlyphID16::static_size)
	  * (HBGlyphID16 *) out = new_gid;
	else
	  * (HBGlyphID24 *) out = new_gid;

	unsigned cursor = component.gid_offset + component.gid_size;
	if (flags & (unsigned) VarComponent::flags_t::HAVE_CONDITION)
	{
	  if (unlikely (!condition_map.has (component.condition_index) ||
			!append_bytes (cursor, component.condition_offset) ||
			!append_varint (condition_map.get (component.condition_index))))
	    return_trace (fail ());
	  cursor = component.condition_offset + component.condition_size;
	}
	if (flags & (unsigned) VarComponent::flags_t::HAVE_AXES)
	{
	  if (unlikely (!axis_indices_map.has (component.axis_indices_index) ||
			!append_bytes (cursor, component.axis_indices_offset) ||
			!append_varint (axis_indices_map.get (component.axis_indices_index))))
	    return_trace (fail ());
	  cursor = component.axis_indices_offset + component.axis_indices_size;
	}
	if (flags & (unsigned) VarComponent::flags_t::AXIS_VALUES_HAVE_VARIATION)
	{
	  if (unlikely (!varidx_map.has (component.axis_values_var_idx) ||
			!append_bytes (cursor, component.axis_values_var_offset) ||
			!append_varint (varidx_map.get (component.axis_values_var_idx))))
	    return_trace (fail ());
	  cursor = component.axis_values_var_offset + component.axis_values_var_size;
	}
	if (flags & (unsigned) VarComponent::flags_t::TRANSFORM_HAS_VARIATION)
	{
	  if (unlikely (!varidx_map.has (component.transform_var_idx) ||
			!append_bytes (cursor, component.transform_var_offset) ||
			!append_varint (varidx_map.get (component.transform_var_idx))))
	    return_trace (fail ());
	  cursor = component.transform_var_offset + component.transform_var_size;
	}
	if (unlikely (!append_bytes (cursor, component.size)))
	  return_trace (fail ());
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

  VARC *out = c->serializer->start_embed<VARC> ();
  if (unlikely (!out || !c->serializer->extend_min (out)))
    return_trace (false);
  out->version = version;

  hb_vector_t<hb_ubytes_t> selected_axis_indices;
  unsigned axis_indices_data_size = 0;
  if (unlikely (!selected_axis_indices.alloc_exact (
					axis_indices_map.get_population ())))
    return_trace (fail ());
  for (unsigned i = 0; i < axis_indices_map.get_population (); i++)
  {
    hb_ubytes_t bytes = source_axis_indices_index[axis_indices_map.backward (i)];
    axis_indices_data_size = hb_unsigned_add_saturate (axis_indices_data_size,
						       bytes.length);
    selected_axis_indices.push (bytes);
  }
  if (unlikely (axis_indices_data_size == UINT_MAX ||
		selected_axis_indices.in_error ()))
    return_trace (fail ());

  if (unlikely (!out->coverage.serialize_serialize (c->serializer,
						     new_gids.iter ()) ||
		(condition_map.get_population () &&
		 !out->conditionList.serialize_serialize (c->serializer,
						       &source_conditions,
						       condition_map,
						       varidx_map)) ||
		(axis_indices_map.get_population () &&
		 !out->axisIndicesList.serialize_serialize (c->serializer,
							selected_axis_indices.iter (),
							&axis_indices_data_size)) ||
		(var_indices.get_population () &&
		 !out->varStore.serialize_serialize (c->serializer,
						  &source_var_store,
						  var_inner_maps.as_array ())) ||
		!out->glyphRecords.serialize_serialize (c->serializer,
						 records.iter (), &data_size)))
    return_trace (false);

  return_trace (true);
}

} /* namespace OT */

#endif /* OT_VAR_VARC_VARC_SUBSET_HH */
