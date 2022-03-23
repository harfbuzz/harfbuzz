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

#ifndef HB_OT_LAYOUT_GSUB_TABLE_HH
#define HB_OT_LAYOUT_GSUB_TABLE_HH

#include "hb-ot-layout-gsubgpos.hh"
#include "OT/Layout/GSUB/Common.hh"
#include "OT/Layout/GSUB/SingleSubst.hh"
#include "OT/Layout/GSUB/MultipleSubst.hh"
#include "OT/Layout/GSUB/AlternateSubst.hh"
#include "OT/Layout/GSUB/LigatureSubst.hh"

namespace OT {

using Layout::GSUB::hb_codepoint_pair_t;
using Layout::GSUB::SingleSubst;
using Layout::GSUB::MultipleSubst;
using Layout::GSUB::AlternateSubst;
using Layout::GSUB::LigatureSubst;

struct ContextSubst : Context {};

struct ChainContextSubst : ChainContext {};

struct ExtensionSubst : Extension<ExtensionSubst>
{
  typedef struct SubstLookupSubTable SubTable;
  bool is_reverse () const;
};


struct ReverseChainSingleSubstFormat1
{
  bool intersects (const hb_set_t *glyphs) const
  {
    if (!(this+coverage).intersects (glyphs))
      return false;

    const Array16OfOffset16To<Coverage> &lookahead = StructAfter<Array16OfOffset16To<Coverage>> (backtrack);

    unsigned int count;

    count = backtrack.len;
    for (unsigned int i = 0; i < count; i++)
      if (!(this+backtrack[i]).intersects (glyphs))
	return false;

    count = lookahead.len;
    for (unsigned int i = 0; i < count; i++)
      if (!(this+lookahead[i]).intersects (glyphs))
	return false;

    return true;
  }

  bool may_have_non_1to1 () const
  { return false; }

  void closure (hb_closure_context_t *c) const
  {
    if (!intersects (c->glyphs)) return;

    const Array16OfOffset16To<Coverage> &lookahead = StructAfter<Array16OfOffset16To<Coverage>> (backtrack);
    const Array16Of<HBGlyphID16> &substitute = StructAfter<Array16Of<HBGlyphID16>> (lookahead);

    + hb_zip (this+coverage, substitute)
    | hb_filter (c->parent_active_glyphs (), hb_first)
    | hb_map (hb_second)
    | hb_sink (c->output)
    ;
  }

  void closure_lookups (hb_closure_lookups_context_t *c) const {}

  void collect_glyphs (hb_collect_glyphs_context_t *c) const
  {
    if (unlikely (!(this+coverage).collect_coverage (c->input))) return;

    unsigned int count;

    count = backtrack.len;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!(this+backtrack[i]).collect_coverage (c->before))) return;

