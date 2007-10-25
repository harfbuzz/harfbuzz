/*
 * By David Turner, The FreeType Project (www.freetype.org)
 *
 * This code is explicitely put in the public domain
 */

#include "harfbuzz-impl.h"

#if 0
#include <stdio.h>
#define  LOG(x)  _hb_log x

static void
_hb_log( const char*   format, ... )
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
static HB_Pointer
_hb_qalloc( HB_UInt   size,
	    HB_Error  *perror )
{
  HB_Error    error = 0;
  HB_Pointer  block = NULL;

  if ( size > 0 )
  {
    block = malloc( size );
    if ( !block )
      error = ERR(HB_Err_Out_Of_Memory);
  }

  *perror = error;
  return block;
}

#undef   QALLOC  /* just in case */
#define  QALLOC(ptr,size)    ( (ptr) = _hb_qalloc( (size), &error ), error != 0 )


HB_INTERNAL HB_Pointer
_hb_alloc( HB_UInt   size,
	   HB_Error *perror )
{
  HB_Error    error = 0;
  HB_Pointer  block = NULL;

  if ( size > 0 )
  {
    block = malloc( size );
    if ( !block )
      error = ERR(HB_Err_Out_Of_Memory);
    else
      memset( (char*)block, 0, (size_t)size );
  }

  *perror = error;
  return block;
}


HB_INTERNAL HB_Pointer
_hb_realloc( HB_Pointer  block,
	     HB_UInt     new_size,
	     HB_Error   *perror )
{
  HB_Pointer  block2 = NULL;
  HB_Error    error  = 0;

  block2 = realloc( block, new_size );
  if ( block2 == NULL && new_size != 0 )
    error = ERR(HB_Err_Out_Of_Memory);

  if ( !error )
    block = block2;

  *perror = error;
  return block;
}


HB_INTERNAL void
_hb_free( HB_Pointer  block )
{
  if ( block )
    free( block );
}


HB_INTERNAL HB_Int
_hb_stream_pos( HB_Stream   stream )
{
  LOG(( "_hb_stream_pos() -> %ld\n", stream->pos ));
  return stream->pos;
}


HB_INTERNAL HB_Error
_hb_stream_seek( HB_Stream stream,
		 HB_Int    pos )
{
  HB_Error  error = 0;

  stream->pos = pos;
  if ( stream->read )
  {
    if ( stream->read( stream, pos, NULL, 0 ) )
      error = ERR(HB_Err_Read_Error);
  }
  else if ( pos > (HB_Int)stream->size )
    error = ERR(HB_Err_Read_Error);

  LOG(( "_hb_stream_seek(%ld) -> %d\n", pos, error ));
  return error;
}


HB_INTERNAL HB_Error
_hb_stream_frame_enter( HB_Stream stream,
			HB_UInt   count )
{
  HB_Error  error = HB_Err_Ok;
  HB_UInt  read_bytes;

  if ( stream->read )
  {
    /* allocate the frame in memory */

    if ( QALLOC( stream->base, count ) )
      goto Exit;

    /* read it */
    read_bytes = stream->read( stream, stream->pos,
                               stream->base, count );
    if ( read_bytes < count )
    {
      FREE( stream->base );
      error = ERR(HB_Err_Read_Error);
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
      error = ERR(HB_Err_Read_Error);
      goto Exit;
    }

    /* set cursor */
    stream->cursor = stream->base + stream->pos;
    stream->limit  = stream->cursor + count;
    stream->pos   += count;
  }

Exit:
  LOG(( "_hb_stream_frame_enter(%ld) -> %d\n", count, error ));
  return error;
}


HB_INTERNAL void
_hb_stream_frame_exit( HB_Stream  stream )
{
  if ( stream->read )
  {
    FREE( stream->base );
  }
  stream->cursor = NULL;
  stream->limit  = NULL;

  LOG(( "_hb_stream_frame_exit()\n" ));
}


HB_INTERNAL HB_Error
_hb_face_goto_table( FT_Face    face,
		     HB_UInt   the_tag,
		     HB_Stream  stream )
{
  HB_Error  error;

  LOG(( "_hb_face_goto_table( %p, %c%c%c%c, %p )\n",
                face, 
                (int)((the_tag >> 24) & 0xFF), 
                (int)((the_tag >> 16) & 0xFF), 
                (int)((the_tag >> 8) & 0xFF), 
                (int)(the_tag & 0xFF),
                stream ));

  if ( !FT_IS_SFNT(face) )
  {
    LOG(( "not a SFNT face !!\n" ));
    error = ERR(HB_Err_Invalid_Argument);
  }
  else
  {
   /* parse the directory table directly, without using
    * FreeType's built-in data structures
    */
    HB_UInt  offset = 0;
    HB_UInt   count, nn;

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
      HB_UInt  tag      = GET_ULong();
      HB_UInt  checksum = GET_ULong();
      HB_UInt  start    = GET_ULong();
      HB_UInt  size     = GET_ULong();

      HB_UNUSED(checksum);
      HB_UNUSED(size);
      
      if ( tag == the_tag )
      {
        LOG(( "TrueType table (start: %ld) (size: %ld)\n", start, size ));
        error = _hb_stream_seek( stream, start );
        goto FoundIt;
      }
    }
    error = HB_Err_Not_Covered;

  FoundIt:
    FORGET_Frame();
  }

Exit:
  LOG(( "TrueType error=%d\n", error ));
  
  return error;
}                        

#undef QALLOC

/* abuse these private header/source files */

/* helper func to set a breakpoint on */
HB_INTERNAL HB_Error
_hb_err (HB_Error code)
{
  return code;
}
