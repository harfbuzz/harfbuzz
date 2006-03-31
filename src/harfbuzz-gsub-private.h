/*******************************************************************
 *
 *  Copyright 1996-2000 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  Copyright 2006  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 *  See the file name COPYING for licensing information.
 *
 ******************************************************************/
#ifndef HARFBUZZ_GSUB_PRIVATE_H
#define HARFBUZZ_GSUB_PRIVATE_H

#include "harfbuzz-gsub.h"

FT_BEGIN_HEADER


typedef union HB_GSUB_SubTable_  HB_GSUB_SubTable;

/* LookupType 1 */

struct  HB_SingleSubstFormat1_
{
  FT_Short  DeltaGlyphID;             /* constant added to get
					 substitution glyph index */
};

typedef struct HB_SingleSubstFormat1_  HB_SingleSubstFormat1;


struct  HB_SingleSubstFormat2_
{
  FT_UShort   GlyphCount;             /* number of glyph IDs in
					 Substitute array              */
  FT_UShort*  Substitute;             /* array of substitute glyph IDs */
};

typedef struct HB_SingleSubstFormat2_  HB_SingleSubstFormat2;


struct  HB_SingleSubst_
{
  FT_UShort     SubstFormat;          /* 1 or 2         */
  HB_Coverage  Coverage;             /* Coverage table */

  union
  {
    HB_SingleSubstFormat1  ssf1;
    HB_SingleSubstFormat2  ssf2;
  } ssf;
};

typedef struct HB_SingleSubst_  HB_SingleSubst;


/* LookupType 2 */

struct  HB_Sequence_
{
  FT_UShort   GlyphCount;             /* number of glyph IDs in the
					 Substitute array           */
  FT_UShort*  Substitute;             /* string of glyph IDs to
					 substitute                 */
};

typedef struct HB_Sequence_  HB_Sequence;


struct  HB_MultipleSubst_
{
  FT_UShort      SubstFormat;         /* always 1                  */
  HB_Coverage   Coverage;            /* Coverage table            */
  FT_UShort      SequenceCount;       /* number of Sequence tables */
  HB_Sequence*  Sequence;            /* array of Sequence tables  */
};

typedef struct HB_MultipleSubst_  HB_MultipleSubst;


/* LookupType 3 */

struct  HB_AlternateSet_
{
  FT_UShort   GlyphCount;             /* number of glyph IDs in the
					 Alternate array              */
  FT_UShort*  Alternate;              /* array of alternate glyph IDs */
};

typedef struct HB_AlternateSet_  HB_AlternateSet;


struct  HB_AlternateSubst_
{
  FT_UShort          SubstFormat;     /* always 1                      */
  HB_Coverage       Coverage;        /* Coverage table                */
  FT_UShort          AlternateSetCount;
				      /* number of AlternateSet tables */
  HB_AlternateSet*  AlternateSet;    /* array of AlternateSet tables  */
};

typedef struct HB_AlternateSubst_  HB_AlternateSubst;


/* LookupType 4 */

struct  HB_Ligature_
{
  FT_UShort   LigGlyph;               /* glyphID of ligature
					 to substitute                    */
  FT_UShort   ComponentCount;         /* number of components in ligature */
  FT_UShort*  Component;              /* array of component glyph IDs     */
};

typedef struct HB_Ligature_  HB_Ligature;


struct  HB_LigatureSet_
{
  FT_UShort      LigatureCount;       /* number of Ligature tables */
  HB_Ligature*  Ligature;            /* array of Ligature tables  */
};

typedef struct HB_LigatureSet_  HB_LigatureSet;


struct  HB_LigatureSubst_
{
  FT_UShort         SubstFormat;      /* always 1                     */
  HB_Coverage      Coverage;         /* Coverage table               */
  FT_UShort         LigatureSetCount; /* number of LigatureSet tables */
  HB_LigatureSet*  LigatureSet;      /* array of LigatureSet tables  */
};

typedef struct HB_LigatureSubst_  HB_LigatureSubst;


/* needed by both lookup type 5 and 6 */

struct  HB_SubstLookupRecord_
{
  FT_UShort  SequenceIndex;           /* index into current
					 glyph sequence               */
  FT_UShort  LookupListIndex;         /* Lookup to apply to that pos. */
};

typedef struct HB_SubstLookupRecord_  HB_SubstLookupRecord;


/* LookupType 5 */

struct  HB_SubRule_
{
  FT_UShort               GlyphCount; /* total number of input glyphs */
  FT_UShort               SubstCount; /* number of SubstLookupRecord
					 tables                       */
  FT_UShort*              Input;      /* array of input glyph IDs     */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of SubstLookupRecord
					 tables                       */
};