    const Array16OfOffset16To<Coverage> &lookahead = StructAfter<Array16OfOffset16To<Coverage>> (backtrack);
    count = lookahead.len;
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!(this+lookahead[i]).collect_coverage (c->after))) return;

    const Array16Of<HBGlyphID16> &substitute = StructAfter<Array16Of<HBGlyphID16>> (lookahead);
    count = substitute.len;
    c->output->add_array (substitute.arrayZ, substitute.len);
  }

  const Coverage &get_coverage () const { return this+coverage; }

  bool would_apply (hb_would_apply_context_t *c) const
  { return c->len == 1 && (this+coverage).get_coverage (c->glyphs[0]) != NOT_COVERED; }

  bool apply (hb_ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    if (unlikely (c->nesting_level_left != HB_MAX_NESTING_LEVEL))
      return_trace (false); /* No chaining to this type */

    unsigned int index = (this+coverage).get_coverage (c->buffer->cur ().codepoint);
    if (likely (index == NOT_COVERED)) return_trace (false);

    const Array16OfOffset16To<Coverage> &lookahead = StructAfter<Array16OfOffset16To<Coverage>> (backtrack);
    const Array16Of<HBGlyphID16> &substitute = StructAfter<Array16Of<HBGlyphID16>> (lookahead);

    if (unlikely (index >= substitute.len)) return_trace (false);

    unsigned int start_index = 0, end_index = 0;
    if (match_backtrack (c,
			 backtrack.len, (HBUINT16 *) backtrack.arrayZ,
			 match_coverage, this,
			 &start_index) &&
	match_lookahead (c,
			 lookahead.len, (HBUINT16 *) lookahead.arrayZ,
			 match_coverage, this,
			 c->buffer->idx + 1, &end_index))
    {
      c->buffer->unsafe_to_break_from_outbuffer (start_index, end_index);
      c->replace_glyph_inplace (substitute[index]);
      /* Note: We DON'T decrease buffer->idx.  The main loop does it
       * for us.  This is useful for preventing surprises if someone
       * calls us through a Context lookup. */
      return_trace (true);
    }
    else
    {
      c->buffer->unsafe_to_concat_from_outbuffer (start_index, end_index);
      return_trace (false);
    }
  }

  template<typename Iterator,
           hb_requires (hb_is_iterator (Iterator))>
  bool serialize_coverage_offset_array (hb_subset_context_t *c, Iterator it) const
  {
    TRACE_SERIALIZE (this);
    auto *out = c->serializer->start_embed<Array16OfOffset16To<Coverage>> ();

    if (unlikely (!c->serializer->allocate_size<HBUINT16> (HBUINT16::static_size)))
      return_trace (false);

    for (auto& offset : it) {
      auto *o = out->serialize_append (c->serializer);
      if (unlikely (!o) || !o->serialize_subset (c, offset, this))
        return_trace (false);
    }

    return_trace (true);
  }

  template<typename Iterator, typename BacktrackIterator, typename LookaheadIterator,
           hb_requires (hb_is_sorted_source_of (Iterator, hb_codepoint_pair_t)),
           hb_requires (hb_is_iterator (BacktrackIterator)),
           hb_requires (hb_is_iterator (LookaheadIterator))>
  bool serialize (hb_subset_context_t *c,
                  Iterator coverage_subst_iter,
                  BacktrackIterator backtrack_iter,
                  LookaheadIterator lookahead_iter) const
  {
    TRACE_SERIALIZE (this);

    auto *out = c->serializer->start_embed (this);
    if (unlikely (!c->serializer->check_success (out))) return_trace (false);
    if (unlikely (!c->serializer->embed (this->format))) return_trace (false);
    if (unlikely (!c->serializer->embed (this->coverage))) return_trace (false);

    if (!serialize_coverage_offset_array (c, backtrack_iter)) return_trace (false);
    if (!serialize_coverage_offset_array (c, lookahead_iter)) return_trace (false);

    auto *substitute_out = c->serializer->start_embed<Array16Of<HBGlyphID16>> ();
    auto substitutes =
    + coverage_subst_iter
    | hb_map (hb_second)
    ;

    auto glyphs =
    + coverage_subst_iter
    | hb_map_retains_sorting (hb_first)
    ;
    if (unlikely (! c->serializer->check_success (substitute_out->serialize (c->serializer, substitutes))))
        return_trace (false);

    if (unlikely (!out->coverage.serialize_serialize (c->serializer, glyphs)))
      return_trace (false);
    return_trace (true);
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();
    const hb_map_t &glyph_map = *c->plan->glyph_map;

    const Array16OfOffset16To<Coverage> &lookahead = StructAfter<Array16OfOffset16To<Coverage>> (backtrack);
    const Array16Of<HBGlyphID16> &substitute = StructAfter<Array16Of<HBGlyphID16>> (lookahead);

    auto it =
    + hb_zip (this+coverage, substitute)
    | hb_filter (glyphset, hb_first)
    | hb_filter (glyphset, hb_second)
    | hb_map_retains_sorting ([&] (hb_pair_t<hb_codepoint_t, const HBGlyphID16 &> p) -> hb_codepoint_pair_t
                              { return hb_pair (glyph_map[p.first], glyph_map[p.second]); })
    ;

    return_trace (bool (it) && serialize (c, it, backtrack.iter (), lookahead.iter ()));
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!(coverage.sanitize (c, this) && backtrack.sanitize (c, this)))
      return_trace (false);
    const Array16OfOffset16To<Coverage> &lookahead = StructAfter<Array16OfOffset16To<Coverage>> (backtrack);
    if (!lookahead.sanitize (c, this))
      return_trace (false);
    const Array16Of<HBGlyphID16> &substitute = StructAfter<Array16Of<HBGlyphID16>> (lookahead);
    return_trace (substitute.sanitize (c));
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 1 */
  Offset16To<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  Array16OfOffset16To<Coverage>
		backtrack;		/* Array of coverage tables
					 * in backtracking sequence, in glyph
					 * sequence order */
  Array16OfOffset16To<Coverage>
		lookaheadX;		/* Array of coverage tables
					 * in lookahead sequence, in glyph
					 * sequence order */
  Array16Of<HBGlyphID16>
		substituteX;		/* Array of substitute
					 * GlyphIDs--ordered by Coverage Index */
  public:
  DEFINE_SIZE_MIN (10);
};

