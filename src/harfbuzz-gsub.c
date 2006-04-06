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
#include "harfbuzz-impl.h"
#include "harfbuzz-gsub-private.h"
#include "harfbuzz-open-private.h"
#include "harfbuzz-gdef-private.h"

static FT_Error  GSUB_Do_Glyph_Lookup( HB_GSUBHeader*   gsub,
				       FT_UShort         lookup_index,
				       HB_Buffer        buffer,
				       FT_UShort         context_length,
				       int               nesting_level );



/**********************
 * Auxiliary functions
 **********************/



FT_Error  HB_Load_GSUB_Table( FT_Face          face,
			      HB_GSUBHeader** retptr,
			      HB_GDEFHeader*  gdef )
{
  FT_Stream        stream = face->stream;
  FT_Memory        memory = face->memory;
  FT_Error         error;
  FT_ULong         cur_offset, new_offset, base_offset;

  FT_UShort        i, num_lookups;
  HB_GSUBHeader*  gsub;
  HB_Lookup*      lo;

  if ( !retptr )
    return FT_Err_Invalid_Argument;

  if (( error = _hb_ftglue_face_goto_table( face, TTAG_GSUB, stream ) ))
    return error;

  base_offset = FILE_Pos();

  if ( ALLOC ( gsub, sizeof( *gsub ) ) )
    return error;

  gsub->memory = memory;

  /* skip version */

  if ( FILE_Seek( base_offset + 4L ) ||
       ACCESS_Frame( 2L ) )
    goto Fail4;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_ScriptList( &gsub->ScriptList,
				  stream ) ) != FT_Err_Ok )
    goto Fail4;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_FeatureList( &gsub->FeatureList,
				   stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_LookupList( &gsub->LookupList,
				  stream, HB_Type_GSUB ) ) != FT_Err_Ok )
    goto Fail2;

  gsub->gdef = gdef;      /* can be NULL */

  /* We now check the LookupFlags for values larger than 0xFF to find
     out whether we need to load the `MarkAttachClassDef' field of the
     GDEF table -- this hack is necessary for OpenType 1.2 tables since
     the version field of the GDEF table hasn't been incremented.

     For constructed GDEF tables, we only load it if
     `MarkAttachClassDef_offset' is not zero (nevertheless, a build of
     a constructed mark attach table is not supported currently).       */

  if ( gdef &&
       gdef->MarkAttachClassDef_offset && !gdef->MarkAttachClassDef.loaded )
  {
    lo          = gsub->LookupList.Lookup;
    num_lookups = gsub->LookupList.LookupCount;

    for ( i = 0; i < num_lookups; i++ )
    {

      if ( lo[i].LookupFlag & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS )
      {
	if ( FILE_Seek( gdef->MarkAttachClassDef_offset ) ||
	     ( error = _HB_OPEN_Load_ClassDefinition( &gdef->MarkAttachClassDef,
					     256, stream ) ) != FT_Err_Ok )
	  goto Fail1;

	break;
      }
    }
  }

  *retptr = gsub;

  return FT_Err_Ok;

Fail1:
  _HB_OPEN_Free_LookupList( &gsub->LookupList, HB_Type_GSUB, memory );

Fail2:
  _HB_OPEN_Free_FeatureList( &gsub->FeatureList, memory );

Fail3:
  _HB_OPEN_Free_ScriptList( &gsub->ScriptList, memory );

Fail4:
  FREE ( gsub );


  return error;
}


FT_Error   HB_Done_GSUB_Table( HB_GSUBHeader* gsub )
{
  FT_Memory memory = gsub->memory;

  _HB_OPEN_Free_LookupList( &gsub->LookupList, HB_Type_GSUB, memory );
  _HB_OPEN_Free_FeatureList( &gsub->FeatureList, memory );
  _HB_OPEN_Free_ScriptList( &gsub->ScriptList, memory );

  FREE( gsub );

  return FT_Err_Ok;
}

/*****************************
 * SubTable related functions
 *****************************/

static FT_Error  Lookup_DefaultSubst( HB_GSUBHeader*    gsub,
				      HB_GSUB_SubTable* st,
				      HB_Buffer         buffer,
				      FT_UShort          flags,
				      FT_UShort          context_length,
				      int                nesting_level )
{
  FT_UNUSED(gsub);
  FT_UNUSED(st);
  FT_UNUSED(buffer);
  FT_UNUSED(flags);
  FT_UNUSED(context_length);
  FT_UNUSED(nesting_level);
  return HB_Err_Not_Covered;
}


/* LookupType 1 */

/* SingleSubstFormat1 */
/* SingleSubstFormat2 */

static FT_Error  Load_SingleSubst( HB_GSUB_SubTable* st,
				   FT_Stream         stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;
  HB_SingleSubst*  ss = &st->single;

  FT_UShort n, count;
  FT_ULong cur_offset, new_offset, base_offset;

  FT_UShort*  s;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  ss->SubstFormat = GET_UShort();
  new_offset      = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &ss->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  switch ( ss->SubstFormat )
  {
  case 1:
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    ss->ssf.ssf1.DeltaGlyphID = GET_UShort();

    FORGET_Frame();

    break;

  case 2:
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    count = ss->ssf.ssf2.GlyphCount = GET_UShort();

    FORGET_Frame();

    ss->ssf.ssf2.Substitute = NULL;

    if ( ALLOC_ARRAY( ss->ssf.ssf2.Substitute, count, FT_UShort ) )
      goto Fail2;

    s = ss->ssf.ssf2.Substitute;

    if ( ACCESS_Frame( count * 2L ) )
      goto Fail1;

    for ( n = 0; n < count; n++ )
      s[n] = GET_UShort();

    FORGET_Frame();

    break;

  default:
    return HB_Err_Invalid_GSUB_SubTable_Format;
  }

  return FT_Err_Ok;

Fail1:
  FREE( s );

Fail2:
  _HB_OPEN_Free_Coverage( &ss->Coverage, memory );
  return error;
}


static void  Free_SingleSubst( HB_GSUB_SubTable* st,
			       FT_Memory         memory )
{
  HB_SingleSubst*  ss = &st->single;

  switch ( ss->SubstFormat )
  {
  case 1:
    break;

  case 2:
    FREE( ss->ssf.ssf2.Substitute );
    break;
  }

  _HB_OPEN_Free_Coverage( &ss->Coverage, memory );
}


static FT_Error  Lookup_SingleSubst( HB_GSUBHeader*   gsub,
				     HB_GSUB_SubTable* st,
				     HB_Buffer        buffer,
				     FT_UShort         flags,
				     FT_UShort         context_length,
				     int               nesting_level )
{
  FT_UShort index, value, property;
  FT_Error  error;
  HB_SingleSubst*  ss = &st->single;
  HB_GDEFHeader*   gdef = gsub->gdef;

  FT_UNUSED(nesting_level);


  if ( context_length != 0xFFFF && context_length < 1 )
    return HB_Err_Not_Covered;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &ss->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  switch ( ss->SubstFormat )
  {
  case 1:
	  value = ( IN_CURGLYPH() + ss->ssf.ssf1.DeltaGlyphID ) & 0xFFFF;
    if ( ADD_Glyph( buffer, value, 0xFFFF, 0xFFFF ) )
      return error;
    break;

  case 2:
    if ( index >= ss->ssf.ssf2.GlyphCount )
      return HB_Err_Invalid_GSUB_SubTable;
    value = ss->ssf.ssf2.Substitute[index];
    if ( ADD_Glyph( buffer, value, 0xFFFF, 0xFFFF ) )
      return error;
    break;

  default:
    return HB_Err_Invalid_GSUB_SubTable;
  }

  if ( gdef && gdef->NewGlyphClasses )
  {
    /* we inherit the old glyph class to the substituted glyph */

    error = _HB_GDEF_Add_Glyph_Property( gdef, value, property );
    if ( error && error != HB_Err_Not_Covered )
      return error;
  }

  return FT_Err_Ok;
}


/* LookupType 2 */

/* Sequence */

static FT_Error  Load_Sequence( HB_Sequence*  s,
				FT_Stream      stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort n, count;
  FT_UShort*  sub;


  if ( ACCESS_Frame( 2L ) )
    return error;

  count = s->GlyphCount = GET_UShort();

  FORGET_Frame();

  s->Substitute = NULL;

  if ( count )
  {
    if ( ALLOC_ARRAY( s->Substitute, count, FT_UShort ) )
      return error;

    sub = s->Substitute;

    if ( ACCESS_Frame( count * 2L ) )
    {
      FREE( sub );
      return error;
    }

    for ( n = 0; n < count; n++ )
      sub[n] = GET_UShort();

    FORGET_Frame();
  }

  return FT_Err_Ok;
}


static void  Free_Sequence( HB_Sequence*  s,
			    FT_Memory      memory )
{
  FREE( s->Substitute );
}


/* MultipleSubstFormat1 */

static FT_Error  Load_MultipleSubst( HB_GSUB_SubTable* st,
				     FT_Stream         stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;
  HB_MultipleSubst*  ms = &st->multiple;

  FT_UShort      n = 0, m, count;
  FT_ULong       cur_offset, new_offset, base_offset;

  HB_Sequence*  s;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  ms->SubstFormat = GET_UShort();             /* should be 1 */
  new_offset      = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &ms->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = ms->SequenceCount = GET_UShort();

  FORGET_Frame();

  ms->Sequence = NULL;

  if ( ALLOC_ARRAY( ms->Sequence, count, HB_Sequence ) )
    goto Fail2;

  s = ms->Sequence;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_Sequence( &s[n], stream ) ) != FT_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_Sequence( &s[m], memory );

  FREE( s );

Fail2:
  _HB_OPEN_Free_Coverage( &ms->Coverage, memory );
  return error;
}


static void  Free_MultipleSubst( HB_GSUB_SubTable* st,
				 FT_Memory         memory )
{
  FT_UShort      n, count;
  HB_MultipleSubst*  ms = &st->multiple;

  HB_Sequence*  s;


  if ( ms->Sequence )
  {
    count = ms->SequenceCount;
    s     = ms->Sequence;

    for ( n = 0; n < count; n++ )
      Free_Sequence( &s[n], memory );

    FREE( s );
  }

  _HB_OPEN_Free_Coverage( &ms->Coverage, memory );
}


static FT_Error  Lookup_MultipleSubst( HB_GSUBHeader*    gsub,
				       HB_GSUB_SubTable* st,
				       HB_Buffer         buffer,
				       FT_UShort          flags,
				       FT_UShort          context_length,
				       int                nesting_level )
{
  FT_Error  error;
  FT_UShort index, property, n, count;
  FT_UShort*s;
  HB_MultipleSubst*  ms = &st->multiple;
  HB_GDEFHeader*     gdef = gsub->gdef;

  FT_UNUSED(nesting_level);

  if ( context_length != 0xFFFF && context_length < 1 )
    return HB_Err_Not_Covered;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &ms->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  if ( index >= ms->SequenceCount )
    return HB_Err_Invalid_GSUB_SubTable;

  count = ms->Sequence[index].GlyphCount;
  s     = ms->Sequence[index].Substitute;

  if ( ADD_String( buffer, 1, count, s, 0xFFFF, 0xFFFF ) )
    return error;

  if ( gdef && gdef->NewGlyphClasses )
  {
    /* this is a guess only ... */

    if ( property == HB_GDEF_LIGATURE )
      property = HB_GDEF_BASE_GLYPH;

    for ( n = 0; n < count; n++ )
    {
      error = _HB_GDEF_Add_Glyph_Property( gdef, s[n], property );
      if ( error && error != HB_Err_Not_Covered )
	return error;
    }
  }

  return FT_Err_Ok;
}


/* LookupType 3 */

/* AlternateSet */

static FT_Error  Load_AlternateSet( HB_AlternateSet*  as,
				    FT_Stream          stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort n, count;
  FT_UShort*  a;


  if ( ACCESS_Frame( 2L ) )
    return error;

  count = as->GlyphCount = GET_UShort();

  FORGET_Frame();

  as->Alternate = NULL;

  if ( ALLOC_ARRAY( as->Alternate, count, FT_UShort ) )
    return error;

  a = as->Alternate;

  if ( ACCESS_Frame( count * 2L ) )
  {
    FREE( a );
    return error;
  }

  for ( n = 0; n < count; n++ )
    a[n] = GET_UShort();

  FORGET_Frame();

  return FT_Err_Ok;
}


static void  Free_AlternateSet( HB_AlternateSet*  as,
				FT_Memory          memory )
{
  FREE( as->Alternate );
}


/* AlternateSubstFormat1 */

