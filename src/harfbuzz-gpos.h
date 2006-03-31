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
#ifndef HARFBUZZ_GPOS_H
#define HARFBUZZ_GPOS_H

#include "harfbuzz-gdef.h"
#include "harfbuzz-buffer.h"

FT_BEGIN_HEADER

#define HB_Err_Invalid_GPOS_SubTable_Format  0x1020
#define HB_Err_Invalid_GPOS_SubTable         0x1021


/* Lookup types for glyph positioning */

#define HB_GPOS_LOOKUP_SINGLE     1
#define HB_GPOS_LOOKUP_PAIR       2
#define HB_GPOS_LOOKUP_CURSIVE    3
#define HB_GPOS_LOOKUP_MARKBASE   4
#define HB_GPOS_LOOKUP_MARKLIG    5
#define HB_GPOS_LOOKUP_MARKMARK   6
#define HB_GPOS_LOOKUP_CONTEXT    7
#define HB_GPOS_LOOKUP_CHAIN      8
#define HB_GPOS_LOOKUP_EXTENSION  9


/* A pointer to a function which loads a glyph.  Its parameters are
   the same as in a call to Load_Glyph() -- if no glyph loading
   function will be registered with HB_GPOS_Register_Glyph_Function(),
   Load_Glyph() will be called indeed.  The purpose of this function
   pointer is to provide a hook for caching glyph outlines and sbits
   (using the instance's generic pointer to hold the data).

   If for some reason no outline data is available (e.g. for an
   embedded bitmap glyph), _glyph->outline.n_points should be set to
   zero.  _glyph can be computed with

      _glyph = HANDLE_Glyph( glyph )                                    */

typedef FT_Error  (*HB_GlyphFunction)(FT_Face      face,
				       FT_UInt      glyphIndex,
				       FT_Int       loadFlags );


/* A pointer to a function which accesses the PostScript interpreter.
   Multiple Master fonts need this interface to convert a metric ID
   (as stored in an OpenType font version 1.2 or higher) `metric_id'
   into a metric value (returned in `metric_value').

   `data' points to the user-defined structure specified during a
   call to HB_GPOS_Register_MM_Function().

   `metric_value' must be returned as a scaled value (but shouldn't
   be rounded).                                                       */

typedef FT_Error  (*HB_MMFunction)(FT_Face      face,
				    FT_UShort    metric_id,
				    FT_Pos*      metric_value,
				    void*        data );


struct  HB_GPOSHeader_
{
  FT_Memory          memory;
  
  FT_Fixed           Version;

  HB_ScriptList     ScriptList;
  HB_FeatureList    FeatureList;
  HB_LookupList     LookupList;

  HB_GDEFHeader*    gdef;

  /* the next field is used for a callback function to get the
     glyph outline.                                            */

  HB_GlyphFunction  gfunc;

  /* this is OpenType 1.2 -- Multiple Master fonts need this
     callback function to get various metric values from the
     PostScript interpreter.                                 */

  HB_MMFunction     mmfunc;
  void*              data;
};

typedef struct HB_GPOSHeader_  HB_GPOSHeader;
typedef HB_GPOSHeader* HB_GPOS;


FT_Error  HB_Load_GPOS_Table( FT_Face          face,
			      HB_GPOSHeader** gpos,
			      HB_GDEFHeader*  gdef );


FT_Error  HB_Done_GPOS_Table( HB_GPOSHeader* gpos );


FT_Error  HB_GPOS_Select_Script( HB_GPOSHeader*  gpos,
				 FT_ULong         script_tag,
				 FT_UShort*       script_index );

FT_Error  HB_GPOS_Select_Language( HB_GPOSHeader*  gpos,
				   FT_ULong         language_tag,
				   FT_UShort        script_index,
				   FT_UShort*       language_index,
				   FT_UShort*       req_feature_index );

FT_Error  HB_GPOS_Select_Feature( HB_GPOSHeader*  gpos,
				  FT_ULong         feature_tag,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_UShort*       feature_index );


FT_Error  HB_GPOS_Query_Scripts( HB_GPOSHeader*  gpos,
				 FT_ULong**       script_tag_list );

FT_Error  HB_GPOS_Query_Languages( HB_GPOSHeader*  gpos,
				   FT_UShort        script_index,
				   FT_ULong**       language_tag_list );

FT_Error  HB_GPOS_Query_Features( HB_GPOSHeader*  gpos,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_ULong**       feature_tag_list );


FT_Error  HB_GPOS_Add_Feature( HB_GPOSHeader*  gpos,
			       FT_UShort        feature_index,
			       FT_UInt          property );

FT_Error  HB_GPOS_Clear_Features( HB_GPOSHeader*  gpos );


FT_Error  HB_GPOS_Register_Glyph_Function( HB_GPOSHeader*    gpos,
					   HB_GlyphFunction  gfunc );


FT_Error  HB_GPOS_Register_MM_Function( HB_GPOSHeader*  gpos,
					HB_MMFunction   mmfunc,
					void*            data );

/* If `dvi' is TRUE, glyph contour points for anchor points and device
   tables are ignored -- you will get device independent values.         */


FT_Error  HB_GPOS_Apply_String( FT_Face           face,
				HB_GPOSHeader*   gpos,
				FT_UShort         load_flags,
				HB_Buffer        buffer,
				FT_Bool           dvi,
				FT_Bool           r2l );

FT_END_HEADER

#endif /* HARFBUZZ_GPOS_H */
