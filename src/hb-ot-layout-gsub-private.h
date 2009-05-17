/*
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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
 */

#ifndef HB_OT_LAYOUT_GSUB_PRIVATE_H
#define HB_OT_LAYOUT_GSUB_PRIVATE_H

#include "hb-ot-layout-private.h"

#include "hb-ot-layout-open-private.h"
#include "hb-ot-layout-gdef-private.h"

#include "harfbuzz-buffer-private.h" /* XXX */

#define LOOKUP_ARGS_DEF \
	hb_ot_layout_t *layout, \
	hb_buffer_t    *buffer, \
	unsigned int    context_length HB_GNUC_UNUSED, \
	unsigned int    nesting_level_left HB_GNUC_UNUSED, \
	unsigned int    lookup_flag
#define LOOKUP_ARGS \
	layout, \
	buffer, \
	context_length, \
	nesting_level_left, \
	lookup_flag

struct SingleSubstFormat1 {

  friend struct SingleSubst;

  private:

  inline bool single_substitute (hb_codepoint_t &glyph_id) const {

    unsigned int index = (this+coverage) (glyph_id);
    if (NOT_COVERED == index)
      return false;

    glyph_id += deltaGlyphID;

    return true;
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  SHORT		deltaGlyphID;		/* Add to original GlyphID to get
					 * substitute GlyphID */
};
ASSERT_SIZE (SingleSubstFormat1, 6);

struct SingleSubstFormat2 {

  friend struct SingleSubst;

  private:

  inline bool single_substitute (hb_codepoint_t &glyph_id) const {

    unsigned int index = (this+coverage) (glyph_id);

    if (index >= substitute.len)
      return false;

    glyph_id = substitute[index];
    return true;
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 2 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  ArrayOf<GlyphID>
		substitute;		/* Array of substitute
					 * GlyphIDs--ordered by Coverage Index */
};
ASSERT_SIZE (SingleSubstFormat2, 6);

struct SingleSubst {

  friend struct SubstLookupSubTable;

  private:

  inline bool single_substitute (hb_codepoint_t &glyph_id) const {
    switch (u.substFormat) {
    case 1: return u.format1.single_substitute (glyph_id);
    case 2: return u.format2.single_substitute (glyph_id);
    default:return false;
    }
  }

  inline bool substitute (LOOKUP_ARGS_DEF) const {

    unsigned int property;
    if (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM (), lookup_flag, &property))
      return false;

    hb_codepoint_t glyph_id = IN_CURGLYPH ();

    if (!single_substitute (glyph_id))
      return false;

    _hb_buffer_replace_glyph (buffer, glyph_id);

    if ( _hb_ot_layout_has_new_glyph_classes (layout) )
    {
      /* we inherit the old glyph class to the substituted glyph */
      _hb_ot_layout_set_glyph_property (layout, glyph_id, property);
    }

    return true;
  }

  private:
  union {
  USHORT	substFormat;	/* Format identifier */
  SingleSubstFormat1	format1;
  SingleSubstFormat2	format2;
  } u;
};
DEFINE_NULL (SingleSubst, 2);


struct Sequence {

  friend struct MultipleSubstFormat1;

  private:

  inline void set_glyph_class (hb_ot_layout_t *layout, unsigned int property) const {

    unsigned int count = substitute.len;
    for (unsigned int n = 0; n < count; n++)
      _hb_ot_layout_set_glyph_property (layout, substitute[n], property);
  }

  inline bool substitute_sequence (LOOKUP_ARGS_DEF, unsigned int property) const {

    if (HB_UNLIKELY (!substitute.len))
      return false;

    _hb_buffer_add_output_glyph_ids (buffer, 1,
				     substitute.len, substitute.array,
				     0xFFFF, 0xFFFF);

    if ( _hb_ot_layout_has_new_glyph_classes (layout) )
    {
      /* this is a guess only ... */

      if ( property == HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE )
        property = HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH;

      set_glyph_class (layout, property);
    }

    return true;
  }

  private:
  ArrayOf<GlyphID>
		substitute;		/* String of GlyphIDs to substitute */
};
ASSERT_SIZE (Sequence, 2);

struct MultipleSubstFormat1 {

  friend struct MultipleSubst;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {

    unsigned int property;
    if (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM (), lookup_flag, &property))
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    return (this+sequence[index]).substitute_sequence (LOOKUP_ARGS, property);
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetArrayOf<Sequence>
		sequence;		/* Array of Sequence tables
					 * ordered by Coverage Index */
};
ASSERT_SIZE (MultipleSubstFormat1, 6);

struct MultipleSubst {

