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
#ifndef HARFBUZZ_GDEF_H
#define HARFBUZZ_GDEF_H

#include "harfbuzz-open.h"

FT_BEGIN_HEADER

#define HB_Err_Invalid_GDEF_SubTable_Format  0x1030
#define HB_Err_Invalid_GDEF_SubTable         0x1031


/* GDEF glyph properties.  Note that HB_GDEF_COMPONENT has no corresponding
 * flag in the LookupFlag field.     */
#define HB_GDEF_BASE_GLYPH  0x0002
#define HB_GDEF_LIGATURE    0x0004
#define HB_GDEF_MARK        0x0008
#define HB_GDEF_COMPONENT   0x0010


typedef struct HB_AttachPoint_  HB_AttachPoint;


struct  HB_AttachList_
{
  FT_Bool           loaded;

  HB_Coverage       Coverage;         /* Coverage table              */
  FT_UShort         GlyphCount;       /* number of glyphs with
					 attachments                 */
  HB_AttachPoint*   AttachPoint;      /* array of AttachPoint tables */
};

typedef struct HB_AttachList_  HB_AttachList;

typedef struct HB_LigGlyph_  HB_LigGlyph;

struct  HB_LigCaretList_
{
  FT_Bool        loaded;

  HB_Coverage    Coverage;            /* Coverage table            */
  FT_UShort      LigGlyphCount;       /* number of ligature glyphs */
  HB_LigGlyph*   LigGlyph;            /* array of LigGlyph tables  */
};

typedef struct HB_LigCaretList_  HB_LigCaretList;



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

struct  HB_GDEFHeader_
{
  FT_Memory            memory;
  FT_ULong             offset;

  FT_Fixed             Version;

  HB_ClassDefinition   GlyphClassDef;
  HB_AttachList        AttachList;
  HB_LigCaretList      LigCaretList;
  FT_ULong             MarkAttachClassDef_offset;
  HB_ClassDefinition   MarkAttachClassDef;        /* new in OT 1.2 */

  FT_UShort            LastGlyph;
  FT_UShort**          NewGlyphClasses;
};

typedef struct HB_GDEFHeader_   HB_GDEFHeader;
typedef struct HB_GDEFHeader_*  HB_GDEF;


FT_Error  HB_New_GDEF_Table( FT_Face          face,
			     HB_GDEFHeader** retptr );
      

FT_Error  HB_Load_GDEF_Table( FT_Face          face,
			      HB_GDEFHeader** gdef );


FT_Error  HB_Done_GDEF_Table ( HB_GDEFHeader* gdef );


FT_Error  HB_GDEF_Get_Glyph_Property( HB_GDEFHeader*  gdef,
				      FT_UShort        glyphID,
				      FT_UShort*       property );

FT_Error  HB_GDEF_Build_ClassDefinition( HB_GDEFHeader*  gdef,
					 FT_UShort        num_glyphs,
					 FT_UShort        glyph_count,
					 FT_UShort*       glyph_array,
					 FT_UShort*       class_array );


FT_END_HEADER

#endif /* HARFBUZZ_GDEF_H */