typedef struct HB_SubRule_  HB_SubRule;


struct  HB_SubRuleSet_
{
  FT_UShort     SubRuleCount;         /* number of SubRule tables */
  HB_SubRule*  SubRule;              /* array of SubRule tables  */
};

typedef struct HB_SubRuleSet_  HB_SubRuleSet;


struct  HB_ContextSubstFormat1_
{
  HB_Coverage     Coverage;          /* Coverage table              */
  FT_UShort        SubRuleSetCount;   /* number of SubRuleSet tables */
  HB_SubRuleSet*  SubRuleSet;        /* array of SubRuleSet tables  */
};

typedef struct HB_ContextSubstFormat1_  HB_ContextSubstFormat1;


struct  HB_SubClassRule_
{
  FT_UShort               GlyphCount; /* total number of context classes */
  FT_UShort               SubstCount; /* number of SubstLookupRecord
					 tables                          */
  FT_UShort*              Class;      /* array of classes                */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of SubstLookupRecord
					 tables                          */
};

typedef struct HB_SubClassRule_  HB_SubClassRule;


struct  HB_SubClassSet_
{
  FT_UShort          SubClassRuleCount;
				      /* number of SubClassRule tables */
  HB_SubClassRule*  SubClassRule;    /* array of SubClassRule tables  */
};

typedef struct HB_SubClassSet_  HB_SubClassSet;


/* The `MaxContextLength' field is not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the context rules.                    */

struct  HB_ContextSubstFormat2_
{
  FT_UShort            MaxContextLength;
				      /* maximal context length       */
  HB_Coverage         Coverage;      /* Coverage table               */
  HB_ClassDefinition  ClassDef;      /* ClassDef table               */
  FT_UShort            SubClassSetCount;
				      /* number of SubClassSet tables */
  HB_SubClassSet*     SubClassSet;   /* array of SubClassSet tables  */
};

typedef struct HB_ContextSubstFormat2_  HB_ContextSubstFormat2;


struct  HB_ContextSubstFormat3_
{
  FT_UShort               GlyphCount; /* number of input glyphs        */
  FT_UShort               SubstCount; /* number of SubstLookupRecords  */
  HB_Coverage*           Coverage;   /* array of Coverage tables      */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of substitution lookups */
};

typedef struct HB_ContextSubstFormat3_  HB_ContextSubstFormat3;


struct  HB_ContextSubst_
{
  FT_UShort  SubstFormat;             /* 1, 2, or 3 */

  union
  {
    HB_ContextSubstFormat1  csf1;
    HB_ContextSubstFormat2  csf2;
    HB_ContextSubstFormat3  csf3;
  } csf;
};

typedef struct HB_ContextSubst_  HB_ContextSubst;


/* LookupType 6 */

struct  HB_ChainSubRule_
{
  FT_UShort               BacktrackGlyphCount;
				      /* total number of backtrack glyphs */
  FT_UShort*              Backtrack;  /* array of backtrack glyph IDs     */
  FT_UShort               InputGlyphCount;
				      /* total number of input glyphs     */
  FT_UShort*              Input;      /* array of input glyph IDs         */
  FT_UShort               LookaheadGlyphCount;
				      /* total number of lookahead glyphs */
  FT_UShort*              Lookahead;  /* array of lookahead glyph IDs     */
  FT_UShort               SubstCount; /* number of SubstLookupRecords     */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of SubstLookupRecords      */
};

typedef struct HB_ChainSubRule_  HB_ChainSubRule;


struct  HB_ChainSubRuleSet_
{
  FT_UShort          ChainSubRuleCount;
				      /* number of ChainSubRule tables */
  HB_ChainSubRule*  ChainSubRule;    /* array of ChainSubRule tables  */
};

typedef struct HB_ChainSubRuleSet_  HB_ChainSubRuleSet;


struct  HB_ChainContextSubstFormat1_
{
  HB_Coverage          Coverage;     /* Coverage table                   */
  FT_UShort             ChainSubRuleSetCount;
				      /* number of ChainSubRuleSet tables */
  HB_ChainSubRuleSet*  ChainSubRuleSet;
				      /* array of ChainSubRuleSet tables  */
};

typedef struct HB_ChainContextSubstFormat1_  HB_ChainContextSubstFormat1;


struct  HB_ChainSubClassRule_
{
  FT_UShort               BacktrackGlyphCount;
				      /* total number of backtrack
					 classes                         */
  FT_UShort*              Backtrack;  /* array of backtrack classes      */
  FT_UShort               InputGlyphCount;
				      /* total number of context classes */
  FT_UShort*              Input;      /* array of context classes        */
  FT_UShort               LookaheadGlyphCount;
				      /* total number of lookahead
					 classes                         */
  FT_UShort*              Lookahead;  /* array of lookahead classes      */
  FT_UShort               SubstCount; /* number of SubstLookupRecords    */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of substitution lookups   */
};

