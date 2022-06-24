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

using Layout::GPOS::PosLookup;
using OT::Layout::GPOS::ATTACH_TYPE_NONE;
using OT::Layout::GPOS::ATTACH_TYPE_MARK;
using OT::Layout::GPOS::ATTACH_TYPE_CURSIVE;

// struct MarkArray;
// static void Markclass_closure_and_remap_indexes (const Coverage  &mark_coverage,
// 						 const MarkArray &mark_array,
// 						 const hb_set_t  &glyphset,
// 						 hb_map_t*        klass_mapping /* INOUT */);


/* Shared Tables: ValueRecord, Anchor Table, and MarkArray */

/* Lookups */


static void
reverse_cursive_minor_offset (hb_glyph_position_t *pos, unsigned int i, hb_direction_t direction, unsigned int new_parent)
{
  int chain = pos[i].attach_chain(), type = pos[i].attach_type();
  if (likely (!chain || 0 == (type & OT::Layout::GPOS::ATTACH_TYPE_CURSIVE)))
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

// TODO(garretrieger): Move into new layout directory.
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
