/*
 * Copyright © 2007,2008,2009,2010  Red Hat, Inc.
 * Copyright © 2010  Google, Inc.
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

#include "hb-ot-layout-gsubgpos-private.hh"



struct SingleSubstFormat1
{
  friend struct SingleSubst;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    bool ret = false;
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      hb_codepoint_t glyph_id = iter.get_glyph ();
      if (c->glyphs->has (glyph_id))
	ret = c->glyphs->add ((glyph_id + deltaGlyphID) & 0xFFFF) || ret;
    }
    return ret;
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    return (this+coverage) (glyph_id) != NOT_COVERED;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->idx].codepoint;
    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    /* According to the Adobe Annotated OpenType Suite, result is always
     * limited to 16bit. */
    glyph_id = (glyph_id + deltaGlyphID) & 0xFFFF;
    c->replace_glyph (glyph_id);

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& deltaGlyphID.sanitize (c);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  SHORT		deltaGlyphID;		/* Add to original GlyphID to get
					 * substitute GlyphID */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct SingleSubstFormat2
{
  friend struct SingleSubst;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    bool ret = false;
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	ret = c->glyphs->add (substitute[iter.get_coverage ()]) || ret;
    }
    return ret;
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    return (this+coverage) (glyph_id) != NOT_COVERED;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->idx].codepoint;
    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    if (unlikely (index >= substitute.len))
      return false;

    glyph_id = substitute[index];
    c->replace_glyph (glyph_id);

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& substitute.sanitize (c);
  }

  private:
  USHORT	format;			/* Format identifier--format = 2 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  ArrayOf<GlyphID>
		substitute;		/* Array of substitute
					 * GlyphIDs--ordered by Coverage Index */
  public:
  DEFINE_SIZE_ARRAY (6, substitute);
};

struct SingleSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: return u.format1.closure (c);
    case 2: return u.format2.closure (c);
    default:return false;
    }
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1.would_apply (glyph_id);
    case 2: return u.format2.would_apply (glyph_id);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    case 2: return u.format2.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    case 2: return u.format2.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  SingleSubstFormat1	format1;
  SingleSubstFormat2	format2;
  } u;
};


struct Sequence
{
  friend struct MultipleSubstFormat1;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    unsigned int count = substitute.len;
    bool ret = false;
    for (unsigned int i = 0; i < count; i++)
      ret = c->glyphs->add (substitute[i]) || ret;
    return ret;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    if (unlikely (!substitute.len))
      return false;

    if (c->property & HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE)
      c->guess_glyph_class (HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH);
    c->replace_glyphs_be16 (1, substitute.len, (const uint16_t *) substitute.array);

    return true;
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return substitute.sanitize (c);
  }

  private:
  ArrayOf<GlyphID>
		substitute;		/* String of GlyphIDs to substitute */
  public:
  DEFINE_SIZE_ARRAY (2, substitute);
};

struct MultipleSubstFormat1
{
  friend struct MultipleSubst;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    bool ret = false;
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	ret = (this+sequence[iter.get_coverage ()]).closure (c) || ret;
    }
    return ret;
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    return (this+coverage) (glyph_id) != NOT_COVERED;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();

    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    return (this+sequence[index]).apply (c);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& sequence.sanitize (c, this);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetArrayOf<Sequence>
		sequence;		/* Array of Sequence tables
					 * ordered by Coverage Index */
  public:
  DEFINE_SIZE_ARRAY (6, sequence);
};

struct MultipleSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: return u.format1.closure (c);
    default:return false;
    }
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1.would_apply (glyph_id);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  MultipleSubstFormat1	format1;
  } u;
};


typedef ArrayOf<GlyphID> AlternateSet;	/* Array of alternate GlyphIDs--in
					 * arbitrary order */

