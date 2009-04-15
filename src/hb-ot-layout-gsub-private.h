/*
 * Copyright (C) 2007,2008  Red Hat, Inc.
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


struct SingleSubstFormat1 {

  friend struct SingleSubst;

  private:
  inline bool substitute (hb_ot_layout_t *layout,
			  hb_buffer_t    *buffer,
			  unsigned int    context_length,
			  unsigned int    nesting_level_left) const {
//    if (get_coverage (IN_CURGLYPH()))
//      return ;
  }

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  SHORT		deltaGlyphID;		/* Add to original GlyphID to get
					 * substitute GlyphID */
};
ASSERT_SIZE (SingleSubstFormat1, 6);

struct SingleSubstFormat2 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 2 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  USHORT	glyphCount;		/* Number of GlyphIDs in the Substitute
					 * array */
  GlyphID	substitute[];		/* Array of substitute
					 * GlyphIDs--ordered by Coverage  Index */
};
ASSERT_SIZE (SingleSubstFormat2, 6);

struct MultipleSubstFormat1 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  USHORT	sequenceCount;		/* Number of Sequence table offsets in
					 * the Sequence array */
  Offset	sequence[];		/* Array of offsets to Sequence
					 * tables--from beginning of
					 * Substitution table--ordered by
					 * Coverage Index */
};
ASSERT_SIZE (MultipleSubstFormat1, 6);

struct Sequence {
  /* TODO */

  private:
  USHORT	glyphCount;		/* Number of GlyphIDs in the Substitute
					 * array. This should always  be
					 * greater than 0. */
  GlyphID	substitute[];		/* String of GlyphIDs to substitute */
};
DEFINE_NULL_ASSERT_SIZE (Sequence, 2);

struct AlternateSubstFormat1 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  USHORT	alternateSetCount;	/* Number of AlternateSet tables */
  Offset	alternateSet[];		/* Array of offsets to AlternateSet
					 * tables--from beginning of
					 * Substitution table--ordered by
					 * Coverage Index */
};
ASSERT_SIZE (AlternateSubstFormat1, 6);

struct AlternateSet {
  /* TODO */

  private:
  USHORT	glyphCount;		/* Number of GlyphIDs in the Alternate
					 * array */
  GlyphID	alternate[];		/* Array of alternate GlyphIDs--in
					 * arbitrary order */
};
DEFINE_NULL_ASSERT_SIZE (AlternateSet, 2);

struct LigatureSubstFormat1 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  USHORT	ligSetCount;		/* Number of LigatureSet tables */
  Offset	ligatureSet[];		/* Array of offsets to LigatureSet
					 * tables--from beginning of
					 * Substitution table--ordered by
					 * Coverage Index */
};
ASSERT_SIZE (LigatureSubstFormat1, 6);

struct LigatureSet {
  /* TODO */

  private:
  USHORT	ligatureCount;		/* Number of Ligature tables */
  Offset	ligature[];		/* Array of offsets to Ligature
					 * tables--from beginning of
					 * LigatureSet table--ordered by
					 * preference */
};
DEFINE_NULL_ASSERT_SIZE (LigatureSet, 2);

struct Ligature {
  /* TODO */

  private:
  GlyphID	ligGlyph;		/* GlyphID of ligature to substitute */
  USHORT	compCount;		/* Number of components in the ligature */
  GlyphID	component[];		/* Array of component GlyphIDs--start
					 * with the second  component--ordered
					 * in writing direction */
};
DEFINE_NULL_ASSERT_SIZE (Ligature, 4);

struct SubstLookupRecord {
  /* TODO */

  private:
  USHORT	sequenceIndex;		/* Index into current glyph
					 * sequence--first glyph = 0 */
  USHORT	lookupListIndex;	/* Lookup to apply to that
					 * position--zero--based */
};
DEFINE_NULL_ASSERT_SIZE (SubstLookupRecord, 4);

struct ContextSubstFormat1 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 1 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  USHORT	subRuleSetCount;	/* Number of SubRuleSet tables--must
					 * equal GlyphCount in Coverage  table */
  Offset	subRuleSet[];		/* Array of offsets to SubRuleSet
					 * tables--from beginning of
					 * Substitution table--ordered by
					 * Coverage Index */
};
ASSERT_SIZE (ContextSubstFormat1, 6);