  friend struct SubstLookupSubTable;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {
    switch (u.substFormat) {
    case 1: return u.format1.substitute (LOOKUP_ARGS);
    default:return false;
    }
  }

  private:
  union {
  USHORT	substFormat;	/* Format identifier */
  MultipleSubstFormat1	format1;
  } u;
};
DEFINE_NULL (MultipleSubst, 2);


typedef ArrayOf<GlyphID> AlternateSet;	/* Array of alternate GlyphIDs--in
					 * arbitrary order */
ASSERT_SIZE (AlternateSet, 2);

struct AlternateSubstFormat1 {

  friend struct AlternateSubst;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {

    unsigned int property;
    if (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM (), lookup_flag, &property))
      return false;

    hb_codepoint_t glyph_id = IN_CURGLYPH ();

    unsigned int index = (this+coverage) (glyph_id);
    const AlternateSet &alt_set = this+alternateSet[index];

    if (HB_UNLIKELY (!alt_set.len))
      return false;

    unsigned int alt_index = 0;

    /* XXX callback to user to choose alternate
    if ( gsub->altfunc )
      alt_index = (gsub->altfunc)( buffer->out_pos, glyph_id,
				   aset.GlyphCount, aset.Alternate,
				   gsub->data );
				   */

    if (HB_UNLIKELY (alt_index >= alt_set.len))
      return false;

    glyph_id = alt_set[alt_index];

    _hb_buffer_replace_glyph (buffer, glyph_id);

    if ( _hb_ot_layout_has_new_glyph_classes (layout) )
    {
      /* we inherit the old glyph class to the substituted glyph */
      _hb_ot_layout_set_glyph_property (layout, glyph_id, property);
    }

    return true;
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetArrayOf<AlternateSet>
		alternateSet;		/* Array of AlternateSet tables
					 * ordered by Coverage Index */
};
ASSERT_SIZE (AlternateSubstFormat1, 6);

struct AlternateSubst {

  friend struct SubstLookupSubTable;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {
    switch (u.substFormat) {
    case 1: return u.format1.substitute (LOOKUP_ARGS);
    default:return false;
    }
  }

  private:
  union {
  USHORT	substFormat;	/* Format identifier */
  AlternateSubstFormat1	format1;
  } u;
};
DEFINE_NULL (AlternateSubst, 2);


struct Ligature {

  friend struct LigatureSet;

  private:
  DEFINE_ARRAY_TYPE (GlyphID, component, (compCount ? compCount - 1 : 0));

  inline bool substitute_ligature (LOOKUP_ARGS_DEF, bool is_mark) const {

    unsigned int i, j;
    unsigned int property;
    unsigned int count = compCount;

    if (HB_UNLIKELY (buffer->in_pos + count > buffer->in_length ||
		     context_length < count))
      return false; /* Not enough glyphs in input or context */

    for (i = 1, j = buffer->in_pos + 1; i < count; i++, j++) {
      while (!_hb_ot_layout_check_glyph_property (layout, IN_ITEM (j), lookup_flag, &property)) {
	if (HB_UNLIKELY (j + count - i == buffer->in_length))
	  return false;
	j++;
      }

      if (!(property == HB_OT_LAYOUT_GLYPH_CLASS_MARK ||
	    property &  LookupFlag::MarkAttachmentType))
	is_mark = FALSE;

      if (HB_LIKELY (IN_GLYPH(j) != (*this)[i - 1]))
        return false;
    }
    if ( _hb_ot_layout_has_new_glyph_classes (layout) )
      /* this is just a guess ... */
      hb_ot_layout_set_glyph_class (layout, ligGlyph,
				    is_mark ? HB_OT_LAYOUT_GLYPH_CLASS_MARK
					    : HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE);

    if (j == buffer->in_pos + i) /* No input glyphs skipped */
      /* We don't use a new ligature ID if there are no skipped
	 glyphs and the ligature already has an ID. */
      _hb_buffer_add_output_glyph_ids (buffer, i,
				       1, &ligGlyph,
				       0xFFFF,
				       IN_LIGID (buffer->in_pos) ?
				       0xFFFF : _hb_buffer_allocate_ligid (buffer));
    else
    {
      unsigned int lig_id = _hb_buffer_allocate_ligid (buffer);
      _hb_buffer_add_output_glyph (buffer, ligGlyph, 0xFFFF, lig_id);

      /* Now we must do a second loop to copy the skipped glyphs to
	 `out' and assign component values to it.  We start with the
	 glyph after the first component.  Glyphs between component
	 i and i+1 belong to component i.  Together with the lig_id
	 value it is later possible to check whether a specific
	 component value really belongs to a given ligature. */

      for ( i = 0; i < count - 1; i++ )
      {
	while (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM(), lookup_flag, &property))
	  _hb_buffer_add_output_glyph (buffer, IN_CURGLYPH(), i, lig_id);

	(buffer->in_pos)++;
      }

      /* XXX We  should possibly reassign lig_id and component for any
       * components of a previous ligature that s now being removed as part of
       * this ligature. */
    }

