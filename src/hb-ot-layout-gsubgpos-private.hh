/*
 * Copyright (C) 2007,2008,2009  Red Hat, Inc.
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
 */

#ifndef HB_OT_LAYOUT_GSUBGPOS_PRIVATE_HH
#define HB_OT_LAYOUT_GSUBGPOS_PRIVATE_HH

#include "hb-buffer-private.h"
#include "hb-ot-layout-gdef-private.hh"


#ifndef HB_DEBUG_APPLY
#define HB_DEBUG_APPLY HB_DEBUG+0
#endif

#define TRACE_APPLY() \
	hb_trace_t<HB_DEBUG_APPLY> trace (&context->debug_depth); \
	trace.log ("APPLY", HB_FUNC, this);


struct hb_apply_context_t
{
  unsigned int debug_depth;
  hb_ot_layout_context_t *layout;
  hb_buffer_t *buffer;
  unsigned int context_length;
  unsigned int nesting_level_left;
  unsigned int lookup_flag;
  unsigned int property; /* propety of first glyph (TODO remove) */
};




#undef BUFFER
#define BUFFER context->buffer


typedef bool (*match_func_t) (hb_codepoint_t glyph_id, const USHORT &value, const char *data);
typedef bool (*apply_lookup_func_t) (hb_apply_context_t *context, unsigned int lookup_index);

struct ContextFuncs
{
  match_func_t match;
  apply_lookup_func_t apply;
};


static inline bool match_glyph (hb_codepoint_t glyph_id, const USHORT &value, const char *data HB_UNUSED)
{
  return glyph_id == value;
}

static inline bool match_class (hb_codepoint_t glyph_id, const USHORT &value, const char *data)
{
  const ClassDef &class_def = *reinterpret_cast<const ClassDef *>(data);
  return class_def.get_class (glyph_id) == value;
}

static inline bool match_coverage (hb_codepoint_t glyph_id, const USHORT &value, const char *data)
{
  const OffsetTo<Coverage> &coverage = (const OffsetTo<Coverage>&)value;
  return (data+coverage) (glyph_id) != NOT_COVERED;
}


static inline bool match_input (hb_apply_context_t *context,
				unsigned int count, /* Including the first glyph (not matched) */
				const USHORT input[], /* Array of input values--start with second glyph */
				match_func_t match_func,
				const char *match_data,
				unsigned int *context_length_out)
{
  unsigned int i, j;
  unsigned int end = MIN (context->buffer->in_length, context->buffer->in_pos + context->context_length);
  if (unlikely (context->buffer->in_pos + count > end))
    return false;

  for (i = 1, j = context->buffer->in_pos + 1; i < count; i++, j++)
  {
    while (_hb_ot_layout_skip_mark (context->layout->face, IN_INFO (j), context->lookup_flag, NULL))
    {
      if (unlikely (j + count - i == end))
	return false;
      j++;
    }

    if (likely (!match_func (IN_GLYPH (j), input[i - 1], match_data)))
      return false;
  }

  *context_length_out = j - context->buffer->in_pos;

  return true;
}

static inline bool match_backtrack (hb_apply_context_t *context,
				    unsigned int count,
				    const USHORT backtrack[],
				    match_func_t match_func,
				    const char *match_data)
{
  if (unlikely (context->buffer->out_pos < count))
    return false;

  for (unsigned int i = 0, j = context->buffer->out_pos - 1; i < count; i++, j--)
  {
    while (_hb_ot_layout_skip_mark (context->layout->face, OUT_INFO (j), context->lookup_flag, NULL))
    {
      if (unlikely (j + 1 == count - i))
	return false;
      j--;
    }

    if (likely (!match_func (OUT_GLYPH (j), backtrack[i], match_data)))
      return false;
  }

  return true;
}