struct SubRuleSet {
  /* TODO */

  private:
  USHORT	subRuleCount;		/* Number of SubRule tables */
  Offset	subRule[];		/* Array of offsets to SubRule
					 * tables--from beginning of SubRuleSet
					 * table--ordered by preference */
};
DEFINE_NULL_ASSERT_SIZE (SubRuleSet, 2);

struct SubRule {
  /* TODO */

  private:
  USHORT	glyphCount;		/* Total number of glyphs in input
					 * glyph sequence--includes the  first
					 * glyph */
  USHORT	substCount;		/* Number of SubstLookupRecords */
  GlyphID	input[];		/* Array of input GlyphIDs--start with
					 * second glyph */
  SubstLookupRecord substLookupRecord[];/* Array of SubstLookupRecords--in
					 * design order */
};
DEFINE_NULL_ASSERT_SIZE (SubRule, 4);

struct ContextSubstFormat2 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 2 */
  Offset	coverage;		/* Offset to Coverage table--from
					 * beginning of Substitution table */
  Offset	classDef;		/* Offset to glyph ClassDef table--from
					 * beginning of Substitution  table */
  USHORT	subClassSetCnt;		/* Number of SubClassSet tables */
  Offset	subClassSet[];		/* Array of offsets to SubClassSet
					 * tables--from beginning of
					 * Substitution table--ordered by
					 * class--may be NULL */
};
ASSERT_SIZE (ContextSubstFormat2, 8);

struct SubClassSet {
  /* TODO */

  private:
  USHORT	subClassRuleCnt;	/* Number of SubClassRule tables */
  Offset	subClassRule[];		/* Array of offsets to SubClassRule
					 * tables--from beginning of
					 * SubClassSet--ordered by preference */
};
DEFINE_NULL_ASSERT_SIZE (SubClassSet, 2);

struct SubClassRule {
  /* TODO */

  private:
  USHORT	glyphCount;		/* Total number of classes
					 * specified for the context in the
					 * rule--includes the first class */
  USHORT	substCount;		/* Number of SubstLookupRecords */
  USHORT	klass[];		/* Array of classes--beginning with the
					 * second class--to be matched  to the
					 * input glyph class sequence */
  SubstLookupRecord substLookupRecord[];/* Array of SubstLookupRecords--in
					 * design order */
};
DEFINE_NULL_ASSERT_SIZE (SubClassRule, 4);

struct ContextSubstFormat3 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier--format = 3 */
  USHORT	glyphCount;		/* Number of glyphs in the input glyph
					 * sequence */
  USHORT	substCount;		/* Number of SubstLookupRecords */
  Offset	coverage[];		/* Array of offsets to Coverage
					 * table--from beginning of
					 * Substitution table--in glyph
					 * sequence order */
  SubstLookupRecord substLookupRecord[];/* Array of SubstLookupRecords--in
					 * design order */
};
ASSERT_SIZE (ContextSubstFormat3, 6);

struct ChainContextSubstFormat1 {
  /* TODO */

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

struct ChainSubRuleSet {
  /* TODO */

  private:
  USHORT	chainSubRuleCount;	/* Number of ChainSubRule tables */
  Offset	chainSubRule[];		/* Array of offsets to ChainSubRule
					 * tables--from beginning of
					 * ChainSubRuleSet table--ordered
					 * by preference */
};
DEFINE_NULL_ASSERT_SIZE (ChainSubRuleSet, 2);

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
  USHORT	substCount;		/* Number of SubstLookupRecords */
  SubstLookupRecord substLookupRecord[];/* Array of SubstLookupRecords--in
					 * design order) */
};
DEFINE_NULL_ASSERT_SIZE (ChainSubRule, 8);

struct ChainContextSubstFormat2 {
  /* TODO */

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
DEFINE_NULL_ASSERT_SIZE (ChainSubClassSet, 2);

struct ChainSubClassRule {
  /* TODO */

