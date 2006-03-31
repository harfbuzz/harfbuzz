/* harfbuzz-buffer.h: Buffer of glyphs for substitution/positioning
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
#ifndef HARFBUZZ_BUFFER_H
#define HARFBUZZ_BUFFER_H

#include <ft2build.h>
#include FT_FREETYPE_H

FT_BEGIN_HEADER

#define HB_GLYPH_PROPERTIES_UNKNOWN 0xFFFF

typedef struct HB_GlyphItemRec_ {
  FT_UInt     gindex;
  FT_UInt     properties;
  FT_UInt     cluster;
  FT_UShort   component;
  FT_UShort   ligID;
  FT_UShort   gproperties;
} HB_GlyphItemRec, *HB_GlyphItem;

typedef struct HB_PositionRec_ {
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
} HB_PositionRec, *HB_Position;


typedef struct HB_BufferRec_{ 
  FT_Memory   memory;
  FT_ULong    allocated;

  FT_ULong    in_length;
  FT_ULong    out_length;
  FT_ULong    in_pos;
  FT_ULong    out_pos;
  
  HB_GlyphItem  in_string;
  HB_GlyphItem  out_string;
  HB_Position   positions;
  FT_UShort      max_ligID;
} HB_BufferRec, *HB_Buffer;

FT_Error
hb_buffer_new( FT_Memory   memory,
		HB_Buffer *buffer );

FT_Error
hb_buffer_swap( HB_Buffer buffer );

FT_Error
hb_buffer_free( HB_Buffer buffer );

FT_Error
hb_buffer_clear( HB_Buffer buffer );

FT_Error
hb_buffer_add_glyph( HB_Buffer buffer,
		      FT_UInt    glyph_index,
		      FT_UInt    properties,
		      FT_UInt    cluster );

FT_Error
hb_buffer_add_output_glyphs( HB_Buffer buffer,
			      FT_UShort  num_in,
			      FT_UShort  num_out,
			      FT_UShort *glyph_data,
			      FT_UShort  component,
			      FT_UShort  ligID );

FT_Error
hb_buffer_add_output_glyph ( HB_Buffer buffer,
			      FT_UInt    glyph_index,
			      FT_UShort  component,
			      FT_UShort  ligID );

FT_Error
hb_buffer_copy_output_glyph ( HB_Buffer buffer );

FT_UShort
hb_buffer_allocate_ligid( HB_Buffer buffer );

FT_END_HEADER

#endif /* HARFBUZZ_BUFFER_H */
