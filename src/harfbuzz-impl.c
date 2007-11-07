/*******************************************************************
 *
 *  Copyright 2005  David Turner, The FreeType Project (www.freetype.org)
 *  Copyright 2007  Trolltech ASA
 *  Copyright 2007  Red Hat, Inc
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 *  See the file name COPYING for licensing information.
 *
 ******************************************************************/
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


/* helper func to set a breakpoint on */
HB_INTERNAL HB_Error
_hb_err (HB_Error code)
{
  return code;
}