static inline bool match_lookahead (hb_apply_context_t *context,
				    unsigned int count,
				    const USHORT lookahead[],
				    match_func_t match_func,
				    const char *match_data,
				    unsigned int offset)
{
  unsigned int i, j;
  unsigned int end = MIN (context->buffer->in_length, context->buffer->in_pos + context->context_length);
  if (unlikely (context->buffer->in_pos + offset + count > end))
    return false;

  for (i = 0, j = context->buffer->in_pos + offset; i < count; i++, j++)
  {
    while (_hb_ot_layout_skip_mark (context->layout->face, OUT_INFO (j), context->lookup_flag, NULL))
    {
      if (unlikely (j + count - i == end))
	return false;
      j++;
    }

    if (likely (!match_func (IN_GLYPH (j), lookahead[i], match_data)))
      return false;
  }

  return true;
}


struct LookupRecord
{
  static inline unsigned int get_size () { return sizeof (LookupRecord); }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  USHORT	sequenceIndex;		/* Index into current glyph
					 * sequence--first glyph = 0 */
  USHORT	lookupListIndex;	/* Lookup to apply to that
					 * position--zero--based */
};
ASSERT_SIZE (LookupRecord, 4);

static inline bool apply_lookup (hb_apply_context_t *context,
				 unsigned int count, /* Including the first glyph */
				 unsigned int lookupCount,
				 const LookupRecord lookupRecord[], /* Array of LookupRecords--in design order */
				 apply_lookup_func_t apply_func)
{
  unsigned int end = MIN (context->buffer->in_length, context->buffer->in_pos + context->context_length);
  if (unlikely (context->buffer->in_pos + count > end))
    return false;

  /* TODO We don't support lookupRecord arrays that are not increasing:
   *      Should be easy for in_place ones at least. */

  /* Note: If sublookup is reverse, i will underflow after the first loop
   * and we jump out of it.  Not entirely disastrous.  So we don't check
   * for reverse lookup here.
   */
  for (unsigned int i = 0; i < count; /* NOP */)
  {
    while (_hb_ot_layout_skip_mark (context->layout->face, IN_CURINFO (), context->lookup_flag, NULL))
    {
      if (unlikely (context->buffer->in_pos == end))
	return true;
      /* No lookup applied for this index */
      _hb_buffer_next_glyph (context->buffer);
    }

    if (lookupCount && i == lookupRecord->sequenceIndex)
    {
      unsigned int old_pos = context->buffer->in_pos;

      /* Apply a lookup */
      bool done = apply_func (context, lookupRecord->lookupListIndex);

      lookupRecord++;
      lookupCount--;
      /* Err, this is wrong if the lookup jumped over some glyphs */
      i += context->buffer->in_pos - old_pos;
      if (unlikely (context->buffer->in_pos == end))
	return true;

      if (!done)
	goto not_applied;
    }
    else
    {
    not_applied:
      /* No lookup applied for this index */
      _hb_buffer_next_glyph (context->buffer);
      i++;
    }
  }

  return true;
}


/* Contextual lookups */

struct ContextLookupContext
{
  ContextFuncs funcs;
  const char *match_data;
};

static inline bool context_lookup (hb_apply_context_t *context,
				   unsigned int inputCount, /* Including the first glyph (not matched) */
				   const USHORT input[], /* Array of input values--start with second glyph */
				   unsigned int lookupCount,
				   const LookupRecord lookupRecord[],
				   ContextLookupContext &lookup_context)
{
  hb_apply_context_t new_context = *context;
  return match_input (context,
		      inputCount, input,
		      lookup_context.funcs.match, lookup_context.match_data,
		      &new_context.context_length)
      && apply_lookup (&new_context,
		       inputCount,
		       lookupCount, lookupRecord,
		       lookup_context.funcs.apply);
}

struct Rule
{
  friend struct RuleSet;

  private:
  inline bool apply (hb_apply_context_t *context, ContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (input, input[0].get_size () * (inputCount ? inputCount - 1 : 0));
    return context_lookup (context,
			   inputCount, input,
			   lookupCount, lookupRecord,
			   lookup_context);
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE (inputCount)
	&& SANITIZE (lookupCount)
	&& SANITIZE_MEM (input,
			 input[0].get_size () * inputCount +
			 lookupRecordX[0].get_size () * lookupCount);
  }