struct AlternateSubstFormat1
{
  friend struct AlternateSubst;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    bool ret = false;
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ())) {
	const AlternateSet &alt_set = this+alternateSet[iter.get_coverage ()];
	unsigned int count = alt_set.len;
	for (unsigned int i = 0; i < count; i++)
	  ret = c->glyphs->add (alt_set[i]) || ret;
      }
    }
    return ret;
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    return (this+coverage) (glyph_id) != NOT_COVERED;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->idx].codepoint;

    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    const AlternateSet &alt_set = this+alternateSet[index];

    if (unlikely (!alt_set.len))
      return false;

    hb_mask_t glyph_mask = c->buffer->info[c->buffer->idx].mask;
    hb_mask_t lookup_mask = c->lookup_mask;

    /* Note: This breaks badly if two features enabled this lookup together. */
    unsigned int shift = _hb_ctz (lookup_mask);
    unsigned int alt_index = ((lookup_mask & glyph_mask) >> shift);

    if (unlikely (alt_index > alt_set.len || alt_index == 0))
      return false;

    glyph_id = alt_set[alt_index - 1];

    c->replace_glyph (glyph_id);

    return true;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& alternateSet.sanitize (c, this);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetArrayOf<AlternateSet>
		alternateSet;		/* Array of AlternateSet tables
					 * ordered by Coverage Index */
  public:
  DEFINE_SIZE_ARRAY (6, alternateSet);
};

struct AlternateSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: return u.format1.closure (c);
    default:return false;
    }
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    switch (u.format) {
    case 1: return u.format1.would_apply (glyph_id);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  AlternateSubstFormat1	format1;
  } u;
};


struct Ligature
{
  friend struct LigatureSet;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    unsigned int count = component.len;
    for (unsigned int i = 1; i < count; i++)
      if (!c->glyphs->has (component[i]))
        return false;
    return c->glyphs->add (ligGlyph);
  }

  inline bool would_apply (hb_codepoint_t second) const
  {
    return component.len == 2 && component[1] == second;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int count = component.len;
    if (unlikely (count < 2))
      return false;

    hb_apply_context_t::mark_skipping_forward_iterator_t skippy_iter (c, c->buffer->idx, count - 1);
    if (skippy_iter.has_no_chance ())
      return false;

    bool first_was_mark = (c->property & HB_OT_LAYOUT_GLYPH_CLASS_MARK);
    bool found_non_mark = false;

    for (unsigned int i = 1; i < count; i++)
    {
      unsigned int property;

      if (!skippy_iter.next (&property))
	return false;

      found_non_mark |= !(property & HB_OT_LAYOUT_GLYPH_CLASS_MARK);

      if (likely (c->buffer->info[skippy_iter.idx].codepoint != component[i]))
        return false;
    }

    if (first_was_mark && found_non_mark)
      c->guess_glyph_class (HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE);

    /* Allocate new ligature id */
    unsigned int lig_id = allocate_lig_id (c->buffer);
    c->buffer->info[c->buffer->idx].lig_comp() = 0;
    c->buffer->info[c->buffer->idx].lig_id() = lig_id;

    if (skippy_iter.idx < c->buffer->idx + count) /* No input glyphs skipped */
    {
      c->replace_glyphs_be16 (count, 1, (const uint16_t *) &ligGlyph);
    }
    else
    {
      c->replace_glyph (ligGlyph);

      /* Now we must do a second loop to copy the skipped glyphs to
	 `out' and assign component values to it.  We start with the
	 glyph after the first component.  Glyphs between component
	 i and i+1 belong to component i.  Together with the lig_id
	 value it is later possible to check whether a specific
	 component value really belongs to a given ligature. */

      for (unsigned int i = 1; i < count; i++)
      {
	while (c->should_mark_skip_current_glyph ())
	{
	  c->buffer->info[c->buffer->idx].lig_comp() = i;
	  c->buffer->info[c->buffer->idx].lig_id() = lig_id;
	  c->replace_glyph (c->buffer->info[c->buffer->idx].codepoint);
	}

	/* Skip the base glyph */
	c->buffer->idx++;
      }
    }

    return true;
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return ligGlyph.sanitize (c)
        && component.sanitize (c);
  }

  private:
  GlyphID	ligGlyph;		/* GlyphID of ligature to substitute */
  HeadlessArrayOf<GlyphID>
		component;		/* Array of component GlyphIDs--start
					 * with the second  component--ordered
					 * in writing direction */
  public:
  DEFINE_SIZE_ARRAY (4, component);
};

