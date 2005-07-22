/*******************************************************************
 *
 *  ftxgdef.c
 *
 *    TrueType Open GDEF table support.
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

#include "ftxopen.h"
#include "ftxopenf.h"

#include "ftglue.h"

#include FT_TRUETYPE_TAGS_H

#define TTAG_GDEF  FT_MAKE_TAG( 'G', 'D', 'E', 'F' )

  static FT_Error  Load_AttachList( TTO_AttachList*  al,
                                    FT_Stream        stream );
  static FT_Error  Load_LigCaretList( TTO_LigCaretList*  lcl,
                                      FT_Stream          stream );

  static void  Free_AttachList( TTO_AttachList*  al,
				FT_Memory        memory );
  static void  Free_LigCaretList( TTO_LigCaretList*  lcl,
				  FT_Memory          memory );

  static void  Free_NewGlyphClasses( TTO_GDEFHeader*  gdef,
				     FT_Memory        memory );



  /**********************
   * Extension Functions
   **********************/

#if 0
#define GDEF_ID  Build_Extension_ID( 'G', 'D', 'E', 'F' )


  static FT_Error  GDEF_Create( void*  ext,
                                PFace  face )
  {
    DEFINE_LOAD_LOCALS( face->stream );

    TTO_GDEFHeader*  gdef = (TTO_GDEFHeader*)ext;
    Long             table;


    /* by convention */

    if ( !gdef )
      return TT_Err_Ok;

    /* a null offset indicates that there is no GDEF table */

    gdef->offset = 0;

    /* we store the start offset and the size of the subtable */

    table = TT_LookUp_Table( face, TTAG_GDEF );
    if ( table < 0 )
      return TT_Err_Ok;             /* The table is optional */

    if ( FILE_Seek( face->dirTables[table].Offset ) ||
         ACCESS_Frame( 4L ) )
      return error;

    gdef->offset  = FILE_Pos() - 4L;    /* undo ACCESS_Frame() */
    gdef->Version = GET_ULong();

    FORGET_Frame();

    gdef->loaded = FALSE;

    return TT_Err_Ok;
  }


  static FT_Error  GDEF_Destroy( void*  ext,
                                 PFace  face )
  {
    TTO_GDEFHeader*  gdef = (TTO_GDEFHeader*)ext;


    /* by convention */

    if ( !gdef )
      return TT_Err_Ok;

    if ( gdef->loaded )
    {
      Free_LigCaretList( &gdef->LigCaretList, memory );
      Free_AttachList( &gdef->AttachList, memory );
      Free_ClassDefinition( &gdef->GlyphClassDef, memory );
      Free_ClassDefinition( &gdef->MarkAttachClassDef, memory );

      Free_NewGlyphClasses( gdef, memory );
    }

    return TT_Err_Ok;
  }


  EXPORT_FUNC
  FT_Error  TT_Init_GDEF_Extension( TT_Engine  engine )
  {
    PEngine_Instance  _engine = HANDLE_Engine( engine );


    if ( !_engine )
      return TT_Err_Invalid_Engine;

    return  TT_Register_Extension( _engine,
                                   GDEF_ID,
                                   sizeof ( TTO_GDEFHeader ),
                                   GDEF_Create,
                                   GDEF_Destroy );
  }