  private:
  USHORT	inputCount;		/* Total number of glyphs in input
					 * glyph sequence--includes the  first
					 * glyph */
  USHORT	lookupCount;		/* Number of LookupRecords */
  USHORT	input[VAR];		/* Array of match inputs--start with
					 * second glyph */
  LookupRecord	lookupRecordX[VAR];	/* Array of LookupRecords--in
					 * design order */
};
ASSERT_SIZE_VAR2 (Rule, 4, USHORT, LookupRecord);

struct RuleSet
{
  inline bool apply (hb_apply_context_t *context, ContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
    {
      if ((this+rule[i]).apply (context, lookup_context))
        return true;
    }

    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_WITH_BASE (this, rule);
  }

  private:
  OffsetArrayOf<Rule>
		rule;			/* Array of Rule tables
					 * ordered by preference */
};


struct ContextFormat1
{
  friend struct Context;

  private:
  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    const RuleSet &rule_set = this+ruleSet[index];
    struct ContextLookupContext lookup_context = {
      {match_glyph, apply_func},
      NULL
    };
    return rule_set.apply (context, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_WITH_BASE (this, coverage)
	&& SANITIZE_WITH_BASE (this, ruleSet);
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


struct ContextFormat2
{
  friend struct Context;

  private:
  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    const ClassDef &class_def = this+classDef;
    index = class_def (IN_CURGLYPH ());
    const RuleSet &rule_set = this+ruleSet[index];
    /* LONGTERMTODO: Old code fetches glyph classes at most once and caches
     * them across subrule lookups.  Not sure it's worth it.
     */
    struct ContextLookupContext lookup_context = {
     {match_class, apply_func},
      CharP(&class_def)
    };
    return rule_set.apply (context, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_WITH_BASE (this, coverage)
        && SANITIZE_WITH_BASE (this, classDef)
	&& SANITIZE_WITH_BASE (this, ruleSet);
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


struct ContextFormat3
{
  friend struct Context;

  private:
  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage[0]) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    const LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, coverage[0].get_size () * glyphCount);
    struct ContextLookupContext lookup_context = {
      {match_coverage, apply_func},
       CharP(this)
    };
    return context_lookup (context,
			   glyphCount, (const USHORT *) (coverage + 1),
			   lookupCount, lookupRecord,
			   lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!SANITIZE_SELF ()) return false;
    unsigned int count = glyphCount;
    if (!SANITIZE_ARRAY (coverage, OffsetTo<Coverage>::get_size (), count)) return false;
    for (unsigned int i = 0; i < count; i++)
      if (!SANITIZE_WITH_BASE (this, coverage[i])) return false;
    LookupRecord *lookupRecord = &StructAtOffset<LookupRecord> (coverage, OffsetTo<Coverage>::get_size () * count);
    return SANITIZE_ARRAY (lookupRecord, LookupRecord::get_size (), lookupCount);
  }

  private:
  USHORT	format;			/* Format identifier--format = 3 */
  USHORT	glyphCount;		/* Number of glyphs in the input glyph
					 * sequence */
  USHORT	lookupCount;		/* Number of LookupRecords */
  OffsetTo<Coverage>
		coverage[VAR];		/* Array of offsets to Coverage
					 * table in glyph sequence order */
  LookupRecord	lookupRecordX[VAR];	/* Array of LookupRecords--in
					 * design order */
};
ASSERT_SIZE_VAR2 (ContextFormat3, 6, OffsetTo<Coverage>, LookupRecord);

struct Context
{
  protected:
  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context, apply_func);
    case 2: return u.format2->apply (context, apply_func);
    case 3: return u.format3->apply (context, apply_func);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    case 2: return u.format2->sanitize (context);
    case 3: return u.format3->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  ContextFormat1	format1[VAR];
  ContextFormat2	format2[VAR];
  ContextFormat3	format3[VAR];
  } u;
};


