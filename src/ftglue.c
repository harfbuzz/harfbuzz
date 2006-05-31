/* ftglue.c: Glue code for compiling the OpenType code from
 *           FreeType 1 using only the public API of FreeType 2
 *
 * By David Turner, The FreeType Project (www.freetype.org)
 *
 * This code is explicitely put in the public domain
 *
 * See ftglue.h for more information.
 */

#include "ftglue.h"

#if 0
#include <stdio.h>
#define  LOG(x)  _hb_ftglue_log x

static void
_hb_ftglue_log( const char*   format, ... )
{
  va_list  ap;
 
  va_start( ap, format );
  vfprintf( stderr, format, ap );
  va_end( ap );
}

#else
#define  LOG(x)  do {} while (0)
#endif

/* only used internally */
static FT_Pointer
_hb_ftglue_qalloc( FT_Memory  memory,
               FT_ULong   size,
               FT_Error  *perror )
{
  FT_Error    error = 0;
  FT_Pointer  block = NULL;

  if ( size > 0 )
  {
    block = memory->alloc( memory, size );
    if ( !block )
      error = FT_Err_Out_Of_Memory;
  }

  *perror = error;
  return block;
}

#undef   QALLOC  /* just in case */
#define  QALLOC(ptr,size)    ( (ptr) = _hb_ftglue_qalloc( memory, (size), &error ), error != 0 )


FTGLUE_APIDEF( FT_Pointer )
_hb_ftglue_alloc( FT_Memory  memory,
              FT_ULong   size,
              FT_Error  *perror )
{
  FT_Error    error = 0;
  FT_Pointer  block = NULL;

  if ( size > 0 )
  {
    block = memory->alloc( memory, size );
    if ( !block )
      error = FT_Err_Out_Of_Memory;
    else
      memset( (char*)block, 0, (size_t)size );
  }

  *perror = error;
  return block;
}


FTGLUE_APIDEF( FT_Pointer )
_hb_ftglue_realloc( FT_Memory   memory,
                FT_Pointer  block,
                FT_ULong    old_size,
                FT_ULong    new_size,
                FT_Error   *perror )
{
  FT_Pointer  block2 = NULL;
  FT_Error    error  = 0;

  if ( old_size == 0 || block == NULL )
  {
    block2 = _hb_ftglue_alloc( memory, new_size, &error );
  }
  else if ( new_size == 0 )
  {
    _hb_ftglue_free( memory, block );
  }
  else
  {
    block2 = memory->realloc( memory, old_size, new_size, block );
    if ( block2 == NULL )
      error = FT_Err_Out_Of_Memory;
    else if ( new_size > old_size )
      memset( (char*)block2 + old_size, 0, (size_t)(new_size - old_size) );
  }

  if ( !error )
    block = block2;

  *perror = error;
  return block;
}


FTGLUE_APIDEF( void )
_hb_ftglue_free( FT_Memory   memory,
             FT_Pointer  block )
{
  if ( block )
    memory->free( memory, block );
}


FTGLUE_APIDEF( FT_Long )
_hb_ftglue_stream_pos( FT_Stream   stream )
{
  LOG(( "ftglue:stream:pos() -> %ld\n", stream->pos ));
  return stream->pos;
}


FTGLUE_APIDEF( FT_Error )
_hb_ftglue_stream_seek( FT_Stream   stream,
                    FT_Long     pos )
{
  FT_Error  error = 0;

  stream->pos = pos;
  if ( stream->read )
  {
    if ( stream->read( stream, pos, NULL, 0 ) )
      error = FT_Err_Invalid_Stream_Operation;
  }
  else if ( pos > (FT_Long)stream->size )
    error = FT_Err_Invalid_Stream_Operation;

  LOG(( "ftglue:stream:seek(%ld) -> %d\n", pos, error ));
  return error;
}