  private:
  USHORT	backtrackGlyphCount;	/* Total number of glyphs in the
					 * backtrack sequence (number of
					 * glyphs to be matched before the
					 * first glyph) */
  USHORT	backtrack[];		/* Array of backtracking classes(to be
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
  USHORT	lookAhead[];		/* Array of lookahead classes(to be
					 * matched after the  input sequence) */
  USHORT	substCount;		/* Number of SubstLookupRecords */
  SubstLookupRecord substLookupRecord[];/* Array of SubstLookupRecords--in
					 * design order) */
};
DEFINE_NULL_ASSERT_SIZE (ChainSubClassRule, 8);

struct ChainContextSubstFormat3 {
  /* TODO */

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
  USHORT	substCount;		/* Number of SubstLookupRecords */
  SubstLookupRecord substLookupRecord[];/* Array of SubstLookupRecords--in
					 * design order */
};
ASSERT_SIZE (ChainContextSubstFormat3, 10);

struct ExtensionSubstFormat1 {
  /* TODO */

  private:
  USHORT	substFormat;		/* Format identifier. Set to 1. */
  USHORT	extensionLookupType;	/* Lookup type of subtable referenced
					 * by ExtensionOffset (i.e. the
					 * extension subtable). */
  ULONG		extensionOffset;	/* Offset to the extension subtable,
					 * of lookup type  subtable. */
};
ASSERT_SIZE (ExtensionSubstFormat1, 8);

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

struct SubstLookupSubTable {
  DEFINE_NON_INSTANTIABLE(SubstLookupSubTable);

  friend struct SubstLookup;

  unsigned int get_size (unsigned int lookup_type) const {
    switch (lookup_type) {
//    case 1: return u.format1.get_size ();
//    case 2: return u.format2.get_size ();
    /*
    case Single:
    case Multiple:
    case Alternate:
    case Ligature:
    case Context:
    case ChainingContext:
    case Extension:
    case ReverseChainingContextSingle:
    */
    default:return sizeof (LookupSubTable);
    }
  }

  inline bool substitute (hb_ot_layout_t *layout,
			  hb_buffer_t    *buffer,
			  unsigned int    context_length,
			  unsigned int    nesting_level_left,
			  unsigned int    lookup_type) const {
  }

  private:
  union {
  USHORT		substFormat;
  CoverageFormat1	format1;
  CoverageFormat2	format2;
  } u;
};

struct SubstLookup : Lookup {

  DEFINE_NON_INSTANTIABLE(SubstLookup);

  static const unsigned int Single				= 1;
  static const unsigned int Multiple				= 2;
  static const unsigned int Alternate				= 3;
  static const unsigned int Ligature				= 4;
  static const unsigned int Context				= 5;
  static const unsigned int ChainingContext			= 6;
  static const unsigned int Extension				= 7;
  static const unsigned int ReverseChainingContextSingle	= 8;

  inline const SubstLookupSubTable& get_subtable (unsigned int i) const {
    return *(SubstLookupSubTable*)&(((Lookup *)this)->get_subtable (i));
  }

  /* Like get_type(), but looks through extension lookups.
   * Never returns SubstLookup::Extension */
  inline unsigned int get_effective_type (void) const {
    unsigned int type = get_type ();

    if (HB_UNLIKELY (type == Extension)) {
      /* Return lookup type of first extension subtable.
       * The spec says all of them should have the same type.
       * XXX check for that somehow */
//XXX      type = get_subtable(0).v.extension.get_type ();
    }

    return type;
  }

  inline bool is_reverse (void) const {
    switch (get_effective_type ()) {
    case ReverseChainingContextSingle:	return true;
    default:				return false;
    }
  }

  inline bool substitute (hb_ot_layout_t *layout,
			  hb_buffer_t    *buffer,
			  unsigned int    context_length,
			  unsigned int    nesting_level_left) const {
    unsigned int lookup_type = get_type ();

    if (HB_UNLIKELY (nesting_level_left == 0))
      return false;
    nesting_level_left--;
  
    for (unsigned int i = 0; i < get_subtable_count (); i++)
      if (get_subtable (i).substitute (layout, buffer,
				       context_length, nesting_level_left,
				       lookup_type))
	return true;
  
    return false;
  }
};
DEFINE_NULL_ALIAS (SubstLookup, Lookup);

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


};
DEFINE_NULL_ALIAS (GSUB, GSUBGPOS);


#endif /* HB_OT_LAYOUT_GSUB_PRIVATE_H */
