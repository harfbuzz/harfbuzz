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
#ifndef HARFBUZZ_GPOS_PRIVATE_H
#define HARFBUZZ_GPOS_PRIVATE_H

#include "harfbuzz-gpos.h"

FT_BEGIN_HEADER


/* shared tables */

struct  HB_ValueRecord_
{
  FT_Short    XPlacement;             /* horizontal adjustment for
					 placement                      */
  FT_Short    YPlacement;             /* vertical adjustment for
					 placement                      */
  FT_Short    XAdvance;               /* horizontal adjustment for
					 advance                        */
  FT_Short    YAdvance;               /* vertical adjustment for
					 advance                        */
  HB_Device  XPlacementDevice;       /* device table for horizontal
					 placement                      */
  HB_Device  YPlacementDevice;       /* device table for vertical
					 placement                      */
  HB_Device  XAdvanceDevice;         /* device table for horizontal
					 advance                        */
  HB_Device  YAdvanceDevice;         /* device table for vertical
					 advance                        */
  FT_UShort   XIdPlacement;           /* horizontal placement metric ID */
  FT_UShort   YIdPlacement;           /* vertical placement metric ID   */
  FT_UShort   XIdAdvance;             /* horizontal advance metric ID   */
  FT_UShort   YIdAdvance;             /* vertical advance metric ID     */
};

typedef struct HB_ValueRecord_  HB_ValueRecord;


/* Mask values to scan the value format of the ValueRecord structure.
 We always expand compressed ValueRecords of the font.              */

#define HB_GPOS_FORMAT_HAVE_X_PLACEMENT         0x0001
#define HB_GPOS_FORMAT_HAVE_Y_PLACEMENT         0x0002
#define HB_GPOS_FORMAT_HAVE_X_ADVANCE           0x0004
#define HB_GPOS_FORMAT_HAVE_Y_ADVANCE           0x0008
#define HB_GPOS_FORMAT_HAVE_X_PLACEMENT_DEVICE  0x0010
#define HB_GPOS_FORMAT_HAVE_Y_PLACEMENT_DEVICE  0x0020
#define HB_GPOS_FORMAT_HAVE_X_ADVANCE_DEVICE    0x0040
#define HB_GPOS_FORMAT_HAVE_Y_ADVANCE_DEVICE    0x0080
#define HB_GPOS_FORMAT_HAVE_X_ID_PLACEMENT      0x0100
#define HB_GPOS_FORMAT_HAVE_Y_ID_PLACEMENT      0x0200
#define HB_GPOS_FORMAT_HAVE_X_ID_ADVANCE        0x0400
#define HB_GPOS_FORMAT_HAVE_Y_ID_ADVANCE        0x0800


struct  HB_AnchorFormat1_
{
  FT_Short   XCoordinate;             /* horizontal value */
  FT_Short   YCoordinate;             /* vertical value   */
};

typedef struct HB_AnchorFormat1_  HB_AnchorFormat1;


struct  HB_AnchorFormat2_
{
  FT_Short   XCoordinate;             /* horizontal value             */
  FT_Short   YCoordinate;             /* vertical value               */
  FT_UShort  AnchorPoint;             /* index to glyph contour point */
};

typedef struct HB_AnchorFormat2_  HB_AnchorFormat2;


struct  HB_AnchorFormat3_
{
  FT_Short    XCoordinate;            /* horizontal value              */
  FT_Short    YCoordinate;            /* vertical value                */
  HB_Device  XDeviceTable;           /* device table for X coordinate */
  HB_Device  YDeviceTable;           /* device table for Y coordinate */
};

typedef struct HB_AnchorFormat3_  HB_AnchorFormat3;


struct  HB_AnchorFormat4_
{
  FT_UShort  XIdAnchor;               /* horizontal metric ID */
  FT_UShort  YIdAnchor;               /* vertical metric ID   */
};

typedef struct HB_AnchorFormat4_  HB_AnchorFormat4;


struct  HB_Anchor_
{
  FT_UShort  PosFormat;               /* 1, 2, 3, or 4 -- 0 indicates
					 that there is no Anchor table */

