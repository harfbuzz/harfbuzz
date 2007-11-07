/* harfbuzz-buffer.h: Buffer of glyphs for substitution/positioning
 *
 * Copyright 2004,7 Red Hat Software
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

#include "harfbuzz-global.h"

HB_BEGIN_HEADER

typedef struct HB_GlyphItemRec_ {
  HB_UInt     gindex;
  HB_UInt     properties;
  HB_UInt     cluster;
  HB_UShort   component;
  HB_UShort   ligID;
  HB_UShort   gproperties;
} HB_GlyphItemRec, *HB_GlyphItem;

typedef struct HB_PositionRec_ {
  HB_Fixed   x_pos;
  HB_Fixed   y_pos;
  HB_Fixed   x_advance;
  HB_Fixed   y_advance;
  HB_UShort  back;            /* number of glyphs to go back
				 for drawing current glyph   */
  HB_Bool    new_advance;     /* if set, the advance width values are
				 absolute, i.e., they won't be
				 added to the original glyph's value
				 but rather replace them.            */
  HB_Short  cursive_chain;   /* character to which this connects,
				 may be positive or negative; used
				 only internally                     */
} HB_PositionRec, *HB_Position;


typedef struct HB_BufferRec_{ 
  HB_UInt    allocated;

  HB_UInt    in_length;
  HB_UInt    out_length;
  HB_UInt    in_pos;
  HB_UInt    out_pos;
  
  HB_Bool       separate_out;
  HB_GlyphItem  in_string;
  HB_GlyphItem  out_string;
  HB_GlyphItem  alt_string;
  HB_Position   positions;
  HB_UShort      max_ligID;
} HB_BufferRec, *HB_Buffer;

HB_Error
hb_buffer_new( HB_Buffer *buffer );

void
hb_buffer_free( HB_Buffer buffer );

void
hb_buffer_clear( HB_Buffer buffer );

HB_Error
hb_buffer_add_glyph( HB_Buffer buffer,
		      HB_UInt    glyph_index,
		      HB_UInt    properties,
		      HB_UInt    cluster );

HB_END_HEADER

#endif /* HARFBUZZ_BUFFER_H */
