/*
 * Copyright (C) 2005  David Turner
 * Copyright (C) 2007  Trolltech ASA
 * Copyright (C) 2007  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "harfbuzz-impl.h"
#include "harfbuzz-stream-private.h"
#include <stdlib.h>

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

HB_INTERNAL HB_Int
_hb_stream_pos( HB_Stream stream )
{
  LOG(( "_hb_stream_pos() -> %ld\n", stream->pos ));
  return stream->pos;
}


HB_INTERNAL HB_Error
_hb_stream_seek( HB_Stream stream,
		 HB_UInt   pos )
{
  HB_Error  error = 0;

  stream->pos = pos;
  if ( stream->read )
  {
    if ( stream->read( stream, pos, NULL, 0 ) )
      error = ERR(HB_Err_Read_Error);
  }
  else if ( pos > stream->size )
    error = ERR(HB_Err_Read_Error);

  LOG(( "_hb_stream_seek(%ld) -> 0x%04X\n", pos, error ));
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
    /* check new position, watch for overflow */
    if (HB_UNLIKELY (stream->pos + count > stream->size ||
		     stream->pos + count < stream->pos))
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
  LOG(( "_hb_stream_frame_enter(%ld) -> 0x%04X\n", count, error ));
  return error;
}


HB_INTERNAL void
_hb_stream_frame_exit( HB_Stream stream )
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
_hb_font_goto_table( HB_Font    font,
		     HB_UInt    the_tag )
{
  HB_Stream  stream = font->stream;

  HB_UInt  offset = 0;
  HB_UInt   count, nn;
  HB_Error  error;

  LOG(( "_hb_font_goto_table( %p, %c%c%c%c, %p )\n",
                font, 
                (int)((the_tag >> 24) & 0xFF), 
                (int)((the_tag >> 16) & 0xFF), 
                (int)((the_tag >> 8) & 0xFF), 
                (int)(the_tag & 0xFF),
                stream ));

  if ( !FT_IS_SFNT(font) )
  {
    LOG(( "not a SFNT font !!\n" ));
    error = ERR(HB_Err_Invalid_Argument);
    goto Exit;
  }

 /* parse the directory table directly, without using
  * FreeType's built-in data structures
  */

  if ( font->num_faces > 1 )
  {
    /* deal with TrueType collections */
    LOG(( ">> This is a TrueType Collection\n" ));

    if ( FILE_Seek( 12 + font->face_index*4 ) ||
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

Exit:
  LOG(( "TrueType error=%d\n", error ));
  
  return error;
}                        

#undef QALLOC