  union
  {
    HB_AnchorFormat1  af1;
    HB_AnchorFormat2  af2;
    HB_AnchorFormat3  af3;
    HB_AnchorFormat4  af4;
  } af;
};

typedef struct HB_Anchor_  HB_Anchor;


struct  HB_MarkRecord_
{
  FT_UShort   Class;                  /* mark class   */
  HB_Anchor  MarkAnchor;             /* anchor table */
};

typedef struct HB_MarkRecord_  HB_MarkRecord;


struct  HB_MarkArray_
{
  FT_UShort        MarkCount;         /* number of MarkRecord tables */
  HB_MarkRecord*  MarkRecord;        /* array of MarkRecord tables  */
};

typedef struct HB_MarkArray_  HB_MarkArray;


/* LookupType 1 */

struct  HB_SinglePosFormat1_
{
  HB_ValueRecord  Value;             /* ValueRecord for all covered
					 glyphs                      */
};

typedef struct HB_SinglePosFormat1_  HB_SinglePosFormat1;


struct  HB_SinglePosFormat2_
{
  FT_UShort         ValueCount;       /* number of ValueRecord tables */
  HB_ValueRecord*  Value;            /* array of ValueRecord tables  */
};

typedef struct HB_SinglePosFormat2_  HB_SinglePosFormat2;


struct  HB_SinglePos_
{
  FT_UShort     PosFormat;            /* 1 or 2         */
  HB_Coverage  Coverage;             /* Coverage table */

  FT_UShort     ValueFormat;          /* format of ValueRecord table */

  union
  {
    HB_SinglePosFormat1  spf1;
    HB_SinglePosFormat2  spf2;
  } spf;
};

typedef struct HB_SinglePos_  HB_SinglePos;


/* LookupType 2 */

struct  HB_PairValueRecord_
{
  FT_UShort        SecondGlyph;       /* glyph ID for second glyph  */
  HB_ValueRecord  Value1;            /* pos. data for first glyph  */
  HB_ValueRecord  Value2;            /* pos. data for second glyph */
};

typedef struct HB_PairValueRecord_  HB_PairValueRecord;


struct  HB_PairSet_
{
  FT_UShort             PairValueCount;
				      /* number of PairValueRecord tables */
  HB_PairValueRecord*  PairValueRecord;
				      /* array of PairValueRecord tables  */
};

typedef struct HB_PairSet_  HB_PairSet;


struct  HB_PairPosFormat1_
{
  FT_UShort     PairSetCount;         /* number of PairSet tables    */
  HB_PairSet*  PairSet;              /* array of PairSet tables     */
};

typedef struct HB_PairPosFormat1_  HB_PairPosFormat1;


struct  HB_Class2Record_
{
  HB_ValueRecord  Value1;            /* pos. data for first glyph  */
  HB_ValueRecord  Value2;            /* pos. data for second glyph */
};

typedef struct HB_Class2Record_  HB_Class2Record;


struct  HB_Class1Record_
{
  HB_Class2Record*  Class2Record;    /* array of Class2Record tables */
};

typedef struct HB_Class1Record_  HB_Class1Record;


struct  HB_PairPosFormat2_
{
  HB_ClassDefinition  ClassDef1;     /* class def. for first glyph     */
  HB_ClassDefinition  ClassDef2;     /* class def. for second glyph    */
  FT_UShort            Class1Count;   /* number of classes in ClassDef1
					 table                          */
  FT_UShort            Class2Count;   /* number of classes in ClassDef2
					 table                          */
  HB_Class1Record*    Class1Record;  /* array of Class1Record tables   */
};

typedef struct HB_PairPosFormat2_  HB_PairPosFormat2;


struct  HB_PairPos_
{
  FT_UShort     PosFormat;            /* 1 or 2         */
  HB_Coverage  Coverage;             /* Coverage table */
  FT_UShort     ValueFormat1;         /* format of ValueRecord table
					 for first glyph             */
  FT_UShort     ValueFormat2;         /* format of ValueRecord table
					 for second glyph            */

