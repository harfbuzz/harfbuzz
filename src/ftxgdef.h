/*******************************************************************
 *
 *  ftxgdef.h
 *
 *    TrueType Open GDEF table support
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

#ifndef FTXGDEF_H
#define FTXGDEF_H

#ifdef __cplusplus
extern "C" {
#endif

#define TTO_Err_Invalid_GDEF_SubTable_Format  0x1030
#define TTO_Err_Invalid_GDEF_SubTable         0x1031


/* GDEF glyph classes */

#define UNCLASSIFIED_GLYPH  0
#define SIMPLE_GLYPH        1
#define LIGATURE_GLYPH      2
#define MARK_GLYPH          3
#define COMPONENT_GLYPH     4

/* GDEF glyph properties, corresponding to class values 1-4.  Note that
   TTO_COMPONENT has no corresponding flag in the LookupFlag field.     */

#define TTO_BASE_GLYPH  0x0002
#define TTO_LIGATURE    0x0004
#define TTO_MARK        0x0008
#define TTO_COMPONENT   0x0010


  /* Attachment related structures */

  struct  TTO_AttachPoint_
  {
    FT_UShort   PointCount;             /* size of the PointIndex array */
    FT_UShort*  PointIndex;             /* array of contour points      */
  };

  typedef struct TTO_AttachPoint_  TTO_AttachPoint;


  struct  TTO_AttachList_
  {
    FT_Bool           loaded;

    TTO_Coverage      Coverage;         /* Coverage table              */
    FT_UShort         GlyphCount;       /* number of glyphs with
                                           attachments                 */
    TTO_AttachPoint*  AttachPoint;      /* array of AttachPoint tables */
  };

  typedef struct TTO_AttachList_  TTO_AttachList;


  /* Ligature Caret related structures */

  struct  TTO_CaretValueFormat1_
  {
    FT_Short  Coordinate;               /* x or y value (in design units) */
  };

  typedef struct TTO_CaretValueFormat1_  TTO_CaretValueFormat1;


  struct  TTO_CaretValueFormat2_
  {
    FT_UShort  CaretValuePoint;         /* contour point index on glyph */
  };

  typedef struct TTO_CaretValueFormat2_  TTO_CaretValueFormat2;


  struct  TTO_CaretValueFormat3_
  {
    FT_Short    Coordinate;             /* x or y value (in design units) */
    TTO_Device  Device;                 /* Device table for x or y value  */
  };

  typedef struct TTO_CaretValueFormat3_  TTO_CaretValueFormat3;


  struct  TTO_CaretValueFormat4_
  {
    FT_UShort  IdCaretValue;            /* metric ID */
  };

  typedef struct TTO_CaretValueFormat4_  TTO_CaretValueFormat4;


  struct  TTO_CaretValue_
  {
    FT_UShort  CaretValueFormat;        /* 1, 2, 3, or 4 */

    union
    {
      TTO_CaretValueFormat1  cvf1;
      TTO_CaretValueFormat2  cvf2;
      TTO_CaretValueFormat3  cvf3;
      TTO_CaretValueFormat4  cvf4;
    } cvf;
  };

  typedef struct TTO_CaretValue_  TTO_CaretValue;


  struct  TTO_LigGlyph_
  {
    FT_Bool          loaded;

    FT_UShort        CaretCount;        /* number of caret values */
    TTO_CaretValue*  CaretValue;        /* array of caret values  */
  };

  typedef struct TTO_LigGlyph_  TTO_LigGlyph;


  struct  TTO_LigCaretList_
  {
    FT_Bool        loaded;

    TTO_Coverage   Coverage;            /* Coverage table            */
    FT_UShort      LigGlyphCount;       /* number of ligature glyphs */
    TTO_LigGlyph*  LigGlyph;            /* array of LigGlyph tables  */
  };

  typedef struct TTO_LigCaretList_  TTO_LigCaretList;


  /* The `NewGlyphClasses' field is not defined in the TTO specification.
     We use it for fonts with a constructed `GlyphClassDef' structure
     (i.e., which don't have a GDEF table) to collect glyph classes
     assigned during the lookup process.  The number of arrays in this
     pointer array is GlyphClassDef->cd.cd2.ClassRangeCount+1; the nth
     array then contains the glyph class values of the glyphs not covered
     by the ClassRangeRecords structures with index n-1 and n.  We store
     glyph class values for four glyphs in a single array element.

     `LastGlyph' is identical to the number of glyphs minus one in the
     font; we need it only if `NewGlyphClasses' is not NULL (to have an
     upper bound for the last array).

     Note that we first store the file offset to the `MarkAttachClassDef'
     field (which has been introduced in OpenType 1.2) -- since the
     `Version' field value hasn't been increased to indicate that we have
     one more field for some obscure reason, we must parse the GSUB table
     to find out whether class values refer to this table.  Only then we
     can finally load the MarkAttachClassDef structure if necessary.      */

  struct  TTO_GDEFHeader_
  {
    FT_Memory            memory;
    FT_ULong             offset;

    FT_Fixed             Version;

    TTO_ClassDefinition  GlyphClassDef;
    TTO_AttachList       AttachList;
    TTO_LigCaretList     LigCaretList;
    FT_ULong             MarkAttachClassDef_offset;
    TTO_ClassDefinition  MarkAttachClassDef;        /* new in OT 1.2 */

    FT_UShort            LastGlyph;
    FT_UShort**          NewGlyphClasses;
  };

  typedef struct TTO_GDEFHeader_   TTO_GDEFHeader;
  typedef struct TTO_GDEFHeader_*  TTO_GDEF;


  /* finally, the GDEF API */

  /*  EXPORT_DEF
      FT_Error  TT_Init_GDEF_Extension( TT_Engine  engine ); */

  EXPORT_FUNC
  FT_Error  TT_New_GDEF_Table( FT_Face          face,
			       TTO_GDEFHeader** retptr );
	
  EXPORT_DEF
  FT_Error  TT_Load_GDEF_Table( FT_Face          face,
                                TTO_GDEFHeader** gdef );

  EXPORT_DEF
  FT_Error  TT_Done_GDEF_Table ( TTO_GDEFHeader* gdef );

  EXPORT_DEF
  FT_Error  TT_GDEF_Get_Glyph_Property( TTO_GDEFHeader*  gdef,
                                        FT_UShort        glyphID,
                                        FT_UShort*       property );
  EXPORT_DEF
  FT_Error  TT_GDEF_Build_ClassDefinition( TTO_GDEFHeader*  gdef,
                                           FT_UShort        num_glyphs,
                                           FT_UShort        glyph_count,
                                           FT_UShort*       glyph_array,
                                           FT_UShort*       class_array );


#ifdef __cplusplus
}
#endif

#endif /* FTXGDEF_H */


/* END */
