/*******************************************************************
 *
 *  ftxgpos.h
 *
 *    TrueType Open GPOS table support
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

#ifndef FTXGPOS_H
#define FTXGPOS_H

#ifdef __cplusplus
extern "C" {
#endif

#define TTO_Err_Invalid_GPOS_SubTable_Format  0x1020
#define TTO_Err_Invalid_GPOS_SubTable         0x1021


/* Lookup types for glyph positioning */

#define GPOS_LOOKUP_SINGLE     1
#define GPOS_LOOKUP_PAIR       2
#define GPOS_LOOKUP_CURSIVE    3
#define GPOS_LOOKUP_MARKBASE   4
#define GPOS_LOOKUP_MARKLIG    5
#define GPOS_LOOKUP_MARKMARK   6
#define GPOS_LOOKUP_CONTEXT    7
#define GPOS_LOOKUP_CHAIN      8
#define GPOS_LOOKUP_EXTENSION  9


  /* A pointer to a function which loads a glyph.  Its parameters are
     the same as in a call to TT_Load_Glyph() -- if no glyph loading
     function will be registered with TTO_GPOS_Register_Glyph_Function(),
     TT_Load_Glyph() will be called indeed.  The purpose of this function
     pointer is to provide a hook for caching glyph outlines and sbits
     (using the instance's generic pointer to hold the data).

     If for some reason no outline data is available (e.g. for an
     embedded bitmap glyph), _glyph->outline.n_points should be set to
     zero.  _glyph can be computed with

        _glyph = HANDLE_Glyph( glyph )                                    */

  typedef FT_Error  (*TTO_GlyphFunction)(FT_Face      face,
					 FT_UInt      glyphIndex,
					 FT_Int       loadFlags );


  /* A pointer to a function which accesses the PostScript interpreter.
     Multiple Master fonts need this interface to convert a metric ID
     (as stored in an OpenType font version 1.2 or higher) `metric_id'
     into a metric value (returned in `metric_value').

     `data' points to the user-defined structure specified during a
     call to TT_GPOS_Register_MM_Function().

     `metric_value' must be returned as a scaled value (but shouldn't
     be rounded).                                                       */

  typedef FT_Error  (*TTO_MMFunction)(FT_Face      face,
				      FT_UShort    metric_id,
                                      FT_Pos*      metric_value,
                                      void*        data );
                                          

  struct  TTO_GPOSHeader_
  {
    FT_Memory          memory;
    
    FT_Fixed           Version;

    TTO_ScriptList     ScriptList;
    TTO_FeatureList    FeatureList;
    TTO_LookupList     LookupList;

    TTO_GDEFHeader*    gdef;

    /* the next field is used for a callback function to get the
       glyph outline.                                            */

    TTO_GlyphFunction  gfunc;

    /* this is OpenType 1.2 -- Multiple Master fonts need this
       callback function to get various metric values from the
       PostScript interpreter.                                 */

    TTO_MMFunction     mmfunc;
    void*              data;
  };

  typedef struct TTO_GPOSHeader_  TTO_GPOSHeader;
  typedef struct TTO_GPOSHeader_* TTO_GPOS;


  /* shared tables */

  struct  TTO_ValueRecord_
  {
    FT_Short    XPlacement;             /* horizontal adjustment for
                                           placement                      */
    FT_Short    YPlacement;             /* vertical adjustment for
                                           placement                      */
    FT_Short    XAdvance;               /* horizontal adjustment for
                                           advance                        */
    FT_Short    YAdvance;               /* vertical adjustment for
                                           advance                        */
    TTO_Device  XPlacementDevice;       /* device table for horizontal
                                           placement                      */
    TTO_Device  YPlacementDevice;       /* device table for vertical
                                           placement                      */
    TTO_Device  XAdvanceDevice;         /* device table for horizontal
                                           advance                        */
    TTO_Device  YAdvanceDevice;         /* device table for vertical
                                           advance                        */
    FT_UShort   XIdPlacement;           /* horizontal placement metric ID */
    FT_UShort   YIdPlacement;           /* vertical placement metric ID   */
    FT_UShort   XIdAdvance;             /* horizontal advance metric ID   */
    FT_UShort   YIdAdvance;             /* vertical advance metric ID     */
  };

  typedef struct TTO_ValueRecord_  TTO_ValueRecord;