  union
  {
    HB_PairPosFormat1  ppf1;
    HB_PairPosFormat2  ppf2;
  } ppf;
};

typedef struct HB_PairPos_  HB_PairPos;


/* LookupType 3 */

struct  HB_EntryExitRecord_
{
  HB_Anchor  EntryAnchor;            /* entry Anchor table */
  HB_Anchor  ExitAnchor;             /* exit Anchor table  */
};


typedef struct HB_EntryExitRecord_  HB_EntryExitRecord;

struct  HB_CursivePos_
{
  FT_UShort             PosFormat;    /* always 1                         */
  HB_Coverage          Coverage;     /* Coverage table                   */
  FT_UShort             EntryExitCount;
				      /* number of EntryExitRecord tables */
  HB_EntryExitRecord*  EntryExitRecord;
				      /* array of EntryExitRecord tables  */
};

typedef struct HB_CursivePos_  HB_CursivePos;


/* LookupType 4 */

struct  HB_BaseRecord_
{
  HB_Anchor*  BaseAnchor;            /* array of base glyph anchor
					 tables                     */
};

typedef struct HB_BaseRecord_  HB_BaseRecord;


struct  HB_BaseArray_
{
  FT_UShort        BaseCount;         /* number of BaseRecord tables */
  HB_BaseRecord*  BaseRecord;        /* array of BaseRecord tables  */
};

typedef struct HB_BaseArray_  HB_BaseArray;


struct  HB_MarkBasePos_
{
  FT_UShort      PosFormat;           /* always 1                  */
  HB_Coverage   MarkCoverage;        /* mark glyph coverage table */
  HB_Coverage   BaseCoverage;        /* base glyph coverage table */
  FT_UShort      ClassCount;          /* number of mark classes    */
  HB_MarkArray  MarkArray;           /* mark array table          */
  HB_BaseArray  BaseArray;           /* base array table          */
};

typedef struct HB_MarkBasePos_  HB_MarkBasePos;


/* LookupType 5 */

struct  HB_ComponentRecord_
{
  HB_Anchor*  LigatureAnchor;        /* array of ligature glyph anchor
					 tables                         */
};

typedef struct HB_ComponentRecord_  HB_ComponentRecord;


struct  HB_LigatureAttach_
{
  FT_UShort             ComponentCount;
				      /* number of ComponentRecord tables */
  HB_ComponentRecord*  ComponentRecord;
				      /* array of ComponentRecord tables  */
};

typedef struct HB_LigatureAttach_  HB_LigatureAttach;


struct  HB_LigatureArray_
{
  FT_UShort            LigatureCount; /* number of LigatureAttach tables */
  HB_LigatureAttach*  LigatureAttach;
				      /* array of LigatureAttach tables  */
};

typedef struct HB_LigatureArray_  HB_LigatureArray;


struct  HB_MarkLigPos_
{
  FT_UShort          PosFormat;       /* always 1                      */
  HB_Coverage       MarkCoverage;    /* mark glyph coverage table     */
  HB_Coverage       LigatureCoverage;
				      /* ligature glyph coverage table */
  FT_UShort          ClassCount;      /* number of mark classes        */
  HB_MarkArray      MarkArray;       /* mark array table              */
  HB_LigatureArray  LigatureArray;   /* ligature array table          */
};

typedef struct HB_MarkLigPos_  HB_MarkLigPos;


/* LookupType 6 */

struct  HB_Mark2Record_
{
  HB_Anchor*  Mark2Anchor;           /* array of mark glyph anchor
					 tables                     */
};

typedef struct HB_Mark2Record_  HB_Mark2Record;


struct  HB_Mark2Array_
{
  FT_UShort         Mark2Count;       /* number of Mark2Record tables */
  HB_Mark2Record*  Mark2Record;      /* array of Mark2Record tables  */
};

typedef struct HB_Mark2Array_  HB_Mark2Array;