typedef struct HB_ChainSubClassRule_  HB_ChainSubClassRule;


struct  HB_ChainSubClassSet_
{
  FT_UShort               ChainSubClassRuleCount;
				      /* number of ChainSubClassRule
					 tables                      */
  HB_ChainSubClassRule*  ChainSubClassRule;
				      /* array of ChainSubClassRule
					 tables                      */
};

typedef struct HB_ChainSubClassSet_  HB_ChainSubClassSet;


/* The `MaxXXXLength' fields are not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the specific context rules.         */

struct  HB_ChainContextSubstFormat2_
{
  HB_Coverage           Coverage;    /* Coverage table             */

  FT_UShort              MaxBacktrackLength;
				      /* maximal backtrack length   */
  HB_ClassDefinition    BacktrackClassDef;
				      /* BacktrackClassDef table    */
  FT_UShort              MaxInputLength;
				      /* maximal input length       */
  HB_ClassDefinition    InputClassDef;
				      /* InputClassDef table        */
  FT_UShort              MaxLookaheadLength;
				      /* maximal lookahead length   */
  HB_ClassDefinition    LookaheadClassDef;
				      /* LookaheadClassDef table    */

  FT_UShort              ChainSubClassSetCount;
				      /* number of ChainSubClassSet
					 tables                     */
  HB_ChainSubClassSet*  ChainSubClassSet;
				      /* array of ChainSubClassSet
					 tables                     */
};

typedef struct HB_ChainContextSubstFormat2_  HB_ChainContextSubstFormat2;


struct  HB_ChainContextSubstFormat3_
{
  FT_UShort               BacktrackGlyphCount;
				      /* number of backtrack glyphs    */
  HB_Coverage*           BacktrackCoverage;
				      /* array of backtrack Coverage
					 tables                        */
  FT_UShort               InputGlyphCount;
				      /* number of input glyphs        */
  HB_Coverage*           InputCoverage;
				      /* array of input coverage
					 tables                        */
  FT_UShort               LookaheadGlyphCount;
				      /* number of lookahead glyphs    */
  HB_Coverage*           LookaheadCoverage;
				      /* array of lookahead coverage
					 tables                        */
  FT_UShort               SubstCount; /* number of SubstLookupRecords  */
  HB_SubstLookupRecord*  SubstLookupRecord;
				      /* array of substitution lookups */
};

typedef struct HB_ChainContextSubstFormat3_  HB_ChainContextSubstFormat3;


struct  HB_ChainContextSubst_
{
  FT_UShort  SubstFormat;             /* 1, 2, or 3 */

  union
  {
    HB_ChainContextSubstFormat1  ccsf1;
    HB_ChainContextSubstFormat2  ccsf2;
    HB_ChainContextSubstFormat3  ccsf3;
  } ccsf;
};

typedef struct HB_ChainContextSubst_  HB_ChainContextSubst;


/* LookupType 8 */
struct HB_ReverseChainContextSubst_
{
  FT_UShort      SubstFormat;         /* always 1 */
  HB_Coverage   Coverage;	        /* coverage table for input glyphs */
  FT_UShort      BacktrackGlyphCount; /* number of backtrack glyphs      */
  HB_Coverage*  BacktrackCoverage;   /* array of backtrack Coverage
					 tables                          */
  FT_UShort      LookaheadGlyphCount; /* number of lookahead glyphs      */
  HB_Coverage*  LookaheadCoverage;   /* array of lookahead Coverage
					 tables                          */
  FT_UShort      GlyphCount;          /* number of Glyph IDs             */
  FT_UShort*     Substitute;          /* array of substitute Glyph ID    */
};

typedef struct HB_ReverseChainContextSubst_  HB_ReverseChainContextSubst;


union  HB_GSUB_SubTable_
{
  HB_SingleSubst              single;
  HB_MultipleSubst            multiple;
  HB_AlternateSubst           alternate;
  HB_LigatureSubst            ligature;
  HB_ContextSubst             context;
  HB_ChainContextSubst        chain;
  HB_ReverseChainContextSubst reverse;
};





FT_Error  _HB_GSUB_Load_SubTable( HB_GSUB_SubTable*  st,
				  FT_Stream     stream,
				  FT_UShort     lookup_type );

void  _HB_GSUB_Free_SubTable( HB_GSUB_SubTable*  st,
			      FT_Memory     memory,
			      FT_UShort     lookup_type );

FT_END_HEADER

#endif /* HARFBUZZ_GSUB_PRIVATE_H */