/* Mask values to scan the value format of the ValueRecord structure.
   We always expand compressed ValueRecords of the font.              */

#define HAVE_X_PLACEMENT         0x0001
#define HAVE_Y_PLACEMENT         0x0002
#define HAVE_X_ADVANCE           0x0004
#define HAVE_Y_ADVANCE           0x0008
#define HAVE_X_PLACEMENT_DEVICE  0x0010
#define HAVE_Y_PLACEMENT_DEVICE  0x0020
#define HAVE_X_ADVANCE_DEVICE    0x0040
#define HAVE_Y_ADVANCE_DEVICE    0x0080
#define HAVE_X_ID_PLACEMENT      0x0100
#define HAVE_Y_ID_PLACEMENT      0x0200
#define HAVE_X_ID_ADVANCE        0x0400
#define HAVE_Y_ID_ADVANCE        0x0800


  struct  TTO_AnchorFormat1_
  {
    FT_Short   XCoordinate;             /* horizontal value */
    FT_Short   YCoordinate;             /* vertical value   */
  };

  typedef struct TTO_AnchorFormat1_  TTO_AnchorFormat1;


  struct  TTO_AnchorFormat2_
  {
    FT_Short   XCoordinate;             /* horizontal value             */
    FT_Short   YCoordinate;             /* vertical value               */
    FT_UShort  AnchorPoint;             /* index to glyph contour point */
  };

  typedef struct TTO_AnchorFormat2_  TTO_AnchorFormat2;


  struct  TTO_AnchorFormat3_
  {
    FT_Short    XCoordinate;            /* horizontal value              */
    FT_Short    YCoordinate;            /* vertical value                */
    TTO_Device  XDeviceTable;           /* device table for X coordinate */
    TTO_Device  YDeviceTable;           /* device table for Y coordinate */
  };

  typedef struct TTO_AnchorFormat3_  TTO_AnchorFormat3;


  struct  TTO_AnchorFormat4_
  {
    FT_UShort  XIdAnchor;               /* horizontal metric ID */
    FT_UShort  YIdAnchor;               /* vertical metric ID   */
  };

  typedef struct TTO_AnchorFormat4_  TTO_AnchorFormat4;


  struct  TTO_Anchor_
  {
    FT_UShort  PosFormat;               /* 1, 2, 3, or 4 -- 0 indicates
                                           that there is no Anchor table */

    union
    {
      TTO_AnchorFormat1  af1;
      TTO_AnchorFormat2  af2;
      TTO_AnchorFormat3  af3;
      TTO_AnchorFormat4  af4;
    } af;
  };

  typedef struct TTO_Anchor_  TTO_Anchor;


  struct  TTO_MarkRecord_
  {
    FT_UShort   Class;                  /* mark class   */
    TTO_Anchor  MarkAnchor;             /* anchor table */
  };

  typedef struct TTO_MarkRecord_  TTO_MarkRecord;


  struct  TTO_MarkArray_
  {
    FT_UShort        MarkCount;         /* number of MarkRecord tables */
    TTO_MarkRecord*  MarkRecord;        /* array of MarkRecord tables  */
  };

  typedef struct TTO_MarkArray_  TTO_MarkArray;


  /* LookupType 1 */

  struct  TTO_SinglePosFormat1_
  {
    TTO_ValueRecord  Value;             /* ValueRecord for all covered
                                           glyphs                      */
  };

  typedef struct TTO_SinglePosFormat1_  TTO_SinglePosFormat1;


  struct  TTO_SinglePosFormat2_
  {
    FT_UShort         ValueCount;       /* number of ValueRecord tables */
    TTO_ValueRecord*  Value;            /* array of ValueRecord tables  */
  };

  typedef struct TTO_SinglePosFormat2_  TTO_SinglePosFormat2;


  struct  TTO_SinglePos_
  {
    FT_UShort     PosFormat;            /* 1 or 2         */
    TTO_Coverage  Coverage;             /* Coverage table */

    FT_UShort     ValueFormat;          /* format of ValueRecord table */

    union
    {
      TTO_SinglePosFormat1  spf1;
      TTO_SinglePosFormat2  spf2;
    } spf;
  };

  typedef struct TTO_SinglePos_  TTO_SinglePos;


  /* LookupType 2 */

  struct  TTO_PairValueRecord_
  {
    FT_UShort        SecondGlyph;       /* glyph ID for second glyph  */
    TTO_ValueRecord  Value1;            /* pos. data for first glyph  */
    TTO_ValueRecord  Value2;            /* pos. data for second glyph */
  };

  typedef struct TTO_PairValueRecord_  TTO_PairValueRecord;


  struct  TTO_PairSet_
  {
    FT_UShort             PairValueCount;
                                        /* number of PairValueRecord tables */
    TTO_PairValueRecord*  PairValueRecord;
                                        /* array of PairValueRecord tables  */
  };

  typedef struct TTO_PairSet_  TTO_PairSet;


  struct  TTO_PairPosFormat1_
  {
    FT_UShort     PairSetCount;         /* number of PairSet tables    */
    TTO_PairSet*  PairSet;              /* array of PairSet tables     */
  };

  typedef struct TTO_PairPosFormat1_  TTO_PairPosFormat1;


  struct  TTO_Class2Record_
  {
    TTO_ValueRecord  Value1;            /* pos. data for first glyph  */
    TTO_ValueRecord  Value2;            /* pos. data for second glyph */
  };

  typedef struct TTO_Class2Record_  TTO_Class2Record;


  struct  TTO_Class1Record_
  {
    TTO_Class2Record*  Class2Record;    /* array of Class2Record tables */
  };

  typedef struct TTO_Class1Record_  TTO_Class1Record;


  struct  TTO_PairPosFormat2_
  {
    TTO_ClassDefinition  ClassDef1;     /* class def. for first glyph     */
    TTO_ClassDefinition  ClassDef2;     /* class def. for second glyph    */
    FT_UShort            Class1Count;   /* number of classes in ClassDef1
                                           table                          */
    FT_UShort            Class2Count;   /* number of classes in ClassDef2
                                           table                          */
    TTO_Class1Record*    Class1Record;  /* array of Class1Record tables   */
  };

  typedef struct TTO_PairPosFormat2_  TTO_PairPosFormat2;


  struct  TTO_PairPos_
  {
    FT_UShort     PosFormat;            /* 1 or 2         */
    TTO_Coverage  Coverage;             /* Coverage table */
    FT_UShort     ValueFormat1;         /* format of ValueRecord table
                                           for first glyph             */
    FT_UShort     ValueFormat2;         /* format of ValueRecord table
                                           for second glyph            */

    union
    {
      TTO_PairPosFormat1  ppf1;
      TTO_PairPosFormat2  ppf2;
    } ppf;
  };

  typedef struct TTO_PairPos_  TTO_PairPos;


  /* LookupType 3 */

  struct  TTO_EntryExitRecord_
  {
    TTO_Anchor  EntryAnchor;            /* entry Anchor table */
    TTO_Anchor  ExitAnchor;             /* exit Anchor table  */
  };


  typedef struct TTO_EntryExitRecord_  TTO_EntryExitRecord;

  struct  TTO_CursivePos_
  {
    FT_UShort             PosFormat;    /* always 1                         */
    TTO_Coverage          Coverage;     /* Coverage table                   */
    FT_UShort             EntryExitCount;
                                        /* number of EntryExitRecord tables */
    TTO_EntryExitRecord*  EntryExitRecord;
                                        /* array of EntryExitRecord tables  */
  };

  typedef struct TTO_CursivePos_  TTO_CursivePos;


  /* LookupType 4 */

  struct  TTO_BaseRecord_
  {
    TTO_Anchor*  BaseAnchor;            /* array of base glyph anchor
                                           tables                     */
  };

  typedef struct TTO_BaseRecord_  TTO_BaseRecord;


  struct  TTO_BaseArray_
  {
    FT_UShort        BaseCount;         /* number of BaseRecord tables */
    TTO_BaseRecord*  BaseRecord;        /* array of BaseRecord tables  */
  };

  typedef struct TTO_BaseArray_  TTO_BaseArray;


  struct  TTO_MarkBasePos_
  {
    FT_UShort      PosFormat;           /* always 1                  */
    TTO_Coverage   MarkCoverage;        /* mark glyph coverage table */
    TTO_Coverage   BaseCoverage;        /* base glyph coverage table */
    FT_UShort      ClassCount;          /* number of mark classes    */
    TTO_MarkArray  MarkArray;           /* mark array table          */
    TTO_BaseArray  BaseArray;           /* base array table          */
  };

  typedef struct TTO_MarkBasePos_  TTO_MarkBasePos;


  /* LookupType 5 */

  struct  TTO_ComponentRecord_
  {
    TTO_Anchor*  LigatureAnchor;        /* array of ligature glyph anchor
                                           tables                         */
  };

  typedef struct TTO_ComponentRecord_  TTO_ComponentRecord;


  struct  TTO_LigatureAttach_
  {
    FT_UShort             ComponentCount;
                                        /* number of ComponentRecord tables */
    TTO_ComponentRecord*  ComponentRecord;
                                        /* array of ComponentRecord tables  */
  };

  typedef struct TTO_LigatureAttach_  TTO_LigatureAttach;


  struct  TTO_LigatureArray_
  {
    FT_UShort            LigatureCount; /* number of LigatureAttach tables */
    TTO_LigatureAttach*  LigatureAttach;
                                        /* array of LigatureAttach tables  */
  };

  typedef struct TTO_LigatureArray_  TTO_LigatureArray;


  struct  TTO_MarkLigPos_
  {
    FT_UShort          PosFormat;       /* always 1                      */
    TTO_Coverage       MarkCoverage;    /* mark glyph coverage table     */
    TTO_Coverage       LigatureCoverage;
                                        /* ligature glyph coverage table */
    FT_UShort          ClassCount;      /* number of mark classes        */
    TTO_MarkArray      MarkArray;       /* mark array table              */
    TTO_LigatureArray  LigatureArray;   /* ligature array table          */
  };

  typedef struct TTO_MarkLigPos_  TTO_MarkLigPos;


  /* LookupType 6 */

  struct  TTO_Mark2Record_
  {
    TTO_Anchor*  Mark2Anchor;           /* array of mark glyph anchor
                                           tables                     */
  };

  typedef struct TTO_Mark2Record_  TTO_Mark2Record;


  struct  TTO_Mark2Array_
  {
    FT_UShort         Mark2Count;       /* number of Mark2Record tables */
    TTO_Mark2Record*  Mark2Record;      /* array of Mark2Record tables  */
  };

  typedef struct TTO_Mark2Array_  TTO_Mark2Array;


  struct  TTO_MarkMarkPos_
  {
    FT_UShort       PosFormat;          /* always 1                         */
    TTO_Coverage    Mark1Coverage;      /* first mark glyph coverage table  */
    TTO_Coverage    Mark2Coverage;      /* second mark glyph coverave table */
    FT_UShort       ClassCount;         /* number of combining mark classes */
    TTO_MarkArray   Mark1Array;         /* MarkArray table for first mark   */
    TTO_Mark2Array  Mark2Array;         /* MarkArray table for second mark  */
  };

  typedef struct TTO_MarkMarkPos_  TTO_MarkMarkPos;


  /* needed by both lookup type 7 and 8 */

  struct  TTO_PosLookupRecord_
  {
    FT_UShort  SequenceIndex;           /* index into current
                                           glyph sequence               */
    FT_UShort  LookupListIndex;         /* Lookup to apply to that pos. */
  };

  typedef struct TTO_PosLookupRecord_  TTO_PosLookupRecord;


  /* LookupType 7 */

  struct  TTO_PosRule_
  {
    FT_UShort             GlyphCount;   /* total number of input glyphs     */
    FT_UShort             PosCount;     /* number of PosLookupRecord tables */
    FT_UShort*            Input;        /* array of input glyph IDs         */
    TTO_PosLookupRecord*  PosLookupRecord;
                                        /* array of PosLookupRecord tables  */
  };

  typedef struct TTO_PosRule_  TTO_PosRule;


  struct  TTO_PosRuleSet_
  {
    FT_UShort     PosRuleCount;         /* number of PosRule tables */
    TTO_PosRule*  PosRule;              /* array of PosRule tables  */
  };

  typedef struct TTO_PosRuleSet_  TTO_PosRuleSet;


  struct  TTO_ContextPosFormat1_
  {
    TTO_Coverage     Coverage;          /* Coverage table              */
    FT_UShort        PosRuleSetCount;   /* number of PosRuleSet tables */
    TTO_PosRuleSet*  PosRuleSet;        /* array of PosRuleSet tables  */
  };

  typedef struct TTO_ContextPosFormat1_  TTO_ContextPosFormat1;


  struct  TTO_PosClassRule_
  {
    FT_UShort             GlyphCount;   /* total number of context classes  */
    FT_UShort             PosCount;     /* number of PosLookupRecord tables */
    FT_UShort*            Class;        /* array of classes                 */
    TTO_PosLookupRecord*  PosLookupRecord;
                                        /* array of PosLookupRecord tables  */
  };

  typedef struct TTO_PosClassRule_  TTO_PosClassRule;


  struct  TTO_PosClassSet_
  {
    FT_UShort          PosClassRuleCount;
                                        /* number of PosClassRule tables */
    TTO_PosClassRule*  PosClassRule;    /* array of PosClassRule tables  */
  };

  typedef struct TTO_PosClassSet_  TTO_PosClassSet;


  /* The `MaxContextLength' field is not defined in the TTO specification
     but simplifies the implementation of this format.  It holds the
     maximal context length used in the context rules.                    */

  struct  TTO_ContextPosFormat2_
  {
    FT_UShort            MaxContextLength;
                                        /* maximal context length       */
    TTO_Coverage         Coverage;      /* Coverage table               */
    TTO_ClassDefinition  ClassDef;      /* ClassDef table               */
    FT_UShort            PosClassSetCount;
                                        /* number of PosClassSet tables */
    TTO_PosClassSet*     PosClassSet;   /* array of PosClassSet tables  */
  };

  typedef struct TTO_ContextPosFormat2_  TTO_ContextPosFormat2;


  struct  TTO_ContextPosFormat3_
  {
    FT_UShort             GlyphCount;   /* number of input glyphs           */
    FT_UShort             PosCount;     /* number of PosLookupRecord tables */
    TTO_Coverage*         Coverage;     /* array of Coverage tables         */
    TTO_PosLookupRecord*  PosLookupRecord;
                                        /* array of PosLookupRecord tables  */
  };

  typedef struct TTO_ContextPosFormat3_  TTO_ContextPosFormat3;


  struct  TTO_ContextPos_
  {
    FT_UShort  PosFormat;               /* 1, 2, or 3     */

    union
    {
      TTO_ContextPosFormat1  cpf1;
      TTO_ContextPosFormat2  cpf2;
      TTO_ContextPosFormat3  cpf3;
    } cpf;
  };

  typedef struct TTO_ContextPos_  TTO_ContextPos;


  /* LookupType 8 */

  struct  TTO_ChainPosRule_
  {
    FT_UShort             BacktrackGlyphCount;
                                        /* total number of backtrack glyphs */
    FT_UShort*            Backtrack;    /* array of backtrack glyph IDs     */
    FT_UShort             InputGlyphCount;
                                        /* total number of input glyphs     */
    FT_UShort*            Input;        /* array of input glyph IDs         */
    FT_UShort             LookaheadGlyphCount;
                                        /* total number of lookahead glyphs */
    FT_UShort*            Lookahead;    /* array of lookahead glyph IDs     */
    FT_UShort             PosCount;     /* number of PosLookupRecords       */
    TTO_PosLookupRecord*  PosLookupRecord;
                                        /* array of PosLookupRecords       */
  };

  typedef struct TTO_ChainPosRule_  TTO_ChainPosRule;


  struct  TTO_ChainPosRuleSet_
  {
    FT_UShort          ChainPosRuleCount;
                                        /* number of ChainPosRule tables */
    TTO_ChainPosRule*  ChainPosRule;    /* array of ChainPosRule tables  */
  };

  typedef struct TTO_ChainPosRuleSet_  TTO_ChainPosRuleSet;


  struct  TTO_ChainContextPosFormat1_
  {
    TTO_Coverage          Coverage;     /* Coverage table                   */
    FT_UShort             ChainPosRuleSetCount;
                                        /* number of ChainPosRuleSet tables */
    TTO_ChainPosRuleSet*  ChainPosRuleSet;
                                        /* array of ChainPosRuleSet tables  */
  };

  typedef struct TTO_ChainContextPosFormat1_  TTO_ChainContextPosFormat1;


  struct  TTO_ChainPosClassRule_
  {
    FT_UShort             BacktrackGlyphCount;
                                        /* total number of backtrack
                                           classes                         */
    FT_UShort*            Backtrack;    /* array of backtrack classes      */
    FT_UShort             InputGlyphCount;
                                        /* total number of context classes */
    FT_UShort*            Input;        /* array of context classes        */
    FT_UShort             LookaheadGlyphCount;
                                        /* total number of lookahead
                                           classes                         */
    FT_UShort*            Lookahead;    /* array of lookahead classes      */
    FT_UShort             PosCount;     /* number of PosLookupRecords      */
    TTO_PosLookupRecord*  PosLookupRecord;
                                        /* array of substitution lookups   */
  };

  typedef struct TTO_ChainPosClassRule_  TTO_ChainPosClassRule;


  struct  TTO_ChainPosClassSet_
  {
    FT_UShort               ChainPosClassRuleCount;
                                        /* number of ChainPosClassRule
                                           tables                      */
    TTO_ChainPosClassRule*  ChainPosClassRule;
                                        /* array of ChainPosClassRule
                                           tables                      */
  };

  typedef struct TTO_ChainPosClassSet_  TTO_ChainPosClassSet;


  /* The `MaxXXXLength' fields are not defined in the TTO specification
     but simplifies the implementation of this format.  It holds the
     maximal context length used in the specific context rules.         */

  struct  TTO_ChainContextPosFormat2_
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

    FT_UShort              ChainPosClassSetCount;
                                        /* number of ChainPosClassSet
                                           tables                     */
    TTO_ChainPosClassSet*  ChainPosClassSet;
                                        /* array of ChainPosClassSet
                                           tables                     */
  };

  typedef struct TTO_ChainContextPosFormat2_  TTO_ChainContextPosFormat2;


  struct  TTO_ChainContextPosFormat3_
  {
    FT_UShort             BacktrackGlyphCount;
                                        /* number of backtrack glyphs    */
    TTO_Coverage*         BacktrackCoverage;
                                        /* array of backtrack Coverage
                                           tables                        */
    FT_UShort             InputGlyphCount;
                                        /* number of input glyphs        */
    TTO_Coverage*         InputCoverage;
                                        /* array of input coverage
                                           tables                        */
    FT_UShort             LookaheadGlyphCount;
                                        /* number of lookahead glyphs    */
    TTO_Coverage*         LookaheadCoverage;
                                        /* array of lookahead coverage
                                           tables                        */
    FT_UShort             PosCount;     /* number of PosLookupRecords    */
    TTO_PosLookupRecord*  PosLookupRecord;
                                        /* array of substitution lookups */
  };

  typedef struct TTO_ChainContextPosFormat3_  TTO_ChainContextPosFormat3;


  struct  TTO_ChainContextPos_
  {
    FT_UShort  PosFormat;             /* 1, 2, or 3 */

    union
    {
      TTO_ChainContextPosFormat1  ccpf1;
      TTO_ChainContextPosFormat2  ccpf2;
      TTO_ChainContextPosFormat3  ccpf3;
    } ccpf;
  };

  typedef struct TTO_ChainContextPos_  TTO_ChainContextPos;


  union  TTO_GPOS_SubTable_
  {
    TTO_SinglePos        single;
    TTO_PairPos          pair;
    TTO_CursivePos       cursive;
    TTO_MarkBasePos      markbase;
    TTO_MarkLigPos       marklig;
    TTO_MarkMarkPos      markmark;
    TTO_ContextPos       context;
    TTO_ChainContextPos  chain;
  };

  typedef union TTO_GPOS_SubTable_  TTO_GPOS_SubTable;


  /* finally, the GPOS API */

  /*  EXPORT_DEF
      FT_Export ( FT_Error )  TT_Init_GPOS_Extension( TT_Engine  engine ); */

  EXPORT_DEF
  FT_Error  TT_Load_GPOS_Table( FT_Face          face,
                                TTO_GPOSHeader** gpos,
                                TTO_GDEFHeader*  gdef );

  EXPORT_DEF
  FT_Error  TT_Done_GPOS_Table( TTO_GPOSHeader* gpos );

  EXPORT_DEF
  FT_Error  TT_GPOS_Select_Script( TTO_GPOSHeader*  gpos,
                                   FT_ULong         script_tag,
                                   FT_UShort*       script_index );
  EXPORT_DEF
  FT_Error  TT_GPOS_Select_Language( TTO_GPOSHeader*  gpos,
                                     FT_ULong         language_tag,
                                     FT_UShort        script_index,
                                     FT_UShort*       language_index,
                                     FT_UShort*       req_feature_index );
  EXPORT_DEF
  FT_Error  TT_GPOS_Select_Feature( TTO_GPOSHeader*  gpos,
                                    FT_ULong         feature_tag,
                                    FT_UShort        script_index,
                                    FT_UShort        language_index,
                                    FT_UShort*       feature_index );

  EXPORT_DEF
  FT_Error  TT_GPOS_Query_Scripts( TTO_GPOSHeader*  gpos,
                                   FT_ULong**       script_tag_list );
  EXPORT_DEF
  FT_Error  TT_GPOS_Query_Languages( TTO_GPOSHeader*  gpos,
                                     FT_UShort        script_index,
                                     FT_ULong**       language_tag_list );
  EXPORT_DEF
  FT_Error  TT_GPOS_Query_Features( TTO_GPOSHeader*  gpos,
                                    FT_UShort        script_index,
                                    FT_UShort        language_index,
                                    FT_ULong**       feature_tag_list );

  EXPORT_DEF
  FT_Error  TT_GPOS_Add_Feature( TTO_GPOSHeader*  gpos,
                                 FT_UShort        feature_index,
                                 FT_UInt          property );
  EXPORT_DEF
  FT_Error  TT_GPOS_Clear_Features( TTO_GPOSHeader*  gpos );

  EXPORT_DEF
  FT_Error  TT_GPOS_Register_Glyph_Function( TTO_GPOSHeader*    gpos,
                                             TTO_GlyphFunction  gfunc );

  EXPORT_DEF
  FT_Error  TT_GPOS_Register_MM_Function( TTO_GPOSHeader*  gpos,
                                          TTO_MMFunction   mmfunc,
                                          void*            data );

  /* If `dvi' is TRUE, glyph contour points for anchor points and device
     tables are ignored -- you will get device independent values.         */

  EXPORT_DEF
  FT_Error  TT_GPOS_Apply_String( FT_Face           face,
                                  TTO_GPOSHeader*   gpos,
                                  FT_UShort         load_flags,
				  OTL_Buffer        buffer,
                                  FT_Bool           dvi,
                                  FT_Bool           r2l );

#ifdef __cplusplus
}
#endif

#endif /* FTXGPOS_H */


/* END */
