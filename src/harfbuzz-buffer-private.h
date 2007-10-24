/* harfbuzz-buffer-private.h: Buffer of glyphs for substitution/positioning
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
#ifndef HARFBUZZ_BUFFER_PRIVATE_H
#define HARFBUZZ_BUFFER_PRIVATE_H

#include "harfbuzz-impl.h"
#include "harfbuzz-buffer.h"

HB_BEGIN_HEADER

#define HB_GLYPH_PROPERTIES_UNKNOWN 0xFFFF

HB_INTERNAL void
_hb_buffer_swap( HB_Buffer buffer );

HB_INTERNAL void
_hb_buffer_clear_output( HB_Buffer buffer );

HB_INTERNAL HB_Error
_hb_buffer_clear_positions( HB_Buffer buffer );

HB_INTERNAL HB_Error
_hb_buffer_add_output_glyphs( HB_Buffer  buffer,
			      HB_UShort  num_in,
			      HB_UShort  num_out,
			      HB_UShort *glyph_data,
			      HB_UShort  component,
			      HB_UShort  ligID );

HB_INTERNAL HB_Error
_hb_buffer_add_output_glyph ( HB_Buffer buffer,
			      HB_UInt   glyph_index,
			      HB_UShort component,
			      HB_UShort ligID );

HB_INTERNAL HB_Error
_hb_buffer_copy_output_glyph ( HB_Buffer buffer );

HB_INTERNAL HB_Error
_hb_buffer_replace_output_glyph ( HB_Buffer buffer,
				  HB_UInt   glyph_index,
				  HB_Bool   inplace );

HB_INTERNAL HB_UShort
_hb_buffer_allocate_ligid( HB_Buffer buffer );

HB_END_HEADER

#endif /* HARFBUZZ_BUFFER_PRIVATE_H */
