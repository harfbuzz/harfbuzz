/*******************************************************************
 *
 *  ftxgsub.h
 *
 *    TrueType Open GSUB table support
 *
 *  Copyright 1996-2000 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  This file is part of the FreeType project, and may only be used
 *  modified and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 ******************************************************************/

#ifndef FTXOPEN_H
#error "Don't include this file! Use ftxopen.h instead."
#endif

#ifndef FTXGSUB_H
#define FTXGSUB_H

#ifdef __cplusplus
extern "C" {
#endif

#define TTO_Err_Invalid_GSUB_SubTable_Format  0x1010
#define TTO_Err_Invalid_GSUB_SubTable         0x1011


/* Lookup types for glyph substitution */

#define GSUB_LOOKUP_SINGLE     1
#define GSUB_LOOKUP_MULTIPLE   2
#define GSUB_LOOKUP_ALTERNATE  3
#define GSUB_LOOKUP_LIGATURE   4
#define GSUB_LOOKUP_CONTEXT    5
#define GSUB_LOOKUP_CHAIN      6
#define GSUB_LOOKUP_EXTENSION  7


/* Use this if a feature applies to all glyphs */

#define ALL_GLYPHS  0xFFFF


  /* A pointer to a function which selects the alternate glyph.  `pos' is
     the position of the glyph with index `glyphID', `num_alternates'
     gives the number of alternates in the `alternates' array.  `data'
     points to the user-defined structure specified during a call to
     TT_GSUB_Register_Alternate_Function().  The function must return an
     index into the `alternates' array.                                   */

  typedef FT_UShort  (*TTO_AltFunction)(FT_ULong    pos,
                                        FT_UShort   glyphID,
                                        FT_UShort   num_alternates,
                                        FT_UShort*  alternates,
                                        void*       data );


  struct  TTO_GSUBHeader_
  {
    FT_Memory        memory;
    
    FT_ULong         offset;

    FT_Fixed         Version;

    TTO_ScriptList   ScriptList;
    TTO_FeatureList  FeatureList;
    TTO_LookupList   LookupList;

    TTO_GDEFHeader*  gdef;

    /* the next two fields are used for an alternate substitution callback
       function to select the proper alternate glyph.                      */

    TTO_AltFunction  altfunc;
    void*            data;
  };

  typedef struct TTO_GSUBHeader_   TTO_GSUBHeader;
  typedef struct TTO_GSUBHeader_*  TTO_GSUB;


  /* LookupType 1 */

  struct  TTO_SingleSubstFormat1_
  {
    FT_Short  DeltaGlyphID;             /* constant added to get
                                           substitution glyph index */
  };

  typedef struct TTO_SingleSubstFormat1_  TTO_SingleSubstFormat1;


  struct  TTO_SingleSubstFormat2_
  {
    FT_UShort   GlyphCount;             /* number of glyph IDs in
                                           Substitute array              */
    FT_UShort*  Substitute;             /* array of substitute glyph IDs */
  };

  typedef struct TTO_SingleSubstFormat2_  TTO_SingleSubstFormat2;


  struct  TTO_SingleSubst_
  {
    FT_UShort     SubstFormat;          /* 1 or 2         */
    TTO_Coverage  Coverage;             /* Coverage table */

    union
    {
      TTO_SingleSubstFormat1  ssf1;
      TTO_SingleSubstFormat2  ssf2;
    } ssf;
  };

  typedef struct TTO_SingleSubst_  TTO_SingleSubst;


  /* LookupType 2 */

  struct  TTO_Sequence_
  {
    FT_UShort   GlyphCount;             /* number of glyph IDs in the
                                           Substitute array           */
    FT_UShort*  Substitute;             /* string of glyph IDs to
                                           substitute                 */
  };

  typedef struct TTO_Sequence_  TTO_Sequence;


  struct  TTO_MultipleSubst_
  {
    FT_UShort      SubstFormat;         /* always 1                  */
    TTO_Coverage   Coverage;            /* Coverage table            */
    FT_UShort      SequenceCount;       /* number of Sequence tables */
    TTO_Sequence*  Sequence;            /* array of Sequence tables  */
  };

  typedef struct TTO_MultipleSubst_  TTO_MultipleSubst;


  /* LookupType 3 */

  struct  TTO_AlternateSet_
  {
    FT_UShort   GlyphCount;             /* number of glyph IDs in the
                                           Alternate array              */
    FT_UShort*  Alternate;              /* array of alternate glyph IDs */
  };

  typedef struct TTO_AlternateSet_  TTO_AlternateSet;


  struct  TTO_AlternateSubst_
  {
    FT_UShort          SubstFormat;     /* always 1                      */
    TTO_Coverage       Coverage;        /* Coverage table                */
    FT_UShort          AlternateSetCount;
                                        /* number of AlternateSet tables */
    TTO_AlternateSet*  AlternateSet;    /* array of AlternateSet tables  */
  };

  typedef struct TTO_AlternateSubst_  TTO_AlternateSubst;


  /* LookupType 4 */

  struct  TTO_Ligature_
  {
    FT_UShort   LigGlyph;               /* glyphID of ligature
                                           to substitute                    */
    FT_UShort   ComponentCount;         /* number of components in ligature */
    FT_UShort*  Component;              /* array of component glyph IDs     */
  };

  typedef struct TTO_Ligature_  TTO_Ligature;


  struct  TTO_LigatureSet_
  {
    FT_UShort      LigatureCount;       /* number of Ligature tables */
    TTO_Ligature*  Ligature;            /* array of Ligature tables  */
  };

  typedef struct TTO_LigatureSet_  TTO_LigatureSet;


  struct  TTO_LigatureSubst_
  {
    FT_UShort         SubstFormat;      /* always 1                     */
    TTO_Coverage      Coverage;         /* Coverage table               */
    FT_UShort         LigatureSetCount; /* number of LigatureSet tables */
    TTO_LigatureSet*  LigatureSet;      /* array of LigatureSet tables  */
  };

  typedef struct TTO_LigatureSubst_  TTO_LigatureSubst;


  /* needed by both lookup type 5 and 6 */

  struct  TTO_SubstLookupRecord_
  {
    FT_UShort  SequenceIndex;           /* index into current
                                           glyph sequence               */
    FT_UShort  LookupListIndex;         /* Lookup to apply to that pos. */
  };

  typedef struct TTO_SubstLookupRecord_  TTO_SubstLookupRecord;


  /* LookupType 5 */

  struct  TTO_SubRule_
  {
    FT_UShort               GlyphCount; /* total number of input glyphs */
    FT_UShort               SubstCount; /* number of SubstLookupRecord
                                           tables                       */
    FT_UShort*              Input;      /* array of input glyph IDs     */
    TTO_SubstLookupRecord*  SubstLookupRecord;
                                        /* array of SubstLookupRecord
                                           tables                       */
  };

  typedef struct TTO_SubRule_  TTO_SubRule;


  struct  TTO_SubRuleSet_
  {
    FT_UShort     SubRuleCount;         /* number of SubRule tables */
    TTO_SubRule*  SubRule;              /* array of SubRule tables  */
  };

  typedef struct TTO_SubRuleSet_  TTO_SubRuleSet;


  struct  TTO_ContextSubstFormat1_
  {
    TTO_Coverage     Coverage;          /* Coverage table              */
    FT_UShort        SubRuleSetCount;   /* number of SubRuleSet tables */
    TTO_SubRuleSet*  SubRuleSet;        /* array of SubRuleSet tables  */
  };

  typedef struct TTO_ContextSubstFormat1_  TTO_ContextSubstFormat1;


  struct  TTO_SubClassRule_
  {
    FT_UShort               GlyphCount; /* total number of context classes */
    FT_UShort               SubstCount; /* number of SubstLookupRecord
                                           tables                          */
    FT_UShort*              Class;      /* array of classes                */
    TTO_SubstLookupRecord*  SubstLookupRecord;
                                        /* array of SubstLookupRecord
                                           tables                          */
  };

  typedef struct TTO_SubClassRule_  TTO_SubClassRule;


  struct  TTO_SubClassSet_
  {
    FT_UShort          SubClassRuleCount;
                                        /* number of SubClassRule tables */
    TTO_SubClassRule*  SubClassRule;    /* array of SubClassRule tables  */
  };

  typedef struct TTO_SubClassSet_  TTO_SubClassSet;


  /* The `MaxContextLength' field is not defined in the TTO specification
     but simplifies the implementation of this format.  It holds the
     maximal context length used in the context rules.                    */

  struct  TTO_ContextSubstFormat2_
  {
    FT_UShort            MaxContextLength;
                                        /* maximal context length       */
    TTO_Coverage         Coverage;      /* Coverage table               */
    TTO_ClassDefinition  ClassDef;      /* ClassDef table               */
    FT_UShort            SubClassSetCount;
                                        /* number of SubClassSet tables */
    TTO_SubClassSet*     SubClassSet;   /* array of SubClassSet tables  */
  };

  typedef struct TTO_ContextSubstFormat2_  TTO_ContextSubstFormat2;


  struct  TTO_ContextSubstFormat3_
  {
    FT_UShort               GlyphCount; /* number of input glyphs        */
    FT_UShort               SubstCount; /* number of SubstLookupRecords  */
    TTO_Coverage*           Coverage;   /* array of Coverage tables      */
    TTO_SubstLookupRecord*  SubstLookupRecord;
                                        /* array of substitution lookups */
  };

  typedef struct TTO_ContextSubstFormat3_  TTO_ContextSubstFormat3;


  struct  TTO_ContextSubst_
  {
    FT_UShort  SubstFormat;             /* 1, 2, or 3 */

    union
    {
      TTO_ContextSubstFormat1  csf1;
      TTO_ContextSubstFormat2  csf2;
      TTO_ContextSubstFormat3  csf3;
    } csf;
  };

  typedef struct TTO_ContextSubst_  TTO_ContextSubst;


  /* LookupType 6 */

  struct  TTO_ChainSubRule_
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
    TTO_SubstLookupRecord*  SubstLookupRecord;
                                        /* array of SubstLookupRecords      */
  };

  typedef struct TTO_ChainSubRule_  TTO_ChainSubRule;


  struct  TTO_ChainSubRuleSet_
  {
    FT_UShort          ChainSubRuleCount;
                                        /* number of ChainSubRule tables */
    TTO_ChainSubRule*  ChainSubRule;    /* array of ChainSubRule tables  */
  };

  typedef struct TTO_ChainSubRuleSet_  TTO_ChainSubRuleSet;


  struct  TTO_ChainContextSubstFormat1_
  {
    TTO_Coverage          Coverage;     /* Coverage table                   */
    FT_UShort             ChainSubRuleSetCount;
                                        /* number of ChainSubRuleSet tables */
    TTO_ChainSubRuleSet*  ChainSubRuleSet;
                                        /* array of ChainSubRuleSet tables  */
  };

  typedef struct TTO_ChainContextSubstFormat1_  TTO_ChainContextSubstFormat1;


  struct  TTO_ChainSubClassRule_
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
    TTO_SubstLookupRecord*  SubstLookupRecord;
                                        /* array of substitution lookups   */
  };

  typedef struct TTO_ChainSubClassRule_  TTO_ChainSubClassRule;


  struct  TTO_ChainSubClassSet_
  {
    FT_UShort               ChainSubClassRuleCount;
                                        /* number of ChainSubClassRule
                                           tables                      */
    TTO_ChainSubClassRule*  ChainSubClassRule;
                                        /* array of ChainSubClassRule
                                           tables                      */
  };

  typedef struct TTO_ChainSubClassSet_  TTO_ChainSubClassSet;


  /* The `MaxXXXLength' fields are not defined in the TTO specification
     but simplifies the implementation of this format.  It holds the
     maximal context length used in the specific context rules.         */

  struct  TTO_ChainContextSubstFormat2_
  {
    TTO_Coverage           Coverage;    /* Coverage table             */

    FT_UShort              MaxBacktrackLength;
                                        /* maximal backtrack length   */
    TTO_ClassDefinition    BacktrackClassDef;
                                        /* BacktrackClassDef table    */
    FT_UShort              MaxInputLength;
                                        /* maximal input length       */
    TTO_ClassDefinition    InputClassDef;
                                        /* InputClassDef table        */
    FT_UShort              MaxLookaheadLength;
                                        /* maximal lookahead length   */
    TTO_ClassDefinition    LookaheadClassDef;
                                        /* LookaheadClassDef table    */

    FT_UShort              ChainSubClassSetCount;
                                        /* number of ChainSubClassSet
                                           tables                     */
    TTO_ChainSubClassSet*  ChainSubClassSet;
                                        /* array of ChainSubClassSet
                                           tables                     */
  };

  typedef struct TTO_ChainContextSubstFormat2_  TTO_ChainContextSubstFormat2;


  struct  TTO_ChainContextSubstFormat3_
  {
    FT_UShort               BacktrackGlyphCount;
                                        /* number of backtrack glyphs    */
    TTO_Coverage*           BacktrackCoverage;
                                        /* array of backtrack Coverage
                                           tables                        */
    FT_UShort               InputGlyphCount;
                                        /* number of input glyphs        */
    TTO_Coverage*           InputCoverage;
                                        /* array of input coverage
                                           tables                        */
    FT_UShort               LookaheadGlyphCount;
                                        /* number of lookahead glyphs    */
    TTO_Coverage*           LookaheadCoverage;
                                        /* array of lookahead coverage
                                           tables                        */
    FT_UShort               SubstCount; /* number of SubstLookupRecords  */
    TTO_SubstLookupRecord*  SubstLookupRecord;
                                        /* array of substitution lookups */
  };

  typedef struct TTO_ChainContextSubstFormat3_  TTO_ChainContextSubstFormat3;


  struct  TTO_ChainContextSubst_
  {
    FT_UShort  SubstFormat;             /* 1, 2, or 3 */

    union
    {
      TTO_ChainContextSubstFormat1  ccsf1;
      TTO_ChainContextSubstFormat2  ccsf2;
      TTO_ChainContextSubstFormat3  ccsf3;
    } ccsf;
  };

  typedef struct TTO_ChainContextSubst_  TTO_ChainContextSubst;


  union  TTO_GSUB_SubTable_
  {
    TTO_SingleSubst        single;
    TTO_MultipleSubst      multiple;
    TTO_AlternateSubst     alternate;
    TTO_LigatureSubst      ligature;
    TTO_ContextSubst       context;
    TTO_ChainContextSubst  chain;
  };

  typedef union TTO_GSUB_SubTable_  TTO_GSUB_SubTable;


  /* A simple string object.  It can both `send' and `receive' data.
     In case of sending, `length' and `pos' will be used.  In case of
     receiving, `pos' points to the first free slot, and `allocated'
     specifies the amount of allocated memory (and the `length' field
     will be ignored).  The routine TT_Add_String() will increase the
     amount of memory if necessary.  After end of receive, `length'
     should be set to the value of `pos', and `pos' will be set to zero.

     `properties' (which is treated as a bit field) gives the glyph's
     properties: If a certain bit is set for a glyph, the feature which
     has the same bit set in its property value is applied.

     `components' is an internal array which tracks components of
     ligatures.  We need this for MarkToLigature Attachment Positioning
     Subtables (in GPOS) together with `ligIDs' (which is used to mark
     ligatures and the skipped glyphs during a ligature lookup).
     `max_ligID' is increased after a successful ligature lookup.

     NEVER modify any elements of the structure!  You should rather copy
     its contents if necessary.

     TT_Add_String() will also handle allocation; you should use
     free() in case you want to destroy the arrays in the object. */


  /* finally, the GSUB API */

  /*  EXPORT_DEF
      TT_Error  TT_Init_GSUB_Extension( TT_Engine  engine ); */

  EXPORT_DEF
  FT_Error  TT_Load_GSUB_Table( FT_Face          face,
                                TTO_GSUBHeader** gsub,
                                TTO_GDEFHeader*  gdef );

  EXPORT_DEF
  FT_Error  TT_Done_GSUB_Table( TTO_GSUBHeader*  gsub );

  EXPORT_DEF
  FT_Error  TT_GSUB_Select_Script( TTO_GSUBHeader*  gsub,
                                   FT_ULong         script_tag,
                                   FT_UShort*       script_index );
  EXPORT_DEF
  FT_Error  TT_GSUB_Select_Language( TTO_GSUBHeader*  gsub,
                                     FT_ULong         language_tag,
                                     FT_UShort        script_index,
                                     FT_UShort*       language_index,
                                     FT_UShort*       req_feature_index );
  EXPORT_DEF
  FT_Error  TT_GSUB_Select_Feature( TTO_GSUBHeader*  gsub,
                                    FT_ULong         feature_tag,
                                    FT_UShort        script_index,
                                    FT_UShort        language_index,
                                    FT_UShort*       feature_index );

  EXPORT_DEF
  FT_Error  TT_GSUB_Query_Scripts( TTO_GSUBHeader*  gsub,
                                   FT_ULong**       script_tag_list );
  EXPORT_DEF
  FT_Error  TT_GSUB_Query_Languages( TTO_GSUBHeader*  gsub,
                                     FT_UShort        script_index,
                                     FT_ULong**       language_tag_list );
  EXPORT_DEF
  FT_Error  TT_GSUB_Query_Features( TTO_GSUBHeader*  gsub,
                                    FT_UShort        script_index,
                                    FT_UShort        language_index,
                                    FT_ULong**       feature_tag_list );

  EXPORT_DEF
  FT_Error  TT_GSUB_Add_Feature( TTO_GSUBHeader*  gsub,
                                 FT_UShort        feature_index,
                                 FT_UInt          property );
  EXPORT_DEF
  FT_Error  TT_GSUB_Clear_Features( TTO_GSUBHeader*  gsub );

  EXPORT_DEF
  FT_Error  TT_GSUB_Register_Alternate_Function( TTO_GSUBHeader*  gsub,
                                                 TTO_AltFunction  altfunc,
                                                 void*            data );
  
  EXPORT_DEF
  FT_Error  TT_GSUB_Apply_String( TTO_GSUBHeader*   gsub,
				  OTL_Buffer        buffer );


#ifdef __cplusplus
}
#endif

#endif /* FTXGSUB_H */


/* END */