struct  HB_MarkMarkPos_
{
  FT_UShort       PosFormat;          /* always 1                         */
  HB_Coverage    Mark1Coverage;      /* first mark glyph coverage table  */
  HB_Coverage    Mark2Coverage;      /* second mark glyph coverave table */
  FT_UShort       ClassCount;         /* number of combining mark classes */
  HB_MarkArray   Mark1Array;         /* MarkArray table for first mark   */
  HB_Mark2Array  Mark2Array;         /* MarkArray table for second mark  */
};

typedef struct HB_MarkMarkPos_  HB_MarkMarkPos;


/* needed by both lookup type 7 and 8 */

struct  HB_PosLookupRecord_
{
  FT_UShort  SequenceIndex;           /* index into current
					 glyph sequence               */
  FT_UShort  LookupListIndex;         /* Lookup to apply to that pos. */
};

typedef struct HB_PosLookupRecord_  HB_PosLookupRecord;


/* LookupType 7 */

struct  HB_PosRule_
{
  FT_UShort             GlyphCount;   /* total number of input glyphs     */
  FT_UShort             PosCount;     /* number of PosLookupRecord tables */
  FT_UShort*            Input;        /* array of input glyph IDs         */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecord tables  */
};

typedef struct HB_PosRule_  HB_PosRule;


struct  HB_PosRuleSet_
{
  FT_UShort     PosRuleCount;         /* number of PosRule tables */
  HB_PosRule*  PosRule;              /* array of PosRule tables  */
};

typedef struct HB_PosRuleSet_  HB_PosRuleSet;


struct  HB_ContextPosFormat1_
{
  HB_Coverage     Coverage;          /* Coverage table              */
  FT_UShort        PosRuleSetCount;   /* number of PosRuleSet tables */
  HB_PosRuleSet*  PosRuleSet;        /* array of PosRuleSet tables  */
};

typedef struct HB_ContextPosFormat1_  HB_ContextPosFormat1;


struct  HB_PosClassRule_
{
  FT_UShort             GlyphCount;   /* total number of context classes  */
  FT_UShort             PosCount;     /* number of PosLookupRecord tables */
  FT_UShort*            Class;        /* array of classes                 */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecord tables  */
};

typedef struct HB_PosClassRule_  HB_PosClassRule;


struct  HB_PosClassSet_
{
  FT_UShort          PosClassRuleCount;
				      /* number of PosClassRule tables */
  HB_PosClassRule*  PosClassRule;    /* array of PosClassRule tables  */
};

typedef struct HB_PosClassSet_  HB_PosClassSet;


/* The `MaxContextLength' field is not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the context rules.                    */

struct  HB_ContextPosFormat2_
{
  FT_UShort            MaxContextLength;
				      /* maximal context length       */
  HB_Coverage         Coverage;      /* Coverage table               */
  HB_ClassDefinition  ClassDef;      /* ClassDef table               */
  FT_UShort            PosClassSetCount;
				      /* number of PosClassSet tables */
  HB_PosClassSet*     PosClassSet;   /* array of PosClassSet tables  */
};

typedef struct HB_ContextPosFormat2_  HB_ContextPosFormat2;


struct  HB_ContextPosFormat3_
{
  FT_UShort             GlyphCount;   /* number of input glyphs           */
  FT_UShort             PosCount;     /* number of PosLookupRecord tables */
  HB_Coverage*         Coverage;     /* array of Coverage tables         */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecord tables  */
};

typedef struct HB_ContextPosFormat3_  HB_ContextPosFormat3;


struct  HB_ContextPos_
{
  FT_UShort  PosFormat;               /* 1, 2, or 3     */

  union
  {
    HB_ContextPosFormat1  cpf1;
    HB_ContextPosFormat2  cpf2;
    HB_ContextPosFormat3  cpf3;
  } cpf;
};

typedef struct HB_ContextPos_  HB_ContextPos;


/* LookupType 8 */

struct  HB_ChainPosRule_
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
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of PosLookupRecords       */
};

typedef struct HB_ChainPosRule_  HB_ChainPosRule;


