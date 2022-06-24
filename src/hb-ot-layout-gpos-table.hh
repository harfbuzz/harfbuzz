/*
 * Copyright © 2007,2008,2009,2010  Red Hat, Inc.
 * Copyright © 2010,2012,2013  Google, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_LAYOUT_GPOS_TABLE_HH
#define HB_OT_LAYOUT_GPOS_TABLE_HH

#include "OT/Layout/GPOS/GPOS.hh"

namespace OT {

struct MarkArray;
static void Markclass_closure_and_remap_indexes (const Coverage  &mark_coverage,
						 const MarkArray &mark_array,
						 const hb_set_t  &glyphset,
						 hb_map_t*        klass_mapping /* INOUT */);


/* Shared Tables: ValueRecord, Anchor Table, and MarkArray */



/* Lookups */


struct MarkMarkPos
{
  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  {
    TRACE_DISPATCH (this, u.format);
    if (unlikely (!c->may_dispatch (this, &u.format))) return_trace (c->no_dispatch_return_value ());
    switch (u.format) {
    case 1: return_trace (c->dispatch (u.format1, std::forward<Ts> (ds)...));
    default:return_trace (c->default_return_value ());
    }
  }

  protected:
  union {
  HBUINT16		format;		/* Format identifier */
  MarkMarkPosFormat1	format1;
  } u;
};


typedef AnchorMatrix Mark2Array;	/* mark2-major--
					 * in order of Mark2Coverage Index--,
					 * mark1-minor--
					 * ordered by class--zero-based. */

struct MarkMarkPosFormat1
{
  bool intersects (const hb_set_t *glyphs) const
  {
    return (this+mark1Coverage).intersects (glyphs) &&
	   (this+mark2Coverage).intersects (glyphs);
  }