struct LigatureSet
{
  friend struct LigatureSubstFormat1;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    bool ret = false;
    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++)
    {
      const Ligature &lig = this+ligature[i];
      ret = lig.closure (c) || ret;
    }
    return ret;
  }

  inline bool would_apply (hb_codepoint_t second) const
  {
    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++)
    {
      const Ligature &lig = this+ligature[i];
      if (lig.would_apply (second))
        return true;
    }
    return false;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++)
    {
      const Ligature &lig = this+ligature[i];
      if (lig.apply (c))
        return true;
    }

    return false;
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return ligature.sanitize (c, this);
  }

  private:
  OffsetArrayOf<Ligature>
		ligature;		/* Array LigatureSet tables
					 * ordered by preference */
  public:
  DEFINE_SIZE_ARRAY (2, ligature);
};

struct LigatureSubstFormat1
{
  friend struct LigatureSubst;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    bool ret = false;
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	ret = (this+ligatureSet[iter.get_coverage ()]).closure (c) || ret;
    }
    return ret;
    return false;
  }

  inline bool would_apply (hb_codepoint_t first, hb_codepoint_t second) const
  {
    unsigned int index;
    return (index = (this+coverage) (first)) != NOT_COVERED &&
	   (this+ligatureSet[index]).would_apply (second);
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    hb_codepoint_t glyph_id = c->buffer->info[c->buffer->idx].codepoint;

    unsigned int index = (this+coverage) (glyph_id);
    if (likely (index == NOT_COVERED))
      return false;

    const LigatureSet &lig_set = this+ligatureSet[index];
    return lig_set.apply (c);
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    return coverage.sanitize (c, this)
	&& ligatureSet.sanitize (c, this);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetArrayOf<LigatureSet>
		ligatureSet;		/* Array LigatureSet tables
					 * ordered by Coverage Index */
  public:
  DEFINE_SIZE_ARRAY (6, ligatureSet);
};

struct LigatureSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: return u.format1.closure (c);
    default:return false;
    }
  }

  inline bool would_apply (hb_codepoint_t first, hb_codepoint_t second) const
  {
    switch (u.format) {
    case 1: return u.format1.would_apply (first, second);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  LigatureSubstFormat1	format1;
  } u;
};


static inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index);
static inline bool closure_lookup (hb_closure_context_t *c, unsigned int lookup_index);

struct ContextSubst : Context
{
  friend struct SubstLookupSubTable;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    return Context::closure (c, closure_lookup);
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return Context::apply (c, substitute_lookup);
  }
};

struct ChainContextSubst : ChainContext
{
  friend struct SubstLookupSubTable;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    return ChainContext::closure (c, closure_lookup);
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    return ChainContext::apply (c, substitute_lookup);
  }
};


struct ExtensionSubst : Extension
{
  friend struct SubstLookupSubTable;
  friend struct SubstLookup;

  private:
  inline const struct SubstLookupSubTable& get_subtable (void) const
  {
    unsigned int offset = get_offset ();
    if (unlikely (!offset)) return Null(SubstLookupSubTable);
    return StructAtOffset<SubstLookupSubTable> (this, offset);
  }

  inline bool closure (hb_closure_context_t *c) const;
  inline bool would_apply (hb_codepoint_t glyph_id) const;
  inline bool would_apply (hb_codepoint_t first, hb_codepoint_t second) const;

  inline bool apply (hb_apply_context_t *c) const;

  inline bool sanitize (hb_sanitize_context_t *c);

  inline bool is_reverse (void) const;
};


struct ReverseChainSingleSubstFormat1
{
  friend struct ReverseChainSingleSubst;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (backtrack);

    unsigned int count;

    count = backtrack.len;
    for (unsigned int i = 0; i < count; i++)
      if (!(this+backtrack[i]).intersects (c->glyphs))
        return false;

    count = lookahead.len;
    for (unsigned int i = 0; i < count; i++)
      if (!(this+lookahead[i]).intersects (c->glyphs))
        return false;

    const ArrayOf<GlyphID> &substitute = StructAfter<ArrayOf<GlyphID> > (lookahead);
    bool ret = false;
    Coverage::Iter iter;
    for (iter.init (this+coverage); iter.more (); iter.next ()) {
      if (c->glyphs->has (iter.get_glyph ()))
	ret = c->glyphs->add (substitute[iter.get_coverage ()]) || ret;
    }
    return false;
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    if (unlikely (c->context_length != NO_CONTEXT))
      return false; /* No chaining to this type */