static FT_Error  Load_AlternateSubst( HB_GSUB_SubTable* st,
				      FT_Stream         stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;
  HB_AlternateSubst* as = &st->alternate;

  FT_UShort          n = 0, m, count;
  FT_ULong           cur_offset, new_offset, base_offset;

  HB_AlternateSet*  aset;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  as->SubstFormat = GET_UShort();             /* should be 1 */
  new_offset      = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &as->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = as->AlternateSetCount = GET_UShort();

  FORGET_Frame();

  as->AlternateSet = NULL;

  if ( ALLOC_ARRAY( as->AlternateSet, count, HB_AlternateSet ) )
    goto Fail2;

  aset = as->AlternateSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_AlternateSet( &aset[n], stream ) ) != FT_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_AlternateSet( &aset[m], memory );

  FREE( aset );

Fail2:
  _HB_OPEN_Free_Coverage( &as->Coverage, memory );
  return error;
}


static void  Free_AlternateSubst( HB_GSUB_SubTable* st,
				  FT_Memory         memory )
{
  FT_UShort          n, count;
  HB_AlternateSubst* as = &st->alternate;

  HB_AlternateSet*  aset;


  if ( as->AlternateSet )
  {
    count = as->AlternateSetCount;
    aset  = as->AlternateSet;

    for ( n = 0; n < count; n++ )
      Free_AlternateSet( &aset[n], memory );

    FREE( aset );
  }

  _HB_OPEN_Free_Coverage( &as->Coverage, memory );
}


static FT_Error  Lookup_AlternateSubst( HB_GSUBHeader*    gsub,
					HB_GSUB_SubTable* st,
					HB_Buffer         buffer,
					FT_UShort          flags,
					FT_UShort          context_length,
					int                nesting_level )
{
  FT_Error          error;
  FT_UShort         index, alt_index, property;
  HB_AlternateSubst* as = &st->alternate;
  HB_GDEFHeader*     gdef = gsub->gdef;
  HB_AlternateSet  aset;

  FT_UNUSED(nesting_level);

  if ( context_length != 0xFFFF && context_length < 1 )
    return HB_Err_Not_Covered;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &as->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  aset = as->AlternateSet[index];

  /* we use a user-defined callback function to get the alternate index */

  if ( gsub->altfunc )
    alt_index = (gsub->altfunc)( buffer->out_pos, IN_CURGLYPH(),
				 aset.GlyphCount, aset.Alternate,
				 gsub->data );
  else
    alt_index = 0;

  if ( ADD_Glyph( buffer, aset.Alternate[alt_index],
		  0xFFFF, 0xFFFF ) )
    return error;

  if ( gdef && gdef->NewGlyphClasses )
  {
    /* we inherit the old glyph class to the substituted glyph */

    error = _HB_GDEF_Add_Glyph_Property( gdef, aset.Alternate[alt_index],
				property );
    if ( error && error != HB_Err_Not_Covered )
      return error;
  }

  return FT_Err_Ok;
}


/* LookupType 4 */

/* Ligature */

static FT_Error  Load_Ligature( HB_Ligature*  l,
				FT_Stream      stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort n, count;
  FT_UShort*  c;


  if ( ACCESS_Frame( 4L ) )
    return error;

  l->LigGlyph       = GET_UShort();
  l->ComponentCount = GET_UShort();

  FORGET_Frame();

  l->Component = NULL;

  count = l->ComponentCount - 1;      /* only ComponentCount - 1 elements */

  if ( ALLOC_ARRAY( l->Component, count, FT_UShort ) )
    return error;

  c = l->Component;

  if ( ACCESS_Frame( count * 2L ) )
  {
    FREE( c );
    return error;
  }

  for ( n = 0; n < count; n++ )
    c[n] = GET_UShort();

  FORGET_Frame();

  return FT_Err_Ok;
}


static void  Free_Ligature( HB_Ligature*  l,
			    FT_Memory      memory )
{
  FREE( l->Component );
}


/* LigatureSet */

static FT_Error  Load_LigatureSet( HB_LigatureSet*  ls,
				   FT_Stream         stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort      n = 0, m, count;
  FT_ULong       cur_offset, new_offset, base_offset;

  HB_Ligature*  l;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = ls->LigatureCount = GET_UShort();

  FORGET_Frame();

  ls->Ligature = NULL;

  if ( ALLOC_ARRAY( ls->Ligature, count, HB_Ligature ) )
    return error;

  l = ls->Ligature;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_Ligature( &l[n], stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_Ligature( &l[m], memory );

  FREE( l );
  return error;
}


static void  Free_LigatureSet( HB_LigatureSet*  ls,
			       FT_Memory         memory )
{
  FT_UShort      n, count;

  HB_Ligature*  l;


  if ( ls->Ligature )
  {
    count = ls->LigatureCount;
    l     = ls->Ligature;

    for ( n = 0; n < count; n++ )
      Free_Ligature( &l[n], memory );

    FREE( l );
  }
}


/* LigatureSubstFormat1 */

static FT_Error  Load_LigatureSubst( HB_GSUB_SubTable* st,
				     FT_Stream         stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;
  HB_LigatureSubst*  ls = &st->ligature;

  FT_UShort         n = 0, m, count;
  FT_ULong          cur_offset, new_offset, base_offset;

  HB_LigatureSet*  lset;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 4L ) )
    return error;

  ls->SubstFormat = GET_UShort();             /* should be 1 */
  new_offset      = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &ls->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = ls->LigatureSetCount = GET_UShort();

  FORGET_Frame();

  ls->LigatureSet = NULL;

  if ( ALLOC_ARRAY( ls->LigatureSet, count, HB_LigatureSet ) )
    goto Fail2;

  lset = ls->LigatureSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_LigatureSet( &lset[n], stream ) ) != FT_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_LigatureSet( &lset[m], memory );

  FREE( lset );

Fail2:
  _HB_OPEN_Free_Coverage( &ls->Coverage, memory );
  return error;
}


static void  Free_LigatureSubst( HB_GSUB_SubTable* st,
				 FT_Memory         memory )
{
  FT_UShort         n, count;
  HB_LigatureSubst*  ls = &st->ligature;

  HB_LigatureSet*  lset;


  if ( ls->LigatureSet )
  {
    count = ls->LigatureSetCount;
    lset  = ls->LigatureSet;

    for ( n = 0; n < count; n++ )
      Free_LigatureSet( &lset[n], memory );

    FREE( lset );
  }

  _HB_OPEN_Free_Coverage( &ls->Coverage, memory );
}


static FT_Error  Lookup_LigatureSubst( HB_GSUBHeader*    gsub,
				       HB_GSUB_SubTable* st,
				       HB_Buffer         buffer,
				       FT_UShort          flags,
				       FT_UShort          context_length,
				       int                nesting_level )
{
  FT_UShort      index, property;
  FT_Error       error;
  FT_UShort      numlig, i, j, is_mark, first_is_mark = FALSE;
  FT_UShort*     c;
  HB_LigatureSubst*  ls = &st->ligature;
  HB_GDEFHeader*     gdef = gsub->gdef;

  HB_Ligature*  lig;

  FT_UNUSED(nesting_level);

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  if ( property == HB_GDEF_MARK || property & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS )
    first_is_mark = TRUE;

  error = _HB_OPEN_Coverage_Index( &ls->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  if ( index >= ls->LigatureSetCount )
     return HB_Err_Invalid_GSUB_SubTable;

  lig = ls->LigatureSet[index].Ligature;

  for ( numlig = ls->LigatureSet[index].LigatureCount;
	numlig;
	numlig--, lig++ )
  {
    if ( buffer->in_pos + lig->ComponentCount > buffer->in_length )
      goto next_ligature;               /* Not enough glyphs in input */

    c    = lig->Component;

    is_mark = first_is_mark;

    if ( context_length != 0xFFFF && context_length < lig->ComponentCount )
      break;

    for ( i = 1, j = buffer->in_pos + 1; i < lig->ComponentCount; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + lig->ComponentCount - i == (FT_Long)buffer->in_length )
	  goto next_ligature;
	j++;
      }

      if ( !( property == HB_GDEF_MARK || property & HB_LOOKUP_FLAG_IGNORE_SPECIAL_MARKS ) )
	is_mark = FALSE;

      if ( IN_GLYPH( j ) != c[i - 1] )
	goto next_ligature;
    }

    if ( gdef && gdef->NewGlyphClasses )
    {
      /* this is just a guess ... */

      error = _HB_GDEF_Add_Glyph_Property( gdef, lig->LigGlyph,
				  is_mark ? HB_GDEF_MARK : HB_GDEF_LIGATURE );
      if ( error && error != HB_Err_Not_Covered )
	return error;
    }

    if ( j == buffer->in_pos + i ) /* No input glyphs skipped */
    {
      /* We don't use a new ligature ID if there are no skipped
	 glyphs and the ligature already has an ID.             */

      if ( IN_LIGID( buffer->in_pos ) )
      {
	if ( ADD_String( buffer, i, 1, &lig->LigGlyph,
			0xFFFF, 0xFFFF ) )
	  return error;
      }
      else
      {
	FT_UShort ligID = hb_buffer_allocate_ligid( buffer );
	if ( ADD_String( buffer, i, 1, &lig->LigGlyph,
			0xFFFF, ligID ) )
	  return error;
      }
    }
    else
    {
      FT_UShort ligID = hb_buffer_allocate_ligid( buffer );
      if ( ADD_Glyph( buffer, lig->LigGlyph,
		      0xFFFF, ligID ) )
	return error;

      /* Now we must do a second loop to copy the skipped glyphs to
	 `out' and assign component values to it.  We start with the
	 glyph after the first component.  Glyphs between component
	 i and i+1 belong to component i.  Together with the ligID
	 value it is later possible to check whether a specific
	 component value really belongs to a given ligature.         */

      for ( i = 0; i < lig->ComponentCount - 1; i++ )
      {
	while ( CHECK_Property( gdef, IN_CURITEM(),
				flags, &property ) )
	  if ( ADD_Glyph( buffer, IN_CURGLYPH(),
			  i, ligID ) )
	    return error;

	(buffer->in_pos)++;
      }
    }

    return FT_Err_Ok;

  next_ligature:
    ;
  }

  return HB_Err_Not_Covered;
}


/* Do the actual substitution for a context substitution (either format
   5 or 6).  This is only called after we've determined that the input
   matches the subrule.                                                 */

static FT_Error  Do_ContextSubst( HB_GSUBHeader*        gsub,
				  FT_UShort              GlyphCount,
				  FT_UShort              SubstCount,
				  HB_SubstLookupRecord* subst,
				  HB_Buffer             buffer,
				  int                    nesting_level )
{
  FT_Error  error;
  FT_UShort i, old_pos;


  i = 0;

  while ( i < GlyphCount )
  {
    if ( SubstCount && i == subst->SequenceIndex )
    {
      old_pos = buffer->in_pos;

      /* Do a substitution */

      error = GSUB_Do_Glyph_Lookup( gsub, subst->LookupListIndex, buffer,
				    GlyphCount, nesting_level );

      subst++;
      SubstCount--;
      i += buffer->in_pos - old_pos;

      if ( error == HB_Err_Not_Covered )
      {
	/* XXX "can't happen" -- but don't count on it */

	if ( ADD_Glyph( buffer, IN_CURGLYPH(),
			0xFFFF, 0xFFFF ) )
	  return error;
	i++;
      }
      else if ( error )
	return error;
    }
    else
    {
      /* No substitution for this index */

      if ( ADD_Glyph( buffer, IN_CURGLYPH(),
		      0xFFFF, 0xFFFF ) )
	return error;
      i++;
    }
  }

  return FT_Err_Ok;
}


/* LookupType 5 */

/* SubRule */

static FT_Error  Load_SubRule( HB_SubRule*  sr,
			       FT_Stream     stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort               n, count;
  FT_UShort*              i;

  HB_SubstLookupRecord*  slr;


  if ( ACCESS_Frame( 4L ) )
    return error;

  sr->GlyphCount = GET_UShort();
  sr->SubstCount = GET_UShort();

  FORGET_Frame();

  sr->Input = NULL;

  count = sr->GlyphCount - 1;         /* only GlyphCount - 1 elements */

  if ( ALLOC_ARRAY( sr->Input, count, FT_UShort ) )
    return error;

  i = sr->Input;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
    i[n] = GET_UShort();

  FORGET_Frame();

  sr->SubstLookupRecord = NULL;

  count = sr->SubstCount;

  if ( ALLOC_ARRAY( sr->SubstLookupRecord, count, HB_SubstLookupRecord ) )
    goto Fail2;

  slr = sr->SubstLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    slr[n].SequenceIndex   = GET_UShort();
    slr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( slr );

Fail2:
  FREE( i );
  return error;
}


static void  Free_SubRule( HB_SubRule*  sr,
			   FT_Memory     memory )
{
  FREE( sr->SubstLookupRecord );
  FREE( sr->Input );
}


/* SubRuleSet */

