/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2004,2007,2009  Red Hat, Inc.
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
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
 * Red Hat Author(s): Owen Taylor, Behdad Esfahbod
 */

#ifndef HB_BUFFER_PRIVATE_H
#define HB_BUFFER_PRIVATE_H

#include "hb-private.h"
#include "hb-buffer.h"

HB_BEGIN_DECLS

#define HB_BUFFER_GLYPH_PROPERTIES_UNKNOWN 0xFFFF

HB_INTERNAL void
_hb_buffer_swap (hb_buffer_t *buffer);

HB_INTERNAL void
_hb_buffer_clear_output (hb_buffer_t *buffer);

HB_INTERNAL void
_hb_buffer_clear_positions (hb_buffer_t *buffer);

HB_INTERNAL void
_hb_buffer_add_output_glyphs (hb_buffer_t *buffer,
			      unsigned int num_in,
			      unsigned int num_out,
			      const uint16_t *glyph_data_be,
			      unsigned short component,
			      unsigned short ligID);

HB_INTERNAL void
_hb_buffer_add_output_glyph (hb_buffer_t *buffer,
			     hb_codepoint_t glyph_index,
			     unsigned short component,
			     unsigned short ligID);

HB_INTERNAL void
_hb_buffer_next_glyph (hb_buffer_t *buffer);

HB_INTERNAL void
_hb_buffer_replace_glyph (hb_buffer_t *buffer,
			  hb_codepoint_t glyph_index);

HB_INTERNAL unsigned short
_hb_buffer_allocate_lig_id (hb_buffer_t *buffer);


/* convenience macros */
#define IN_GLYPH(pos)		(buffer->in_string[(pos)].gindex)
#define IN_INFO(pos)		(&buffer->in_string[(pos)])
#define IN_CURGLYPH()		(buffer->in_string[buffer->in_pos].gindex)
#define IN_CURINFO()		(&buffer->in_string[buffer->in_pos])
#define IN_PROPERTIES(pos)	(buffer->in_string[(pos)].properties)
#define IN_LIGID(pos)		(buffer->in_string[(pos)].ligID)
#define IN_COMPONENT(pos)	(buffer->in_string[(pos)].component)
#define POSITION(pos)		(&buffer->positions[(pos)])
#define CURPOSITION()		(&buffer->positions[buffer->in_pos])
#define OUT_GLYPH(pos)		(buffer->out_string[(pos)].gindex)
#define OUT_INFO(pos)		(&buffer->out_string[(pos)])

HB_END_DECLS

#endif /* HB_BUFFER_PRIVATE_H */
