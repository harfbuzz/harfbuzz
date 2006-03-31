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
#ifndef HARFBUZZ_GSUB_H
#define HARFBUZZ_GSUB_H

#include "harfbuzz-gdef.h"
#include "harfbuzz-buffer.h"

FT_BEGIN_HEADER

#define HB_Err_Invalid_GSUB_SubTable_Format  0x1010
#define HB_Err_Invalid_GSUB_SubTable         0x1011


/* Lookup types for glyph substitution */

#define HB_GSUB_LOOKUP_SINGLE        1
#define HB_GSUB_LOOKUP_MULTIPLE      2
#define HB_GSUB_LOOKUP_ALTERNATE     3
#define HB_GSUB_LOOKUP_LIGATURE      4
#define HB_GSUB_LOOKUP_CONTEXT       5
#define HB_GSUB_LOOKUP_CHAIN         6
#define HB_GSUB_LOOKUP_EXTENSION     7
#define HB_GSUB_LOOKUP_REVERSE_CHAIN 8


/* A pointer to a function which selects the alternate glyph.  `pos' is
   the position of the glyph with index `glyphID', `num_alternates'
   gives the number of alternates in the `alternates' array.  `data'
   points to the user-defined structure specified during a call to
   HB_GSUB_Register_Alternate_Function().  The function must return an
   index into the `alternates' array.                                   */

typedef FT_UShort  (*HB_AltFunction)(FT_ULong    pos,
				      FT_UShort   glyphID,
				      FT_UShort   num_alternates,
				      FT_UShort*  alternates,
				      void*       data );


struct  HB_GSUBHeader_
{
  FT_Memory        memory;
  
  FT_ULong         offset;

  FT_Fixed         Version;

  HB_ScriptList   ScriptList;
  HB_FeatureList  FeatureList;
  HB_LookupList   LookupList;

  HB_GDEFHeader*  gdef;

  /* the next two fields are used for an alternate substitution callback
     function to select the proper alternate glyph.                      */

  HB_AltFunction  altfunc;
  void*            data;
};

typedef struct HB_GSUBHeader_   HB_GSUBHeader;
typedef HB_GSUBHeader*  HB_GSUB;


FT_Error  HB_Load_GSUB_Table( FT_Face          face,
			      HB_GSUBHeader** gsub,
			      HB_GDEFHeader*  gdef );


FT_Error  HB_Done_GSUB_Table( HB_GSUBHeader*  gsub );


FT_Error  HB_GSUB_Select_Script( HB_GSUBHeader*  gsub,
				 FT_ULong         script_tag,
				 FT_UShort*       script_index );

FT_Error  HB_GSUB_Select_Language( HB_GSUBHeader*  gsub,
				   FT_ULong         language_tag,
				   FT_UShort        script_index,
				   FT_UShort*       language_index,
				   FT_UShort*       req_feature_index );

FT_Error  HB_GSUB_Select_Feature( HB_GSUBHeader*  gsub,
				  FT_ULong         feature_tag,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_UShort*       feature_index );


FT_Error  HB_GSUB_Query_Scripts( HB_GSUBHeader*  gsub,
				 FT_ULong**       script_tag_list );

FT_Error  HB_GSUB_Query_Languages( HB_GSUBHeader*  gsub,
				   FT_UShort        script_index,
				   FT_ULong**       language_tag_list );

FT_Error  HB_GSUB_Query_Features( HB_GSUBHeader*  gsub,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_ULong**       feature_tag_list );


FT_Error  HB_GSUB_Add_Feature( HB_GSUBHeader*  gsub,
			       FT_UShort        feature_index,
			       FT_UInt          property );

FT_Error  HB_GSUB_Clear_Features( HB_GSUBHeader*  gsub );


FT_Error  HB_GSUB_Register_Alternate_Function( HB_GSUBHeader*  gsub,
					       HB_AltFunction  altfunc,
					       void*            data );


FT_Error  HB_GSUB_Apply_String( HB_GSUBHeader*   gsub,
				HB_Buffer        buffer );


FT_END_HEADER

#endif /* HARFBUZZ_GSUB_H */