static FT_Error  Load_SubRuleSet( HB_SubRuleSet*  srs,
				  FT_Stream        stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort     n = 0, m, count;
  FT_ULong      cur_offset, new_offset, base_offset;

  HB_SubRule*  sr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = srs->SubRuleCount = GET_UShort();

  FORGET_Frame();

  srs->SubRule = NULL;

  if ( ALLOC_ARRAY( srs->SubRule, count, HB_SubRule ) )
    return error;

  sr = srs->SubRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_SubRule( &sr[n], stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_SubRule( &sr[m], memory );

  FREE( sr );
  return error;
}


static void  Free_SubRuleSet( HB_SubRuleSet*  srs,
			      FT_Memory        memory )
{
  FT_UShort     n, count;

  HB_SubRule*  sr;


  if ( srs->SubRule )
  {
    count = srs->SubRuleCount;
    sr    = srs->SubRule;

    for ( n = 0; n < count; n++ )
      Free_SubRule( &sr[n], memory );

    FREE( sr );
  }
}


/* ContextSubstFormat1 */

static FT_Error  Load_ContextSubst1( HB_ContextSubstFormat1*  csf1,
				     FT_Stream                 stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort        n = 0, m, count;
  FT_ULong         cur_offset, new_offset, base_offset;

  HB_SubRuleSet*  srs;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &csf1->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = csf1->SubRuleSetCount = GET_UShort();

  FORGET_Frame();

  csf1->SubRuleSet = NULL;

  if ( ALLOC_ARRAY( csf1->SubRuleSet, count, HB_SubRuleSet ) )
    goto Fail2;

  srs = csf1->SubRuleSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_SubRuleSet( &srs[n], stream ) ) != FT_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_SubRuleSet( &srs[m], memory );

  FREE( srs );

Fail2:
  _HB_OPEN_Free_Coverage( &csf1->Coverage, memory );
  return error;
}


static void  Free_ContextSubst1( HB_ContextSubstFormat1* csf1,
			    FT_Memory                memory )
{
  FT_UShort        n, count;

  HB_SubRuleSet*  srs;


  if ( csf1->SubRuleSet )
  {
    count = csf1->SubRuleSetCount;
    srs   = csf1->SubRuleSet;

    for ( n = 0; n < count; n++ )
      Free_SubRuleSet( &srs[n], memory );

    FREE( srs );
  }

  _HB_OPEN_Free_Coverage( &csf1->Coverage, memory );
}


/* SubClassRule */

static FT_Error  Load_SubClassRule( HB_ContextSubstFormat2*  csf2,
				    HB_SubClassRule*         scr,
				    FT_Stream                 stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort               n, count;

  FT_UShort*              c;
  HB_SubstLookupRecord*  slr;
  FT_Bool*                d;


  if ( ACCESS_Frame( 4L ) )
    return error;

  scr->GlyphCount = GET_UShort();
  scr->SubstCount = GET_UShort();

  if ( scr->GlyphCount > csf2->MaxContextLength )
    csf2->MaxContextLength = scr->GlyphCount;

  FORGET_Frame();

  scr->Class = NULL;

  count = scr->GlyphCount - 1;        /* only GlyphCount - 1 elements */

  if ( ALLOC_ARRAY( scr->Class, count, FT_UShort ) )
    return error;

  c = scr->Class;
  d = csf2->ClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
  {
    c[n] = GET_UShort();

    /* We check whether the specific class is used at all.  If not,
       class 0 is used instead.                                     */
    if ( !d[c[n]] )
      c[n] = 0;
  }

  FORGET_Frame();

  scr->SubstLookupRecord = NULL;

  count = scr->SubstCount;

  if ( ALLOC_ARRAY( scr->SubstLookupRecord, count, HB_SubstLookupRecord ) )
    goto Fail2;

  slr = scr->SubstLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    slr[n].SequenceIndex   = GET_UShort();
    slr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( slr );

Fail2:
  FREE( c );
  return error;
}


static void  Free_SubClassRule( HB_SubClassRule*  scr,
				FT_Memory          memory )
{
  FREE( scr->SubstLookupRecord );
  FREE( scr->Class );
}


/* SubClassSet */

static FT_Error  Load_SubClassSet( HB_ContextSubstFormat2*  csf2,
				   HB_SubClassSet*          scs,
				   FT_Stream                 stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort          n = 0, m, count;
  FT_ULong           cur_offset, new_offset, base_offset;

  HB_SubClassRule*  scr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = scs->SubClassRuleCount = GET_UShort();

  FORGET_Frame();

  scs->SubClassRule = NULL;

  if ( ALLOC_ARRAY( scs->SubClassRule, count, HB_SubClassRule ) )
    return error;

  scr = scs->SubClassRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_SubClassRule( csf2, &scr[n],
				      stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_SubClassRule( &scr[m], memory );

  FREE( scr );
  return error;
}


static void  Free_SubClassSet( HB_SubClassSet*  scs,
			       FT_Memory         memory )
{
  FT_UShort          n, count;

  HB_SubClassRule*  scr;


  if ( scs->SubClassRule )
  {
    count = scs->SubClassRuleCount;
    scr   = scs->SubClassRule;

    for ( n = 0; n < count; n++ )
      Free_SubClassRule( &scr[n], memory );

    FREE( scr );
  }
}


/* ContextSubstFormat2 */

static FT_Error  Load_ContextSubst2( HB_ContextSubstFormat2*  csf2,
				     FT_Stream                 stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort         n = 0, m, count;
  FT_ULong          cur_offset, new_offset, base_offset;

  HB_SubClassSet*  scs;


  base_offset = FILE_Pos() - 2;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &csf2->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 4L ) )
    goto Fail3;

  new_offset = GET_UShort() + base_offset;

  /* `SubClassSetCount' is the upper limit for class values, thus we
     read it now to make an additional safety check.                 */

  count = csf2->SubClassSetCount = GET_UShort();

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_ClassDefinition( &csf2->ClassDef, count,
				       stream ) ) != FT_Err_Ok )
    goto Fail3;
  (void)FILE_Seek( cur_offset );

  csf2->SubClassSet      = NULL;
  csf2->MaxContextLength = 0;

  if ( ALLOC_ARRAY( csf2->SubClassSet, count, HB_SubClassSet ) )
    goto Fail2;

  scs = csf2->SubClassSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    if ( new_offset != base_offset )      /* not a NULL offset */
    {
      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_SubClassSet( csf2, &scs[n],
				       stream ) ) != FT_Err_Ok )
	goto Fail1;
      (void)FILE_Seek( cur_offset );
    }
    else
    {
      /* we create a SubClassSet table with no entries */

      csf2->SubClassSet[n].SubClassRuleCount = 0;
      csf2->SubClassSet[n].SubClassRule      = NULL;
    }
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_SubClassSet( &scs[m], memory );

  FREE( scs );

Fail2:
  _HB_OPEN_Free_ClassDefinition( &csf2->ClassDef, memory );

Fail3:
  _HB_OPEN_Free_Coverage( &csf2->Coverage, memory );
  return error;
}


static void  Free_ContextSubst2( HB_ContextSubstFormat2*  csf2,
			    FT_Memory                 memory )
{
  FT_UShort         n, count;

  HB_SubClassSet*  scs;


  if ( csf2->SubClassSet )
  {
    count = csf2->SubClassSetCount;
    scs   = csf2->SubClassSet;

    for ( n = 0; n < count; n++ )
      Free_SubClassSet( &scs[n], memory );

    FREE( scs );
  }

  _HB_OPEN_Free_ClassDefinition( &csf2->ClassDef, memory );
  _HB_OPEN_Free_Coverage( &csf2->Coverage, memory );
}


/* ContextSubstFormat3 */

static FT_Error  Load_ContextSubst3( HB_ContextSubstFormat3*  csf3,
				     FT_Stream                 stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort               n = 0, m, count;
  FT_ULong                cur_offset, new_offset, base_offset;

  HB_Coverage*           c;
  HB_SubstLookupRecord*  slr;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 4L ) )
    return error;

  csf3->GlyphCount = GET_UShort();
  csf3->SubstCount = GET_UShort();

  FORGET_Frame();

  csf3->Coverage = NULL;

  count = csf3->GlyphCount;

  if ( ALLOC_ARRAY( csf3->Coverage, count, HB_Coverage ) )
    return error;

  c = csf3->Coverage;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &c[n], stream ) ) != FT_Err_Ok )
      goto Fail2;
    (void)FILE_Seek( cur_offset );
  }

  csf3->SubstLookupRecord = NULL;

  count = csf3->SubstCount;

  if ( ALLOC_ARRAY( csf3->SubstLookupRecord, count,
		    HB_SubstLookupRecord ) )
    goto Fail2;

  slr = csf3->SubstLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    slr[n].SequenceIndex   = GET_UShort();
    slr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( slr );

Fail2:
  for ( m = 0; m < n; m++ )
    _HB_OPEN_Free_Coverage( &c[m], memory );

  FREE( c );
  return error;
}


static void  Free_ContextSubst3( HB_ContextSubstFormat3*  csf3,
			    FT_Memory                 memory )
{
  FT_UShort      n, count;

  HB_Coverage*  c;


  FREE( csf3->SubstLookupRecord );

  if ( csf3->Coverage )
  {
    count = csf3->GlyphCount;
    c     = csf3->Coverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }
}


/* ContextSubst */

static FT_Error  Load_ContextSubst( HB_GSUB_SubTable* st,
				    FT_Stream         stream )
{
  FT_Error error;
  HB_ContextSubst*  cs = &st->context;


  if ( ACCESS_Frame( 2L ) )
    return error;

  cs->SubstFormat = GET_UShort();

  FORGET_Frame();

  switch ( cs->SubstFormat )
  {
  case 1:
    return Load_ContextSubst1( &cs->csf.csf1, stream );

  case 2:
    return Load_ContextSubst2( &cs->csf.csf2, stream );

  case 3:
    return Load_ContextSubst3( &cs->csf.csf3, stream );

  default:
    return HB_Err_Invalid_GSUB_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}


static void  Free_ContextSubst( HB_GSUB_SubTable* st,
			        FT_Memory         memory )
{
  HB_ContextSubst*  cs = &st->context;

  switch ( cs->SubstFormat )
  {
  case 1:
    Free_ContextSubst1( &cs->csf.csf1, memory );
    break;

  case 2:
    Free_ContextSubst2( &cs->csf.csf2, memory );
    break;

  case 3:
    Free_ContextSubst3( &cs->csf.csf3, memory );
    break;
  }
}


static FT_Error  Lookup_ContextSubst1( HB_GSUBHeader*          gsub,
				       HB_ContextSubstFormat1* csf1,
				       HB_Buffer               buffer,
				       FT_UShort                flags,
				       FT_UShort                context_length,
				       int                      nesting_level )
{
  FT_UShort        index, property;
  FT_UShort        i, j, k, numsr;
  FT_Error         error;

  HB_SubRule*     sr;
  HB_GDEFHeader*  gdef;


  gdef = gsub->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &csf1->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  sr    = csf1->SubRuleSet[index].SubRule;
  numsr = csf1->SubRuleSet[index].SubRuleCount;

  for ( k = 0; k < numsr; k++ )
  {
    if ( context_length != 0xFFFF && context_length < sr[k].GlyphCount )
      goto next_subrule;

    if ( buffer->in_pos + sr[k].GlyphCount > buffer->in_length )
      goto next_subrule;                        /* context is too long */

    for ( i = 1, j = buffer->in_pos + 1; i < sr[k].GlyphCount; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + sr[k].GlyphCount - i == (FT_Long)buffer->in_length )
	  goto next_subrule;
	j++;
      }

      if ( IN_GLYPH( j ) != sr[k].Input[i - 1] )
	goto next_subrule;
    }

    return Do_ContextSubst( gsub, sr[k].GlyphCount,
			    sr[k].SubstCount, sr[k].SubstLookupRecord,
			    buffer,
			    nesting_level );
  next_subrule:
    ;
  }

  return HB_Err_Not_Covered;
}