/* Chaining Contextual lookups */

struct ChainContextLookupContext
{
  ContextFuncs funcs;
  const char *match_data[3];
};

static inline bool chain_context_lookup (hb_apply_context_t *context,
					 unsigned int backtrackCount,
					 const USHORT backtrack[],
					 unsigned int inputCount, /* Including the first glyph (not matched) */
					 const USHORT input[], /* Array of input values--start with second glyph */
					 unsigned int lookaheadCount,
					 const USHORT lookahead[],
					 unsigned int lookupCount,
					 const LookupRecord lookupRecord[],
					 ChainContextLookupContext &lookup_context)
{
  /* First guess */
  if (unlikely (context->buffer->out_pos < backtrackCount ||
		context->buffer->in_pos + inputCount + lookaheadCount > context->buffer->in_length ||
		inputCount + lookaheadCount > context->context_length))
    return false;

  hb_apply_context_t new_context = *context;
  return match_backtrack (context,
			  backtrackCount, backtrack,
			  lookup_context.funcs.match, lookup_context.match_data[0])
      && match_input (context,
		      inputCount, input,
		      lookup_context.funcs.match, lookup_context.match_data[1],
		      &new_context.context_length)
      && match_lookahead (context,
			  lookaheadCount, lookahead,
			  lookup_context.funcs.match, lookup_context.match_data[2],
			  new_context.context_length)
      && apply_lookup (&new_context,
		       inputCount,
		       lookupCount, lookupRecord,
		       lookup_context.funcs.apply);
}

struct ChainRule
{
  friend struct ChainRuleSet;

  private:
  inline bool apply (hb_apply_context_t *context, ChainContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    const HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    const ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return chain_context_lookup (context,
				 backtrack.len, backtrack.array(),
				 input.len, input.array(),
				 lookahead.len, lookahead.array(),
				 lookup.len, lookup.array(),
				 lookup_context);
    return false;
  }

  public:
  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!SANITIZE (backtrack)) return false;
    HeadlessArrayOf<USHORT> &input = StructAfter<HeadlessArrayOf<USHORT> > (backtrack);
    if (!SANITIZE (input)) return false;
    ArrayOf<USHORT> &lookahead = StructAfter<ArrayOf<USHORT> > (input);
    if (!SANITIZE (lookahead)) return false;
    ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return SANITIZE (lookup);
  }

  private:
  ArrayOf<USHORT>
		backtrack;		/* Array of backtracking values
					 * (to be matched before the input
					 * sequence) */
  HeadlessArrayOf<USHORT>
		inputX;			/* Array of input values (start with
					 * second glyph) */
  ArrayOf<USHORT>
		lookaheadX;		/* Array of lookahead values's (to be
					 * matched after the input sequence) */
  ArrayOf<LookupRecord>
		lookupX;		/* Array of LookupRecords--in
					 * design order) */
};
ASSERT_SIZE (ChainRule, 8);

struct ChainRuleSet
{
  inline bool apply (hb_apply_context_t *context, ChainContextLookupContext &lookup_context) const
  {
    TRACE_APPLY ();
    unsigned int num_rules = rule.len;
    for (unsigned int i = 0; i < num_rules; i++)
    {
      if ((this+rule[i]).apply (context, lookup_context))
        return true;
    }

    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_WITH_BASE (this, rule);
  }

  private:
  OffsetArrayOf<ChainRule>
		rule;			/* Array of ChainRule tables
					 * ordered by preference */
};
ASSERT_SIZE (ChainRuleSet, 2);

struct ChainContextFormat1
{
  friend struct ChainContext;