  void closure_lookups (hb_closure_lookups_context_t *c) const {}

  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    + hb_zip (this+mark1Coverage, this+mark1Array)
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    | hb_apply ([&] (const MarkRecord& record) { record.collect_variation_indices (c, &(this+mark1Array)); })
    ;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+mark1Coverage, this+mark1Array, *c->glyph_set, &klass_mapping);

    unsigned mark2_count = (this+mark2Array).rows;
    auto mark2_iter =
    + hb_zip (this+mark2Coverage, hb_range (mark2_count))
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    ;

    hb_sorted_vector_t<unsigned> mark2_indexes;
    for (const unsigned row : mark2_iter)
    {
      + hb_range ((unsigned) classCount)
      | hb_filter (klass_mapping)
      | hb_map ([&] (const unsigned col) { return row * (unsigned) classCount + col; })
      | hb_sink (mark2_indexes)
      ;
    }
    (this+mark2Array).collect_variation_indices (c, mark2_indexes.iter ());
  }

  void collect_glyphs (hb_collect_glyphs_context_t *c) const
  {
    if (unlikely (!(this+mark1Coverage).collect_coverage (c->input))) return;
    if (unlikely (!(this+mark2Coverage).collect_coverage (c->input))) return;
  }

  const Coverage &get_coverage () const { return this+mark1Coverage; }

  bool apply (hb_ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    hb_buffer_t *buffer = c->buffer;
    unsigned int mark1_index = (this+mark1Coverage).get_coverage  (buffer->cur().codepoint);
    if (likely (mark1_index == NOT_COVERED)) return_trace (false);

    /* now we search backwards for a suitable mark glyph until a non-mark glyph */
    hb_ot_apply_context_t::skipping_iterator_t &skippy_iter = c->iter_input;
    skippy_iter.reset (buffer->idx, 1);
    skippy_iter.set_lookup_props (c->lookup_props & ~LookupFlag::IgnoreFlags);
    unsigned unsafe_from;
    if (!skippy_iter.prev (&unsafe_from))
    {
      buffer->unsafe_to_concat_from_outbuffer (unsafe_from, buffer->idx + 1);
      return_trace (false);
    }

    if (!_hb_glyph_info_is_mark (&buffer->info[skippy_iter.idx]))
    {
      buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
      return_trace (false);
    }

    unsigned int j = skippy_iter.idx;

    unsigned int id1 = _hb_glyph_info_get_lig_id (&buffer->cur());
    unsigned int id2 = _hb_glyph_info_get_lig_id (&buffer->info[j]);
    unsigned int comp1 = _hb_glyph_info_get_lig_comp (&buffer->cur());
    unsigned int comp2 = _hb_glyph_info_get_lig_comp (&buffer->info[j]);

    if (likely (id1 == id2))
    {
      if (id1 == 0) /* Marks belonging to the same base. */
	goto good;
      else if (comp1 == comp2) /* Marks belonging to the same ligature component. */
	goto good;
    }
    else
    {
      /* If ligature ids don't match, it may be the case that one of the marks
       * itself is a ligature.  In which case match. */
      if ((id1 > 0 && !comp1) || (id2 > 0 && !comp2))
	goto good;
    }

    /* Didn't match. */
    buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
    return_trace (false);

    good:
    unsigned int mark2_index = (this+mark2Coverage).get_coverage  (buffer->info[j].codepoint);
    if (mark2_index == NOT_COVERED)
    {
      buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
      return_trace (false);
    }

    return_trace ((this+mark1Array).apply (c, mark1_index, mark2_index, this+mark2Array, classCount, j));
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();
    const hb_map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);
    out->format = format;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+mark1Coverage, this+mark1Array, glyphset, &klass_mapping);

    if (!klass_mapping.get_population ()) return_trace (false);
    out->classCount = klass_mapping.get_population ();

    auto mark1_iter =
    + hb_zip (this+mark1Coverage, this+mark1Array)
    | hb_filter (glyphset, hb_first)
    ;

    hb_sorted_vector_t<hb_codepoint_t> new_coverage;
    + mark1_iter
    | hb_map (hb_first)
    | hb_map (glyph_map)
    | hb_sink (new_coverage)
    ;

    if (!out->mark1Coverage.serialize_serialize (c->serializer, new_coverage.iter ()))
      return_trace (false);

    out->mark1Array.serialize_subset (c, mark1Array, this,
                                      (this+mark1Coverage).iter (),
                                      &klass_mapping);

    unsigned mark2count = (this+mark2Array).rows;
    auto mark2_iter =
    + hb_zip (this+mark2Coverage, hb_range (mark2count))
    | hb_filter (glyphset, hb_first)
    ;

    new_coverage.reset ();
    + mark2_iter
    | hb_map (hb_first)
    | hb_map (glyph_map)
    | hb_sink (new_coverage)
    ;

    if (!out->mark2Coverage.serialize_serialize (c->serializer, new_coverage.iter ()))
      return_trace (false);

    hb_sorted_vector_t<unsigned> mark2_indexes;
    for (const unsigned row : + mark2_iter
			      | hb_map (hb_second))
    {
      + hb_range ((unsigned) classCount)
      | hb_filter (klass_mapping)
      | hb_map ([&] (const unsigned col) { return row * (unsigned) classCount + col; })
      | hb_sink (mark2_indexes)
      ;
    }

    out->mark2Array.serialize_subset (c, mark2Array, this, mark2_iter.len (), mark2_indexes.iter ());

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  mark1Coverage.sanitize (c, this) &&
		  mark2Coverage.sanitize (c, this) &&
		  mark1Array.sanitize (c, this) &&
		  mark2Array.sanitize (c, this, (unsigned int) classCount));
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 1 */
  Offset16To<Coverage>
		mark1Coverage;		/* Offset to Combining Mark1 Coverage
					 * table--from beginning of MarkMarkPos
					 * subtable */
  Offset16To<Coverage>
		mark2Coverage;		/* Offset to Combining Mark2 Coverage
					 * table--from beginning of MarkMarkPos
					 * subtable */
  HBUINT16	classCount;		/* Number of defined mark classes */
  Offset16To<MarkArray>
		mark1Array;		/* Offset to Mark1Array table--from
					 * beginning of MarkMarkPos subtable */
  Offset16To<Mark2Array>
		mark2Array;		/* Offset to Mark2Array table--from
					 * beginning of MarkMarkPos subtable */
  public:
  DEFINE_SIZE_STATIC (12);
};

struct MarkMarkPos
{
  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  {
    TRACE_DISPATCH (this, u.format);
    if (unlikely (!c->may_dispatch (this, &u.format))) return_trace (c->no_dispatch_return_value ());
    switch (u.format) {
    case 1: return_trace (c->dispatch (u.format1, std::forward<Ts> (ds)...));
    default:return_trace (c->default_return_value ());
    }
  }

  protected:
  union {
  HBUINT16		format;		/* Format identifier */
  MarkMarkPosFormat1	format1;
  } u;
};


struct ContextPos : Context {};

struct ChainContextPos : ChainContext {};

struct ExtensionPos : Extension<ExtensionPos>
{
  typedef struct PosLookupSubTable SubTable;
};




