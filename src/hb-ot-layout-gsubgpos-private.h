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


typedef bool (*match_func_t) (hb_codepoint_t glyph_id, const USHORT &value, char *data);
typedef bool (*apply_lookup_func_t) (LOOKUP_ARGS_DEF, unsigned int lookup_index);

struct ContextFuncs {
  match_func_t match;
  apply_lookup_func_t apply;
};

static inline bool match_glyph (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  return glyph_id == value;
}

static inline bool match_class (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  const ClassDef &class_def = * (const ClassDef *) data;
  return class_def.get_class (glyph_id) == value;
}

static inline bool match_coverage (hb_codepoint_t glyph_id, const USHORT &value, char *data) {
  const OffsetTo<Coverage> &coverage = * (const OffsetTo<Coverage> *) &value;
  return (data+coverage) (glyph_id) != NOT_COVERED;
}

struct LookupRecord {

  USHORT	sequenceIndex;		/* Index into current glyph
					 * sequence--first glyph = 0 */
  USHORT	lookupListIndex;	/* Lookup to apply to that
					 * position--zero--based */
};
ASSERT_SIZE (LookupRecord, 4);


/* Contextual lookups */

struct ContextLookupContext {
  ContextFuncs funcs;
  char *match_data;
};

static inline bool context_lookup (LOOKUP_ARGS_DEF,
				   USHORT inputCount, /* Including the first glyph (not matched) */
				   const USHORT input[], /* Array of input values--start with second glyph */
				   USHORT lookupCount,
				   const LookupRecord lookupRecord[], /* Array of LookupRecords--in design order */
				   ContextLookupContext &context) {

  unsigned int i, j;
  unsigned int count = inputCount;

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

    if (HB_LIKELY (context.funcs.match (IN_GLYPH(j), input[i - 1], context.match_data)))
      return false;
  }

  /* XXX right? or j - buffer_inpos? */
  context_length = count;

  /* XXX We have to jump non-matching glyphs when applying too, right? */
  /* XXX We don't support lookupRecord arrays that are not increasing:
   *     Should be easy for in_place ones at least. */
  unsigned int record_count = lookupCount;
  const LookupRecord *record = lookupRecord;
  for (i = 0; i < count;)
  {
    if (record_count && i == record->sequenceIndex)
    {
      unsigned int old_pos = buffer->in_pos;

      /* Apply a lookup */
      bool done = context.funcs.apply (LOOKUP_ARGS, record->lookupListIndex);

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
  inline bool apply (LOOKUP_ARGS_DEF, ContextLookupContext &context) const {
    const LookupRecord *record = (const LookupRecord *) ((const char *) input + sizeof (input[0]) * (inputCount ? inputCount - 1 : 0));
    return context_lookup (LOOKUP_ARGS,
			   inputCount,
			   input,
			   lookupCount,
			   record,
			   context);
  }

  private:
  USHORT	inputCount;		/* Total number of glyphs in input
					 * glyph sequence--includes the  first
					 * glyph */
  USHORT	lookupCount;		/* Number of LookupRecords */
  USHORT	input[];		/* Array of match inputs--start with
					 * second glyph */
  LookupRecord	lookupRecordX[];	/* Array of LookupRecords--in
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
		rule;			/* Array of Rule tables
					 * ordered by preference */
};


struct ContextFormat1 {

  friend struct Context;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (G_LIKELY (index == NOT_COVERED))
      return false;

    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextLookupContext context = {
      {match_glyph, apply_func},
      NULL
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


struct ContextFormat2 {

  friend struct Context;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (G_LIKELY (index == NOT_COVERED))
      return false;

    const ClassDef &class_def = this+classDef;
    index = class_def (IN_CURGLYPH ());
    const RuleSet &rule_set = this+ruleSet[index];
    /* LONGTERMTODO: Old code fetches glyph classes at most once and caches
     * them across subrule lookups.  Not sure it's worth it.
     */
    struct ContextLookupContext context = {
     {match_class, apply_func},
      (char *) &class_def
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


struct ContextFormat3 {

  friend struct Context;

  private:

  inline bool apply_coverage (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {
    const LookupRecord *record = (const LookupRecord *) ((const char *) coverage + sizeof (coverage[0]) * glyphCount);
    struct ContextLookupContext context = {
      {match_coverage, apply_func},
      (char *) this
    };
    return context_lookup (LOOKUP_ARGS,
			   glyphCount,
			   (const USHORT *) (coverage + 1),
			   lookupCount,
			   record,
			   context);
  }

  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    unsigned int index = (this+coverage[0]) (IN_CURGLYPH ());
    if (G_LIKELY (index == NOT_COVERED))
      return false;

    return apply_coverage (LOOKUP_ARGS, apply_func);
  }

  private:
  USHORT	format;			/* Format identifier--format = 3 */
  USHORT	glyphCount;		/* Number of glyphs in the input glyph
					 * sequence */
  USHORT	lookupCount;		/* Number of LookupRecords */
  OffsetTo<Coverage>
		coverage[];		/* Array of offsets to Coverage
					 * table in glyph sequence order */
  LookupRecord	lookupRecordX[];	/* Array of LookupRecords--in
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

struct ChainContextLookupContext {
  ContextFuncs funcs;
  char *match_data[3];
};


struct ChainRule {

  friend struct ChainRuleSet;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, ChainContextLookupContext &context) const {
    return false;
//    const LookupRecord *record = (const LookupRecord *) ((const char *) input + sizeof (input[0]) * (inputCount ? inputCount - 1 : 0));
//    return context_lookup (LOOKUP_ARGS,
//			   inputCount,
//			   input,
//			   lookupCount,
//			   record,
//			   context);
  }


  private:
  USHORT	backtrackCount;		/* Total number of glyphs in the
					 * backtrack sequence (number of
					 * glyphs to be matched before the
					 * first glyph) */
  USHORT	backtrack[];		/* Array of backtracking values
					 * (to be matched before the input
					 * sequence) */
  USHORT	inputCountX;		/* Total number of values in the input
					 * sequence (includes the first glyph) */
  USHORT	inputX[];		/* Array of input values (start with
					 * second glyph) */
  USHORT	lookaheadCountX;	/* Total number of glyphs in the look
					 * ahead sequence (number of glyphs to
					 * be matched after the input sequence) */
  USHORT	lookaheadX[];		/* Array of lookahead values's (to be
					 * matched after the input sequence) */
  USHORT	lookupCountX;		/* Number of LookupRecords */
  LookupRecord	lookupRecordX[];	/* Array of LookupRecords--in
					 * design order) */
};
ASSERT_SIZE (ChainRule, 8);

struct ChainRuleSet {

  inline bool apply (LOOKUP_ARGS_DEF, ChainContextLookupContext &context) const {

    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++) {
      if ((this+rule[i]).apply (LOOKUP_ARGS, context))
        return true;
    }

    return false;
  }

  private:
  OffsetArrayOf<ChainRule>
		rule;			/* Array of ChainRule tables
					 * ordered by preference */
};
ASSERT_SIZE (ChainRuleSet, 2);

struct ChainContextFormat1 {

  friend struct ChainContext;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (G_LIKELY (index == NOT_COVERED))
      return false;

    const ChainRuleSet &rule_set = this+ruleSet[index];
    struct ChainContextLookupContext context = {
      {match_glyph, apply_func},
      {NULL, NULL, NULL}
    };
    return rule_set.apply (LOOKUP_ARGS, context);
  }
  private:
  USHORT	format;			/* Format identifier--format = 1 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  OffsetArrayOf<ChainRuleSet>
		ruleSet;		/* Array of ChainRuleSet tables
					 * ordered by Coverage Index */
};
ASSERT_SIZE (ChainContextFormat1, 6);

struct ChainContextFormat2 {

  friend struct ChainContext;

  private:
  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (G_LIKELY (index == NOT_COVERED))
      return false;

    const ClassDef &backtrack_class_def = this+backtrackClassDef;
    const ClassDef &input_class_def = this+inputClassDef;
    const ClassDef &lookahead_class_def = this+lookaheadClassDef;

    index = input_class_def (IN_CURGLYPH ());
    const ChainRuleSet &rule_set = this+ruleSet[index];
    /* LONGTERMTODO: Old code fetches glyph classes at most once and caches
     * them across subrule lookups.  Not sure it's worth it.
     */
    struct ChainContextLookupContext context = {
     {match_class, apply_func},
     {(char *) &backtrack_class_def,
      (char *) &input_class_def,
      (char *) &lookahead_class_def}
    };
    return rule_set.apply (LOOKUP_ARGS, context);
  }

  private:
  USHORT	format;			/* Format identifier--format = 2 */
  OffsetTo<Coverage>
		coverage;		/* Offset to Coverage table--from
					 * beginning of table */
  OffsetTo<ClassDef>
		backtrackClassDef;	/* Offset to glyph ClassDef table
					 * containing backtrack sequence
					 * data--from beginning of table */
  OffsetTo<ClassDef>
		inputClassDef;		/* Offset to glyph ClassDef
					 * table containing input sequence
					 * data--from beginning of table */
  OffsetTo<ClassDef>
		lookaheadClassDef;	/* Offset to glyph ClassDef table
					 * containing lookahead sequence
					 * data--from beginning of table */
  OffsetArrayOf<ChainRuleSet>
		ruleSet;		/* Array of ChainRuleSet tables
					 * ordered by class */
};
ASSERT_SIZE (ChainContextFormat2, 12);

struct ChainContextFormat3 {

  friend struct ChainContext;

  private:

  inline bool apply_coverage (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {
//    const LookupRecord *record = (const LookupRecord *) ((const char *) coverage + sizeof (coverage[0]) * glyphCount);
    struct ChainContextLookupContext context = {
      {match_coverage, apply_func},
      {(char *) this, (char *) this, (char *) this}
    };
    /*
    return context_lookup (LOOKUP_ARGS,
			   glyphCount,
			   (const USHORT *) (coverage + 1),
			   lookupCount,
			   record,
			   context);
			   */
    return false;
  }

  inline bool apply (LOOKUP_ARGS_DEF, apply_lookup_func_t apply_func) const {

    /* XXX */
    unsigned int index = 0;//(this+coverage[0]) (IN_CURGLYPH ());
    if (G_LIKELY (index == NOT_COVERED))
      return false;

    return apply_coverage (LOOKUP_ARGS, apply_func);
  }

  private:
  USHORT	format;			/* Format identifier--format = 3 */
  USHORT	backtrackGlyphCount;	/* Number of glyphs in the backtracking
					 * sequence */
  Offset	backtrackCoverage[];	/* Array of offsets to coverage tables
					 * in backtracking sequence, in  glyph
					 * sequence order */
  USHORT	inputGlyphCountX;	/* Number of glyphs in input sequence */
  Offset	inputCoverageX[];	/* Array of offsets to coverage
					 * tables in input sequence, in glyph
					 * sequence order */
  USHORT	lookaheadGlyphCountX;	/* Number of glyphs in lookahead
					 * sequence */
  Offset	lookaheadCoverageX[];	/* Array of offsets to coverage tables
					 * in lookahead sequence, in glyph
					 * sequence order */
  USHORT	lookupCountX;		/* Number of LookupRecords */
  LookupRecord	lookupRecordX[];	/* Array of LookupRecords--in
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
