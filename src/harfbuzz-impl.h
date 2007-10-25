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
#ifndef HARFBUZZ_IMPL_H
#define HARFBUZZ_IMPL_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TAGS_H

#include "harfbuzz-global.h"

HB_BEGIN_HEADER

/***********************************************************************/
/************************ remaining freetype bits **********************/
/***********************************************************************/

typedef FT_Stream HB_Stream;
#define HB_MAKE_TAG(a,b,c,d) FT_MAKE_TAG(a,b,c,d)

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

#ifndef HB_INTERNAL
# define HB_INTERNAL
#endif

#ifndef NULL
# define NULL ((void *)0)
#endif

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#ifndef TTAG_GDEF
# define TTAG_GDEF  HB_MAKE_TAG( 'G', 'D', 'E', 'F' )
#endif
#ifndef TTAG_GPOS
# define TTAG_GPOS  HB_MAKE_TAG( 'G', 'P', 'O', 'S' )
#endif
#ifndef TTAG_GSUB
# define TTAG_GSUB  HB_MAKE_TAG( 'G', 'S', 'U', 'B' )
#endif

#ifndef HB_UNUSED
# define HB_UNUSED(arg) ((arg) = (arg))
#endif

#define HB_LIKELY(cond) (cond)
#define HB_UNLIKELY(cond) (cond)

#define ARRAY_LEN(Array) ((int)(sizeof (Array) / sizeof (Array)[0]))


/* memory and stream management */

#define  SET_ERR(c)   ( (error = (c)) != 0 )

/* stream macros used by the OpenType parser */
#define  GOTO_Table(tag) SET_ERR( _hb_face_goto_table( face, tag, stream ) )
#define  FILE_Pos()      _hb_stream_pos( stream )
#define  FILE_Seek(pos)  SET_ERR( _hb_stream_seek( stream, pos ) )
#define  ACCESS_Frame(size)  SET_ERR( _hb_stream_frame_enter( stream, size ) )
#define  FORGET_Frame()      _hb_stream_frame_exit( stream )

#define  GET_Byte()      (*stream->cursor++)
#define  GET_Short()     (stream->cursor += 2, (HB_Short)( \
				(*(((FT_Byte*)stream->cursor)-2) << 8) | \
				 *(((FT_Byte*)stream->cursor)-1) \
			 ))
#define  GET_Long()      (stream->cursor += 4, (HB_Int)( \
				(*(((FT_Byte*)stream->cursor)-4) << 24) | \
				(*(((FT_Byte*)stream->cursor)-3) << 16) | \
				(*(((FT_Byte*)stream->cursor)-2) << 8) | \
				 *(((FT_Byte*)stream->cursor)-1) \
			 ))


#define  GET_Char()      ((FT_Char)GET_Byte())
#define  GET_UShort()    ((HB_UShort)GET_Short())
#define  GET_ULong()     ((HB_UInt)GET_Long())
#define  GET_Tag4()      GET_ULong()


HB_INTERNAL HB_Int
_hb_stream_pos( HB_Stream   stream );

HB_INTERNAL HB_Error
_hb_stream_seek( HB_Stream   stream,
                    HB_Int     pos );

HB_INTERNAL HB_Error
_hb_stream_frame_enter( HB_Stream   stream,
                           HB_UInt    size );

HB_INTERNAL void
_hb_stream_frame_exit( HB_Stream  stream );

HB_INTERNAL HB_Error
_hb_face_goto_table( FT_Face    face,
                        HB_UInt   tag,
                        HB_Stream  stream );

#define  ALLOC(_ptr,_size)   \
           ( (_ptr) = _hb_alloc( _size, &error ), error != 0 )

#define  REALLOC(_ptr,_newsz)  \
           ( (_ptr) = _hb_realloc( (_ptr), (_newsz), &error ), error != 0 )

#define  FREE(_ptr)                    \
  do {                                 \
    if ( (_ptr) )                      \
    {                                  \
      _hb_free( _ptr );                \
      _ptr = NULL;                     \
    }                                  \
  } while (0)

#define  ALLOC_ARRAY(_ptr,_count,_type)   \
           ALLOC(_ptr,(_count)*sizeof(_type))

#define  REALLOC_ARRAY(_ptr,_newcnt,_type) \
           REALLOC(_ptr,(_newcnt)*sizeof(_type))

#define  MEM_Copy(dest,source,count)   memcpy( (char*)(dest), (const char*)(source), (size_t)(count) )

#define ERR(err)   _hb_err (err)


HB_INTERNAL FT_Pointer
_hb_alloc( HB_UInt   size,
	   HB_Error  *perror_ );

HB_INTERNAL FT_Pointer
_hb_realloc( FT_Pointer  block,
	     HB_UInt    new_size,
	     HB_Error   *perror_ );

HB_INTERNAL void
_hb_free( FT_Pointer  block );


/* helper func to set a breakpoint on */
HB_INTERNAL HB_Error
_hb_err (HB_Error code);


/* buffer access macros */

#define IN_GLYPH( pos )        (buffer->in_string[(pos)].gindex)
#define IN_ITEM( pos )         (&buffer->in_string[(pos)])
#define IN_CURGLYPH()          (buffer->in_string[buffer->in_pos].gindex)
#define IN_CURITEM()           (&buffer->in_string[buffer->in_pos])
#define IN_PROPERTIES( pos )   (buffer->in_string[(pos)].properties)
#define IN_LIGID( pos )        (buffer->in_string[(pos)].ligID)
#define IN_COMPONENT( pos )    (buffer->in_string[(pos)].component)
#define POSITION( pos )        (&buffer->positions[(pos)])
#define OUT_GLYPH( pos )       (buffer->out_string[(pos)].gindex)
#define OUT_ITEM( pos )        (&buffer->out_string[(pos)])

#define CHECK_Property( gdef, index, flags, property )					\
          ( ( error = _HB_GDEF_Check_Property( (gdef), (index), (flags),		\
                                      (property) ) ) != HB_Err_Ok )

#define ADD_String( buffer, num_in, num_out, glyph_data, component, ligID )             \
          ( ( error = _hb_buffer_add_output_glyphs( (buffer),                            \
						    (num_in), (num_out),                \
                                                    (glyph_data), (component), (ligID)  \
                                                  ) ) != HB_Err_Ok )
#define ADD_Glyph( buffer, glyph_index, component, ligID )				\
          ( ( error = _hb_buffer_add_output_glyph( (buffer),                             \
                                                    (glyph_index), (component), (ligID) \
                                                  ) ) != HB_Err_Ok )
#define REPLACE_Glyph( buffer, glyph_index, nesting_level )				\
          ( ( error = _hb_buffer_replace_output_glyph( (buffer), (glyph_index),		\
						      (nesting_level) == 1 ) ) != HB_Err_Ok )
#define COPY_Glyph( buffer )								\
	  ( (error = _hb_buffer_copy_output_glyph ( buffer ) ) != HB_Err_Ok )

HB_END_HEADER

#endif /* HARFBUZZ_IMPL_H */