static FT_Error  Lookup_ContextSubst2( HB_GSUBHeader*          gsub,
				       HB_ContextSubstFormat2* csf2,
				       HB_Buffer               buffer,
				       FT_UShort                flags,
				       FT_UShort                context_length,
				       int                      nesting_level )
{
  FT_UShort          index, property;
  FT_Error           error;
  FT_Memory          memory = gsub->memory;
  FT_UShort          i, j, k, known_classes;

  FT_UShort*         classes;
  FT_UShort*         cl;

  HB_SubClassSet*   scs;
  HB_SubClassRule*  sr;
  HB_GDEFHeader*    gdef;


  gdef = gsub->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  /* Note: The coverage table in format 2 doesn't give an index into
	   anything.  It just lets us know whether or not we need to
	   do any lookup at all.                                     */

  error = _HB_OPEN_Coverage_Index( &csf2->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  if ( ALLOC_ARRAY( classes, csf2->MaxContextLength, FT_UShort ) )
    return error;

  error = _HB_OPEN_Get_Class( &csf2->ClassDef, IN_CURGLYPH(),
		     &classes[0], NULL );
  if ( error && error != HB_Err_Not_Covered )
    goto End;
  known_classes = 0;

  scs = &csf2->SubClassSet[classes[0]];
  if ( !scs )
  {
    error = HB_Err_Invalid_GSUB_SubTable;
    goto End;
  }

  for ( k = 0; k < scs->SubClassRuleCount; k++ )
  {
    sr  = &scs->SubClassRule[k];

    if ( context_length != 0xFFFF && context_length < sr->GlyphCount )
      goto next_subclassrule;

    if ( buffer->in_pos + sr->GlyphCount > buffer->in_length )
      goto next_subclassrule;                      /* context is too long */

    cl   = sr->Class;

    /* Start at 1 because [0] is implied */

    for ( i = 1, j = buffer->in_pos + 1; i < sr->GlyphCount; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  goto End;

	if ( j + sr->GlyphCount - i < (FT_Long)buffer->in_length )
	  goto next_subclassrule;
	j++;
      }

      if ( i > known_classes )
      {
	/* Keeps us from having to do this for each rule */

	error = _HB_OPEN_Get_Class( &csf2->ClassDef, IN_GLYPH( j ), &classes[i], NULL );
	if ( error && error != HB_Err_Not_Covered )
	  goto End;
	known_classes = i;
      }

      if ( cl[i - 1] != classes[i] )
	goto next_subclassrule;
    }

    error = Do_ContextSubst( gsub, sr->GlyphCount,
			     sr->SubstCount, sr->SubstLookupRecord,
			     buffer,
			     nesting_level );
    goto End;

  next_subclassrule:
    ;
  }

  error = HB_Err_Not_Covered;

End:
  FREE( classes );
  return error;
}


static FT_Error  Lookup_ContextSubst3( HB_GSUBHeader*          gsub,
				       HB_ContextSubstFormat3* csf3,
				       HB_Buffer               buffer,
				       FT_UShort                flags,
				       FT_UShort                context_length,
				       int                      nesting_level )
{
  FT_Error         error;
  FT_UShort        index, i, j, property;

  HB_Coverage*    c;
  HB_GDEFHeader*  gdef;


  gdef = gsub->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  if ( context_length != 0xFFFF && context_length < csf3->GlyphCount )
    return HB_Err_Not_Covered;

  if ( buffer->in_pos + csf3->GlyphCount > buffer->in_length )
    return HB_Err_Not_Covered;         /* context is too long */

  c    = csf3->Coverage;

  for ( i = 1, j = buffer->in_pos + 1; i < csf3->GlyphCount; i++, j++ )
  {
    while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
    {
      if ( error && error != HB_Err_Not_Covered )
	return error;

      if ( j + csf3->GlyphCount - i == (FT_Long)buffer->in_length )
	return HB_Err_Not_Covered;
      j++;
    }

    error = _HB_OPEN_Coverage_Index( &c[i], IN_GLYPH( j ), &index );
    if ( error )
      return error;
  }

  return Do_ContextSubst( gsub, csf3->GlyphCount,
			  csf3->SubstCount, csf3->SubstLookupRecord,
			  buffer,
			  nesting_level );
}


static FT_Error  Lookup_ContextSubst( HB_GSUBHeader*    gsub,
				      HB_GSUB_SubTable* st,
				      HB_Buffer         buffer,
				      FT_UShort          flags,
				      FT_UShort          context_length,
				      int                nesting_level )
{
  HB_ContextSubst*  cs = &st->context;

  switch ( cs->SubstFormat )
  {
  case 1:
    return Lookup_ContextSubst1( gsub, &cs->csf.csf1, buffer,
				 flags, context_length, nesting_level );

  case 2:
    return Lookup_ContextSubst2( gsub, &cs->csf.csf2, buffer,
				 flags, context_length, nesting_level );

  case 3:
    return Lookup_ContextSubst3( gsub, &cs->csf.csf3, buffer,
				 flags, context_length, nesting_level );

  default:
    return HB_Err_Invalid_GSUB_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}


/* LookupType 6 */

/* ChainSubRule */

static FT_Error  Load_ChainSubRule( HB_ChainSubRule*  csr,
				    FT_Stream          stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort               n, count;
  FT_UShort*              b;
  FT_UShort*              i;
  FT_UShort*              l;

  HB_SubstLookupRecord*  slr;


  if ( ACCESS_Frame( 2L ) )
    return error;

  csr->BacktrackGlyphCount = GET_UShort();

  FORGET_Frame();

  csr->Backtrack = NULL;

  count = csr->BacktrackGlyphCount;

  if ( ALLOC_ARRAY( csr->Backtrack, count, FT_UShort ) )
    return error;

  b = csr->Backtrack;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail4;

  for ( n = 0; n < count; n++ )
    b[n] = GET_UShort();

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail4;

  csr->InputGlyphCount = GET_UShort();

  FORGET_Frame();

  csr->Input = NULL;

  count = csr->InputGlyphCount - 1;  /* only InputGlyphCount - 1 elements */

  if ( ALLOC_ARRAY( csr->Input, count, FT_UShort ) )
    goto Fail4;

  i = csr->Input;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail3;

  for ( n = 0; n < count; n++ )
    i[n] = GET_UShort();

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  csr->LookaheadGlyphCount = GET_UShort();

  FORGET_Frame();

  csr->Lookahead = NULL;

  count = csr->LookaheadGlyphCount;

  if ( ALLOC_ARRAY( csr->Lookahead, count, FT_UShort ) )
    goto Fail3;

  l = csr->Lookahead;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
    l[n] = GET_UShort();

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  csr->SubstCount = GET_UShort();

  FORGET_Frame();

  csr->SubstLookupRecord = NULL;

  count = csr->SubstCount;

  if ( ALLOC_ARRAY( csr->SubstLookupRecord, count, HB_SubstLookupRecord ) )
    goto Fail2;

  slr = csr->SubstLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    slr[n].SequenceIndex   = GET_UShort();
    slr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( slr );

Fail2:
  FREE( l );

Fail3:
  FREE( i );

Fail4:
  FREE( b );
  return error;
}


static void  Free_ChainSubRule( HB_ChainSubRule*  csr,
				FT_Memory          memory )
{
  FREE( csr->SubstLookupRecord );
  FREE( csr->Lookahead );
  FREE( csr->Input );
  FREE( csr->Backtrack );
}


/* ChainSubRuleSet */

static FT_Error  Load_ChainSubRuleSet( HB_ChainSubRuleSet*  csrs,
				       FT_Stream             stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort          n = 0, m, count;
  FT_ULong           cur_offset, new_offset, base_offset;

  HB_ChainSubRule*  csr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = csrs->ChainSubRuleCount = GET_UShort();

  FORGET_Frame();

  csrs->ChainSubRule = NULL;

  if ( ALLOC_ARRAY( csrs->ChainSubRule, count, HB_ChainSubRule ) )
    return error;

  csr = csrs->ChainSubRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_ChainSubRule( &csr[n], stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_ChainSubRule( &csr[m], memory );

  FREE( csr );
  return error;
}


static void  Free_ChainSubRuleSet( HB_ChainSubRuleSet*  csrs,
				   FT_Memory             memory )
{
  FT_UShort          n, count;

  HB_ChainSubRule*  csr;


  if ( csrs->ChainSubRule )
  {
    count = csrs->ChainSubRuleCount;
    csr   = csrs->ChainSubRule;

    for ( n = 0; n < count; n++ )
      Free_ChainSubRule( &csr[n], memory );

    FREE( csr );
  }
}


/* ChainContextSubstFormat1 */

static FT_Error  Load_ChainContextSubst1(
		   HB_ChainContextSubstFormat1*  ccsf1,
		   FT_Stream                      stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort             n = 0, m, count;
  FT_ULong              cur_offset, new_offset, base_offset;

  HB_ChainSubRuleSet*  csrs;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &ccsf1->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  count = ccsf1->ChainSubRuleSetCount = GET_UShort();

  FORGET_Frame();

  ccsf1->ChainSubRuleSet = NULL;

  if ( ALLOC_ARRAY( ccsf1->ChainSubRuleSet, count, HB_ChainSubRuleSet ) )
    goto Fail2;

  csrs = ccsf1->ChainSubRuleSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_ChainSubRuleSet( &csrs[n], stream ) ) != FT_Err_Ok )
      goto Fail1;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_ChainSubRuleSet( &csrs[m], memory );

  FREE( csrs );

Fail2:
  _HB_OPEN_Free_Coverage( &ccsf1->Coverage, memory );
  return error;
}


static void  Free_ChainContextSubst1( HB_ChainContextSubstFormat1*  ccsf1,
				 FT_Memory                      memory )
{
  FT_UShort             n, count;

  HB_ChainSubRuleSet*  csrs;


  if ( ccsf1->ChainSubRuleSet )
  {
    count = ccsf1->ChainSubRuleSetCount;
    csrs  = ccsf1->ChainSubRuleSet;

    for ( n = 0; n < count; n++ )
      Free_ChainSubRuleSet( &csrs[n], memory );

    FREE( csrs );
  }

  _HB_OPEN_Free_Coverage( &ccsf1->Coverage, memory );
}


/* ChainSubClassRule */

static FT_Error  Load_ChainSubClassRule(
		   HB_ChainContextSubstFormat2*  ccsf2,
		   HB_ChainSubClassRule*         cscr,
		   FT_Stream                      stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort               n, count;

  FT_UShort*              b;
  FT_UShort*              i;
  FT_UShort*              l;
  HB_SubstLookupRecord*  slr;
  FT_Bool*                d;


  if ( ACCESS_Frame( 2L ) )
    return error;

  cscr->BacktrackGlyphCount = GET_UShort();

  FORGET_Frame();

  if ( cscr->BacktrackGlyphCount > ccsf2->MaxBacktrackLength )
    ccsf2->MaxBacktrackLength = cscr->BacktrackGlyphCount;

  cscr->Backtrack = NULL;

  count = cscr->BacktrackGlyphCount;

  if ( ALLOC_ARRAY( cscr->Backtrack, count, FT_UShort ) )
    return error;

  b = cscr->Backtrack;
  d = ccsf2->BacktrackClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail4;

  for ( n = 0; n < count; n++ )
  {
    b[n] = GET_UShort();

    /* We check whether the specific class is used at all.  If not,
       class 0 is used instead.                                     */

    if ( !d[b[n]] )
      b[n] = 0;
  }

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail4;

  cscr->InputGlyphCount = GET_UShort();

  FORGET_Frame();

  if ( cscr->InputGlyphCount > ccsf2->MaxInputLength )
    ccsf2->MaxInputLength = cscr->InputGlyphCount;

  cscr->Input = NULL;

  count = cscr->InputGlyphCount - 1; /* only InputGlyphCount - 1 elements */

  if ( ALLOC_ARRAY( cscr->Input, count, FT_UShort ) )
    goto Fail4;

  i = cscr->Input;
  d = ccsf2->InputClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail3;

  for ( n = 0; n < count; n++ )
  {
    i[n] = GET_UShort();

    if ( !d[i[n]] )
      i[n] = 0;
  }

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  cscr->LookaheadGlyphCount = GET_UShort();

  FORGET_Frame();

  if ( cscr->LookaheadGlyphCount > ccsf2->MaxLookaheadLength )
    ccsf2->MaxLookaheadLength = cscr->LookaheadGlyphCount;

  cscr->Lookahead = NULL;

  count = cscr->LookaheadGlyphCount;

  if ( ALLOC_ARRAY( cscr->Lookahead, count, FT_UShort ) )
    goto Fail3;

  l = cscr->Lookahead;
  d = ccsf2->LookaheadClassDef.Defined;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail2;

  for ( n = 0; n < count; n++ )
  {
    l[n] = GET_UShort();

    if ( !d[l[n]] )
      l[n] = 0;
  }

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  cscr->SubstCount = GET_UShort();

  FORGET_Frame();

  cscr->SubstLookupRecord = NULL;

  count = cscr->SubstCount;

  if ( ALLOC_ARRAY( cscr->SubstLookupRecord, count,
		    HB_SubstLookupRecord ) )
    goto Fail2;

  slr = cscr->SubstLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    slr[n].SequenceIndex   = GET_UShort();
    slr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( slr );

Fail2:
  FREE( l );

Fail3:
  FREE( i );

Fail4:
  FREE( b );
  return error;
}


static void  Free_ChainSubClassRule( HB_ChainSubClassRule*  cscr,
				     FT_Memory               memory )
{
  FREE( cscr->SubstLookupRecord );
  FREE( cscr->Lookahead );
  FREE( cscr->Input );
  FREE( cscr->Backtrack );
}


/* SubClassSet */