    return true;
  }

  private:
  GlyphID	ligGlyph;		/* GlyphID of ligature to substitute */
  USHORT	compCount;		/* Number of components in the ligature */
  GlyphID	component[];		/* Array of component GlyphIDs--start
					 * with the second  component--ordered
					 * in writing direction */
};
ASSERT_SIZE (Ligature, 4);

struct LigatureSet {

  friend struct LigatureSubstFormat1;

  private:

  inline bool substitute_ligature (LOOKUP_ARGS_DEF, bool is_mark) const {

    unsigned int num_ligs = ligature.len;
    for (unsigned int i = 0; i < num_ligs; i++) {
      const Ligature &lig = this+ligature[i];
      if (lig.substitute_ligature (LOOKUP_ARGS, is_mark))
        return true;
    }

    return false;
  }

  private:
  OffsetArrayOf<Ligature>
		ligature;		/* Array LigatureSet tables
					 * ordered by preference */
};
ASSERT_SIZE (LigatureSet, 2);

struct LigatureSubstFormat1 {

  friend struct LigatureSubst;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {

    unsigned int property;
    if (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM (), lookup_flag, &property))
      return false;

    hb_codepoint_t glyph_id = IN_CURGLYPH ();

    bool first_is_mark = (property == HB_OT_LAYOUT_GLYPH_CLASS_MARK ||
			  property &  LookupFlag::MarkAttachmentType);

    unsigned int index = (this+coverage) (glyph_id);
    const LigatureSet &lig_set = this+ligatureSet[index];
    return lig_set.substitute_ligature (LOOKUP_ARGS, first_is_mark);
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetArrayOf<LigatureSet>\
		ligatureSet;		/* Array LigatureSet tables
					 * ordered by Coverage Index */
};
ASSERT_SIZE (LigatureSubstFormat1, 6);

struct LigatureSubst {

  friend struct SubstLookupSubTable;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {
    switch (u.substFormat) {
    case 1: return u.format1.substitute (LOOKUP_ARGS);
    default:return false;
    }
  }

  private:
  union {
  USHORT	substFormat;	/* Format identifier */
  LigatureSubstFormat1	format1;
  } u;
};
DEFINE_NULL (LigatureSubst, 2);


typedef bool (*match_func_t) (hb_codepoint_t glyph_id, const USHORT &value, char *data);
typedef bool (*apply_lookup_func_t) (LOOKUP_ARGS_DEF, unsigned int lookup_index);

struct ContextLookupContext {
  inline bool match (hb_codepoint_t glyph_id, const USHORT &value) const {
    return match_func (glyph_id, value, match_data);
  }
  inline bool apply (LOOKUP_ARGS_DEF, unsigned int lookup_index) const {
    return apply_func (LOOKUP_ARGS, lookup_index);
  }

  match_func_t match_func;
  char *match_data;
  apply_lookup_func_t apply_func;
};

struct LookupRecord {

  inline bool apply (LOOKUP_ARGS_DEF, ContextLookupContext &context) const {
    return context.apply (LOOKUP_ARGS, lookupListIndex);
  }

  USHORT	sequenceIndex;		/* Index into current glyph
					 * sequence--first glyph = 0 */
  USHORT	lookupListIndex;	/* Lookup to apply to that
					 * position--zero--based */
};
ASSERT_SIZE (LookupRecord, 4);

