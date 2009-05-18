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

#ifndef HB_OT_LAYOUT_GSUBGPOS_PRIVATE_H
#define HB_OT_LAYOUT_GSUBGPOS_PRIVATE_H

#include "hb-ot-layout-gdef-private.h"
#include "harfbuzz-buffer-private.h" /* XXX */


#define LOOKUP_ARGS_DEF \
	hb_ot_layout_t *layout, \
	hb_buffer_t    *buffer, \
	unsigned int    context_length HB_GNUC_UNUSED, \
	unsigned int    nesting_level_left HB_GNUC_UNUSED, \
	unsigned int    lookup_flag, \
	unsigned int    property HB_GNUC_UNUSED /* propety of first glyph */
#define LOOKUP_ARGS \
	layout, \
	buffer, \
	context_length, \
	nesting_level_left, \
	lookup_flag, \
	property


/* Contextual lookups */

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

  /* XXX We have to jump non-matching glyphs when applying too, right? */
  unsigned int record_count = recordCount;
  const LookupRecord *record = lookupRecord;
  for (i = 0; i < count;)
  {
    if (record_count && i == record->sequenceIndex)
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

  inline bool apply (LOOKUP_ARGS_DEF, ContextLookupContext &context) const {
    const LookupRecord *record = (const LookupRecord *) ((const char *) value + sizeof (value[0]) * (glyphCount ? glyphCount - 1 : 0));
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

  inline bool apply (LOOKUP_ARGS_DEF, ContextLookupContext &context) const {

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


static inline bool glyph_match (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  return glyph_id == value;
}

struct ContextFormat1 {

  friend struct Context;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextLookupContext context = {
      glyph_match, NULL,
      apply_func
    };
    return rule_set.apply (LOOKUP_ARGS, context);
  }

  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  OffsetArrayOf<RuleSet>
		ruleSet;		/* Array of RuleSet tables
					 * ordered by Coverage Index */
};
ASSERT_SIZE (ContextFormat1, 6);


static inline bool class_match (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  const ClassDef &class_def = * (const ClassDef *) data;
  return class_def.get_class (glyph_id) == value;
}

struct ContextFormat2 {

  friend struct Context;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    const RuleSet &rule_set = this+ruleSet[index];
    /* LONGTERMTODO: Old code fetches glyph classes at most once and caches
     * them across subrule lookups.  Not sure it's worth it.
     */
    struct ContextLookupContext context = {
      class_match, (char *) &(this+classDef),
      apply_func
    };
    return rule_set.apply (LOOKUP_ARGS, context);
  }

  private:
  USHORT	format;			/* Format identifier--format = 2 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  OffsetTo<ClassDef>
		classDef;		/* Offset to glyph ClassDef table--from
					 * beginning of table */
  OffsetArrayOf<RuleSet>
		ruleSet;		/* Array of RuleSet tables
					 * ordered by class */
};
ASSERT_SIZE (ContextFormat2, 8);


static inline bool coverage_match (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  const OffsetTo<Coverage> &coverage = * (const OffsetTo<Coverage> *) &value;
  return (data+coverage) (glyph_id) != NOT_COVERED;
}

struct ContextFormat3 {

  friend struct Context;

  private:

  /* Coverage tables, in glyph sequence order */
  DEFINE_OFFSET_ARRAY_TYPE (Coverage, coverage, glyphCount);

  inline bool apply_coverage (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {
    const LookupRecord *record = (const LookupRecord *) ((const char *) coverage + sizeof (coverage[0]) * glyphCount);
    struct ContextLookupContext context = {
      coverage_match, (char *) this,
      apply_func
    };
    return context_lookup (LOOKUP_ARGS,
			   glyphCount,
			   recordCount,
			   (const USHORT *) (coverage + 1),
			   record,
			   context);
  }

  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    if ((*this)[0].get_coverage (IN_CURGLYPH () == NOT_COVERED))
      return false;

    return apply_coverage (LOOKUP_ARGS, apply_func);
  }

  private:
  USHORT	format;			/* Format identifier--format = 3 */
  USHORT	glyphCount;		/* Number of glyphs in the input glyph
					 * sequence */
  USHORT	recordCount;		/* Number of LookupRecords */
  Offset	coverage[];		/* Array of offsets to Coverage
					 * table--from beginning of
					 * table--in glyph
					 * sequence order */
  LookupRecord	lookupRecord[];		/* Array of LookupRecords--in
					 * design order */
};
ASSERT_SIZE (ContextFormat3, 6);

struct Context {

  protected:
  bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {
    switch (u.format) {
    case 1: return u.format1->apply (LOOKUP_ARGS, apply_func);
    case 2: return u.format2->apply (LOOKUP_ARGS, apply_func);
    case 3: return u.format3->apply (LOOKUP_ARGS, apply_func);
    default:return false;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  ContextFormat1	format1[];
  ContextFormat2	format2[];
  ContextFormat3	format3[];
  } u;
};
ASSERT_SIZE (Context, 2);


/* Chaining Contextual lookups */

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

struct ChainContextFormat1 {

  friend struct ChainContext;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {
  /* TODO */

//    unsigned int index = (this+coverage) (IN_CURGLYPH ());
//    const RuleSet &rule_set = this+ruleSet[index];
//    struct ContextLookupContext context = {
//      glyph_match, NULL,
//      apply_func
//    };
//    return rule_set.apply (LOOKUP_ARGS, context);
  }


  private:
  USHORT	format;			/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  USHORT	chainSubRuleSetCount;	/* Number of ChainSubRuleSet
					 * tables--must equal GlyphCount in
					 * Coverage table */
  Offset	chainSubRuleSet[];	/* Array of offsets to ChainSubRuleSet
					 * tables--from beginning of
					 * table--ordered by Coverage Index */
};
ASSERT_SIZE (ChainContextFormat1, 6);

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

struct ChainContextFormat2 {

  friend struct ChainContext;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

//    unsigned int index = (this+coverage) (IN_CURGLYPH ());
//    const RuleSet &rule_set = this+ruleSet[index];
    /* LONGTERMTODO: Old code fetches glyph classes at most once and caches
     * them across subrule lookups.  Not sure it's worth it.
     */
//    struct ContextLookupContext context = {
//      class_match, (char *) &(this+classDef),
//      apply_func
//    };
//    return rule_set.apply (LOOKUP_ARGS, context);
  }

  private:
  USHORT	format;			/* Format identifier--format = 2 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  Offset	backtrackClassDef;	/* Offset to glyph ClassDef table
					 * containing backtrack sequence
					 * data--from beginning of table */
  Offset	inputClassDef;		/* Offset to glyph ClassDef
					 * table containing input sequence
					 * data--from beginning of table */
  Offset	lookaheadClassDef;	/* Offset to glyph ClassDef table
					 * containing lookahead sequence
					 * data--from beginning of table */
  USHORT	chainSubClassSetCnt;	/* Number of ChainSubClassSet tables */
  Offset	chainSubClassSet[];	/* Array of offsets to ChainSubClassSet
					 * tables--from beginning of
					 * table--ordered by input
					 * class--may be NULL */
};
ASSERT_SIZE (ChainContextFormat2, 12);

struct ChainContextFormat3 {

  friend struct ChainContext;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

//    if ((*this)[0].get_coverage (IN_CURGLYPH () == NOT_COVERED))
//      return false;

//    return apply_coverage (LOOKUP_ARGS, apply_func);
  }

  private:
  USHORT	format;			/* Format identifier--format = 3 */
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
					 * in lookahead sequence, in glyph
					 * sequence order */
  USHORT	substCount;		/* Number of LookupRecords */
  LookupRecord	substLookupRecord[];	/* Array of LookupRecords--in
					 * design order */
};
ASSERT_SIZE (ChainContextFormat3, 10);