static FT_Error  Load_ChainSubClassSet(
		   HB_ChainContextSubstFormat2*  ccsf2,
		   HB_ChainSubClassSet*          cscs,
		   FT_Stream                      stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort               n = 0, m, count;
  FT_ULong                cur_offset, new_offset, base_offset;

  HB_ChainSubClassRule*  cscr;


  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  count = cscs->ChainSubClassRuleCount = GET_UShort();

  FORGET_Frame();

  cscs->ChainSubClassRule = NULL;

  if ( ALLOC_ARRAY( cscs->ChainSubClassRule, count,
		    HB_ChainSubClassRule ) )
    return error;

  cscr = cscs->ChainSubClassRule;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = Load_ChainSubClassRule( ccsf2, &cscr[n],
					   stream ) ) != FT_Err_Ok )
      goto Fail;
    (void)FILE_Seek( cur_offset );
  }

  return FT_Err_Ok;

Fail:
  for ( m = 0; m < n; m++ )
    Free_ChainSubClassRule( &cscr[m], memory );

  FREE( cscr );
  return error;
}


static void  Free_ChainSubClassSet( HB_ChainSubClassSet*  cscs,
				    FT_Memory              memory )
{
  FT_UShort               n, count;

  HB_ChainSubClassRule*  cscr;


  if ( cscs->ChainSubClassRule )
  {
    count = cscs->ChainSubClassRuleCount;
    cscr  = cscs->ChainSubClassRule;

    for ( n = 0; n < count; n++ )
      Free_ChainSubClassRule( &cscr[n], memory );

    FREE( cscr );
  }
}

static FT_Error GSUB_Load_EmptyOrClassDefinition( HB_ClassDefinition*  cd,
					     FT_UShort             limit,
					     FT_ULong              class_offset,
					     FT_ULong              base_offset,
					     FT_Stream             stream )
{
  FT_Error error;
  FT_ULong               cur_offset;

  cur_offset = FILE_Pos();

  if ( class_offset )
    {
      if ( !FILE_Seek( class_offset + base_offset ) )
	error = _HB_OPEN_Load_ClassDefinition( cd, limit, stream );
    }
  else
     error = _HB_OPEN_Load_EmptyClassDefinition ( cd, stream );

  if (error == FT_Err_Ok)
    (void)FILE_Seek( cur_offset ); /* Changes error as a side-effect */

  return error;
}


/* ChainContextSubstFormat2 */

static FT_Error  Load_ChainContextSubst2(
		   HB_ChainContextSubstFormat2*  ccsf2,
		   FT_Stream                      stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort              n = 0, m, count;
  FT_ULong               cur_offset, new_offset, base_offset;
  FT_ULong               backtrack_offset, input_offset, lookahead_offset;

  HB_ChainSubClassSet*  cscs;


  base_offset = FILE_Pos() - 2;

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &ccsf2->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );

  if ( ACCESS_Frame( 8L ) )
    goto Fail5;

  backtrack_offset = GET_UShort();
  input_offset     = GET_UShort();
  lookahead_offset = GET_UShort();

  /* `ChainSubClassSetCount' is the upper limit for input class values,
     thus we read it now to make an additional safety check. No limit
     is known or needed for the other two class definitions          */

  count = ccsf2->ChainSubClassSetCount = GET_UShort();

  FORGET_Frame();

  if ( ( error = GSUB_Load_EmptyOrClassDefinition( &ccsf2->BacktrackClassDef, 65535,
					      backtrack_offset, base_offset,
					      stream ) ) != FT_Err_Ok )
      goto Fail5;

  if ( ( error = GSUB_Load_EmptyOrClassDefinition( &ccsf2->InputClassDef, count,
					      input_offset, base_offset,
					      stream ) ) != FT_Err_Ok )
      goto Fail4;
  if ( ( error = GSUB_Load_EmptyOrClassDefinition( &ccsf2->LookaheadClassDef, 65535,
					      lookahead_offset, base_offset,
					      stream ) ) != FT_Err_Ok )
    goto Fail3;

  ccsf2->ChainSubClassSet   = NULL;
  ccsf2->MaxBacktrackLength = 0;
  ccsf2->MaxInputLength     = 0;
  ccsf2->MaxLookaheadLength = 0;

  if ( ALLOC_ARRAY( ccsf2->ChainSubClassSet, count, HB_ChainSubClassSet ) )
    goto Fail2;

  cscs = ccsf2->ChainSubClassSet;

  for ( n = 0; n < count; n++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail1;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    if ( new_offset != base_offset )      /* not a NULL offset */
    {
      cur_offset = FILE_Pos();
      if ( FILE_Seek( new_offset ) ||
	   ( error = Load_ChainSubClassSet( ccsf2, &cscs[n],
					    stream ) ) != FT_Err_Ok )
	goto Fail1;
      (void)FILE_Seek( cur_offset );
    }
    else
    {
      /* we create a ChainSubClassSet table with no entries */

      ccsf2->ChainSubClassSet[n].ChainSubClassRuleCount = 0;
      ccsf2->ChainSubClassSet[n].ChainSubClassRule      = NULL;
    }
  }

  return FT_Err_Ok;

Fail1:
  for ( m = 0; m < n; m++ )
    Free_ChainSubClassSet( &cscs[m], memory );

  FREE( cscs );

Fail2:
  _HB_OPEN_Free_ClassDefinition( &ccsf2->LookaheadClassDef, memory );

Fail3:
  _HB_OPEN_Free_ClassDefinition( &ccsf2->InputClassDef, memory );

Fail4:
  _HB_OPEN_Free_ClassDefinition( &ccsf2->BacktrackClassDef, memory );

Fail5:
  _HB_OPEN_Free_Coverage( &ccsf2->Coverage, memory );
  return error;
}


static void  Free_ChainContextSubst2( HB_ChainContextSubstFormat2*  ccsf2,
				 FT_Memory                      memory )
{
  FT_UShort              n, count;

  HB_ChainSubClassSet*  cscs;


  if ( ccsf2->ChainSubClassSet )
  {
    count = ccsf2->ChainSubClassSetCount;
    cscs  = ccsf2->ChainSubClassSet;

    for ( n = 0; n < count; n++ )
      Free_ChainSubClassSet( &cscs[n], memory );

    FREE( cscs );
  }

  _HB_OPEN_Free_ClassDefinition( &ccsf2->LookaheadClassDef, memory );
  _HB_OPEN_Free_ClassDefinition( &ccsf2->InputClassDef, memory );
  _HB_OPEN_Free_ClassDefinition( &ccsf2->BacktrackClassDef, memory );

  _HB_OPEN_Free_Coverage( &ccsf2->Coverage, memory );
}


/* ChainContextSubstFormat3 */

static FT_Error  Load_ChainContextSubst3(
		   HB_ChainContextSubstFormat3*  ccsf3,
		   FT_Stream                      stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;

  FT_UShort               n, nb = 0, ni =0, nl = 0, m, count;
  FT_UShort               backtrack_count, input_count, lookahead_count;
  FT_ULong                cur_offset, new_offset, base_offset;

  HB_Coverage*           b;
  HB_Coverage*           i;
  HB_Coverage*           l;
  HB_SubstLookupRecord*  slr;


  base_offset = FILE_Pos() - 2L;

  if ( ACCESS_Frame( 2L ) )
    return error;

  ccsf3->BacktrackGlyphCount = GET_UShort();

  FORGET_Frame();

  ccsf3->BacktrackCoverage = NULL;

  backtrack_count = ccsf3->BacktrackGlyphCount;

  if ( ALLOC_ARRAY( ccsf3->BacktrackCoverage, backtrack_count,
		    HB_Coverage ) )
    return error;

  b = ccsf3->BacktrackCoverage;

  for ( nb = 0; nb < backtrack_count; nb++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail4;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &b[nb], stream ) ) != FT_Err_Ok )
      goto Fail4;
    (void)FILE_Seek( cur_offset );
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail4;

  ccsf3->InputGlyphCount = GET_UShort();

  FORGET_Frame();

  ccsf3->InputCoverage = NULL;

  input_count = ccsf3->InputGlyphCount;

  if ( ALLOC_ARRAY( ccsf3->InputCoverage, input_count, HB_Coverage ) )
    goto Fail4;

  i = ccsf3->InputCoverage;

  for ( ni = 0; ni < input_count; ni++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail3;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &i[ni], stream ) ) != FT_Err_Ok )
      goto Fail3;
    (void)FILE_Seek( cur_offset );
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  ccsf3->LookaheadGlyphCount = GET_UShort();

  FORGET_Frame();

  ccsf3->LookaheadCoverage = NULL;

  lookahead_count = ccsf3->LookaheadGlyphCount;

  if ( ALLOC_ARRAY( ccsf3->LookaheadCoverage, lookahead_count,
		    HB_Coverage ) )
    goto Fail3;

  l = ccsf3->LookaheadCoverage;

  for ( nl = 0; nl < lookahead_count; nl++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &l[nl], stream ) ) != FT_Err_Ok )
      goto Fail2;
    (void)FILE_Seek( cur_offset );
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  ccsf3->SubstCount = GET_UShort();

  FORGET_Frame();

  ccsf3->SubstLookupRecord = NULL;

  count = ccsf3->SubstCount;

  if ( ALLOC_ARRAY( ccsf3->SubstLookupRecord, count,
		    HB_SubstLookupRecord ) )
    goto Fail2;

  slr = ccsf3->SubstLookupRecord;

  if ( ACCESS_Frame( count * 4L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
  {
    slr[n].SequenceIndex   = GET_UShort();
    slr[n].LookupListIndex = GET_UShort();
  }

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( slr );

Fail2:
  for ( m = 0; m < nl; m++ )
    _HB_OPEN_Free_Coverage( &l[m], memory );

  FREE( l );

Fail3:
  for ( m = 0; m < ni; m++ )
    _HB_OPEN_Free_Coverage( &i[m], memory );

  FREE( i );

Fail4:
  for ( m = 0; m < nb; m++ )
    _HB_OPEN_Free_Coverage( &b[m], memory );

  FREE( b );
  return error;
}


static void  Free_ChainContextSubst3( HB_ChainContextSubstFormat3*  ccsf3,
				 FT_Memory                      memory )
{
  FT_UShort      n, count;

  HB_Coverage*  c;


  FREE( ccsf3->SubstLookupRecord );

  if ( ccsf3->LookaheadCoverage )
  {
    count = ccsf3->LookaheadGlyphCount;
    c     = ccsf3->LookaheadCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }

  if ( ccsf3->InputCoverage )
  {
    count = ccsf3->InputGlyphCount;
    c     = ccsf3->InputCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }

  if ( ccsf3->BacktrackCoverage )
  {
    count = ccsf3->BacktrackGlyphCount;
    c     = ccsf3->BacktrackCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }
}


/* ChainContextSubst */

static FT_Error  Load_ChainContextSubst( HB_GSUB_SubTable* st,
					 FT_Stream         stream )
{
  FT_Error error;
  HB_ChainContextSubst*  ccs = &st->chain;

  if ( ACCESS_Frame( 2L ) )
    return error;

  ccs->SubstFormat = GET_UShort();

  FORGET_Frame();

  switch ( ccs->SubstFormat )
  {
  case 1:
    return Load_ChainContextSubst1( &ccs->ccsf.ccsf1, stream );

  case 2:
    return Load_ChainContextSubst2( &ccs->ccsf.ccsf2, stream );

  case 3:
    return Load_ChainContextSubst3( &ccs->ccsf.ccsf3, stream );

  default:
    return HB_Err_Invalid_GSUB_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}


static void  Free_ChainContextSubst( HB_GSUB_SubTable* st,
				     FT_Memory         memory )
{
  HB_ChainContextSubst*  ccs = &st->chain;

  switch ( ccs->SubstFormat )
  {
  case 1:
    Free_ChainContextSubst1( &ccs->ccsf.ccsf1, memory );
    break;

  case 2:
    Free_ChainContextSubst2( &ccs->ccsf.ccsf2, memory );
    break;

  case 3:
    Free_ChainContextSubst3( &ccs->ccsf.ccsf3, memory );
    break;
  }
}


static FT_Error  Lookup_ChainContextSubst1( HB_GSUBHeader*               gsub,
					    HB_ChainContextSubstFormat1* ccsf1,
					    HB_Buffer                    buffer,
					    FT_UShort                     flags,
					    FT_UShort                     context_length,
					    int                           nesting_level )
{
  FT_UShort          index, property;
  FT_UShort          i, j, k, num_csr;
  FT_UShort          bgc, igc, lgc;
  FT_Error           error;

  HB_ChainSubRule*  csr;
  HB_ChainSubRule   curr_csr;
  HB_GDEFHeader*    gdef;


  gdef = gsub->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  error = _HB_OPEN_Coverage_Index( &ccsf1->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  csr     = ccsf1->ChainSubRuleSet[index].ChainSubRule;
  num_csr = ccsf1->ChainSubRuleSet[index].ChainSubRuleCount;

  for ( k = 0; k < num_csr; k++ )
  {
    curr_csr = csr[k];
    bgc      = curr_csr.BacktrackGlyphCount;
    igc      = curr_csr.InputGlyphCount;
    lgc      = curr_csr.LookaheadGlyphCount;

    if ( context_length != 0xFFFF && context_length < igc )
      goto next_chainsubrule;

    /* check whether context is too long; it is a first guess only */

    if ( bgc > buffer->out_pos || buffer->in_pos + igc + lgc > buffer->in_length )
      goto next_chainsubrule;

    if ( bgc )
    {
      /* since we don't know in advance the number of glyphs to inspect,
	 we search backwards for matches in the backtrack glyph array    */

      for ( i = 0, j = buffer->out_pos - 1; i < bgc; i++, j-- )
      {
	while ( CHECK_Property( gdef, OUT_ITEM( j ), flags, &property ) )
	{
	  if ( error && error != HB_Err_Not_Covered )
	    return error;

	  if ( j + 1 == bgc - i )
	    goto next_chainsubrule;
	  j--;
	}

	/* In OpenType 1.3, it is undefined whether the offsets of
	   backtrack glyphs is in logical order or not.  Version 1.4
	   will clarify this:

	     Logical order -      a  b  c  d  e  f  g  h  i  j
					      i
	     Input offsets -                  0  1
	     Backtrack offsets -  3  2  1  0
	     Lookahead offsets -                    0  1  2  3           */

	if ( OUT_GLYPH( j ) != curr_csr.Backtrack[i] )
	  goto next_chainsubrule;
      }
    }

    /* Start at 1 because [0] is implied */

    for ( i = 1, j = buffer->in_pos + 1; i < igc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + igc - i + lgc == (FT_Long)buffer->in_length )
	  goto next_chainsubrule;
	j++;
      }

      if ( IN_GLYPH( j ) != curr_csr.Input[i - 1] )
	  goto next_chainsubrule;
    }

    /* we are starting to check for lookahead glyphs right after the
       last context glyph                                            */

    for ( i = 0; i < lgc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + lgc - i == (FT_Long)buffer->in_length )
	  goto next_chainsubrule;
	j++;
      }

      if ( IN_GLYPH( j ) != curr_csr.Lookahead[i] )
	goto next_chainsubrule;
    }

    return Do_ContextSubst( gsub, igc,
			    curr_csr.SubstCount,
			    curr_csr.SubstLookupRecord,
			    buffer,
			    nesting_level );

  next_chainsubrule:
    ;
  }

  return HB_Err_Not_Covered;
}


