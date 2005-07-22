/* otlbuffer.c: Buffer of glyphs for substitution/positioning
 *
 * Copyright 2004 Red Hat Software
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
#include <otlbuffer.h>

/* To get the gcc-3.3 strict-aliasing compatible versions
 * FREE/REALLOC_ARRAY/etc. rather than the FT_* versions
 * that
 */
#include "ftglue.h"

  static FT_Error
  otl_buffer_ensure( OTL_Buffer buffer,
		     FT_ULong   size )
  {
    FT_Memory memory = buffer->memory;
    FT_ULong new_allocated = buffer->allocated;

    if (size > new_allocated)
      {
	FT_Error error;

	while (size > new_allocated)
	  new_allocated += (new_allocated >> 1) + 8;
	
	if ( REALLOC_ARRAY( buffer->in_string, buffer->allocated, new_allocated, OTL_GlyphItemRec ) )
	  return error;
	if ( REALLOC_ARRAY( buffer->out_string, buffer->allocated, new_allocated, OTL_GlyphItemRec ) )
	  return error;
	if ( REALLOC_ARRAY( buffer->positions, buffer->allocated, new_allocated, OTL_PositionRec ) )
	  return error;

	buffer->allocated = new_allocated;
      }

    return FT_Err_Ok;
  }

  FT_Error
  otl_buffer_new( FT_Memory   memory,
		  OTL_Buffer *buffer )
  {
    FT_Error error;
    
    if ( ALLOC( *buffer, sizeof( OTL_BufferRec ) ) )
      return error;

    (*buffer)->memory = memory;
    (*buffer)->in_length = 0;
    (*buffer)->out_length = 0;
    (*buffer)->allocated = 0;
    (*buffer)->in_pos = 0;
    (*buffer)->out_pos = 0;

    (*buffer)->in_string = NULL;
    (*buffer)->out_string = NULL;
    (*buffer)->positions = NULL;
    (*buffer)->max_ligID = 0;

    return FT_Err_Ok;
  }

  FT_Error
  otl_buffer_swap( OTL_Buffer buffer )
  {
    OTL_GlyphItem tmp_string;

    tmp_string = buffer->in_string;
    buffer->in_string = buffer->out_string;
    buffer->out_string = tmp_string;

    buffer->in_length = buffer->out_length;
    buffer->out_length = 0;
    
    buffer->in_pos = 0;
    buffer->out_pos = 0;

    return FT_Err_Ok;
  }

  FT_Error
  otl_buffer_free( OTL_Buffer buffer )
  {
    FT_Memory memory = buffer->memory;

    FREE( buffer->in_string );
    FREE( buffer->out_string );
    FREE( buffer->positions );
    FREE( buffer );

    return FT_Err_Ok;
  }

  FT_Error
  otl_buffer_clear( OTL_Buffer buffer )
  {
    buffer->in_length = 0;
    buffer->out_length = 0;
    buffer->in_pos = 0;
    buffer->out_pos = 0;
    
    return FT_Err_Ok;
  }

  FT_Error
  otl_buffer_add_glyph( OTL_Buffer buffer,
			FT_UInt    glyph_index,
			FT_UInt    properties,
			FT_UInt    cluster )
  {
    FT_Error error;
    OTL_GlyphItem glyph;
    
    error = otl_buffer_ensure( buffer, buffer->in_length + 1 );
    if ( error )
      return error;

    glyph = &buffer->in_string[buffer->in_length];
    glyph->gindex = glyph_index;
    glyph->properties = properties;
    glyph->cluster = cluster;
    glyph->component = 0;
    glyph->ligID = 0;
    glyph->gproperties = OTL_GLYPH_PROPERTIES_UNKNOWN;
    
    buffer->in_length++;

    return FT_Err_Ok;
  }

  /* The following function copies `num_out' elements from `glyph_data'
     to `buffer->out_string', advancing the in array pointer in the structure
     by `num_in' elements, and the out array pointer by `num_out' elements.
     Finally, it sets the `length' field of `out' equal to
     `pos' of the `out' structure.

     If `component' is 0xFFFF, the component value from buffer->in_pos
     will copied `num_out' times, otherwise `component' itself will
     be used to fill the `component' fields.

     If `ligID' is 0xFFFF, the ligID value from buffer->in_pos
     will copied `num_out' times, otherwise `ligID' itself will
     be used to fill the `ligID' fields.

     The properties for all replacement glyphs are taken
     from the glyph at position `buffer->in_pos'.

     The cluster value for the glyph at position buffer->in_pos is used
     for all replacement glyphs */
  FT_Error
  otl_buffer_add_output_glyphs( OTL_Buffer buffer,
				FT_UShort  num_in,
				FT_UShort  num_out,
				FT_UShort *glyph_data,
				FT_UShort  component,
				FT_UShort  ligID )
  {
    FT_Error  error;
    FT_UShort i;
    FT_UInt properties;
    FT_UInt cluster;

    error = otl_buffer_ensure( buffer, buffer->out_pos + num_out );
    if ( error )
      return error;

    properties = buffer->in_string[buffer->in_pos].properties;
    cluster = buffer->in_string[buffer->in_pos].cluster;
    if ( component == 0xFFFF )
      component = buffer->in_string[buffer->in_pos].component;
    if ( ligID == 0xFFFF )
      ligID = buffer->in_string[buffer->in_pos].ligID;

    for ( i = 0; i < num_out; i++ )
    {
      OTL_GlyphItem item = &buffer->out_string[buffer->out_pos + i];

      item->gindex = glyph_data[i];
      item->properties = properties;
      item->cluster = cluster;
      item->component = component;
      item->ligID = ligID;
      item->gproperties = OTL_GLYPH_PROPERTIES_UNKNOWN;
    }

    buffer->in_pos  += num_in;
    buffer->out_pos += num_out;

    buffer->out_length = buffer->out_pos;

    return FT_Err_Ok;
  }

  FT_Error
  otl_buffer_add_output_glyph( OTL_Buffer buffer,	
			       FT_UInt    glyph_index,
			       FT_UShort  component,
			       FT_UShort  ligID )
  {
    FT_UShort glyph_data =  glyph_index;

    return otl_buffer_add_output_glyphs ( buffer, 1, 1,
					  &glyph_data, component, ligID );
  }

  FT_Error
  otl_buffer_copy_output_glyph ( OTL_Buffer buffer )
  {  
    FT_Error  error;

    error = otl_buffer_ensure( buffer, buffer->out_pos + 1 );
    if ( error )
      return error;
    
    buffer->out_string[buffer->out_pos++] = buffer->in_string[buffer->in_pos++];
    buffer->out_length = buffer->out_pos;

    return FT_Err_Ok;
  }

  FT_UShort
  otl_buffer_allocate_ligid( OTL_Buffer buffer )
  {
    return buffer->max_ligID++;
  }
