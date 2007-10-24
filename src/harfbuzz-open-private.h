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

#include "harfbuzz-impl.h"
#include "harfbuzz-open.h"
#include "harfbuzz-gsub-private.h"
#include "harfbuzz-gpos-private.h"

HB_BEGIN_HEADER


struct  HB_SubTable_
{
  union
  {
    HB_GSUB_SubTable  gsub;
    HB_GPOS_SubTable  gpos;
  } st;
};


HB_INTERNAL HB_Error
_HB_OPEN_Load_ScriptList( HB_ScriptList* sl,
			   FT_Stream     stream );
HB_INTERNAL HB_Error
_HB_OPEN_Load_FeatureList( HB_FeatureList* fl,
			   FT_Stream       input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_LookupList( HB_LookupList*  ll,
			  FT_Stream       input,
			  HB_Type         type );

HB_INTERNAL HB_Error
_HB_OPEN_Load_Coverage( HB_Coverage* c,
			FT_Stream    input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_ClassDefinition( HB_ClassDefinition* cd,
			       HB_UShort           limit,
			       FT_Stream           input );
HB_INTERNAL HB_Error
_HB_OPEN_Load_EmptyOrClassDefinition( HB_ClassDefinition* cd,
				      HB_UShort           limit,
				      HB_UInt            class_offset,
				      HB_UInt            base_offset,
				      FT_Stream           stream );
HB_INTERNAL HB_Error
_HB_OPEN_Load_Device( HB_Device* d,
		      FT_Stream  input );

HB_INTERNAL void  _HB_OPEN_Free_ScriptList( HB_ScriptList*  sl );
HB_INTERNAL void  _HB_OPEN_Free_FeatureList( HB_FeatureList*  fl );
HB_INTERNAL void  _HB_OPEN_Free_LookupList( HB_LookupList*  ll,
					    HB_Type         type );

HB_INTERNAL void  _HB_OPEN_Free_Coverage( HB_Coverage*  c );
HB_INTERNAL void  _HB_OPEN_Free_ClassDefinition( HB_ClassDefinition*  cd );
HB_INTERNAL void  _HB_OPEN_Free_Device( HB_Device*  d );



HB_INTERNAL HB_Error
_HB_OPEN_Coverage_Index( HB_Coverage* c,
			 HB_UShort    glyphID,
			 HB_UShort*   index );
HB_INTERNAL HB_Error
_HB_OPEN_Get_Class( HB_ClassDefinition* cd,
		    HB_UShort           glyphID,
		    HB_UShort*          klass,
		    HB_UShort*          index );
HB_INTERNAL HB_Error
_HB_OPEN_Get_Device( HB_Device* d,
		     HB_UShort  size,
		     HB_Short*  value );

HB_END_HEADER

#endif /* HARFBUZZ_OPEN_PRIVATE_H */