struct ReverseChainSingleSubst
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
  HBUINT16				format;		/* Format identifier */
  ReverseChainSingleSubstFormat1	format1;
  } u;
};



/*
 * SubstLookup
 */

struct SubstLookupSubTable
{
  friend struct Lookup;
  friend struct SubstLookup;

  enum Type {
    Single		= 1,
    Multiple		= 2,
    Alternate		= 3,
    Ligature		= 4,
    Context		= 5,
    ChainContext	= 6,
    Extension		= 7,
    ReverseChainSingle	= 8
  };

  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, unsigned int lookup_type, Ts&&... ds) const
  {
    TRACE_DISPATCH (this, lookup_type);
    switch (lookup_type) {
    case Single:		return_trace (u.single.dispatch (c, std::forward<Ts> (ds)...));
    case Multiple:		return_trace (u.multiple.dispatch (c, std::forward<Ts> (ds)...));
    case Alternate:		return_trace (u.alternate.dispatch (c, std::forward<Ts> (ds)...));
    case Ligature:		return_trace (u.ligature.dispatch (c, std::forward<Ts> (ds)...));
    case Context:		return_trace (u.context.dispatch (c, std::forward<Ts> (ds)...));
    case ChainContext:		return_trace (u.chainContext.dispatch (c, std::forward<Ts> (ds)...));
    case Extension:		return_trace (u.extension.dispatch (c, std::forward<Ts> (ds)...));
    case ReverseChainSingle:	return_trace (u.reverseChainContextSingle.dispatch (c, std::forward<Ts> (ds)...));
    default:			return_trace (c->default_return_value ());
    }
  }

  bool intersects (const hb_set_t *glyphs, unsigned int lookup_type) const
  {
    hb_intersects_context_t c (glyphs);
    return dispatch (&c, lookup_type);
  }

  protected:
  union {
  SingleSubst			single;
  MultipleSubst			multiple;
  AlternateSubst		alternate;
  LigatureSubst			ligature;
  ContextSubst			context;
  ChainContextSubst		chainContext;
  ExtensionSubst		extension;
  ReverseChainSingleSubst	reverseChainContextSingle;
  } u;
  public:
  DEFINE_SIZE_MIN (0);
};


struct SubstLookup : Lookup
{
  typedef SubstLookupSubTable SubTable;

  const SubTable& get_subtable (unsigned int i) const
  { return Lookup::get_subtable<SubTable> (i); }

  static inline bool lookup_type_is_reverse (unsigned int lookup_type)
  { return lookup_type == SubTable::ReverseChainSingle; }

  bool is_reverse () const
  {
    unsigned int type = get_type ();
    if (unlikely (type == SubTable::Extension))
      return reinterpret_cast<const ExtensionSubst &> (get_subtable (0)).is_reverse ();
    return lookup_type_is_reverse (type);
  }

  bool may_have_non_1to1 () const
  {
    hb_have_non_1to1_context_t c;
    return dispatch (&c);
  }

  bool apply (hb_ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    return_trace (dispatch (c));
  }

  bool intersects (const hb_set_t *glyphs) const
  {
    hb_intersects_context_t c (glyphs);
    return dispatch (&c);
  }

  hb_closure_context_t::return_t closure (hb_closure_context_t *c, unsigned int this_index) const
  {
    if (!c->should_visit_lookup (this_index))
      return hb_closure_context_t::default_return_value ();

    c->set_recurse_func (dispatch_closure_recurse_func);

    hb_closure_context_t::return_t ret = dispatch (c);

    c->flush ();

    return ret;
  }

  hb_closure_lookups_context_t::return_t closure_lookups (hb_closure_lookups_context_t *c, unsigned this_index) const
  {
    if (c->is_lookup_visited (this_index))
      return hb_closure_lookups_context_t::default_return_value ();

    c->set_lookup_visited (this_index);
    if (!intersects (c->glyphs))
    {
      c->set_lookup_inactive (this_index);
      return hb_closure_lookups_context_t::default_return_value ();
    }

    c->set_recurse_func (dispatch_closure_lookups_recurse_func);

    hb_closure_lookups_context_t::return_t ret = dispatch (c);
    return ret;
  }