FTGLUE_APIDEF( FT_Error )
_hb_ftglue_stream_frame_enter( FT_Stream   stream,
                           FT_ULong    count )
{
  FT_Error  error = FT_Err_Ok;
  FT_ULong  read_bytes;

  if ( stream->read )
  {
    /* allocate the frame in memory */
    FT_Memory  memory = stream->memory;


    if ( QALLOC( stream->base, count ) )
      goto Exit;

    /* read it */
    read_bytes = stream->read( stream, stream->pos,
                               stream->base, count );
    if ( read_bytes < count )
    {
      FREE( stream->base );
      error = FT_Err_Invalid_Stream_Operation;
    }
    stream->cursor = stream->base;
    stream->limit  = stream->cursor + count;
    stream->pos   += read_bytes;
  }
  else
  {
    /* check current and new position */
    if ( stream->pos >= stream->size        ||
         stream->pos + count > stream->size )
    {
      error = FT_Err_Invalid_Stream_Operation;
      goto Exit;
    }

    /* set cursor */
    stream->cursor = stream->base + stream->pos;
    stream->limit  = stream->cursor + count;
    stream->pos   += count;
  }

Exit:
  LOG(( "ftglue:stream:frame_enter(%ld) -> %d\n", count, error ));
  return error;
}


FTGLUE_APIDEF( void )
_hb_ftglue_stream_frame_exit( FT_Stream  stream )
{
  if ( stream->read )
  {
    FT_Memory  memory = stream->memory;

    FREE( stream->base );
  }
  stream->cursor = NULL;
  stream->limit  = NULL;

  LOG(( "ftglue:stream:frame_exit()\n" ));
}


FTGLUE_APIDEF( FT_Error )
_hb_ftglue_face_goto_table( FT_Face    face,
                        FT_ULong   the_tag,
                        FT_Stream  stream )
{
  FT_Error  error;

  LOG(( "_hb_ftglue_face_goto_table( %p, %c%c%c%c, %p )\n",
                face, 
                (int)((the_tag >> 24) & 0xFF), 
                (int)((the_tag >> 16) & 0xFF), 
                (int)((the_tag >> 8) & 0xFF), 
                (int)(the_tag & 0xFF),
                stream ));

  if ( !FT_IS_SFNT(face) )
  {
    LOG(( "not a SFNT face !!\n" ));
    error = FT_Err_Invalid_Face_Handle;
  }
  else
  {
   /* parse the directory table directly, without using
    * FreeType's built-in data structures
    */
    FT_ULong  offset = 0;
    FT_UInt   count, nn;

    if ( face->num_faces > 1 )
    {
      /* deal with TrueType collections */
      LOG(( ">> This is a TrueType Collection\n" ));

      if ( FILE_Seek( 12 + face->face_index*4 ) ||
           ACCESS_Frame( 4 )                    )
        goto Exit;

      offset = GET_ULong();

      FORGET_Frame();
    }

    LOG(( "TrueType offset = %ld\n", offset ));

    if ( FILE_Seek( offset+4 ) ||
         ACCESS_Frame( 2 )     )
      goto Exit;

    count = GET_UShort();

    FORGET_Frame();

    if ( FILE_Seek( offset+12 )   ||
         ACCESS_Frame( count*16 ) )
      goto Exit;

    for ( nn = 0; nn < count; nn++ )
    {
      FT_ULong  tag      = GET_ULong();
      FT_ULong  checksum = GET_ULong();
      FT_ULong  start    = GET_ULong();
      FT_ULong  size     = GET_ULong();

      FT_UNUSED(checksum);
      FT_UNUSED(size);
      
      if ( tag == the_tag )
      {
        LOG(( "TrueType table (start: %ld) (size: %ld)\n", start, size ));
        error = _hb_ftglue_stream_seek( stream, start );
        goto FoundIt;
      }
    }
    error = FT_Err_Table_Missing;

  FoundIt:
    FORGET_Frame();
  }

Exit:
  LOG(( "TrueType error=%d\n", error ));
  
  return error;
}                        

#undef QALLOC
