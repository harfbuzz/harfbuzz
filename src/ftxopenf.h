/*******************************************************************
 *
 *  ftxopenf.h
 *
 *    internal TrueType Open functions
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

#ifndef FTXOPENF_H
#define FTXOPENF_H

#include "ftxopen.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* functions from ftxopen.c */

  FT_Error  Load_ScriptList( TTO_ScriptList*  sl,
			     FT_Stream     stream );
  FT_Error  Load_FeatureList( TTO_FeatureList*  fl,
                              FT_Stream         input );
  FT_Error  Load_LookupList( TTO_LookupList*  ll,
                             FT_Stream        input,
                             TTO_Type         type );

  FT_Error  Load_Coverage( TTO_Coverage*  c,
                           FT_Stream      input );
  FT_Error  Load_ClassDefinition( TTO_ClassDefinition*  cd,
                                  FT_UShort             limit,
                                  FT_Stream             input );
  FT_Error  Load_EmptyClassDefinition( TTO_ClassDefinition*  cd,
                                       FT_Stream             input );
  FT_Error  Load_Device( TTO_Device*  d,
                         FT_Stream    input );

  void  Free_ScriptList( TTO_ScriptList*  sl, 
                         FT_Memory        memory );
  void  Free_FeatureList( TTO_FeatureList*  fl,
			  FT_Memory         memory );
  void  Free_LookupList( TTO_LookupList*  ll,
                         TTO_Type         type,
			 FT_Memory        memory );

  void  Free_Coverage( TTO_Coverage*  c,
		       FT_Memory      memory );
  void  Free_ClassDefinition( TTO_ClassDefinition*  cd,
			      FT_Memory             memory );
  void  Free_Device( TTO_Device*  d,
		     FT_Memory    memory );


  /* functions from ftxgsub.c */

  FT_Error  Load_SingleSubst( TTO_SingleSubst*  ss,
                              FT_Stream         input );
  FT_Error  Load_MultipleSubst( TTO_MultipleSubst*  ms,
                                FT_Stream           input );
  FT_Error  Load_AlternateSubst( TTO_AlternateSubst*  as,
                                 FT_Stream            input );
  FT_Error  Load_LigatureSubst( TTO_LigatureSubst*  ls,
                                FT_Stream           input );
  FT_Error  Load_ContextSubst( TTO_ContextSubst*  cs,
                               FT_Stream          input );
  FT_Error  Load_ChainContextSubst( TTO_ChainContextSubst*  ccs,
                                    FT_Stream               input );

  void  Free_SingleSubst( TTO_SingleSubst*  ss,
			  FT_Memory         memory );
  void  Free_MultipleSubst( TTO_MultipleSubst*  ms,
			    FT_Memory         memory );
  void  Free_AlternateSubst( TTO_AlternateSubst*  as,
			     FT_Memory            memory );
  void  Free_LigatureSubst( TTO_LigatureSubst*  ls,
			    FT_Memory           memory );
  void  Free_ContextSubst( TTO_ContextSubst*  cs,
			   FT_Memory         memory );
  void  Free_ChainContextSubst( TTO_ChainContextSubst*  ccs,
				FT_Memory               memory );


  /* functions from ftxgpos.c */

  FT_Error  Load_SinglePos( TTO_SinglePos*  sp,
                            FT_Stream       input );
  FT_Error  Load_PairPos( TTO_PairPos*  pp,
                          FT_Stream     input );
  FT_Error  Load_CursivePos( TTO_CursivePos*  cp,
                             FT_Stream        input );
  FT_Error  Load_MarkBasePos( TTO_MarkBasePos*  mbp,
                              FT_Stream         input );
  FT_Error  Load_MarkLigPos( TTO_MarkLigPos*  mlp,
                             FT_Stream        input );
  FT_Error  Load_MarkMarkPos( TTO_MarkMarkPos*  mmp,
                              FT_Stream         input );
  FT_Error  Load_ContextPos( TTO_ContextPos*  cp,
                             FT_Stream        input );
  FT_Error  Load_ChainContextPos( TTO_ChainContextPos*  ccp,
                                  FT_Stream             input );

  void  Free_SinglePos( TTO_SinglePos*  sp,
			FT_Memory       memory );
  void  Free_PairPos( TTO_PairPos*  pp,
		      FT_Memory     memory );
  void  Free_CursivePos( TTO_CursivePos*  cp,
   		         FT_Memory     memory );
  void  Free_MarkBasePos( TTO_MarkBasePos*  mbp,
			  FT_Memory         memory );
  void  Free_MarkLigPos( TTO_MarkLigPos*  mlp,
			 FT_Memory        memory );
  void  Free_MarkMarkPos( TTO_MarkMarkPos*  mmp,
			  FT_Memory         memory );
  void  Free_ContextPos( TTO_ContextPos*  cp,
			 FT_Memory         memory );
  void  Free_ChainContextPos( TTO_ChainContextPos*  ccp,
			      FT_Memory             memory );
  /* query functions */

  FT_Error  Coverage_Index( TTO_Coverage*  c,
                            FT_UShort      glyphID,
                            FT_UShort*     index );
  FT_Error  Get_Class( TTO_ClassDefinition*  cd,
                       FT_UShort             glyphID,
                       FT_UShort*            class,
                       FT_UShort*            index );
  FT_Error  Get_Device( TTO_Device*  d,
                        FT_UShort    size,
                        FT_Short*    value );


  /* functions from ftxgdef.c */

  FT_Error  Add_Glyph_Property( TTO_GDEFHeader*  gdef,
                                FT_UShort        glyphID,
                                FT_UShort        property );

  FT_Error  Check_Property( TTO_GDEFHeader*  gdef,
                            OTL_GlyphItem    item,
                            FT_UShort        flags,
                            FT_UShort*       property );

#define CHECK_Property( gdef, index, flags, property )              \
          ( ( error = Check_Property( (gdef), (index), (flags),     \
                                      (property) ) ) != TT_Err_Ok )

#ifdef __cplusplus
}
#endif

#endif /* FTXOPENF_H */


/* END */