static FT_Error  Lookup_ChainContextSubst2( HB_GSUBHeader*               gsub,
					    HB_ChainContextSubstFormat2* ccsf2,
					    HB_Buffer                    buffer,
					    FT_UShort                     flags,
					    FT_UShort                     context_length,
					    int                           nesting_level )
{
  FT_UShort              index, property;
  FT_Memory              memory;
  FT_Error               error;
  FT_UShort              i, j, k;
  FT_UShort              bgc, igc, lgc;
  FT_UShort              known_backtrack_classes,
			 known_input_classes,
			 known_lookahead_classes;

  FT_UShort*             backtrack_classes;
  FT_UShort*             input_classes;
  FT_UShort*             lookahead_classes;

  FT_UShort*             bc;
  FT_UShort*             ic;
  FT_UShort*             lc;

  HB_ChainSubClassSet*  cscs;
  HB_ChainSubClassRule  ccsr;
  HB_GDEFHeader*        gdef;


  gdef = gsub->gdef;
  memory = gsub->memory;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  /* Note: The coverage table in format 2 doesn't give an index into
	   anything.  It just lets us know whether or not we need to
	   do any lookup at all.                                     */

  error = _HB_OPEN_Coverage_Index( &ccsf2->Coverage, IN_CURGLYPH(), &index );
  if ( error )
    return error;

  if ( ALLOC_ARRAY( backtrack_classes, ccsf2->MaxBacktrackLength, FT_UShort ) )
    return error;
  known_backtrack_classes = 0;

  if ( ALLOC_ARRAY( input_classes, ccsf2->MaxInputLength, FT_UShort ) )
    goto End3;
  known_input_classes = 1;

  if ( ALLOC_ARRAY( lookahead_classes, ccsf2->MaxLookaheadLength, FT_UShort ) )
    goto End2;
  known_lookahead_classes = 0;

  error = _HB_OPEN_Get_Class( &ccsf2->InputClassDef, IN_CURGLYPH(),
		     &input_classes[0], NULL );
  if ( error && error != HB_Err_Not_Covered )
    goto End1;

  cscs = &ccsf2->ChainSubClassSet[input_classes[0]];
  if ( !cscs )
  {
    error = HB_Err_Invalid_GSUB_SubTable;
    goto End1;
  }

  for ( k = 0; k < cscs->ChainSubClassRuleCount; k++ )
  {
    ccsr = cscs->ChainSubClassRule[k];
    bgc  = ccsr.BacktrackGlyphCount;
    igc  = ccsr.InputGlyphCount;
    lgc  = ccsr.LookaheadGlyphCount;

    if ( context_length != 0xFFFF && context_length < igc )
      goto next_chainsubclassrule;

    /* check whether context is too long; it is a first guess only */

    if ( bgc > buffer->out_pos || buffer->in_pos + igc + lgc > buffer->in_length )
      goto next_chainsubclassrule;

    if ( bgc )
    {
      /* Since we don't know in advance the number of glyphs to inspect,
	 we search backwards for matches in the backtrack glyph array.
	 Note that `known_backtrack_classes' starts at index 0.         */

      bc       = ccsr.Backtrack;

      for ( i = 0, j = buffer->out_pos - 1; i < bgc; i++, j-- )
      {
	while ( CHECK_Property( gdef, OUT_ITEM( j ), flags, &property ) )
	{
	  if ( error && error != HB_Err_Not_Covered )
	    goto End1;

	  if ( j + 1 == bgc - i )
	    goto next_chainsubclassrule;
	  j--;
	}

	if ( i >= known_backtrack_classes )
	{
	  /* Keeps us from having to do this for each rule */

	  error = _HB_OPEN_Get_Class( &ccsf2->BacktrackClassDef, OUT_GLYPH( j ),
			     &backtrack_classes[i], NULL );
	  if ( error && error != HB_Err_Not_Covered )
	    goto End1;
	  known_backtrack_classes = i;
	}

	if ( bc[i] != backtrack_classes[i] )
	  goto next_chainsubclassrule;
      }
    }

    ic       = ccsr.Input;

    /* Start at 1 because [0] is implied */

    for ( i = 1, j = buffer->in_pos + 1; i < igc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;

	if ( j + igc - i + lgc == (FT_Long)buffer->in_length )
	  goto next_chainsubclassrule;
	j++;
      }

      if ( i >= known_input_classes )
      {
	error = _HB_OPEN_Get_Class( &ccsf2->InputClassDef, IN_GLYPH( j ),
			   &input_classes[i], NULL );
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;
	known_input_classes = i;
      }

      if ( ic[i - 1] != input_classes[i] )
	goto next_chainsubclassrule;
    }

    /* we are starting to check for lookahead glyphs right after the
       last context glyph                                            */

    lc       = ccsr.Lookahead;

    for ( i = 0; i < lgc; i++, j++ )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;

	if ( j + lgc - i == (FT_Long)buffer->in_length )
	  goto next_chainsubclassrule;
	j++;
      }

      if ( i >= known_lookahead_classes )
      {
	error = _HB_OPEN_Get_Class( &ccsf2->LookaheadClassDef, IN_GLYPH( j ),
			   &lookahead_classes[i], NULL );
	if ( error && error != HB_Err_Not_Covered )
	  goto End1;
	known_lookahead_classes = i;
      }

      if ( lc[i] != lookahead_classes[i] )
	goto next_chainsubclassrule;
    }

    error = Do_ContextSubst( gsub, igc,
			     ccsr.SubstCount,
			     ccsr.SubstLookupRecord,
			     buffer,
			     nesting_level );
    goto End1;

  next_chainsubclassrule:
    ;
  }

  error = HB_Err_Not_Covered;

End1:
  FREE( lookahead_classes );

End2:
  FREE( input_classes );

End3:
  FREE( backtrack_classes );
  return error;
}


static FT_Error  Lookup_ChainContextSubst3( HB_GSUBHeader*               gsub,
					    HB_ChainContextSubstFormat3* ccsf3,
					    HB_Buffer                    buffer,
					    FT_UShort                     flags,
					    FT_UShort                     context_length,
					    int                           nesting_level )
{
  FT_UShort        index, i, j, property;
  FT_UShort        bgc, igc, lgc;
  FT_Error         error;

  HB_Coverage*    bc;
  HB_Coverage*    ic;
  HB_Coverage*    lc;
  HB_GDEFHeader*  gdef;


  gdef = gsub->gdef;

  if ( CHECK_Property( gdef, IN_CURITEM(), flags, &property ) )
    return error;

  bgc = ccsf3->BacktrackGlyphCount;
  igc = ccsf3->InputGlyphCount;
  lgc = ccsf3->LookaheadGlyphCount;

  if ( context_length != 0xFFFF && context_length < igc )
    return HB_Err_Not_Covered;

  /* check whether context is too long; it is a first guess only */

  if ( bgc > buffer->out_pos || buffer->in_pos + igc + lgc > buffer->in_length )
    return HB_Err_Not_Covered;

  if ( bgc )
  {
    /* Since we don't know in advance the number of glyphs to inspect,
       we search backwards for matches in the backtrack glyph array    */

    bc       = ccsf3->BacktrackCoverage;

    for ( i = 0, j = buffer->out_pos - 1; i < bgc; i++, j-- )
    {
      while ( CHECK_Property( gdef, OUT_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + 1 == bgc - i )
	  return HB_Err_Not_Covered;
	j--;
      }

      error = _HB_OPEN_Coverage_Index( &bc[i], OUT_GLYPH( j ), &index );
      if ( error )
	return error;
    }
  }

  ic       = ccsf3->InputCoverage;

  for ( i = 0, j = buffer->in_pos; i < igc; i++, j++ )
  {
    /* We already called CHECK_Property for IN_GLYPH( buffer->in_pos ) */
    while ( j > buffer->in_pos && CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
    {
      if ( error && error != HB_Err_Not_Covered )
	return error;

      if ( j + igc - i + lgc == (FT_Long)buffer->in_length )
	return HB_Err_Not_Covered;
      j++;
    }

    error = _HB_OPEN_Coverage_Index( &ic[i], IN_GLYPH( j ), &index );
    if ( error )
      return error;
  }

  /* we are starting for lookahead glyphs right after the last context
     glyph                                                             */

  lc       = ccsf3->LookaheadCoverage;

  for ( i = 0; i < lgc; i++, j++ )
  {
    while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
    {
      if ( error && error != HB_Err_Not_Covered )
	return error;

      if ( j + lgc - i == (FT_Long)buffer->in_length )
	return HB_Err_Not_Covered;
      j++;
    }

    error = _HB_OPEN_Coverage_Index( &lc[i], IN_GLYPH( j ), &index );
    if ( error )
      return error;
  }

  return Do_ContextSubst( gsub, igc,
			  ccsf3->SubstCount,
			  ccsf3->SubstLookupRecord,
			  buffer,
			  nesting_level );
}


static FT_Error  Lookup_ChainContextSubst( HB_GSUBHeader*    gsub,
					   HB_GSUB_SubTable* st,
					   HB_Buffer         buffer,
					   FT_UShort          flags,
					   FT_UShort          context_length,
					   int                nesting_level )
{
  HB_ChainContextSubst*  ccs = &st->chain;

  switch ( ccs->SubstFormat )
  {
  case 1:
    return Lookup_ChainContextSubst1( gsub, &ccs->ccsf.ccsf1, buffer,
				      flags, context_length,
				      nesting_level );

  case 2:
    return Lookup_ChainContextSubst2( gsub, &ccs->ccsf.ccsf2, buffer,
				      flags, context_length,
				      nesting_level );

  case 3:
    return Lookup_ChainContextSubst3( gsub, &ccs->ccsf.ccsf3, buffer,
				      flags, context_length,
				      nesting_level );

  default:
    return HB_Err_Invalid_GSUB_SubTable_Format;
  }

  return FT_Err_Ok;               /* never reached */
}


static FT_Error  Load_ReverseChainContextSubst( HB_GSUB_SubTable* st,
					        FT_Stream         stream )
{
  FT_Error error;
  FT_Memory memory = stream->memory;
  HB_ReverseChainContextSubst*  rccs = &st->reverse;

  FT_UShort               m, count;

  FT_UShort               nb = 0, nl = 0, n;
  FT_UShort               backtrack_count, lookahead_count;
  FT_ULong                cur_offset, new_offset, base_offset;

  HB_Coverage*           b;
  HB_Coverage*           l;
  FT_UShort*              sub;

  base_offset = FILE_Pos();

  if ( ACCESS_Frame( 2L ) )
    return error;

  rccs->SubstFormat = GET_UShort();

  if ( rccs->SubstFormat != 1 )
    return HB_Err_Invalid_GSUB_SubTable_Format;

  FORGET_Frame();

  if ( ACCESS_Frame( 2L ) )
    return error;

  new_offset = GET_UShort() + base_offset;

  FORGET_Frame();

  cur_offset = FILE_Pos();
  if ( FILE_Seek( new_offset ) ||
       ( error = _HB_OPEN_Load_Coverage( &rccs->Coverage, stream ) ) != FT_Err_Ok )
    return error;
  (void)FILE_Seek( cur_offset );


  if ( ACCESS_Frame( 2L ) )
    goto Fail4;

  rccs->BacktrackGlyphCount = GET_UShort();

  FORGET_Frame();

  rccs->BacktrackCoverage = NULL;

  backtrack_count = rccs->BacktrackGlyphCount;

  if ( ALLOC_ARRAY( rccs->BacktrackCoverage, backtrack_count,
		    HB_Coverage ) )
    goto Fail4;

  b = rccs->BacktrackCoverage;

  for ( nb = 0; nb < backtrack_count; nb++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail3;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &b[nb], stream ) ) != FT_Err_Ok )
      goto Fail3;
    (void)FILE_Seek( cur_offset );
  }


  if ( ACCESS_Frame( 2L ) )
    goto Fail3;

  rccs->LookaheadGlyphCount = GET_UShort();

  FORGET_Frame();

  rccs->LookaheadCoverage = NULL;

  lookahead_count = rccs->LookaheadGlyphCount;

  if ( ALLOC_ARRAY( rccs->LookaheadCoverage, lookahead_count,
		    HB_Coverage ) )
    goto Fail3;

  l = rccs->LookaheadCoverage;

  for ( nl = 0; nl < lookahead_count; nl++ )
  {
    if ( ACCESS_Frame( 2L ) )
      goto Fail2;

    new_offset = GET_UShort() + base_offset;

    FORGET_Frame();

    cur_offset = FILE_Pos();
    if ( FILE_Seek( new_offset ) ||
	 ( error = _HB_OPEN_Load_Coverage( &l[nl], stream ) ) != FT_Err_Ok )
      goto Fail2;
    (void)FILE_Seek( cur_offset );
  }

  if ( ACCESS_Frame( 2L ) )
    goto Fail2;

  rccs->GlyphCount = GET_UShort();

  FORGET_Frame();

  rccs->Substitute = NULL;

  count = rccs->GlyphCount;

  if ( ALLOC_ARRAY( rccs->Substitute, count,
		    FT_UShort ) )
    goto Fail2;

  sub = rccs->Substitute;

  if ( ACCESS_Frame( count * 2L ) )
    goto Fail1;

  for ( n = 0; n < count; n++ )
    sub[n] = GET_UShort();

  FORGET_Frame();

  return FT_Err_Ok;