#endif

  EXPORT_FUNC
  FT_Error  TT_New_GDEF_Table( FT_Face          face,
			       TTO_GDEFHeader** retptr )
  {
    FT_Error         error;
    FT_Memory        memory = face->memory;

    TTO_GDEFHeader*  gdef;

    if ( !retptr )
      return TT_Err_Invalid_Argument;

    if ( ALLOC( gdef, sizeof( *gdef ) ) )
      return error;

    gdef->memory = face->memory;

    gdef->GlyphClassDef.loaded = FALSE;
    gdef->AttachList.loaded = FALSE;
    gdef->LigCaretList.loaded = FALSE;
    gdef->MarkAttachClassDef_offset = 0;
    gdef->MarkAttachClassDef.loaded = FALSE;

    gdef->LastGlyph = 0;
    gdef->NewGlyphClasses = NULL;

    *retptr = gdef;

    return TT_Err_Ok;
  }

  EXPORT_FUNC
  FT_Error  TT_Load_GDEF_Table( FT_Face          face,
                                TTO_GDEFHeader** retptr )
  {
    FT_Error         error;
    FT_Memory        memory = face->memory;
    FT_Stream        stream = face->stream;
    FT_ULong         cur_offset, new_offset, base_offset;

    TTO_GDEFHeader*  gdef;


    if ( !retptr )
      return TT_Err_Invalid_Argument;

    if (( error = ftglue_face_goto_table( face, TTAG_GDEF, stream ) ))
      return error;

    if (( error = TT_New_GDEF_Table ( face, &gdef ) ))
      return error;

    base_offset = FILE_Pos();

    /* skip version */

    if ( FILE_Seek( base_offset + 4L ) ||
         ACCESS_Frame( 2L ) )
      goto Fail0;

    new_offset = GET_UShort();

    FORGET_Frame();

    /* all GDEF subtables are optional */

    if ( new_offset )
    {
      new_offset += base_offset;

      /* only classes 1-4 are allowed here */

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_ClassDefinition( &gdef->GlyphClassDef, 5,
                                           stream ) ) != TT_Err_Ok )
        goto Fail0;
      (void)FILE_Seek( cur_offset );
    }

    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_AttachList( &gdef->AttachList,
                                      stream ) ) != TT_Err_Ok )
        goto Fail1;
      (void)FILE_Seek( cur_offset );
    }

    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
    {
      new_offset += base_offset;

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_LigCaretList( &gdef->LigCaretList,
                                        stream ) ) != TT_Err_Ok )
        goto Fail2;
      (void)FILE_Seek( cur_offset );
    }

    /* OpenType 1.2 has introduced the `MarkAttachClassDef' field.  We
       first have to scan the LookupFlag values to find out whether we
       must load it or not.  Here we only store the offset of the table. */

    if ( ACCESS_Frame( 2L ) )
      goto Fail3;

    new_offset = GET_UShort();

    FORGET_Frame();

    if ( new_offset )
      gdef->MarkAttachClassDef_offset = new_offset + base_offset;
    else
      gdef->MarkAttachClassDef_offset = 0;

    *retptr = gdef;

    return TT_Err_Ok;

  Fail3:
    Free_LigCaretList( &gdef->LigCaretList, memory );
    
  Fail2:
    Free_AttachList( &gdef->AttachList, memory );

  Fail1:
    Free_ClassDefinition( &gdef->GlyphClassDef, memory );

  Fail0:
    FREE( gdef );

    return error;
  }

  EXPORT_FUNC
  FT_Error  TT_Done_GDEF_Table ( TTO_GDEFHeader* gdef ) 
  {
    FT_Memory memory = gdef->memory;
    
    Free_LigCaretList( &gdef->LigCaretList, memory );
    Free_AttachList( &gdef->AttachList, memory );
    Free_ClassDefinition( &gdef->GlyphClassDef, memory );
    Free_ClassDefinition( &gdef->MarkAttachClassDef, memory );
    
    Free_NewGlyphClasses( gdef, memory );

    FREE( gdef );

    return TT_Err_Ok;
  }




  /*******************************
   * AttachList related functions
   *******************************/


  /* AttachPoint */

  static FT_Error  Load_AttachPoint( TTO_AttachPoint*  ap,
                                     FT_Stream         stream )
  {
    FT_Memory memory = stream->memory;
    FT_Error  error;

    FT_UShort   n, count;
    FT_UShort*  pi;


    if ( ACCESS_Frame( 2L ) )
      return error;

    count = ap->PointCount = GET_UShort();

    FORGET_Frame();

    ap->PointIndex = NULL;

    if ( count )
    {
      if ( ALLOC_ARRAY( ap->PointIndex, count, FT_UShort ) )
        return error;

      pi = ap->PointIndex;

      if ( ACCESS_Frame( count * 2L ) )
      {
        FREE( pi );
        return error;
      }

      for ( n = 0; n < count; n++ )
        pi[n] = GET_UShort();

      FORGET_Frame();
    }

    return TT_Err_Ok;
  }


  static void  Free_AttachPoint( TTO_AttachPoint*  ap,
				 FT_Memory        memory )
  {
    FREE( ap->PointIndex );
  }


  /* AttachList */

  static FT_Error  Load_AttachList( TTO_AttachList*  al,
                                    FT_Stream        stream )
  {
    FT_Memory memory = stream->memory;
    FT_Error  error;

    FT_UShort         n, m, count;
    FT_ULong          cur_offset, new_offset, base_offset;

    TTO_AttachPoint*  ap;


    base_offset = FILE_Pos();

    if ( ACCESS_Frame( 2L ) )
      return error;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
         ( error = Load_Coverage( &al->Coverage, stream ) ) != TT_Err_Ok )
      return error;
    (void)FILE_Seek( cur_offset );

    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    count = al->GlyphCount = GET_UShort();

    FORGET_Frame();

    al->AttachPoint = NULL;

    if ( ALLOC_ARRAY( al->AttachPoint, count, TTO_AttachPoint ) )
      goto Fail2;

    ap = al->AttachPoint;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
        goto Fail1;

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_AttachPoint( &ap[n], stream ) ) != TT_Err_Ok )
        goto Fail1;
      (void)FILE_Seek( cur_offset );
    }

    al->loaded = TRUE;

    return TT_Err_Ok;

  Fail1:
    for ( m = 0; m < n; m++ )
      Free_AttachPoint( &ap[m], memory );

    FREE( ap );

  Fail2:
    Free_Coverage( &al->Coverage, memory );
    return error;
  }


  static void  Free_AttachList( TTO_AttachList*  al,
				FT_Memory        memory )
  {
    FT_UShort         n, count;

    TTO_AttachPoint*  ap;


    if ( !al->loaded )
      return;

    if ( al->AttachPoint )
    {
      count = al->GlyphCount;
      ap    = al->AttachPoint;

      for ( n = 0; n < count; n++ )
        Free_AttachPoint( &ap[n], memory );

      FREE( ap );
    }

    Free_Coverage( &al->Coverage, memory );
  }



  /*********************************
   * LigCaretList related functions
   *********************************/


  /* CaretValueFormat1 */
  /* CaretValueFormat2 */
  /* CaretValueFormat3 */
  /* CaretValueFormat4 */

  static FT_Error  Load_CaretValue( TTO_CaretValue*  cv,
                                    FT_Stream        stream )
  {
    FT_Error  error;

    FT_ULong cur_offset, new_offset, base_offset;


    base_offset = FILE_Pos();

    if ( ACCESS_Frame( 2L ) )
      return error;

    cv->CaretValueFormat = GET_UShort();

    FORGET_Frame();

    switch ( cv->CaretValueFormat )
    {
    case 1:
      if ( ACCESS_Frame( 2L ) )
        return error;

      cv->cvf.cvf1.Coordinate = GET_Short();

      FORGET_Frame();

      break;

    case 2:
      if ( ACCESS_Frame( 2L ) )
        return error;

      cv->cvf.cvf2.CaretValuePoint = GET_UShort();

      FORGET_Frame();

      break;

    case 3:
      if ( ACCESS_Frame( 4L ) )
        return error;

      cv->cvf.cvf3.Coordinate = GET_Short();

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_Device( &cv->cvf.cvf3.Device,
                                  stream ) ) != TT_Err_Ok )
        return error;
      (void)FILE_Seek( cur_offset );

      break;

    case 4:
      if ( ACCESS_Frame( 2L ) )
        return error;

      cv->cvf.cvf4.IdCaretValue = GET_UShort();

      FORGET_Frame();
      break;

    default:
      return TTO_Err_Invalid_GDEF_SubTable_Format;
    }

    return TT_Err_Ok;
  }


  static void  Free_CaretValue( TTO_CaretValue*  cv,
				FT_Memory        memory )
  {
    if ( cv->CaretValueFormat == 3 )
      Free_Device( &cv->cvf.cvf3.Device, memory );
  }


  /* LigGlyph */

  static FT_Error  Load_LigGlyph( TTO_LigGlyph*  lg,
                                  FT_Stream      stream )
  {
    FT_Memory memory = stream->memory;
    FT_Error  error;

    FT_UShort        n, m, count;
    FT_ULong         cur_offset, new_offset, base_offset;

    TTO_CaretValue*  cv;


    base_offset = FILE_Pos();

    if ( ACCESS_Frame( 2L ) )
      return error;

    count = lg->CaretCount = GET_UShort();

    FORGET_Frame();

    lg->CaretValue = NULL;

    if ( ALLOC_ARRAY( lg->CaretValue, count, TTO_CaretValue ) )
      return error;

    cv = lg->CaretValue;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
        goto Fail;

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_CaretValue( &cv[n], stream ) ) != TT_Err_Ok )
        goto Fail;
      (void)FILE_Seek( cur_offset );
    }

    return TT_Err_Ok;

  Fail:
    for ( m = 0; m < n; m++ )
      Free_CaretValue( &cv[m], memory );

    FREE( cv );
    return error;
  }


  static void  Free_LigGlyph( TTO_LigGlyph*  lg,
			      FT_Memory      memory )
  {
    FT_UShort        n, count;

    TTO_CaretValue*  cv;


    if ( lg->CaretValue )
    {
      count = lg->CaretCount;
      cv    = lg->CaretValue;

      for ( n = 0; n < count; n++ )
        Free_CaretValue( &cv[n], memory );

      FREE( cv );
    }
  }


  /* LigCaretList */

  static FT_Error  Load_LigCaretList( TTO_LigCaretList*  lcl,
                                      FT_Stream          stream )
  {
    FT_Memory memory = stream->memory;
    FT_Error  error;

    FT_UShort      m, n, count;
    FT_ULong       cur_offset, new_offset, base_offset;

    TTO_LigGlyph*  lg;


    base_offset = FILE_Pos();

    if ( ACCESS_Frame( 2L ) )
      return error;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
         ( error = Load_Coverage( &lcl->Coverage, stream ) ) != TT_Err_Ok )
      return error;
    (void)FILE_Seek( cur_offset );

    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    count = lcl->LigGlyphCount = GET_UShort();

    FORGET_Frame();

    lcl->LigGlyph = NULL;

    if ( ALLOC_ARRAY( lcl->LigGlyph, count, TTO_LigGlyph ) )
      goto Fail2;

    lg = lcl->LigGlyph;

    for ( n = 0; n < count; n++ )
    {
      if ( ACCESS_Frame( 2L ) )
        goto Fail1;

      new_offset = GET_UShort() + base_offset;

      FORGET_Frame();

      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
           ( error = Load_LigGlyph( &lg[n], stream ) ) != TT_Err_Ok )
        goto Fail1;
      (void)FILE_Seek( cur_offset );
    }

    lcl->loaded = TRUE;

    return TT_Err_Ok;

  Fail1:
    for ( m = 0; m < n; m++ )
      Free_LigGlyph( &lg[m], memory );

    FREE( lg );

  Fail2:
    Free_Coverage( &lcl->Coverage, memory );
    return error;
  }


  static void  Free_LigCaretList( TTO_LigCaretList*  lcl,
				  FT_Memory           memory )
  {
    FT_UShort      n, count;

    TTO_LigGlyph*  lg;


    if ( !lcl->loaded )
      return;

    if ( lcl->LigGlyph )
    {
      count = lcl->LigGlyphCount;
      lg    = lcl->LigGlyph;

      for ( n = 0; n < count; n++ )
        Free_LigGlyph( &lg[n], memory );

      FREE( lg );
    }

    Free_Coverage( &lcl->Coverage, memory );
  }



  /***********
   * GDEF API
   ***********/


  static FT_UShort  Get_New_Class( TTO_GDEFHeader*  gdef,
				   FT_UShort        glyphID,
				   FT_UShort        index )
  {
    FT_UShort              glyph_index, array_index, count;
    FT_UShort              byte, bits;
    
    TTO_ClassRangeRecord*  gcrr;
    FT_UShort**            ngc;


    if ( glyphID >= gdef->LastGlyph )
      return 0;

    count = gdef->GlyphClassDef.cd.cd2.ClassRangeCount;
    gcrr = gdef->GlyphClassDef.cd.cd2.ClassRangeRecord;
    ngc  = gdef->NewGlyphClasses;

    if ( index < count && glyphID < gcrr[index].Start )
    {
      array_index = index;
      if ( index == 0 )
        glyph_index = glyphID;
      else
        glyph_index = glyphID - gcrr[index - 1].End - 1;
    }
    else
    {
      array_index = index + 1;
      glyph_index = glyphID - gcrr[index].End - 1;
    }

    byte = ngc[array_index][glyph_index / 4];
    bits = byte >> ( 16 - ( glyph_index % 4 + 1 ) * 4 );

    return bits & 0x000F;
  }


  EXPORT_FUNC
  FT_Error  TT_GDEF_Get_Glyph_Property( TTO_GDEFHeader*  gdef,
                                        FT_UShort        glyphID,
                                        FT_UShort*       property )
  {
    FT_UShort class, index;

    FT_Error  error;


    if ( !gdef || !property )
      return TT_Err_Invalid_Argument;

    /* first, we check for mark attach classes */

    if ( gdef->MarkAttachClassDef.loaded )
    {
      error = Get_Class( &gdef->MarkAttachClassDef, glyphID, &class, &index );
      if ( error && error != TTO_Err_Not_Covered )
        return error;
      if ( !error )
      {
        *property = class << 8;
        return TT_Err_Ok;
      }
    }

    error = Get_Class( &gdef->GlyphClassDef, glyphID, &class, &index );
    if ( error && error != TTO_Err_Not_Covered )
      return error;

    /* if we have a constructed class table, check whether additional
       values have been assigned                                      */

    if ( error == TTO_Err_Not_Covered && gdef->NewGlyphClasses )
      class = Get_New_Class( gdef, glyphID, index );

    switch ( class )
    {
    case UNCLASSIFIED_GLYPH:
      *property = 0;
      break;

    case SIMPLE_GLYPH:
      *property = TTO_BASE_GLYPH;
      break;

    case LIGATURE_GLYPH:
      *property = TTO_LIGATURE;
      break;

    case MARK_GLYPH:
      *property = TTO_MARK;
      break;

    case COMPONENT_GLYPH:
      *property = TTO_COMPONENT;
      break;
    }

    return TT_Err_Ok;
  }


  static FT_Error  Make_ClassRange( TTO_ClassDefinition*  cd,
                                    FT_UShort             start,
                                    FT_UShort             end,
                                    FT_UShort             class,
				    FT_Memory             memory )
  {
    FT_Error               error;
    FT_UShort              index;

    TTO_ClassDefFormat2*   cdf2;
    TTO_ClassRangeRecord*  crr;


    cdf2 = &cd->cd.cd2;

    if ( REALLOC_ARRAY( cdf2->ClassRangeRecord,
			cdf2->ClassRangeCount,
			cdf2->ClassRangeCount + 1 ,
                        TTO_ClassRangeRecord ) )
      return error;

    cdf2->ClassRangeCount++;

    crr   = cdf2->ClassRangeRecord;
    index = cdf2->ClassRangeCount - 1;

    crr[index].Start = start;
    crr[index].End   = end;
    crr[index].Class = class;

    cd->Defined[class] = TRUE;

    return TT_Err_Ok;
  }


  EXPORT_FUNC
  FT_Error  TT_GDEF_Build_ClassDefinition( TTO_GDEFHeader*  gdef,
                                           FT_UShort        num_glyphs,
                                           FT_UShort        glyph_count,
                                           FT_UShort*       glyph_array,
                                           FT_UShort*       class_array )
  {
    FT_UShort              start, curr_glyph, curr_class;
    FT_UShort              n, m, count;
    FT_Error               error;
    FT_Memory              memory = gdef->memory;

    TTO_ClassDefinition*   gcd;
    TTO_ClassRangeRecord*  gcrr;
    FT_UShort**            ngc;


    if ( !gdef || !glyph_array || !class_array )
      return TT_Err_Invalid_Argument;

    gcd = &gdef->GlyphClassDef;

    /* We build a format 2 table */

    gcd->ClassFormat = 2;

    /* A GlyphClassDef table contains at most 5 different class values */

    if ( ALLOC_ARRAY( gcd->Defined, 5, FT_Bool ) )
      return error;

    gcd->cd.cd2.ClassRangeCount  = 0;
    gcd->cd.cd2.ClassRangeRecord = NULL;

    start      = glyph_array[0];
    curr_class = class_array[0];
    curr_glyph = start;

    if ( curr_class >= 5 )
    {
      error = TT_Err_Invalid_Argument;
      goto Fail4;
    }

    glyph_count--;

    for ( n = 0; n <= glyph_count; n++ )
    {
      if ( curr_glyph == glyph_array[n] && curr_class == class_array[n] )
      {
        if ( n == glyph_count )
        {
          if ( ( error = Make_ClassRange( gcd, start,
                                          curr_glyph,
                                          curr_class,
					  memory ) ) != TT_Err_Ok )
            goto Fail3;
        }
        else
        {
          if ( curr_glyph == 0xFFFF )
          {
            error = TT_Err_Invalid_Argument;
            goto Fail3;
          }
          else
            curr_glyph++;
        }
      }
      else
      {
        if ( ( error = Make_ClassRange( gcd, start,
                                        curr_glyph - 1,
                                        curr_class,
					memory ) ) != TT_Err_Ok )
          goto Fail3;

        if ( curr_glyph > glyph_array[n] )
        {
          error = TT_Err_Invalid_Argument;
          goto Fail3;
        }

        start      = glyph_array[n];
        curr_class = class_array[n];
        curr_glyph = start;

        if ( curr_class >= 5 )
        {
          error = TT_Err_Invalid_Argument;
          goto Fail3;
        }

        if ( n == glyph_count )
        {
          if ( ( error = Make_ClassRange( gcd, start,
                                          curr_glyph,
                                          curr_class,
					  memory ) ) != TT_Err_Ok )
            goto Fail3;
        }
        else
        {
          if ( curr_glyph == 0xFFFF )
          {
            error = TT_Err_Invalid_Argument;
            goto Fail3;
          }
          else
            curr_glyph++;
        }
      }
    }

    /* now prepare the arrays for class values assigned during the lookup
       process                                                            */

    if ( ALLOC_ARRAY( gdef->NewGlyphClasses,
                      gcd->cd.cd2.ClassRangeCount + 1, FT_UShort* ) )
      goto Fail3;

    count = gcd->cd.cd2.ClassRangeCount;
    gcrr  = gcd->cd.cd2.ClassRangeRecord;
    ngc   = gdef->NewGlyphClasses;

    /* We allocate arrays for all glyphs not covered by the class range
       records.  Each element holds four class values.                  */

    if ( count > 0 )
    {
	if ( gcrr[0].Start )
	{
	  if ( ALLOC_ARRAY( ngc[0], ( gcrr[0].Start + 3 ) / 4, FT_UShort ) )
	    goto Fail2;
	}

	for ( n = 1; n < count; n++ )
	{
	  if ( gcrr[n].Start - gcrr[n - 1].End > 1 )
	    if ( ALLOC_ARRAY( ngc[n],
			      ( gcrr[n].Start - gcrr[n - 1].End + 2 ) / 4,
			      FT_UShort ) )
	      goto Fail1;
	}

	if ( gcrr[count - 1].End != num_glyphs - 1 )
	{
	  if ( ALLOC_ARRAY( ngc[count],
			    ( num_glyphs - gcrr[count - 1].End + 2 ) / 4,
			    FT_UShort ) )
	      goto Fail1;
	}
    }
    else if ( num_glyphs > 0 )
    {
	if ( ALLOC_ARRAY( ngc[count],
			  ( num_glyphs + 3 ) / 4,
			  FT_UShort ) )
	    goto Fail2;
    }
	
    gdef->LastGlyph = num_glyphs - 1;

    gdef->MarkAttachClassDef_offset = 0L;
    gdef->MarkAttachClassDef.loaded = FALSE;

    gcd->loaded = TRUE;

    return TT_Err_Ok;

  Fail1:
    for ( m = 0; m < n; m++ )
      FREE( ngc[m] );

  Fail2:
    FREE( gdef->NewGlyphClasses );

  Fail3:
    FREE( gcd->cd.cd2.ClassRangeRecord );

  Fail4:
    FREE( gcd->Defined );
    return error;
  }


  static void  Free_NewGlyphClasses( TTO_GDEFHeader*  gdef,
				     FT_Memory        memory )
  {
    FT_UShort**  ngc;
    FT_UShort    n, count;


    if ( gdef->NewGlyphClasses )
    {
      count = gdef->GlyphClassDef.cd.cd2.ClassRangeCount + 1;
      ngc   = gdef->NewGlyphClasses;

      for ( n = 0; n < count; n++ )
        FREE( ngc[n] );

      FREE( ngc );
    }
  }


  FT_Error  Add_Glyph_Property( TTO_GDEFHeader*  gdef,
                                FT_UShort        glyphID,
                                FT_UShort        property )
  {
    FT_Error               error;
    FT_UShort              class, new_class, index;
    FT_UShort              byte, bits, mask;
    FT_UShort              array_index, glyph_index, count;

    TTO_ClassRangeRecord*  gcrr;
    FT_UShort**            ngc;


    error = Get_Class( &gdef->GlyphClassDef, glyphID, &class, &index );
    if ( error && error != TTO_Err_Not_Covered )
      return error;

    /* we don't accept glyphs covered in `GlyphClassDef' */

    if ( !error )
      return TTO_Err_Not_Covered;

    switch ( property )
    {
    case 0:
      new_class = UNCLASSIFIED_GLYPH;
      break;

    case TTO_BASE_GLYPH:
      new_class = SIMPLE_GLYPH;
      break;

    case TTO_LIGATURE:
      new_class = LIGATURE_GLYPH;
      break;

    case TTO_MARK:
      new_class = MARK_GLYPH;
      break;

    case TTO_COMPONENT:
      new_class = COMPONENT_GLYPH;
      break;

    default:
      return TT_Err_Invalid_Argument;
    }

    count = gdef->GlyphClassDef.cd.cd2.ClassRangeCount;
    gcrr = gdef->GlyphClassDef.cd.cd2.ClassRangeRecord;
    ngc  = gdef->NewGlyphClasses;

    if ( index < count && glyphID < gcrr[index].Start )
    {
      array_index = index;
      if ( index == 0 )
        glyph_index = glyphID;
      else
        glyph_index = glyphID - gcrr[index - 1].End - 1;
    }
    else
    {
      array_index = index + 1;
      glyph_index = glyphID - gcrr[index].End - 1;
    }

    byte  = ngc[array_index][glyph_index / 4];
    bits  = byte >> ( 16 - ( glyph_index % 4 + 1 ) * 4 );
    class = bits & 0x000F;

    /* we don't overwrite existing entries */

    if ( !class )
    {
      bits = new_class << ( 16 - ( glyph_index % 4 + 1 ) * 4 );
      mask = ~( 0x000F << ( 16 - ( glyph_index % 4 + 1 ) * 4 ) );

      ngc[array_index][glyph_index / 4] &= mask;
      ngc[array_index][glyph_index / 4] |= bits;
    }

    return TT_Err_Ok;
  }


  FT_Error  Check_Property( TTO_GDEFHeader*  gdef,
			    OTL_GlyphItem    gitem,
                            FT_UShort        flags,
                            FT_UShort*       property )
  {
    FT_Error  error;

    if ( gdef )
    {
      FT_UShort basic_glyph_class;
      FT_UShort desired_attachment_class;

      if ( gitem->gproperties == OTL_GLYPH_PROPERTIES_UNKNOWN )
      {
	error = TT_GDEF_Get_Glyph_Property( gdef, gitem->gindex, &gitem->gproperties );
	if ( error )
	  return error;
      }

      *property = gitem->gproperties;

      /* If the glyph was found in the MarkAttachmentClass table,
       * then that class value is the high byte of the result,
       * otherwise the low byte contains the basic type of the glyph
       * as defined by the GlyphClassDef table.
       */
      if ( *property & IGNORE_SPECIAL_MARKS  )
	basic_glyph_class = TTO_MARK;
      else
	basic_glyph_class = *property;

      /* Return Not_Covered, if, for example, basic_glyph_class
       * is TTO_LIGATURE and LookFlags includes IGNORE_LIGATURES
       */
      if ( flags & basic_glyph_class )
	return TTO_Err_Not_Covered;
      
      /* The high byte of LookupFlags has the meaning
       * "ignore marks of attachment type different than
       * the attachment type specified."
       */
      desired_attachment_class = flags & IGNORE_SPECIAL_MARKS;
      if ( desired_attachment_class )
      {
	if ( basic_glyph_class == TTO_MARK &&
	     *property != desired_attachment_class )
	  return TTO_Err_Not_Covered;
      }
    }

    return TT_Err_Ok;
  }


/* END */
