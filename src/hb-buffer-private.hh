/*
 * Copyright (C) 1998-2004  David Turner and Werner Lemberg
 * Copyright (C) 2004,2007,2009,2010  Red Hat, Inc.
 *
 * This is part of HarfBuzz, a text shaping library.
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
#include "hb-unicode-private.h"

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
  hb_position_t  x_advance;
  hb_position_t  y_advance;
  hb_position_t  x_offset;
  hb_position_t  y_offset;
  uint32_t       back : 16;		/* number of glyphs to go back
					   for drawing current glyph */
  int32_t        cursive_chain : 16;	/* character to which this connects,
					   may be positive or negative */
} hb_internal_glyph_position_t;

ASSERT_STATIC (sizeof (hb_glyph_info_t) == sizeof (hb_internal_glyph_info_t));
ASSERT_STATIC (sizeof (hb_glyph_position_t) == sizeof (hb_internal_glyph_position_t));
ASSERT_STATIC (sizeof (hb_glyph_info_t) == sizeof (hb_glyph_position_t));



HB_INTERNAL void
_hb_buffer_swap (hb_buffer_t *buffer);

HB_INTERNAL void
_hb_buffer_clear_output (hb_buffer_t *buffer);

HB_INTERNAL void
_hb_buffer_add_output_glyphs (hb_buffer_t *buffer,
			      unsigned int num_in,
			      unsigned int num_out,
			      const hb_codepoint_t *glyph_data,
			      unsigned short component,
			      unsigned short ligID);

HB_INTERNAL void
_hb_buffer_add_output_glyphs_be16 (hb_buffer_t *buffer,
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



struct _hb_buffer_t {
  hb_reference_count_t ref_count;

  /* Information about how the text in the buffer should be treated */
  hb_unicode_funcs_t *unicode;
  hb_direction_t      direction;
  hb_script_t         script;
  hb_language_t       language;

  /* Buffer contents */

  unsigned int allocated;

  hb_bool_t    have_output; /* whether we have an output buffer going on */
  hb_bool_t    have_positions; /* whether we have positions */
  unsigned int in_length;
  unsigned int out_length;
  unsigned int in_pos;
  unsigned int out_pos; /* out_length and out_pos are actually always the same */

  hb_internal_glyph_info_t     *in_string;
  hb_internal_glyph_info_t     *out_string;
  hb_internal_glyph_position_t *positions;

  /* Other stuff */

  unsigned int         max_lig_id;


  /* Methods */
  inline unsigned int allocate_lig_id (void) { return max_lig_id++; }
  inline void swap (void) { _hb_buffer_swap (this); }
  inline void clear_output (void) { _hb_buffer_clear_output (this); }
  inline void next_glyph (void) { _hb_buffer_next_glyph (this); }
  inline void add_output_glyphs (unsigned int num_in,
				 unsigned int num_out,
				 const hb_codepoint_t *glyph_data,
				 unsigned short component,
				 unsigned short ligID)
  { _hb_buffer_add_output_glyphs (this, num_in, num_out, glyph_data, component, ligID); }
  inline void add_output_glyphs_be16 (unsigned int num_in,
				      unsigned int num_out,
				      const uint16_t *glyph_data_be,
				      unsigned short component,
				      unsigned short ligID)
  { _hb_buffer_add_output_glyphs_be16 (this, num_in, num_out, glyph_data_be, component, ligID); }
  inline void add_output_glyph (hb_codepoint_t glyph_index,
				unsigned short component = 0xFFFF,
				unsigned short ligID = 0xFFFF)
  { _hb_buffer_add_output_glyph (this, glyph_index, component, ligID); }
  inline void replace_glyph (hb_codepoint_t glyph_index) { add_output_glyph (glyph_index); }
};



#ifndef BUFFER
#define BUFFER buffer
#endif

/* convenience macros */
#define IN_INFO(pos)		(&BUFFER->in_string[(pos)])
#define IN_CURGLYPH()		(BUFFER->in_string[BUFFER->in_pos].codepoint)
#define IN_NEXTGLYPH()		(BUFFER->in_string[BUFFER->in_pos + 1].codepoint)
#define IN_CURINFO()		(&BUFFER->in_string[BUFFER->in_pos])

HB_END_DECLS

#endif /* HB_BUFFER_PRIVATE_H */