struct ChainContext {

  protected:
  bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {
    switch (u.format) {
    case 1: return u.format1->apply (LOOKUP_ARGS, apply_func);
    case 2: return u.format2->apply (LOOKUP_ARGS, apply_func);
    case 3: return u.format3->apply (LOOKUP_ARGS, apply_func);
    default:return false;
    }
  }

  private:
  union {
  USHORT		format;	/* Format identifier */
  ChainContextFormat1	format1[];
  ChainContextFormat2	format2[];
  ChainContextFormat3	format3[];
  } u;
};
ASSERT_SIZE (ChainContext, 2);


/*
 * GSUB/GPOS Common
 */

struct GSUBGPOS {
  static const hb_tag_t GSUBTag		= HB_TAG ('G','S','U','B');
  static const hb_tag_t GPOSTag		= HB_TAG ('G','P','O','S');

  STATIC_DEFINE_GET_FOR_DATA (GSUBGPOS);
  /* XXX check version here? */

  DEFINE_TAG_LIST_INTERFACE (Script,  script );	/* get_script_count (), get_script (i), get_script_tag (i) */
  DEFINE_TAG_LIST_INTERFACE (Feature, feature);	/* get_feature_count(), get_feature(i), get_feature_tag(i) */
  DEFINE_LIST_INTERFACE     (Lookup,  lookup );	/* get_lookup_count (), get_lookup (i) */

  // LONGTERMTODO bsearch
  DEFINE_TAG_FIND_INTERFACE (Script,  script );	/* find_script_index (), get_script_by_tag (tag) */
  DEFINE_TAG_FIND_INTERFACE (Feature, feature);	/* find_feature_index(), get_feature_by_tag(tag) */

  private:
  Fixed_Version	version;	/* Version of the GSUB/GPOS table--initially set
				 * to 0x00010000 */
  OffsetTo<ScriptList>
		scriptList;  	/* ScriptList table */
  OffsetTo<FeatureList>
		featureList; 	/* FeatureList table */
  OffsetTo<LookupList>
		lookupList; 	/* LookupList table */
};
ASSERT_SIZE (GSUBGPOS, 10);


#endif /* HB_OT_LAYOUT_GSUBGPOS_PRIVATE_H */