struct  HB_ChainPosRuleSet_
{
  FT_UShort          ChainPosRuleCount;
				      /* number of ChainPosRule tables */
  HB_ChainPosRule*  ChainPosRule;    /* array of ChainPosRule tables  */
};

typedef struct HB_ChainPosRuleSet_  HB_ChainPosRuleSet;


struct  HB_ChainContextPosFormat1_
{
  HB_Coverage          Coverage;     /* Coverage table                   */
  FT_UShort             ChainPosRuleSetCount;
				      /* number of ChainPosRuleSet tables */
  HB_ChainPosRuleSet*  ChainPosRuleSet;
				      /* array of ChainPosRuleSet tables  */
};

typedef struct HB_ChainContextPosFormat1_  HB_ChainContextPosFormat1;


struct  HB_ChainPosClassRule_
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
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of substitution lookups   */
};

typedef struct HB_ChainPosClassRule_  HB_ChainPosClassRule;


struct  HB_ChainPosClassSet_
{
  FT_UShort               ChainPosClassRuleCount;
				      /* number of ChainPosClassRule
					 tables                      */
  HB_ChainPosClassRule*  ChainPosClassRule;
				      /* array of ChainPosClassRule
					 tables                      */
};

typedef struct HB_ChainPosClassSet_  HB_ChainPosClassSet;


/* The `MaxXXXLength' fields are not defined in the TTO specification
   but simplifies the implementation of this format.  It holds the
   maximal context length used in the specific context rules.         */

struct  HB_ChainContextPosFormat2_
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

  FT_UShort              ChainPosClassSetCount;
				      /* number of ChainPosClassSet
					 tables                     */
  HB_ChainPosClassSet*  ChainPosClassSet;
				      /* array of ChainPosClassSet
					 tables                     */
};

typedef struct HB_ChainContextPosFormat2_  HB_ChainContextPosFormat2;


struct  HB_ChainContextPosFormat3_
{
  FT_UShort             BacktrackGlyphCount;
				      /* number of backtrack glyphs    */
  HB_Coverage*         BacktrackCoverage;
				      /* array of backtrack Coverage
					 tables                        */
  FT_UShort             InputGlyphCount;
				      /* number of input glyphs        */
  HB_Coverage*         InputCoverage;
				      /* array of input coverage
					 tables                        */
  FT_UShort             LookaheadGlyphCount;
				      /* number of lookahead glyphs    */
  HB_Coverage*         LookaheadCoverage;
				      /* array of lookahead coverage
					 tables                        */
  FT_UShort             PosCount;     /* number of PosLookupRecords    */
  HB_PosLookupRecord*  PosLookupRecord;
				      /* array of substitution lookups */
};

typedef struct HB_ChainContextPosFormat3_  HB_ChainContextPosFormat3;


struct  HB_ChainContextPos_
{
  FT_UShort  PosFormat;             /* 1, 2, or 3 */

  union
  {
    HB_ChainContextPosFormat1  ccpf1;
    HB_ChainContextPosFormat2  ccpf2;
    HB_ChainContextPosFormat3  ccpf3;
  } ccpf;
};

typedef struct HB_ChainContextPos_  HB_ChainContextPos;


union  HB_GPOS_SubTable_
{
  HB_SinglePos        single;
  HB_PairPos          pair;
  HB_CursivePos       cursive;
  HB_MarkBasePos      markbase;
  HB_MarkLigPos       marklig;
  HB_MarkMarkPos      markmark;
  HB_ContextPos       context;
  HB_ChainContextPos  chain;
};

typedef union HB_GPOS_SubTable_  HB_GPOS_SubTable;



FT_Error  _HB_GPOS_Load_SubTable( HB_GPOS_SubTable*  st,
				  FT_Stream     stream,
				  FT_UShort     lookup_type );

void  _HB_GPOS_Free_SubTable( HB_GPOS_SubTable*  st,
			      FT_Memory     memory,
			      FT_UShort     lookup_type );

FT_END_HEADER

#endif /* HARFBUZZ_GPOS_PRIVATE_H */
