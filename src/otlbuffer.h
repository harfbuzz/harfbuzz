/* otlbuffer.h: Buffer of glyphs for substitution/positioning
 *
 * Copyrigh 2004 Red Hat Software
 *
 * Portions Copyright 1996-2000 by
 * David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 * This file is part of the FreeType project, and may only be used
 * modified and distributed under the terms of the FreeType project
 * license, LICENSE.TXT.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 */
#include <ft2build.h>
#include FT_FREETYPE_H

#include <glib.h>

G_BEGIN_DECLS

#define OTL_GLYPH_PROPERTIES_UNKNOWN 0xFFFF

  typedef struct OTL_GlyphItemRec_ {
    FT_UInt     gindex;
    FT_UInt     properties;
    FT_UInt     cluster;
    FT_UShort   component;
    FT_UShort   ligID;
    FT_UShort   gproperties;
  } OTL_GlyphItemRec, *OTL_GlyphItem;

  typedef struct OTL_PositionRec_ {
    FT_Pos     x_pos;
    FT_Pos     y_pos;
    FT_Pos     x_advance;
    FT_Pos     y_advance;
    FT_UShort  back;            /* number of glyphs to go back
                                   for drawing current glyph   */
    FT_Bool    new_advance;     /* if set, the advance width values are
                                   absolute, i.e., they won't be
                                   added to the original glyph's value
                                   but rather replace them.            */
    FT_Short  cursive_chain;   /* character to which this connects,
 				   may be positive or negative; used
 				   only internally                     */
  } OTL_PositionRec, *OTL_Position;


  typedef struct OTL_BufferRec_{ 
    FT_Memory   memory;
    FT_ULong    allocated;

    FT_ULong    in_length;
    FT_ULong    out_length;
    FT_ULong    in_pos;
    FT_ULong    out_pos;
    
    OTL_GlyphItem  in_string;
    OTL_GlyphItem  out_string;
    OTL_Position   positions;
    FT_UShort      max_ligID;
  } OTL_BufferRec, *OTL_Buffer;

  FT_Error
  otl_buffer_new( FT_Memory   memory,
		  OTL_Buffer *buffer );

  FT_Error
  otl_buffer_swap( OTL_Buffer buffer );

  FT_Error
  otl_buffer_free( OTL_Buffer buffer );

  FT_Error
  otl_buffer_clear( OTL_Buffer buffer );

  FT_Error
  otl_buffer_add_glyph( OTL_Buffer buffer,
			FT_UInt    glyph_index,
			FT_UInt    properties,
			FT_UInt    cluster );

  FT_Error
  otl_buffer_add_output_glyphs( OTL_Buffer buffer,
				FT_UShort  num_in,
				FT_UShort  num_out,
				FT_UShort *glyph_data,
				FT_UShort  component,
				FT_UShort  ligID );

  FT_Error
  otl_buffer_add_output_glyph ( OTL_Buffer buffer,
			        FT_UInt    glyph_index,
			        FT_UShort  component,
			        FT_UShort  ligID );

  FT_Error
  otl_buffer_copy_output_glyph ( OTL_Buffer buffer );

  FT_UShort
  otl_buffer_allocate_ligid( OTL_Buffer buffer );

G_END_DECLS