  private:
  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    const ChainRuleSet &rule_set = this+ruleSet[index];
    struct ChainContextLookupContext lookup_context = {
      {match_glyph, apply_func},
      {NULL, NULL, NULL}
    };
    return rule_set.apply (context, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_WITH_BASE (this, coverage)
	&& SANITIZE_WITH_BASE (this, ruleSet);
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

struct ChainContextFormat2
{
  friend struct ChainContext;

  private:
  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    unsigned int index = (this+coverage) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    const ClassDef &backtrack_class_def = this+backtrackClassDef;
    const ClassDef &input_class_def = this+inputClassDef;
    const ClassDef &lookahead_class_def = this+lookaheadClassDef;

    index = input_class_def (IN_CURGLYPH ());
    const ChainRuleSet &rule_set = this+ruleSet[index];
    /* LONGTERMTODO: Old code fetches glyph classes at most once and caches
     * them across subrule lookups.  Not sure it's worth it.
     */
    struct ChainContextLookupContext lookup_context = {
     {match_class, apply_func},
     {CharP(&backtrack_class_def),
      CharP(&input_class_def),
      CharP(&lookahead_class_def)}
    };
    return rule_set.apply (context, lookup_context);
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_WITH_BASE (this, coverage)
	&& SANITIZE_WITH_BASE (this, backtrackClassDef)
	&& SANITIZE_WITH_BASE (this, inputClassDef)
	&& SANITIZE_WITH_BASE (this, lookaheadClassDef)
	&& SANITIZE_WITH_BASE (this, ruleSet);
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

struct ChainContextFormat3
{
  friend struct ChainContext;

  private:

  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    const OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);

    unsigned int index = (this+input[0]) (IN_CURGLYPH ());
    if (likely (index == NOT_COVERED))
      return false;

    const OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    const ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    struct ChainContextLookupContext lookup_context = {
      {match_coverage, apply_func},
      {CharP(this), CharP(this), CharP(this)}
    };
    return chain_context_lookup (context,
				 backtrack.len, (const USHORT *) backtrack.array(),
				 input.len, (const USHORT *) input.array() + 1,
				 lookahead.len, (const USHORT *) lookahead.array(),
				 lookup.len, lookup.array(),
				 lookup_context);
    return false;
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!SANITIZE_WITH_BASE (this, backtrack)) return false;
    OffsetArrayOf<Coverage> &input = StructAfter<OffsetArrayOf<Coverage> > (backtrack);
    if (!SANITIZE_WITH_BASE (this, input)) return false;
    OffsetArrayOf<Coverage> &lookahead = StructAfter<OffsetArrayOf<Coverage> > (input);
    if (!SANITIZE_WITH_BASE (this, lookahead)) return false;
    ArrayOf<LookupRecord> &lookup = StructAfter<ArrayOf<LookupRecord> > (lookahead);
    return SANITIZE (lookup);
  }

  private:
  USHORT	format;			/* Format identifier--format = 3 */
  OffsetArrayOf<Coverage>
		backtrack;		/* Array of coverage tables
					 * in backtracking sequence, in  glyph
					 * sequence order */
  OffsetArrayOf<Coverage>
		inputX		;	/* Array of coverage
					 * tables in input sequence, in glyph
					 * sequence order */
  OffsetArrayOf<Coverage>
		lookaheadX;		/* Array of coverage tables
					 * in lookahead sequence, in glyph
					 * sequence order */
  ArrayOf<LookupRecord>
		lookupX;		/* Array of LookupRecords--in
					 * design order) */
};
ASSERT_SIZE (ChainContextFormat3, 10);

struct ChainContext
{
  protected:
  inline bool apply (hb_apply_context_t *context, apply_lookup_func_t apply_func) const
  {
    TRACE_APPLY ();
    switch (u.format) {
    case 1: return u.format1->apply (context, apply_func);
    case 2: return u.format2->apply (context, apply_func);
    case 3: return u.format3->apply (context, apply_func);
    default:return false;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    case 2: return u.format2->sanitize (context);
    case 3: return u.format3->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;	/* Format identifier */
  ChainContextFormat1	format1[VAR];
  ChainContextFormat2	format2[VAR];
  ChainContextFormat3	format3[VAR];
  } u;
};


struct ExtensionFormat1
{
  friend struct Extension;