    unsigned int index = (this+coverage) (c->buffer->info[c->buffer->idx].codepoint);
    if (likely (index == NOT_COVERED))
      return false;

    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    const ArrayOf<GlyphID> &substitute = StructAfter<ArrayOf<GlyphID> > (lookahead);

    if (match_backtrack (c,
			 backtrack.len, (USHORT *) backtrack.array,
			 match_coverage, this) &&
        match_lookahead (c,
			 lookahead.len, (USHORT *) lookahead.array,
			 match_coverage, this,
			 1))
    {
      c->buffer->info[c->buffer->idx].codepoint = substitute[index];
      c->buffer->idx--; /* Reverse! */
      return true;
    }

    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!(coverage.sanitize (c, this)
       && backtrack.sanitize (c, this)))
      return false;
    OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    if (!lookahead.sanitize (c, this))
      return false;
    ArrayOf<GlyphID> &substitute = StructAfter<ArrayOf<GlyphID> > (lookahead);
    return substitute.sanitize (c);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  OffsetArrayOf<Coverage>
		backtrack;		/* Array of coverage tables
					 * in backtracking sequence, in  glyph
					 * sequence order */
  OffsetArrayOf<Coverage>
		lookaheadX;		/* Array of coverage tables
					 * in lookahead sequence, in glyph
					 * sequence order */
  ArrayOf<GlyphID>
		substituteX;		/* Array of substitute
					 * GlyphIDs--ordered by Coverage Index */
  public:
  DEFINE_SIZE_MIN (10);
};

struct ReverseChainSingleSubst
{
  friend struct SubstLookupSubTable;

  private:

  inline bool closure (hb_closure_context_t *c) const
  {
    TRACE_CLOSURE ();
    switch (u.format) {
    case 1: return u.format1.closure (c);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (!u.format.sanitize (c)) return false;
    switch (u.format) {
    case 1: return u.format1.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT				format;		/* Format identifier */
  ReverseChainSingleSubstFormat1	format1;
  } u;
};



/*
 * SubstLookup
 */

struct SubstLookupSubTable
{
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

  inline bool closure (hb_closure_context_t *c,
		       unsigned int    lookup_type) const
  {
    TRACE_CLOSURE ();
    switch (lookup_type) {
    case Single:		return u.single.closure (c);
    case Multiple:		return u.multiple.closure (c);
    case Alternate:		return u.alternate.closure (c);
    case Ligature:		return u.ligature.closure (c);
    case Context:		return u.c.closure (c);
    case ChainContext:		return u.chainContext.closure (c);
    case Extension:		return u.extension.closure (c);
    case ReverseChainSingle:	return u.reverseChainContextSingle.closure (c);
    default:return false;
    }
  }

  inline bool would_apply (hb_codepoint_t glyph_id,
			   unsigned int lookup_type) const
  {
    switch (lookup_type) {
    case Single:		return u.single.would_apply (glyph_id);
    case Multiple:		return u.multiple.would_apply (glyph_id);
    case Alternate:		return u.alternate.would_apply (glyph_id);
    case Extension:		return u.extension.would_apply (glyph_id);
    default:return false;
    }
  }
  inline bool would_apply (hb_codepoint_t first,
			   hb_codepoint_t second,
			   unsigned int lookup_type) const
  {
    switch (lookup_type) {
    case Ligature:		return u.ligature.would_apply (first, second);
    case Extension:		return u.extension.would_apply (first, second);
    default:return false;
    }
  }

  inline bool apply (hb_apply_context_t *c, unsigned int lookup_type) const
  {
    TRACE_APPLY ();
    switch (lookup_type) {
    case Single:		return u.single.apply (c);
    case Multiple:		return u.multiple.apply (c);
    case Alternate:		return u.alternate.apply (c);
    case Ligature:		return u.ligature.apply (c);
    case Context:		return u.c.apply (c);
    case ChainContext:		return u.chainContext.apply (c);
    case Extension:		return u.extension.apply (c);
    case ReverseChainSingle:	return u.reverseChainContextSingle.apply (c);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *c, unsigned int lookup_type) {
    TRACE_SANITIZE ();
    switch (lookup_type) {
    case Single:		return u.single.sanitize (c);
    case Multiple:		return u.multiple.sanitize (c);
    case Alternate:		return u.alternate.sanitize (c);
    case Ligature:		return u.ligature.sanitize (c);
    case Context:		return u.c.sanitize (c);
    case ChainContext:		return u.chainContext.sanitize (c);
    case Extension:		return u.extension.sanitize (c);
    case ReverseChainSingle:	return u.reverseChainContextSingle.sanitize (c);
    default:return true;
    }
  }

  private:
  union {
  USHORT			sub_format;
  SingleSubst			single;
  MultipleSubst			multiple;
  AlternateSubst		alternate;
  LigatureSubst			ligature;
  ContextSubst			c;
  ChainContextSubst		chainContext;
  ExtensionSubst		extension;
  ReverseChainSingleSubst	reverseChainContextSingle;
  } u;
  public:
  DEFINE_SIZE_UNION (2, sub_format);
};


struct SubstLookup : Lookup
{
  inline const SubstLookupSubTable& get_subtable (unsigned int i) const
  { return this+CastR<OffsetArrayOf<SubstLookupSubTable> > (subTable)[i]; }

  inline static bool lookup_type_is_reverse (unsigned int lookup_type)
  { return lookup_type == SubstLookupSubTable::ReverseChainSingle; }

  inline bool is_reverse (void) const
  {
    unsigned int type = get_type ();
    if (unlikely (type == SubstLookupSubTable::Extension))
      return CastR<ExtensionSubst> (get_subtable(0)).is_reverse ();
    return lookup_type_is_reverse (type);
  }

  inline bool closure (hb_closure_context_t *c) const
  {
    unsigned int lookup_type = get_type ();
    bool ret = false;
    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      ret = get_subtable (i).closure (c, lookup_type) || ret;
    return ret;
  }

  inline bool would_apply (hb_codepoint_t glyph_id) const
  {
    unsigned int lookup_type = get_type ();
    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      if (get_subtable (i).would_apply (glyph_id, lookup_type))
	return true;
    return false;
  }
  inline bool would_apply (hb_codepoint_t first, hb_codepoint_t second) const
  {
    unsigned int lookup_type = get_type ();
    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      if (get_subtable (i).would_apply (first, second, lookup_type))
	return true;
    return false;
  }

  inline bool apply_once (hb_apply_context_t *c) const
  {
    unsigned int lookup_type = get_type ();

    if (!_hb_ot_layout_check_glyph_property (c->face, &c->buffer->info[c->buffer->idx], c->lookup_props, &c->property))
      return false;

    if (unlikely (lookup_type == SubstLookupSubTable::Extension))
    {
      /* The spec says all subtables should have the same type.
       * This is specially important if one has a reverse type!
       *
       * This is rather slow to do this here for every glyph,
       * but it's easiest, and who uses extension lookups anyway?!*/
      unsigned int type = get_subtable(0).u.extension.get_type ();
      unsigned int count = get_subtable_count ();
      for (unsigned int i = 1; i < count; i++)
        if (get_subtable(i).u.extension.get_type () != type)
	  return false;
    }

    unsigned int count = get_subtable_count ();
    for (unsigned int i = 0; i < count; i++)
      if (get_subtable (i).apply (c, lookup_type))
	return true;

    return false;
  }

  inline bool apply_string (hb_apply_context_t *c) const
  {
    bool ret = false;

    if (unlikely (!c->buffer->len))
      return false;

    c->set_lookup (*this);

    if (likely (!is_reverse ()))
    {
	/* in/out forward substitution */
	c->buffer->clear_output ();
	c->buffer->idx = 0;
	while (c->buffer->idx < c->buffer->len)
	{
	  if ((c->buffer->info[c->buffer->idx].mask & c->lookup_mask) && apply_once (c))
	    ret = true;
	  else
	    c->buffer->next_glyph ();

	}
	if (ret)
	  c->buffer->swap_buffers ();
    }
    else
    {
	/* in-place backward substitution */
	c->buffer->idx = c->buffer->len - 1;
	do
	{
	  if ((c->buffer->info[c->buffer->idx].mask & c->lookup_mask) && apply_once (c))
	    ret = true;
	  else
	    c->buffer->idx--;

	}
	while ((int) c->buffer->idx >= 0);
    }

    return ret;
  }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!Lookup::sanitize (c))) return false;
    OffsetArrayOf<SubstLookupSubTable> &list = CastR<OffsetArrayOf<SubstLookupSubTable> > (subTable);
    return list.sanitize (c, this, get_type ());
  }
};

