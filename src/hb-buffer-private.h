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


typedef struct _hb_internal_glyph_info_t {
  hb_codepoint_t codepoint;
  hb_mask_t      mask;
  uint32_t       cluster;
  uint16_t       component;
  uint16_t       lig_id;
  uint32_t       gproperty;
} hb_internal_glyph_info_t;

typedef struct _hb_internal_glyph_position_t {
  hb_position_t  x_pos;
  hb_position_t  y_pos;
  hb_position_t  x_advance;
  hb_position_t  y_advance;
  uint32_t       new_advance :1;	/* if set, the advance width values are
					   absolute, i.e., they won't be
					   added to the original glyph's value
					   but rather replace them */
  uint32_t       back : 15;		/* number of glyphs to go back
					   for drawing current glyph */
  int32_t        cursive_chain : 16;	/* character to which this connects,
					   may be positive or negative; used
					   only internally */
} hb_internal_glyph_position_t;

ASSERT_STATIC (sizeof (hb_glyph_info_t) == sizeof (hb_internal_glyph_info_t));
ASSERT_STATIC (sizeof (hb_glyph_position_t) == sizeof (hb_internal_glyph_position_t));
ASSERT_STATIC (sizeof (hb_glyph_info_t) == sizeof (hb_glyph_position_t));


struct _hb_buffer_t {
  hb_reference_count_t ref_count;

  unsigned int allocated;

  hb_bool_t    have_output; /* weather we have an output buffer going on */
  unsigned int in_length;
  unsigned int out_length;
  unsigned int in_pos;
  unsigned int out_pos; /* out_length and out_pos are actually always the same */

  hb_internal_glyph_info_t     *in_string;
  hb_internal_glyph_info_t     *out_string;
  hb_internal_glyph_position_t *positions;

  hb_direction_t       direction;
  unsigned int         max_lig_id;
};


HB_INTERNAL void
_hb_buffer_swap (hb_buffer_t *buffer);

HB_INTERNAL void
_hb_buffer_clear_output (hb_buffer_t *buffer);

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
#define IN_GLYPH(pos)		(buffer->in_string[(pos)].codepoint)
#define IN_INFO(pos)		(&buffer->in_string[(pos)])
#define IN_CURGLYPH()		(buffer->in_string[buffer->in_pos].codepoint)
#define IN_CURINFO()		(&buffer->in_string[buffer->in_pos])
#define IN_MASK(pos)		(buffer->in_string[(pos)].mask)
#define IN_LIGID(pos)		(buffer->in_string[(pos)].lig_id)
#define IN_COMPONENT(pos)	(buffer->in_string[(pos)].component)
#define POSITION(pos)		(&buffer->positions[(pos)])
#define CURPOSITION()		(&buffer->positions[buffer->in_pos])
#define OUT_GLYPH(pos)		(buffer->out_string[(pos)].codepoint)
#define OUT_INFO(pos)		(&buffer->out_string[(pos)])

HB_END_DECLS

#endif /* HB_BUFFER_PRIVATE_H */