static inline bool context_lookup (LOOKUP_ARGS_DEF,
				   USHORT glyphCount, /* Including the first glyph (not matched) */
				   USHORT recordCount,
				   const USHORT value[], /* Array of match values--start with second glyph */
				   const LookupRecord lookupRecord[], /* Array of LookupRecords--in design order */
				   ContextLookupContext &context) {

  unsigned int i, j;
  unsigned int property;
  unsigned int count = glyphCount;

  if (HB_UNLIKELY (buffer->in_pos + count > buffer->in_length ||
		   context_length < count))
    return false; /* Not enough glyphs in input or context */

  /* XXX context_length should also be checked when skipping glyphs, right?
   * What does context_length really mean, anyway? */

  for (i = 1, j = buffer->in_pos + 1; i < count; i++, j++) {
    while (!_hb_ot_layout_check_glyph_property (layout, IN_ITEM (j), lookup_flag, &property)) {
      if (HB_UNLIKELY (j + count - i == buffer->in_length))
	return false;
      j++;
    }

    if (HB_LIKELY (context.match (IN_GLYPH(j), value[i - 1])))
      return false;
  }

  /* XXX right? or j - buffer_inpos? */
  context_length = count;

  unsigned int record_count = recordCount;
  const LookupRecord *record = lookupRecord;
  for (i = 0; i < count;)
  {
    if ( record_count && i == record->sequenceIndex )
    {
      unsigned int old_pos = buffer->in_pos;

      /* Apply a lookup */
      bool done = record->apply (LOOKUP_ARGS, context);

      record++;
      record_count--;
      i += buffer->in_pos - old_pos;

      if (!done)
	goto not_applied;
    }
    else
    {
    not_applied:
      /* No lookup applied for this index */
      _hb_buffer_next_glyph (buffer);
      i++;
    }
  }

  return true;
}
struct Rule {

  friend struct RuleSet;

  private:
  DEFINE_ARRAY_TYPE (USHORT, value, (glyphCount ? glyphCount - 1 : 0));

  bool apply (LOOKUP_ARGS_DEF, ContextLookupContext &context) const {
    const LookupRecord *record = (const LookupRecord *) ((const char *) value + sizeof (value[0]) * (glyphCount - 1));
    return context_lookup (LOOKUP_ARGS,
			   glyphCount,
			   recordCount,
			   value,
			   record,
			   context);
  }

  private:
  USHORT	glyphCount;		/* Total number of glyphs in input
					 * glyph sequence--includes the  first
					 * glyph */
  USHORT	recordCount;		/* Number of LookupRecords */
  USHORT	value[];		/* Array of match values--start with
					 * second glyph */
  LookupRecord	lookupRecord[];		/* Array of LookupRecords--in
					 * design order */
};
ASSERT_SIZE (Rule, 4);

struct RuleSet {

  bool apply (LOOKUP_ARGS_DEF, ContextLookupContext &context) const {

    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++) {
      if ((this+rule[i]).apply (LOOKUP_ARGS, context))
        return true;
    }

    return false;
  }

  private:
  OffsetArrayOf<Rule>
		rule;			/* Array SubRule tables
					 * ordered by preference */
};

static inline bool substitute_one_lookup (LOOKUP_ARGS_DEF, unsigned int lookup_index);

static inline bool glyph_match (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  return glyph_id == value;
}

struct SubRuleSet : RuleSet {
  inline bool substitute (LOOKUP_ARGS_DEF) const {
    struct ContextLookupContext context = {
      glyph_match, NULL,
      substitute_one_lookup
    };
    return apply (LOOKUP_ARGS, context);
  }
};
ASSERT_SIZE (SubRuleSet, 2);

struct ContextSubstFormat1 {

  friend struct ContextSubst;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {

    unsigned int property;
    if (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM (), lookup_flag, &property))
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    const SubRuleSet &rule_set = this+subRuleSet[index];
    return rule_set.substitute (LOOKUP_ARGS);
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetArrayOf<SubRuleSet>
		subRuleSet;		/* Array of SubRuleSet tables
					 * ordered by Coverage Index */
};
ASSERT_SIZE (ContextSubstFormat1, 6);

static inline bool class_match (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  const ClassDef &class_def = * (const ClassDef *) data;
  return class_def.get_class (glyph_id) == value;
}

struct SubClassSet : RuleSet {
  inline bool substitute_class (LOOKUP_ARGS_DEF, const ClassDef &class_def) const {
    /* LONGTERMTODO: Old code fetches glyph classes at most once and caches
     * them across subrule lookups.  Not sure it's worth it.
     */
    struct ContextLookupContext context = {
      class_match, (char *) &class_def,
      substitute_one_lookup
    };
    return apply (LOOKUP_ARGS, context);
  }
};
ASSERT_SIZE (SubClassSet, 2);