Fail1:
  FREE( sub );

Fail2:
  for ( m = 0; m < nl; m++ )
    _HB_OPEN_Free_Coverage( &l[m], memory );

  FREE( l );

Fail3:
  for ( m = 0; m < nb; m++ )
    _HB_OPEN_Free_Coverage( &b[m], memory );

  FREE( b );

Fail4:
  _HB_OPEN_Free_Coverage( &rccs->Coverage, memory );
  return error;
}


static void  Free_ReverseChainContextSubst( HB_GSUB_SubTable* st,
					    FT_Memory         memory )
{
  FT_UShort      n, count;
  HB_ReverseChainContextSubst*  rccs = &st->reverse;

  HB_Coverage*  c;

  _HB_OPEN_Free_Coverage( &rccs->Coverage, memory );

  if ( rccs->LookaheadCoverage )
  {
    count = rccs->LookaheadGlyphCount;
    c     = rccs->LookaheadCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }

  if ( rccs->BacktrackCoverage )
  {
    count = rccs->BacktrackGlyphCount;
    c     = rccs->BacktrackCoverage;

    for ( n = 0; n < count; n++ )
      _HB_OPEN_Free_Coverage( &c[n], memory );

    FREE( c );
  }

  FREE ( rccs->Substitute );
}


static FT_Error  Lookup_ReverseChainContextSubst( HB_GSUBHeader*    gsub,
						  HB_GSUB_SubTable* st,
						  HB_Buffer         buffer,
						  FT_UShort          flags,
      /* note different signature here: */	  FT_ULong           string_index )
{
  FT_UShort        index, input_index, i, j, property;
  FT_UShort        bgc, lgc;
  FT_Error         error;

  HB_ReverseChainContextSubst*  rccs = &st->reverse;
  HB_Coverage*    bc;
  HB_Coverage*    lc;
  HB_GDEFHeader*  gdef;

  gdef = gsub->gdef;

  if ( CHECK_Property( gdef, IN_ITEM( string_index ), flags, &property ) )
    return error;

  bgc = rccs->BacktrackGlyphCount;
  lgc = rccs->LookaheadGlyphCount;

  /* check whether context is too long; it is a first guess only */

  if ( bgc > string_index || string_index + 1 + lgc > buffer->in_length )
    return HB_Err_Not_Covered;

  if ( bgc )
  {
    /* Since we don't know in advance the number of glyphs to inspect,
       we search backwards for matches in the backtrack glyph array    */

    bc       = rccs->BacktrackCoverage;

    for ( i = 0, j = string_index - 1; i < bgc; i++, j-- )
    {
      while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
      {
	if ( error && error != HB_Err_Not_Covered )
	  return error;

	if ( j + 1 == bgc - i )
	  return HB_Err_Not_Covered;
	j--;
      }

      error = _HB_OPEN_Coverage_Index( &bc[i], IN_GLYPH( j ), &index );
      if ( error )
	return error;
    }
  }

  j = string_index;

  error = _HB_OPEN_Coverage_Index( &rccs->Coverage, IN_GLYPH( j ), &input_index );
  if ( error )
      return error;

  /* we are starting for lookahead glyphs right after the last context
     glyph                                                             */

  j += 1;

  lc       = rccs->LookaheadCoverage;

  for ( i = 0; i < lgc; i++, j++ )
  {
    while ( CHECK_Property( gdef, IN_ITEM( j ), flags, &property ) )
    {
      if ( error && error != HB_Err_Not_Covered )
	return error;

      if ( j + lgc - i == (FT_Long)buffer->in_length )
	return HB_Err_Not_Covered;
      j++;
    }

    error = _HB_OPEN_Coverage_Index( &lc[i], IN_GLYPH( j ), &index );
    if ( error )
      return error;
  }

  IN_GLYPH( string_index ) = rccs->Substitute[input_index];

  return error;
}



/***********
 * GSUB API
 ***********/



FT_Error  HB_GSUB_Select_Script( HB_GSUBHeader*  gsub,
				 FT_ULong         script_tag,
				 FT_UShort*       script_index )
{
  FT_UShort          n;

  HB_ScriptList*    sl;
  HB_ScriptRecord*  sr;


  if ( !gsub || !script_index )
    return FT_Err_Invalid_Argument;

  sl = &gsub->ScriptList;
  sr = sl->ScriptRecord;

  for ( n = 0; n < sl->ScriptCount; n++ )
    if ( script_tag == sr[n].ScriptTag )
    {
      *script_index = n;

      return FT_Err_Ok;
    }

  return HB_Err_Not_Covered;
}



FT_Error  HB_GSUB_Select_Language( HB_GSUBHeader*  gsub,
				   FT_ULong         language_tag,
				   FT_UShort        script_index,
				   FT_UShort*       language_index,
				   FT_UShort*       req_feature_index )
{
  FT_UShort           n;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;


  if ( !gsub || !language_index || !req_feature_index )
    return FT_Err_Invalid_Argument;

  sl = &gsub->ScriptList;
  sr = sl->ScriptRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  for ( n = 0; n < s->LangSysCount; n++ )
    if ( language_tag == lsr[n].LangSysTag )
    {
      *language_index = n;
      *req_feature_index = lsr[n].LangSys.ReqFeatureIndex;

      return FT_Err_Ok;
    }

  return HB_Err_Not_Covered;
}


/* selecting 0xFFFF for language_index asks for the values of the
   default language (DefaultLangSys)                              */


FT_Error  HB_GSUB_Select_Feature( HB_GSUBHeader*  gsub,
				  FT_ULong         feature_tag,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_UShort*       feature_index )
{
  FT_UShort           n;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;
  HB_LangSys*        ls;
  FT_UShort*          fi;

  HB_FeatureList*    fl;
  HB_FeatureRecord*  fr;


  if ( !gsub || !feature_index )
    return FT_Err_Invalid_Argument;

  sl = &gsub->ScriptList;
  sr = sl->ScriptRecord;

  fl = &gsub->FeatureList;
  fr = fl->FeatureRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  if ( language_index == 0xFFFF )
    ls = &s->DefaultLangSys;
  else
  {
    if ( language_index >= s->LangSysCount )
      return FT_Err_Invalid_Argument;

    ls = &lsr[language_index].LangSys;
  }

  fi = ls->FeatureIndex;

  for ( n = 0; n < ls->FeatureCount; n++ )
  {
    if ( fi[n] >= fl->FeatureCount )
      return HB_Err_Invalid_GSUB_SubTable_Format;

    if ( feature_tag == fr[fi[n]].FeatureTag )
    {
      *feature_index = fi[n];

      return FT_Err_Ok;
    }
  }

  return HB_Err_Not_Covered;
}


/* The next three functions return a null-terminated list */


FT_Error  HB_GSUB_Query_Scripts( HB_GSUBHeader*  gsub,
				 FT_ULong**       script_tag_list )
{
  FT_UShort          n;
  FT_Error           error;
  FT_Memory          memory;
  FT_ULong*          stl;

  HB_ScriptList*    sl;
  HB_ScriptRecord*  sr;


  if ( !gsub || !script_tag_list )
    return FT_Err_Invalid_Argument;

  memory = gsub->memory;

  sl = &gsub->ScriptList;
  sr = sl->ScriptRecord;

  if ( ALLOC_ARRAY( stl, sl->ScriptCount + 1, FT_ULong ) )
    return error;

  for ( n = 0; n < sl->ScriptCount; n++ )
    stl[n] = sr[n].ScriptTag;
  stl[n] = 0;

  *script_tag_list = stl;

  return FT_Err_Ok;
}



FT_Error  HB_GSUB_Query_Languages( HB_GSUBHeader*  gsub,
				   FT_UShort        script_index,
				   FT_ULong**       language_tag_list )
{
  FT_UShort           n;
  FT_Error            error;
  FT_Memory           memory;
  FT_ULong*           ltl;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;


  if ( !gsub || !language_tag_list )
    return FT_Err_Invalid_Argument;

  memory = gsub->memory;

  sl = &gsub->ScriptList;
  sr = sl->ScriptRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  if ( ALLOC_ARRAY( ltl, s->LangSysCount + 1, FT_ULong ) )
    return error;

  for ( n = 0; n < s->LangSysCount; n++ )
    ltl[n] = lsr[n].LangSysTag;
  ltl[n] = 0;

  *language_tag_list = ltl;

  return FT_Err_Ok;
}


/* selecting 0xFFFF for language_index asks for the values of the
   default language (DefaultLangSys)                              */


FT_Error  HB_GSUB_Query_Features( HB_GSUBHeader*  gsub,
				  FT_UShort        script_index,
				  FT_UShort        language_index,
				  FT_ULong**       feature_tag_list )
{
  FT_UShort           n;
  FT_Error            error;
  FT_Memory           memory;
  FT_ULong*           ftl;

  HB_ScriptList*     sl;
  HB_ScriptRecord*   sr;
  HB_Script*         s;
  HB_LangSysRecord*  lsr;
  HB_LangSys*        ls;
  FT_UShort*          fi;

  HB_FeatureList*    fl;
  HB_FeatureRecord*  fr;


  if ( !gsub || !feature_tag_list )
    return FT_Err_Invalid_Argument;

  memory = gsub->memory;

  sl = &gsub->ScriptList;
  sr = sl->ScriptRecord;

  fl = &gsub->FeatureList;
  fr = fl->FeatureRecord;

  if ( script_index >= sl->ScriptCount )
    return FT_Err_Invalid_Argument;

  s   = &sr[script_index].Script;
  lsr = s->LangSysRecord;

  if ( language_index == 0xFFFF )
    ls = &s->DefaultLangSys;
  else
  {
    if ( language_index >= s->LangSysCount )
      return FT_Err_Invalid_Argument;

    ls = &lsr[language_index].LangSys;
  }

  fi = ls->FeatureIndex;

  if ( ALLOC_ARRAY( ftl, ls->FeatureCount + 1, FT_ULong ) )
    return error;

  for ( n = 0; n < ls->FeatureCount; n++ )
  {
    if ( fi[n] >= fl->FeatureCount )
    {
      FREE( ftl );
      return HB_Err_Invalid_GSUB_SubTable_Format;
    }
    ftl[n] = fr[fi[n]].FeatureTag;
  }
  ftl[n] = 0;

  *feature_tag_list = ftl;

  return FT_Err_Ok;
}