static void
reverse_cursive_minor_offset (hb_glyph_position_t *pos, unsigned int i, hb_direction_t direction, unsigned int new_parent)
{
  int chain = pos[i].attach_chain(), type = pos[i].attach_type();
  if (likely (!chain || 0 == (type & ATTACH_TYPE_CURSIVE)))
    return;

  pos[i].attach_chain() = 0;

  unsigned int j = (int) i + chain;

  /* Stop if we see new parent in the chain. */
  if (j == new_parent)
    return;

  reverse_cursive_minor_offset (pos, j, direction, new_parent);

  if (HB_DIRECTION_IS_HORIZONTAL (direction))
    pos[j].y_offset = -pos[i].y_offset;
  else
    pos[j].x_offset = -pos[i].x_offset;

  pos[j].attach_chain() = -chain;
  pos[j].attach_type() = type;
}
static void
propagate_attachment_offsets (hb_glyph_position_t *pos,
			      unsigned int len,
			      unsigned int i,
			      hb_direction_t direction,
			      unsigned nesting_level = HB_MAX_NESTING_LEVEL)
{
  /* Adjusts offsets of attached glyphs (both cursive and mark) to accumulate
   * offset of glyph they are attached to. */
  int chain = pos[i].attach_chain(), type = pos[i].attach_type();
  if (likely (!chain))
    return;

  pos[i].attach_chain() = 0;

  unsigned int j = (int) i + chain;

  if (unlikely (j >= len))
    return;

  if (unlikely (!nesting_level))
    return;

  propagate_attachment_offsets (pos, len, j, direction, nesting_level - 1);

  assert (!!(type & ATTACH_TYPE_MARK) ^ !!(type & ATTACH_TYPE_CURSIVE));

  if (type & ATTACH_TYPE_CURSIVE)
  {
    if (HB_DIRECTION_IS_HORIZONTAL (direction))
      pos[i].y_offset += pos[j].y_offset;
    else
      pos[i].x_offset += pos[j].x_offset;
  }
  else /*if (type & ATTACH_TYPE_MARK)*/
  {
    pos[i].x_offset += pos[j].x_offset;
    pos[i].y_offset += pos[j].y_offset;

    assert (j < i);
    if (HB_DIRECTION_IS_FORWARD (direction))
      for (unsigned int k = j; k < i; k++) {
	pos[i].x_offset -= pos[k].x_advance;
	pos[i].y_offset -= pos[k].y_advance;
      }
    else
      for (unsigned int k = j + 1; k < i + 1; k++) {
	pos[i].x_offset += pos[k].x_advance;
	pos[i].y_offset += pos[k].y_advance;
      }
  }
}

void
GPOS::position_start (hb_font_t *font HB_UNUSED, hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    buffer->pos[i].attach_chain() = buffer->pos[i].attach_type() = 0;
}

void
GPOS::position_finish_advances (hb_font_t *font HB_UNUSED, hb_buffer_t *buffer HB_UNUSED)
{
  //_hb_buffer_assert_gsubgpos_vars (buffer);
}

void
GPOS::position_finish_offsets (hb_font_t *font, hb_buffer_t *buffer)
{
  _hb_buffer_assert_gsubgpos_vars (buffer);

  unsigned int len;
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, &len);
  hb_direction_t direction = buffer->props.direction;

  /* Handle attachments */
  if (buffer->scratch_flags & HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT)
    for (unsigned i = 0; i < len; i++)
      propagate_attachment_offsets (pos, len, i, direction);

  if (unlikely (font->slant))
  {
    for (unsigned i = 0; i < len; i++)
      if (unlikely (pos[i].y_offset))
        pos[i].x_offset += _hb_roundf (font->slant_xy * pos[i].y_offset);
  }
}


struct GPOS_accelerator_t : GPOS::accelerator_t {
  GPOS_accelerator_t (hb_face_t *face) : GPOS::accelerator_t (face) {}
};


/* Out-of-class implementation for methods recursing */

#ifndef HB_NO_OT_LAYOUT
template <typename context_t>
/*static*/ typename context_t::return_t PosLookup::dispatch_recurse_func (context_t *c, unsigned int lookup_index)
{
  const PosLookup &l = c->face->table.GPOS.get_relaxed ()->table->get_lookup (lookup_index);
  return l.dispatch (c);
}

template <>
inline hb_closure_lookups_context_t::return_t
PosLookup::dispatch_recurse_func<hb_closure_lookups_context_t> (hb_closure_lookups_context_t *c, unsigned this_index)
{
  const PosLookup &l = c->face->table.GPOS.get_relaxed ()->table->get_lookup (this_index);
  return l.closure_lookups (c, this_index);
}

template <>
inline bool PosLookup::dispatch_recurse_func<hb_ot_apply_context_t> (hb_ot_apply_context_t *c, unsigned int lookup_index)
{
  const PosLookup &l = c->face->table.GPOS.get_relaxed ()->table->get_lookup (lookup_index);
  unsigned int saved_lookup_props = c->lookup_props;
  unsigned int saved_lookup_index = c->lookup_index;
  c->set_lookup_index (lookup_index);
  c->set_lookup_props (l.get_props ());
  bool ret = l.dispatch (c);
  c->set_lookup_index (saved_lookup_index);
  c->set_lookup_props (saved_lookup_props);
  return ret;
}
#endif


} /* namespace OT */


#endif /* HB_OT_LAYOUT_GPOS_TABLE_HH */
