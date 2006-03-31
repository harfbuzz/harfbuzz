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
#ifndef HARFBUZZ_OPEN_PRIVATE_H
#define HARFBUZZ_OPEN_PRIVATE_H

#include "harfbuzz-open.h"
#include "harfbuzz-gsub-private.h"
#include "harfbuzz-gpos-private.h"

FT_BEGIN_HEADER


struct  HB_SubTable_
{
  union
  {
    HB_GSUB_SubTable  gsub;
    HB_GPOS_SubTable  gpos;
  } st;
};


FT_Error  _HB_OPEN_Load_ScriptList( HB_ScriptList*  sl,
			   FT_Stream     stream );
FT_Error  _HB_OPEN_Load_FeatureList( HB_FeatureList*  fl,
			    FT_Stream         input );
FT_Error  _HB_OPEN_Load_LookupList( HB_LookupList*  ll,
			   FT_Stream        input,
			   HB_Type         type );

FT_Error  _HB_OPEN_Load_Coverage( HB_Coverage*  c,
			 FT_Stream      input );
FT_Error  _HB_OPEN_Load_ClassDefinition( HB_ClassDefinition*  cd,
				FT_UShort             limit,
				FT_Stream             input );
FT_Error  _HB_OPEN_Load_EmptyClassDefinition( HB_ClassDefinition*  cd,
				     FT_Stream             input );
FT_Error  _HB_OPEN_Load_Device( HB_Device*  d,
		       FT_Stream    input );

void  _HB_OPEN_Free_ScriptList( HB_ScriptList*  sl, 
		       FT_Memory        memory );
void  _HB_OPEN_Free_FeatureList( HB_FeatureList*  fl,
			FT_Memory         memory );
void  _HB_OPEN_Free_LookupList( HB_LookupList*  ll,
		       HB_Type         type,
		       FT_Memory        memory );

void  _HB_OPEN_Free_Coverage( HB_Coverage*  c,
		     FT_Memory      memory );
void  _HB_OPEN_Free_ClassDefinition( HB_ClassDefinition*  cd,
			    FT_Memory             memory );
void  _HB_OPEN_Free_Device( HB_Device*  d,
		   FT_Memory    memory );



FT_Error  _HB_OPEN_Coverage_Index( HB_Coverage*  c,
			  FT_UShort      glyphID,
			  FT_UShort*     index );
FT_Error  _HB_OPEN_Get_Class( HB_ClassDefinition*  cd,
		     FT_UShort             glyphID,
		     FT_UShort*            class,
		     FT_UShort*            index );
FT_Error  _HB_OPEN_Get_Device( HB_Device*  d,
		      FT_UShort    size,
		      FT_Short*    value );

FT_END_HEADER

#endif /* HARFBUZZ_OPEN_PRIVATE_H */