typedef FT_Error  (*Lookup_Subst_Func_Type)( HB_GSUBHeader*    gsub,
					     HB_GSUB_SubTable* st,
					     HB_Buffer         buffer,
					     FT_UShort          flags,
					     FT_UShort          context_length,
					     int                nesting_level );
static const Lookup_Subst_Func_Type Lookup_Subst_Call_Table[] = {
  Lookup_DefaultSubst,
  Lookup_SingleSubst,		/* HB_GSUB_LOOKUP_SINGLE        1 */
  Lookup_MultipleSubst,		/* HB_GSUB_LOOKUP_MULTIPLE      2 */
  Lookup_AlternateSubst,	/* HB_GSUB_LOOKUP_ALTERNATE     3 */
  Lookup_LigatureSubst,		/* HB_GSUB_LOOKUP_LIGATURE      4 */
  Lookup_ContextSubst,		/* HB_GSUB_LOOKUP_CONTEXT       5 */
  Lookup_ChainContextSubst,	/* HB_GSUB_LOOKUP_CHAIN         6 */
  Lookup_DefaultSubst		/* HB_GSUB_LOOKUP_EXTENSION     7 */
};
/* Note that the following lookup does not belong to the table above:
 * Lookup_ReverseChainContextSubst,	 HB_GSUB_LOOKUP_REVERSE_CHAIN 8
 * because it's invalid to happen where this table is used.  It's
 * signature is different too...
 */

/* Do an individual subtable lookup.  Returns FT_Err_Ok if substitution
   has been done, or HB_Err_Not_Covered if not.                        */
static FT_Error  GSUB_Do_Glyph_Lookup( HB_GSUBHeader* gsub,
				       FT_UShort       lookup_index,
				       HB_Buffer      buffer,
				       FT_UShort       context_length,
				       int             nesting_level )
{
  FT_Error               error = HB_Err_Not_Covered;
  FT_UShort              i, flags, lookup_count;
  HB_Lookup*             lo;
  int                    lookup_type;
  Lookup_Subst_Func_Type Func;


  nesting_level++;

  if ( nesting_level > HB_MAX_NESTING_LEVEL )
    return HB_Err_Too_Many_Nested_Contexts;

  lookup_count = gsub->LookupList.LookupCount;
  if (lookup_index >= lookup_count)
    return error;

  lo    = &gsub->LookupList.Lookup[lookup_index];
  flags = lo->LookupFlag;
  lookup_type = lo->LookupType;

  if (lookup_type >= ARRAY_LEN (Lookup_Subst_Call_Table))
    lookup_type = 0;
  Func = Lookup_Subst_Call_Table[lookup_type];

  for ( i = 0; i < lo->SubTableCount; i++ )
  {
    error = Func ( gsub,
		   &lo->SubTable[i].st.gsub,
		   buffer,
		   flags, context_length,
		   nesting_level );

    /* Check whether we have a successful substitution or an error other
       than HB_Err_Not_Covered                                          */

    if ( error != HB_Err_Not_Covered )
      return error;
  }

  return HB_Err_Not_Covered;
}


static FT_Error  Load_DefaultSubst( HB_GSUB_SubTable* st,
				    FT_Stream         stream )
{
  FT_UNUSED(st);
  FT_UNUSED(stream);

  return HB_Err_Invalid_GSUB_SubTable_Format;
}

typedef FT_Error  (*Load_Subst_Func_Type)( HB_GSUB_SubTable* st,
					   FT_Stream         stream );
static const Load_Subst_Func_Type Load_Subst_Call_Table[] = {
  Load_DefaultSubst,
  Load_SingleSubst,		/* HB_GSUB_LOOKUP_SINGLE        1 */
  Load_MultipleSubst,		/* HB_GSUB_LOOKUP_MULTIPLE      2 */
  Load_AlternateSubst,		/* HB_GSUB_LOOKUP_ALTERNATE     3 */
  Load_LigatureSubst,		/* HB_GSUB_LOOKUP_LIGATURE      4 */
  Load_ContextSubst,		/* HB_GSUB_LOOKUP_CONTEXT       5 */
  Load_ChainContextSubst,	/* HB_GSUB_LOOKUP_CHAIN         6 */
  Load_DefaultSubst,		/* HB_GSUB_LOOKUP_EXTENSION     7 */
  Load_ReverseChainContextSubst /* HB_GSUB_LOOKUP_REVERSE_CHAIN 8 */
};

FT_Error  _HB_GSUB_Load_SubTable( HB_GSUB_SubTable*  st,
				  FT_Stream     stream,
				  FT_UShort     lookup_type )
{
  Load_Subst_Func_Type Func;

  if (lookup_type >= ARRAY_LEN (Load_Subst_Call_Table))
    lookup_type = 0;

  Func = Load_Subst_Call_Table[lookup_type];

  return Func ( st, stream );
}


static void  Free_DefaultSubst( HB_GSUB_SubTable* st,
				FT_Memory         memory )
{
  FT_UNUSED(st);
  FT_UNUSED(memory);
}

typedef void  (*Free_Subst_Func_Type)( HB_GSUB_SubTable* st,
				       FT_Memory         memory );
static const Free_Subst_Func_Type Free_Subst_Call_Table[] = {
  Free_DefaultSubst,
  Free_SingleSubst,		/* HB_GSUB_LOOKUP_SINGLE        1 */
  Free_MultipleSubst,		/* HB_GSUB_LOOKUP_MULTIPLE      2 */
  Free_AlternateSubst,		/* HB_GSUB_LOOKUP_ALTERNATE     3 */
  Free_LigatureSubst,		/* HB_GSUB_LOOKUP_LIGATURE      4 */
  Free_ContextSubst,		/* HB_GSUB_LOOKUP_CONTEXT       5 */
  Free_ChainContextSubst,	/* HB_GSUB_LOOKUP_CHAIN         6 */
  Free_DefaultSubst,		/* HB_GSUB_LOOKUP_EXTENSION     7 */
  Free_ReverseChainContextSubst /* HB_GSUB_LOOKUP_REVERSE_CHAIN 8 */
};

void  _HB_GSUB_Free_SubTable( HB_GSUB_SubTable*  st,
			      FT_Memory     memory,
			      FT_UShort     lookup_type )
{
  Free_Subst_Func_Type Func;

  if (lookup_type >= ARRAY_LEN (Free_Subst_Call_Table))
    lookup_type = 0;

  Func = Free_Subst_Call_Table[lookup_type];

  Func ( st, memory );
}



/* apply one lookup to the input string object */

static FT_Error  GSUB_Do_String_Lookup( HB_GSUBHeader*   gsub,
				   FT_UShort         lookup_index,
				   HB_Buffer        buffer )
{
  FT_Error  error, retError = HB_Err_Not_Covered;

  FT_UInt*  properties = gsub->LookupList.Properties;

  int      nesting_level = 0;


  while ( buffer->in_pos < buffer->in_length )
  {
    if ( ~IN_PROPERTIES( buffer->in_pos ) & properties[lookup_index] )
    {
      /* 0xFFFF indicates that we don't have a context length yet */
      error = GSUB_Do_Glyph_Lookup( gsub, lookup_index, buffer,
				    0xFFFF, nesting_level );
      if ( error )
      {
	if ( error != HB_Err_Not_Covered )
	  return error;
      }
      else
	retError = error;
    }
    else
      error = HB_Err_Not_Covered;

    if ( error == HB_Err_Not_Covered )
      if ( hb_buffer_copy_output_glyph ( buffer ) )
	return error;
  }

  return retError;
}


static FT_Error  Apply_ReverseChainContextSubst( HB_GSUBHeader*   gsub,
						 FT_UShort         lookup_index,
						 HB_Buffer        buffer )
{
  FT_UInt*     properties =  gsub->LookupList.Properties;
  FT_Error     error, retError = HB_Err_Not_Covered;
  FT_ULong     subtable_Count, string_index;
  FT_UShort    flags;
  HB_Lookup*  lo;

  if ( buffer->in_length == 0 )
    return HB_Err_Not_Covered;

  lo    = &gsub->LookupList.Lookup[lookup_index];
  flags = lo->LookupFlag;

  for ( subtable_Count = 0; subtable_Count < lo->SubTableCount; subtable_Count++ )
  {
    string_index  = buffer->in_length - 1;
    do
    {
      if ( ~IN_PROPERTIES( buffer->in_pos ) & properties[lookup_index] )
	{
	  error = Lookup_ReverseChainContextSubst( gsub, &lo->SubTable[subtable_Count].st.gsub,
						   buffer, flags, string_index );
	  if ( error )
	    {
	      if ( error != HB_Err_Not_Covered )
		return error;
	    }
	  else
	    retError = error;
	}
    }
    while (string_index--);
  }

  return retError;
}


FT_Error  HB_GSUB_Add_Feature( HB_GSUBHeader*  gsub,
			       FT_UShort        feature_index,
			       FT_UInt          property )
{
  FT_UShort    i;

  HB_Feature  feature;
  FT_UInt*     properties;
  FT_UShort*   index;
  FT_UShort    lookup_count;

  /* Each feature can only be added once */

  if ( !gsub ||
       feature_index >= gsub->FeatureList.FeatureCount ||
       gsub->FeatureList.ApplyCount == gsub->FeatureList.FeatureCount )
    return FT_Err_Invalid_Argument;

  gsub->FeatureList.ApplyOrder[gsub->FeatureList.ApplyCount++] = feature_index;

  properties = gsub->LookupList.Properties;

  feature = gsub->FeatureList.FeatureRecord[feature_index].Feature;
  index   = feature.LookupListIndex;
  lookup_count = gsub->LookupList.LookupCount;

  for ( i = 0; i < feature.LookupListCount; i++ )
  {
    FT_UShort lookup_index = index[i];
    if (lookup_index < lookup_count)
      properties[lookup_index] |= property;
  }

  return FT_Err_Ok;
}



FT_Error  HB_GSUB_Clear_Features( HB_GSUBHeader*  gsub )
{
  FT_UShort i;

  FT_UInt*  properties;


  if ( !gsub )
    return FT_Err_Invalid_Argument;

  gsub->FeatureList.ApplyCount = 0;

  properties = gsub->LookupList.Properties;

  for ( i = 0; i < gsub->LookupList.LookupCount; i++ )
    properties[i] = 0;

  return FT_Err_Ok;
}



FT_Error  HB_GSUB_Register_Alternate_Function( HB_GSUBHeader*  gsub,
					       HB_AltFunction  altfunc,
					       void*            data )
{
  if ( !gsub )
    return FT_Err_Invalid_Argument;

  gsub->altfunc = altfunc;
  gsub->data    = data;

  return FT_Err_Ok;
}



FT_Error  HB_GSUB_Apply_String( HB_GSUBHeader*   gsub,
				HB_Buffer        buffer )
{
  FT_Error          error, retError = HB_Err_Not_Covered;
  FT_UShort         i, j, lookup_count;

  if ( !gsub ||
       !buffer || buffer->in_length == 0 || buffer->in_pos >= buffer->in_length )
    return FT_Err_Invalid_Argument;

  lookup_count = gsub->LookupList.LookupCount;

  for ( i = 0; i < gsub->FeatureList.ApplyCount; i++)
  {
    FT_UShort         feature_index;
    HB_Feature       feature;

    feature_index = gsub->FeatureList.ApplyOrder[i];
    feature = gsub->FeatureList.FeatureRecord[feature_index].Feature;

    for ( j = 0; j < feature.LookupListCount; j++ )
    {
      FT_UShort         lookup_index;
      HB_Lookup*       lookup;
      FT_Bool           need_swap;

      lookup_index = feature.LookupListIndex[j];

      /* Skip nonexistant lookups */
      if (lookup_index >= lookup_count)
       continue;

      lookup = &gsub->LookupList.Lookup[lookup_index];

      if ( lookup->LookupType == HB_GSUB_LOOKUP_REVERSE_CHAIN )
      {
	error = Apply_ReverseChainContextSubst( gsub, lookup_index, buffer);
	need_swap = FALSE; /* We do ReverseChainContextSubst in-place */
      }
      else
      {
	error = GSUB_Do_String_Lookup( gsub, lookup_index, buffer );
	need_swap = TRUE;
      }

      if ( error )
      {
	if ( error != HB_Err_Not_Covered )
	  goto End;
      }
      else
	retError = error;

      if ( need_swap )
      {
	error = hb_buffer_swap( buffer );
	if ( error )
	  goto End;
      }
    }
  }

  error = retError;

End:
  return error;
}


/* END */