struct ContextSubstFormat2 {

  friend struct ContextSubst;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {

    unsigned int property;
    if (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM (), lookup_flag, &property))
      return false;

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    const SubClassSet &class_set = this+subClassSet[index];
    return class_set.substitute_class (LOOKUP_ARGS, this+classDef);
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 2 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  OffsetTo<ClassDef>
		classDef;		/* Offset to glyph ClassDef table--from
					 * beginning of Substitution  table */
  OffsetArrayOf<SubClassSet>
		subClassSet;		/* Array of SubClassSet tables
					 * ordered by class */
};
ASSERT_SIZE (ContextSubstFormat2, 8);

static inline bool coverage_match (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  const OffsetTo<Coverage> &coverage = * (const OffsetTo<Coverage> *) &value;
  return (data+coverage) (glyph_id) != NOT_COVERED;
}

struct ContextSubstFormat3 {

  friend struct ContextSubst;

  private:

  /* Coverage tables, in glyph sequence order */
  DEFINE_OFFSET_ARRAY_TYPE (Coverage, coverage, glyphCount);

  inline bool substitute_coverage (LOOKUP_ARGS_DEF) const {
    struct ContextLookupContext context = {
      coverage_match, (char *) this,
      substitute_one_lookup
    };
    const LookupRecord *record = (const LookupRecord *) ((const char *) coverage + sizeof (coverage[0]) * glyphCount);
    return context_lookup (LOOKUP_ARGS,
			   glyphCount,
			   recordCount,
			   (const USHORT *) (coverage + 1),
			   record,
			   context);
  }

  inline bool substitute (LOOKUP_ARGS_DEF) const {

    unsigned int property;
    if (!_hb_ot_layout_check_glyph_property (layout, IN_CURITEM (), lookup_flag, &property))
      return false;

    if ((*this)[0].get_coverage (IN_CURGLYPH () == NOT_COVERED))
      return false;

    return substitute_coverage (LOOKUP_ARGS);
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 3 */
  USHORT	glyphCount;		/* Number of glyphs in the input glyph
					 * sequence */
  USHORT	recordCount;		/* Number of LookupRecords */
  Offset	coverage[];		/* Array of offsets to Coverage
					 * table--from beginning of
					 * Substitution table--in glyph
					 * sequence order */
  LookupRecord	lookupRecord[];		/* Array of LookupRecords--in
					 * design order */
};
ASSERT_SIZE (ContextSubstFormat3, 6);

struct ContextSubst {

  friend struct SubstLookupSubTable;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {
    switch (u.substFormat) {
    case 1: return u.format1.substitute (LOOKUP_ARGS);
    case 2: return u.format2.substitute (LOOKUP_ARGS);
    case 3: return u.format3.substitute (LOOKUP_ARGS);
    default:return false;
    }
  }

  private:
  union {
  USHORT	substFormat;	/* Format identifier */
  ContextSubstFormat1	format1;
  ContextSubstFormat2	format2;
  ContextSubstFormat3	format3;
  } u;
};
DEFINE_NULL (ContextSubst, 2);


struct ChainSubRule {
  /* TODO */

  private:
  USHORT	backtrackGlyphCount;	/* Total number of glyphs in the
					 * backtrack sequence (number of
					 * glyphs to be matched before the
					 * first glyph) */
  GlyphID	backtrack[];		/* Array of backtracking GlyphID's
					 * (to be matched before the input
					 * sequence) */
  USHORT	inputGlyphCount;	/* Total number of glyphs in the input
					 * sequence (includes the first  glyph) */
  GlyphID	input[];		/* Array of input GlyphIDs (start with
					 * second glyph) */
  USHORT	lookaheadGlyphCount;	/* Total number of glyphs in the look
					 * ahead sequence (number of  glyphs to
					 * be matched after the input sequence) */
  GlyphID	lookAhead[];		/* Array of lookahead GlyphID's (to be
					 * matched after  the input sequence) */
  USHORT	substCount;		/* Number of LookupRecords */
  LookupRecord	substLookupRecord[];	/* Array of LookupRecords--in
					 * design order) */
};
ASSERT_SIZE (ChainSubRule, 8);

struct ChainSubRuleSet {
  /* TODO */