  hb_collect_glyphs_context_t::return_t collect_glyphs (hb_collect_glyphs_context_t *c) const
  {
    c->set_recurse_func (dispatch_recurse_func<hb_collect_glyphs_context_t>);
    return dispatch (c);
  }

  template <typename set_t>
  void collect_coverage (set_t *glyphs) const
  {
    hb_collect_coverage_context_t<set_t> c (glyphs);
    dispatch (&c);
  }

  bool would_apply (hb_would_apply_context_t *c,
		    const hb_ot_layout_lookup_accelerator_t *accel) const
  {
    if (unlikely (!c->len)) return false;
    if (!accel->may_have (c->glyphs[0])) return false;
      return dispatch (c);
  }

  static inline bool apply_recurse_func (hb_ot_apply_context_t *c, unsigned int lookup_index);

  bool serialize_single (hb_serialize_context_t *c,
			 uint32_t lookup_props,
			 hb_sorted_array_t<const HBGlyphID16> glyphs,
			 hb_array_t<const HBGlyphID16> substitutes)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!Lookup::serialize (c, SubTable::Single, lookup_props, 1))) return_trace (false);
    if (c->push<SubTable> ()->u.single.serialize (c, hb_zip (glyphs, substitutes)))
    {
      c->add_link (get_subtables<SubTable> ()[0], c->pop_pack ());
      return_trace (true);
    }
    c->pop_discard ();
    return_trace (false);
  }

  bool serialize_multiple (hb_serialize_context_t *c,
			   uint32_t lookup_props,
			   hb_sorted_array_t<const HBGlyphID16> glyphs,
			   hb_array_t<const unsigned int> substitute_len_list,
			   hb_array_t<const HBGlyphID16> substitute_glyphs_list)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!Lookup::serialize (c, SubTable::Multiple, lookup_props, 1))) return_trace (false);
    if (c->push<SubTable> ()->u.multiple.
        serialize (c,
                   glyphs,
                   substitute_len_list,
                   substitute_glyphs_list))
    {
      c->add_link (get_subtables<SubTable> ()[0], c->pop_pack ());
      return_trace (true);
    }
    c->pop_discard ();
    return_trace (false);
  }

  bool serialize_alternate (hb_serialize_context_t *c,
			    uint32_t lookup_props,
			    hb_sorted_array_t<const HBGlyphID16> glyphs,
			    hb_array_t<const unsigned int> alternate_len_list,
			    hb_array_t<const HBGlyphID16> alternate_glyphs_list)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!Lookup::serialize (c, SubTable::Alternate, lookup_props, 1))) return_trace (false);

    if (c->push<SubTable> ()->u.alternate.
        serialize (c,
                   glyphs,
                   alternate_len_list,
                   alternate_glyphs_list))
    {
      c->add_link (get_subtables<SubTable> ()[0], c->pop_pack ());
      return_trace (true);
    }
    c->pop_discard ();
    return_trace (false);
  }

  bool serialize_ligature (hb_serialize_context_t *c,
			   uint32_t lookup_props,
			   hb_sorted_array_t<const HBGlyphID16> first_glyphs,
			   hb_array_t<const unsigned int> ligature_per_first_glyph_count_list,
			   hb_array_t<const HBGlyphID16> ligatures_list,
			   hb_array_t<const unsigned int> component_count_list,
			   hb_array_t<const HBGlyphID16> component_list /* Starting from second for each ligature */)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!Lookup::serialize (c, SubTable::Ligature, lookup_props, 1))) return_trace (false);
    if (c->push<SubTable> ()->u.ligature.
        serialize (c,
                   first_glyphs,
                   ligature_per_first_glyph_count_list,
                   ligatures_list,
                   component_count_list,
                   component_list))
    {
      c->add_link (get_subtables<SubTable> ()[0], c->pop_pack ());
      return_trace (true);
    }
    c->pop_discard ();
    return_trace (false);
  }

  template <typename context_t>
  static inline typename context_t::return_t dispatch_recurse_func (context_t *c, unsigned int lookup_index);

  static inline typename hb_closure_context_t::return_t closure_glyphs_recurse_func (hb_closure_context_t *c, unsigned lookup_index, hb_set_t *covered_seq_indices, unsigned seq_index, unsigned end_index);

  static inline hb_closure_context_t::return_t dispatch_closure_recurse_func (hb_closure_context_t *c, unsigned lookup_index, hb_set_t *covered_seq_indices, unsigned seq_index, unsigned end_index)
  {
    if (!c->should_visit_lookup (lookup_index))
      return hb_empty_t ();

    hb_closure_context_t::return_t ret = closure_glyphs_recurse_func (c, lookup_index, covered_seq_indices, seq_index, end_index);

    /* While in theory we should flush here, it will cause timeouts because a recursive
     * lookup can keep growing the glyph set.  Skip, and outer loop will retry up to
     * HB_CLOSURE_MAX_STAGES time, which should be enough for every realistic font. */
    //c->flush ();

    return ret;
  }

  HB_INTERNAL static hb_closure_lookups_context_t::return_t dispatch_closure_lookups_recurse_func (hb_closure_lookups_context_t *c, unsigned lookup_index);

  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  { return Lookup::dispatch<SubTable> (c, std::forward<Ts> (ds)...); }

  bool subset (hb_subset_context_t *c) const
  { return Lookup::subset<SubTable> (c); }

  bool sanitize (hb_sanitize_context_t *c) const
  { return Lookup::sanitize<SubTable> (c); }
};