typedef OffsetListOf<SubstLookup> SubstLookupList;

/*
 * GSUB -- The Glyph Substitution Table
 */

struct GSUB : GSUBGPOS
{
  static const hb_tag_t Tag	= HB_OT_TAG_GSUB;

  inline const SubstLookup& get_lookup (unsigned int i) const
  { return CastR<SubstLookup> (GSUBGPOS::get_lookup (i)); }

  inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index) const
  { return get_lookup (lookup_index).apply_string (c); }

  static inline void substitute_start (hb_buffer_t *buffer);
  static inline void substitute_finish (hb_buffer_t *buffer);

  inline bool closure_lookup (hb_closure_context_t *c,
			      unsigned int          lookup_index) const
  { return get_lookup (lookup_index).closure (c); }

  inline bool sanitize (hb_sanitize_context_t *c) {
    TRACE_SANITIZE ();
    if (unlikely (!GSUBGPOS::sanitize (c))) return false;
    OffsetTo<SubstLookupList> &list = CastR<OffsetTo<SubstLookupList> > (lookupList);
    return list.sanitize (c, this);
  }
  public:
  DEFINE_SIZE_STATIC (10);
};


void
GSUB::substitute_start (hb_buffer_t *buffer)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, props_cache);
  HB_BUFFER_ALLOCATE_VAR (buffer, lig_id);
  HB_BUFFER_ALLOCATE_VAR (buffer, lig_comp);

  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    buffer->info[i].props_cache() = buffer->info[i].lig_id() = buffer->info[i].lig_comp() = 0;
}

void
GSUB::substitute_finish (hb_buffer_t *buffer)
{
}


/* Out-of-class implementation for methods recursing */

inline bool ExtensionSubst::closure (hb_closure_context_t *c) const
{
  return get_subtable ().closure (c, get_type ());
}

inline bool ExtensionSubst::would_apply (hb_codepoint_t glyph_id) const
{
  return get_subtable ().would_apply (glyph_id, get_type ());
}

inline bool ExtensionSubst::would_apply (hb_codepoint_t first, hb_codepoint_t second) const
{
  return get_subtable ().would_apply (first, second, get_type ());
}

inline bool ExtensionSubst::apply (hb_apply_context_t *c) const
{
  TRACE_APPLY ();
  return get_subtable ().apply (c, get_type ());
}

inline bool ExtensionSubst::sanitize (hb_sanitize_context_t *c)
{
  TRACE_SANITIZE ();
  if (unlikely (!Extension::sanitize (c))) return false;
  unsigned int offset = get_offset ();
  if (unlikely (!offset)) return true;
  return StructAtOffset<SubstLookupSubTable> (this, offset).sanitize (c, get_type ());
}

inline bool ExtensionSubst::is_reverse (void) const
{
  unsigned int type = get_type ();
  if (unlikely (type == SubstLookupSubTable::Extension))
    return CastR<ExtensionSubst> (get_subtable()).is_reverse ();
  return SubstLookup::lookup_type_is_reverse (type);
}

static inline bool closure_lookup (hb_closure_context_t *c, unsigned int lookup_index)
{
  const GSUB &gsub = *(c->face->ot_layout->gsub);
  const SubstLookup &l = gsub.get_lookup (lookup_index);

  if (unlikely (c->nesting_level_left == 0))
    return false;

  c->nesting_level_left--;
  bool ret = l.closure (c);
  c->nesting_level_left++;

  return ret;
}

static inline bool substitute_lookup (hb_apply_context_t *c, unsigned int lookup_index)
{
  const GSUB &gsub = *(c->face->ot_layout->gsub);
  const SubstLookup &l = gsub.get_lookup (lookup_index);

  if (unlikely (c->nesting_level_left == 0))
    return false;

  if (unlikely (c->context_length < 1))
    return false;

  hb_apply_context_t new_c (*c);
  new_c.nesting_level_left--;
  new_c.set_lookup (l);
  return l.apply_once (&new_c);
}



#endif /* HB_OT_LAYOUT_GSUB_TABLE_HH */