  private:
  USHORT	chainSubRuleCount;	/* Number of ChainSubRule tables */
  Offset	chainSubRule[];		/* Array of offsets to ChainSubRule
					 * tables--from beginning of
					 * ChainSubRuleSet table--ordered
					 * by preference */
};
ASSERT_SIZE (ChainSubRuleSet, 2);

struct ChainContextSubstFormat1 {
  /* TODO */
  inline bool substitute (LOOKUP_ARGS_DEF) const {
    return false;
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  USHORT	chainSubRuleSetCount;	/* Number of ChainSubRuleSet
					 * tables--must equal GlyphCount in
					 * Coverage table */
  Offset	chainSubRuleSet[];	/* Array of offsets to ChainSubRuleSet
					 * tables--from beginning of
					 * Substitution table--ordered by
					 * Coverage Index */
};
ASSERT_SIZE (ChainContextSubstFormat1, 6);

struct ChainSubClassRule {
  /* TODO */

  private:
  USHORT	backtrackGlyphCount;	/* Total number of glyphs in the
					 * backtrack sequence (number of
					 * glyphs to be matched before the
					 * first glyph) */
  USHORT	backtrack[];		/* Array of backtracking classes (to be
					 * matched before the input  sequence) */
  USHORT	inputGlyphCount;	/* Total number of classes in the input
					 * sequence (includes the  first class) */
  USHORT	input[];		/* Array of input classes(start with
					 * second class; to  be matched with
					 * the input glyph sequence) */
  USHORT	lookaheadGlyphCount;	/* Total number of classes in the
					 * look ahead sequence (number of
					 * classes to be matched after the
					 * input sequence) */
  USHORT	lookAhead[];		/* Array of lookahead classes (to be
					 * matched after the  input sequence) */
  USHORT	substCount;		/* Number of LookupRecords */
  LookupRecord	substLookupRecord[];	/* Array of LookupRecords--in
					 * design order) */
};
ASSERT_SIZE (ChainSubClassRule, 8);

struct ChainSubClassSet {
  /* TODO */

  private:
  USHORT	chainSubClassRuleCnt;	/* Number of ChainSubClassRule tables */
  Offset	chainSubClassRule[];	/* Array of offsets
					 * to ChainSubClassRule
					 * tables--from beginning of
					 * ChainSubClassSet--ordered by
					 * preference */
};
ASSERT_SIZE (ChainSubClassSet, 2);

struct ChainContextSubstFormat2 {
  /* TODO */
  inline bool substitute (LOOKUP_ARGS_DEF) const {
    return false;
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 2 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  Offset	backtrackClassDef;	/* Offset to glyph ClassDef table
					 * containing backtrack sequence
					 * data--from beginning of Substitution
					 * table */
  Offset	inputClassDef;		/* Offset to glyph ClassDef
					 * table containing input sequence
					 * data--from beginning of Substitution
					 * table */
  Offset	lookaheadClassDef;	/* Offset to glyph ClassDef table
					 * containing lookahead sequence
					 * data--from beginning of Substitution
					 * table */
  USHORT	chainSubClassSetCnt;	/* Number of ChainSubClassSet tables */
  Offset	chainSubClassSet[];	/* Array of offsets to ChainSubClassSet
					 * tables--from beginning of
					 * Substitution table--ordered by input
					 * class--may be NULL */
};
ASSERT_SIZE (ChainContextSubstFormat2, 12);

struct ChainContextSubstFormat3 {
  /* TODO */
  inline bool substitute (LOOKUP_ARGS_DEF) const {
    return false;
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 3 */
  USHORT	backtrackGlyphCount;	/* Number of glyphs in the backtracking
					 * sequence */
  Offset	backtrackCoverage[];	/* Array of offsets to coverage tables
					 * in backtracking sequence, in  glyph
					 * sequence order */
  USHORT	inputGlyphCount;	/* Number of glyphs in input sequence */
  Offset	inputCoverage[];	/* Array of offsets to coverage
					 * tables in input sequence, in glyph
					 * sequence order */
  USHORT	lookaheadGlyphCount;	/* Number of glyphs in lookahead
					 * sequence */
  Offset	lookaheadCoverage[];	/* Array of offsets to coverage tables
					 * in lookahead sequence, in  glyph
					 * sequence order */
  USHORT	substCount;		/* Number of LookupRecords */
  LookupRecord	substLookupRecord[];	/* Array of LookupRecords--in
					 * design order */
};
ASSERT_SIZE (ChainContextSubstFormat3, 10);

struct ChainContextSubst {

  friend struct SubstLookupSubTable;

  private:

  inline bool substitute (LOOKUP_ARGS_DEF) const {
    switch (u.substFormat) {
    case 1: return u.format1.substitute (LOOKUP_ARGS);
    case 2: return u.format2.substitute (LOOKUP_ARGS);
    case 3: return u.format3.substitute (LOOKUP_ARGS);
    default:return false;
    }
  }

  private:
  union {
  USHORT	substFormat;	/* Format identifier */
  ChainContextSubstFormat1	format1;
  ChainContextSubstFormat2	format2;
  ChainContextSubstFormat3	format3;
  } u;
};
DEFINE_NULL (ChainContextSubst, 2);


struct ExtensionSubstFormat1 {

  friend struct ExtensionSubst;

  private:
  inline unsigned int get_type (void) const { return extensionLookupType; }
  inline bool substitute (LOOKUP_ARGS_DEF) const;

  private:
  USHORT	substFormat;		/* Format identifier. Set to 1. */
  USHORT	extensionLookupType;	/* Lookup type of subtable referenced
					 * by ExtensionOffset (i.e. the
					 * extension subtable). */
  ULONG		extensionOffset;	/* Offset to the extension subtable,
					 * of lookup type  subtable. */
};
ASSERT_SIZE (ExtensionSubstFormat1, 8);

struct ExtensionSubst {

  friend struct SubstLookup;
  friend struct SubstLookupSubTable;

  private:

  inline unsigned int get_type (void) const {
    switch (u.substFormat) {
    case 1: return u.format1.get_type ();
    default:return 0;
    }
  }

  inline bool substitute (LOOKUP_ARGS_DEF) const {
    switch (u.substFormat) {
    case 1: return u.format1.substitute (LOOKUP_ARGS);
    default:return false;
    }
  }

  private:
  union {
  USHORT	substFormat;	/* Format identifier */
  ExtensionSubstFormat1	format1;
  } u;
};
DEFINE_NULL (ExtensionSubst, 2);



struct ReverseChainSingleSubstFormat1 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table -- from
					 * beginning of Substitution table */
  USHORT	backtrackGlyphCount;	/* Number of glyphs in the backtracking
					 * sequence */
  Offset	backtrackCoverage[];	/* Array of offsets to coverage tables
					 * in backtracking sequence, in  glyph
					 * sequence order */
  USHORT	lookaheadGlyphCount;	/* Number of glyphs in lookahead
					 * sequence */
  Offset	lookaheadCoverage[];	/* Array of offsets to coverage tables
					 * in lookahead sequence, in  glyph
					 * sequence order */
  USHORT	glyphCount;		/* Number of GlyphIDs in the Substitute
					 * array */
  GlyphID	substitute[];		/* Array of substitute
					 * GlyphIDs--ordered by Coverage  Index */
};
ASSERT_SIZE (ReverseChainSingleSubstFormat1, 10);

/*
 * SubstLookup
 */

enum {
  GSUB_Single				= 1,
  GSUB_Multiple				= 2,
  GSUB_Alternate			= 3,
  GSUB_Ligature				= 4,
  GSUB_Context				= 5,
  GSUB_ChainingContext			= 6,
  GSUB_Extension			= 7,
  GSUB_ReverseChainingContextSingle	= 8,
};

struct SubstLookupSubTable {

  friend struct SubstLookup;

  inline bool substitute (LOOKUP_ARGS_DEF,
			  unsigned int    lookup_type) const {
    switch (lookup_type) {
    case GSUB_Single:				return u.single.substitute (LOOKUP_ARGS);
    case GSUB_Multiple:				return u.multiple.substitute (LOOKUP_ARGS);
    case GSUB_Alternate:			return u.alternate.substitute (LOOKUP_ARGS);
    case GSUB_Ligature:				return u.ligature.substitute (LOOKUP_ARGS);
    case GSUB_Context:				return u.context.substitute (LOOKUP_ARGS);
    /*
    case GSUB_ChainingContext:			return u.chainingContext.substitute (LOOKUP_ARGS);
    */
    case GSUB_Extension:			return u.extension.substitute (LOOKUP_ARGS);
			/*
    case GSUB_ReverseChainingContextSingle:	return u.reverseChainingContextSingle.substitute (LOOKUP_ARGS);
    */
    default:return false;
    }
  }

