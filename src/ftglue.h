/* ftglue.h: Glue code for compiling the OpenType code from
 *           FreeType 1 using only the public API of FreeType 2
 *
 * By David Turner, The FreeType Project (www.freetype.org)
 *
 * This code is explicitely put in the public domain
 *
 * ==========================================================================
 *
 * the OpenType parser codes was originally written as an extension to
 * FreeType 1.x. As such, its source code was embedded within the library,
 * and used many internal FreeType functions to deal with memory and
 * stream i/o.
 *
 * When it was 'salvaged' for Pango and Qt, the code was "ported" to FreeType 2,
 * which basically means that some macro tricks were performed in order to
 * directly access FT2 _internal_ functions.
 *
 * these functions were never part of FT2 public API, and _did_ change between
 * various releases. This created chaos for many users: when they upgraded the
 * FreeType library on their system, they couldn't run Gnome anymore since
 * Pango refused to link.
 *
 * Very fortunately, it's possible to completely avoid this problem because
 * the FT_StreamRec and FT_MemoryRec structure types, which describe how
 * memory and stream implementations interface with the rest of the font
 * library, have always been part of the public API, and never changed.
 *
 * What we do thus is re-implement, within the OpenType parser, the few
 * functions that depend on them. This only adds one or two kilobytes of
 * code, and ensures that the parser can work with _any_ version
 * of FreeType installed on your system. How sweet... !
 *
 * Note that we assume that Pango doesn't use any other internal functions
 * from FreeType. It used to in old versions, but this should no longer
 * be the case. (crossing my fingers).
 *
 *  - David Turner
 *  - The FreeType Project  (www.freetype.org)
 *
 * PS: This "glue" code is explicitely put in the public domain
 */
#ifndef FTGLUE_H
#define FTGLUE_H

#include <ft2build.h>
#include FT_FREETYPE_H

FT_BEGIN_HEADER


/* utility macros */

#define  SET_ERR(c)   ( (error = (c)) != 0 )

#ifndef FTGLUE_API
#define FTGLUE_API(x)  extern x
#endif

#ifndef FTGLUE_APIDEF
#define FTGLUE_APIDEF(x)  x
#endif

/* stream macros used by the OpenType parser */
#define  FILE_Pos()      _hb_ftglue_stream_pos( stream )
#define  FILE_Seek(pos)  SET_ERR( _hb_ftglue_stream_seek( stream, pos ) )
#define  ACCESS_Frame(size)  SET_ERR( _hb_ftglue_stream_frame_enter( stream, size ) )
#define  FORGET_Frame()      _hb_ftglue_stream_frame_exit( stream )

#define  GET_Byte()      (*stream->cursor++)
#define  GET_Short()     (stream->cursor += 2, (FT_Short)( \
				(*(((FT_Byte*)stream->cursor)-2) << 8) | \
				 *(((FT_Byte*)stream->cursor)-1) \
			 ))
#define  GET_Long()      (stream->cursor += 4, (FT_Long)( \
				(*(((FT_Byte*)stream->cursor)-4) << 24) | \
				(*(((FT_Byte*)stream->cursor)-3) << 16) | \
				(*(((FT_Byte*)stream->cursor)-2) << 8) | \
				 *(((FT_Byte*)stream->cursor)-1) \
			 ))


#define  GET_Char()      ((FT_Char)GET_Byte())
#define  GET_UShort()    ((FT_UShort)GET_Short())
#define  GET_ULong()     ((FT_ULong)GET_Long())
#define  GET_Tag4()      GET_ULong()

FTGLUE_API( FT_Long )
_hb_ftglue_stream_pos( FT_Stream   stream );

FTGLUE_API( FT_Error )
_hb_ftglue_stream_seek( FT_Stream   stream,
                    FT_Long     pos );

FTGLUE_API( FT_Error )
_hb_ftglue_stream_frame_enter( FT_Stream   stream,
                           FT_ULong    size );

FTGLUE_API( void )
_hb_ftglue_stream_frame_exit( FT_Stream  stream );

FTGLUE_API( FT_Error )
_hb_ftglue_face_goto_table( FT_Face    face,
                        FT_ULong   tag,
                        FT_Stream  stream );

/* memory macros used by the OpenType parser */
#define  ALLOC(_ptr,_size)   \
           ( (_ptr) = _hb_ftglue_alloc( memory, _size, &error ), error != 0 )

#define  REALLOC(_ptr,_oldsz,_newsz)  \
           ( (_ptr) = _hb_ftglue_realloc( memory, (_ptr), (_oldsz), (_newsz), &error ), error != 0 )

#define  FREE(_ptr)                    \
  do {                                 \
    if ( (_ptr) )                      \
    {                                  \
      _hb_ftglue_free( memory, _ptr );     \
      _ptr = NULL;                     \
    }                                  \
  } while (0)

#define  ALLOC_ARRAY(_ptr,_count,_type)   \
           ALLOC(_ptr,(_count)*sizeof(_type))

#define  REALLOC_ARRAY(_ptr,_oldcnt,_newcnt,_type) \
           REALLOC(_ptr,(_oldcnt)*sizeof(_type),(_newcnt)*sizeof(_type))

#define  MEM_Copy(dest,source,count)   memcpy( (char*)(dest), (const char*)(source), (size_t)(count) )


FTGLUE_API( FT_Pointer )
_hb_ftglue_alloc( FT_Memory  memory,
              FT_ULong   size,
              FT_Error  *perror_ );

FTGLUE_API( FT_Pointer )
_hb_ftglue_realloc( FT_Memory   memory,
                FT_Pointer  block,
                FT_ULong    old_size,
                FT_ULong    new_size,
                FT_Error   *perror_ );

FTGLUE_API( void )
_hb_ftglue_free( FT_Memory   memory,
             FT_Pointer  block );

FT_END_HEADER

#endif /* FTGLUE_H */
