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

FT_BEGIN_HEADER

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
# define TTAG_GDEF  FT_MAKE_TAG( 'G', 'D', 'E', 'F' )
#endif
#ifndef TTAG_GPOS
# define TTAG_GPOS  FT_MAKE_TAG( 'G', 'P', 'O', 'S' )
#endif
#ifndef TTAG_GSUB
# define TTAG_GSUB  FT_MAKE_TAG( 'G', 'S', 'U', 'B' )
#endif

#ifndef FT_UNUSED
# define FT_UNUSED(arg) ((arg) = (arg))
#endif

#include "ftglue.h"

#define ARRAY_LEN(Array) ((int)(sizeof (Array) / sizeof (Array)[0]))




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

#define CHECK_Property( gdef, index, flags, property )              \
          ( ( error = _HB_GDEF_Check_Property( (gdef), (index), (flags),     \
                                      (property) ) ) != FT_Err_Ok )

#define ADD_String( buffer, num_in, num_out, glyph_data, component, ligID )             \
          ( ( error = hb_buffer_add_output_glyphs( (buffer),                           \
						    (num_in), (num_out),                \
                                                    (glyph_data), (component), (ligID)  \
                                                  ) ) != FT_Err_Ok )
#define ADD_Glyph( buffer, glyph_index, component, ligID )             		 	 \
          ( ( error = hb_buffer_add_output_glyph( (buffer),                             \
                                                    (glyph_index), (component), (ligID)  \
                                                  ) ) != FT_Err_Ok )


FT_END_HEADER

#endif /* HARFBUZZ_IMPL_H */