  private:
  union {
  USHORT				substFormat;
  SingleSubst				single;
  MultipleSubst				multiple;
  AlternateSubst			alternate;
  LigatureSubst				ligature;
  ContextSubst				context;
  /*
  ChainingContextSubst			chainingContext;
  */
  ExtensionSubst			extension;
  /*
  ReverseChainingContextSingleSubst	reverseChainingContextSingle;
  */
  } u;
};


struct SubstLookup : Lookup {

  inline const SubstLookupSubTable& get_subtable (unsigned int i) const {
    return *(SubstLookupSubTable*)&(((Lookup *)this)->get_subtable (i));
  }

  /* Like get_type(), but looks through extension lookups.
   * Never returns Extension */
  inline unsigned int get_effective_type (void) const {
    unsigned int type = get_type ();

    if (HB_UNLIKELY (type == GSUB_Extension)) {
      /* Return lookup type of first extension subtable.
       * The spec says all of them should have the same type.
       * XXX check for that somehow */
      type = get_subtable(0).u.extension.get_type ();
    }

    return type;
  }

  inline bool is_reverse (void) const {
    switch (get_effective_type ()) {
    case GSUB_ReverseChainingContextSingle:	return true;
    default:					return false;
    }
  }

  bool substitute_once (hb_ot_layout_t *layout,
			hb_buffer_t    *buffer,
			unsigned int    context_length,
			unsigned int    nesting_level_left) const {

    unsigned int lookup_type = get_type ();
    unsigned int lookup_flag = get_flag ();

    if (HB_UNLIKELY (nesting_level_left == 0))
      return false;
    nesting_level_left--;

    if (HB_UNLIKELY (context_length < 1))
      return false;

    for (unsigned int i = 0; i < get_subtable_count (); i++)
      if (get_subtable (i).substitute (LOOKUP_ARGS,
				       lookup_type))
	return true;

    return false;
  }

  bool substitute_string (hb_ot_layout_t *layout,
			  hb_buffer_t    *buffer,
			  hb_ot_layout_feature_mask_t mask) const {

    bool ret = false;

    if (HB_LIKELY (!is_reverse ())) {

	/* in/out forward substitution */
	_hb_buffer_clear_output (buffer);
	buffer->in_pos = 0;
	while (buffer->in_pos < buffer->in_length) {

	  if ((~IN_PROPERTIES (buffer->in_pos) & mask) &&
	      substitute_once (layout, buffer, NO_CONTEXT, MAX_NESTING_LEVEL))
	    ret = true;
	  else
	    _hb_buffer_next_glyph (buffer);

	}
	if (ret)
	  _hb_buffer_swap (buffer);

    } else {

	/* in-place backward substitution */
	buffer->in_pos = buffer->in_length - 1;
	do {

	  if ((~IN_PROPERTIES (buffer->in_pos) & mask) &&
	      substitute_once (layout, buffer, NO_CONTEXT, MAX_NESTING_LEVEL))
	    ret = true;
	  else
	    buffer->in_pos--;

	} while ((int) buffer->in_pos >= 0);
    }

    return ret;
  }
};


/*
 * GSUB
 */

struct GSUB : GSUBGPOS {
  static const hb_tag_t Tag		= HB_TAG ('G','S','U','B');

  STATIC_DEFINE_GET_FOR_DATA (GSUB);
  /* XXX check version here? */

  inline const SubstLookup& get_lookup (unsigned int i) const {
    return *(SubstLookup*)&(((GSUBGPOS *)this)->get_lookup (i));
  }

  inline bool substitute_lookup (hb_ot_layout_t *layout,
				 hb_buffer_t    *buffer,
			         unsigned int    lookup_index,
				 hb_ot_layout_feature_mask_t  mask) const {
    return get_lookup (lookup_index).substitute_string (layout, buffer, mask);
  }

};




/* Out-of-class implementation for methods chaining */

inline bool ExtensionSubstFormat1::substitute (LOOKUP_ARGS_DEF) const {
  /* XXX either check in sanitize or here that the lookuptype is not 7 again,
   * or we can loop indefinitely. */
  return (*(SubstLookupSubTable *)(((char *) this) + extensionOffset)).substitute (LOOKUP_ARGS,
										   get_type ());
}

static inline bool substitute_one_lookup (LOOKUP_ARGS_DEF, unsigned int lookup_index) {
  const GSUB &gsub = *(layout->gsub);
  const SubstLookup &l = gsub.get_lookup (lookup_index);

  return l.substitute_once (layout, buffer, context_length, nesting_level_left);
}



#endif /* HB_OT_LAYOUT_GSUB_PRIVATE_H */