  protected:
  inline unsigned int get_type (void) const { return extensionLookupType; }
  inline unsigned int get_offset (void) const { return extensionOffset; }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE_SELF ();
  }

  private:
  USHORT	format;			/* Format identifier. Set to 1. */
  USHORT	extensionLookupType;	/* Lookup type of subtable referenced
					 * by ExtensionOffset (i.e. the
					 * extension subtable). */
  ULONG		extensionOffset;	/* Offset to the extension subtable,
					 * of lookup type subtable. */
};
ASSERT_SIZE (ExtensionFormat1, 8);

struct Extension
{
  inline unsigned int get_type (void) const
  {
    switch (u.format) {
    case 1: return u.format1->get_type ();
    default:return 0;
    }
  }
  inline unsigned int get_offset (void) const
  {
    switch (u.format) {
    case 1: return u.format1->get_offset ();
    default:return 0;
    }
  }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    if (!SANITIZE (u.format)) return false;
    switch (u.format) {
    case 1: return u.format1->sanitize (context);
    default:return true;
    }
  }

  private:
  union {
  USHORT		format;		/* Format identifier */
  ExtensionFormat1	format1[VAR];
  } u;
};


/*
 * GSUB/GPOS Common
 */

struct GSUBGPOS
{
  static const hb_tag_t GSUBTag	= HB_OT_TAG_GSUB;
  static const hb_tag_t GPOSTag	= HB_OT_TAG_GPOS;

  inline unsigned int get_script_count (void) const
  { return (this+scriptList).len; }
  inline const Tag& get_script_tag (unsigned int i) const
  { return (this+scriptList).get_tag (i); }
  inline unsigned int get_script_tags (unsigned int start_offset,
				       unsigned int *script_count /* IN/OUT */,
				       hb_tag_t     *script_tags /* OUT */) const
  { return (this+scriptList).get_tags (start_offset, script_count, script_tags); }
  inline const Script& get_script (unsigned int i) const
  { return (this+scriptList)[i]; }
  inline bool find_script_index (hb_tag_t tag, unsigned int *index) const
  { return (this+scriptList).find_index (tag, index); }

  inline unsigned int get_feature_count (void) const
  { return (this+featureList).len; }
  inline const Tag& get_feature_tag (unsigned int i) const
  { return (this+featureList).get_tag (i); }
  inline unsigned int get_feature_tags (unsigned int start_offset,
					unsigned int *feature_count /* IN/OUT */,
					hb_tag_t     *feature_tags /* OUT */) const
  { return (this+featureList).get_tags (start_offset, feature_count, feature_tags); }
  inline const Feature& get_feature (unsigned int i) const
  { return (this+featureList)[i]; }
  inline bool find_feature_index (hb_tag_t tag, unsigned int *index) const
  { return (this+featureList).find_index (tag, index); }

  inline unsigned int get_lookup_count (void) const
  { return (this+lookupList).len; }
  inline const Lookup& get_lookup (unsigned int i) const
  { return (this+lookupList)[i]; }

  inline bool sanitize (hb_sanitize_context_t *context) {
    TRACE_SANITIZE ();
    return SANITIZE (version) && likely (version.major == 1)
	&& SANITIZE_WITH_BASE (this, scriptList)
	&& SANITIZE_WITH_BASE (this, featureList)
	&& SANITIZE_WITH_BASE (this, lookupList);
  }

  protected:
  FixedVersion	version;	/* Version of the GSUB/GPOS table--initially set
				 * to 0x00010000 */
  OffsetTo<ScriptList>
		scriptList;  	/* ScriptList table */
  OffsetTo<FeatureList>
		featureList; 	/* FeatureList table */
  OffsetTo<LookupList>
		lookupList; 	/* LookupList table */
};
ASSERT_SIZE (GSUBGPOS, 10);


#endif /* HB_OT_LAYOUT_GSUBGPOS_PRIVATE_HH */