/*
 * GSUB -- Glyph Substitution
 * https://docs.microsoft.com/en-us/typography/opentype/spec/gsub
 */

struct GSUB : GSUBGPOS
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_GSUB;

  const SubstLookup& get_lookup (unsigned int i) const
  { return static_cast<const SubstLookup &> (GSUBGPOS::get_lookup (i)); }

  bool subset (hb_subset_context_t *c) const
  {
    hb_subset_layout_context_t l (c, tableTag, c->plan->gsub_lookups, c->plan->gsub_langsys, c->plan->gsub_features);
    return GSUBGPOS::subset<SubstLookup> (&l);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  { return GSUBGPOS::sanitize<SubstLookup> (c); }

  HB_INTERNAL bool is_blocklisted (hb_blob_t *blob,
				   hb_face_t *face) const;

  void closure_lookups (hb_face_t      *face,
			const hb_set_t *glyphs,
			hb_set_t       *lookup_indexes /* IN/OUT */) const
  { GSUBGPOS::closure_lookups<SubstLookup> (face, glyphs, lookup_indexes); }

  typedef GSUBGPOS::accelerator_t<GSUB> accelerator_t;
};


struct GSUB_accelerator_t : GSUB::accelerator_t {
  GSUB_accelerator_t (hb_face_t *face) : GSUB::accelerator_t (face) {}
};


/* Out-of-class implementation for methods recursing */

#ifndef HB_NO_OT_LAYOUT
/*static*/ inline bool ExtensionSubst::is_reverse () const
{
  return SubstLookup::lookup_type_is_reverse (get_type ());
}
template <typename context_t>
/*static*/ typename context_t::return_t SubstLookup::dispatch_recurse_func (context_t *c, unsigned int lookup_index)
{
  const SubstLookup &l = c->face->table.GSUB.get_relaxed ()->table->get_lookup (lookup_index);
  return l.dispatch (c);
}

/*static*/ typename hb_closure_context_t::return_t SubstLookup::closure_glyphs_recurse_func (hb_closure_context_t *c, unsigned lookup_index, hb_set_t *covered_seq_indices, unsigned seq_index, unsigned end_index)
{
  const SubstLookup &l = c->face->table.GSUB.get_relaxed ()->table->get_lookup (lookup_index);
  if (l.may_have_non_1to1 ())
      hb_set_add_range (covered_seq_indices, seq_index, end_index);
  return l.dispatch (c);
}

/*static*/ inline hb_closure_lookups_context_t::return_t SubstLookup::dispatch_closure_lookups_recurse_func (hb_closure_lookups_context_t *c, unsigned this_index)
{
  const SubstLookup &l = c->face->table.GSUB.get_relaxed ()->table->get_lookup (this_index);
  return l.closure_lookups (c, this_index);
}

/*static*/ bool SubstLookup::apply_recurse_func (hb_ot_apply_context_t *c, unsigned int lookup_index)
{
  const SubstLookup &l = c->face->table.GSUB.get_relaxed ()->table->get_lookup (lookup_index);
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


#endif /* HB_OT_LAYOUT_GSUB_TABLE_HH */
