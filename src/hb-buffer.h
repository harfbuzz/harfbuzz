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

#ifndef HB_BUFFER_H
#define HB_BUFFER_H

#include "hb-common.h"

HB_BEGIN_DECLS

typedef struct _hb_buffer_t hb_buffer_t;

typedef enum _hb_direction_t {
  HB_DIRECTION_LTR,
  HB_DIRECTION_RTL,
  HB_DIRECTION_TTB,
  HB_DIRECTION_BTT
} hb_direction_t;

typedef struct _hb_glyph_info_t {
  hb_codepoint_t codepoint;
  hb_mask_t      mask;
  uint32_t       cluster;
  uint32_t       internal1;
  uint32_t       internal2;
} hb_glyph_info_t;

typedef struct _hb_glyph_position_t {
  hb_position_t  x_pos;
  hb_position_t  y_pos;
  hb_position_t  x_advance;
  hb_position_t  y_advance;
  /* XXX these should all be replaced by "uint32_t internal" */
  uint32_t       new_advance :1;	/* if set, the advance width values are
					   absolute, i.e., they won't be
					   added to the original glyph's value
					   but rather replace them */
  uint32_t       back : 15;		/* number of glyphs to go back
					   for drawing current glyph */
  int32_t        cursive_chain : 16;	/* character to which this connects,
					   may be positive or negative; used
					   only internally */
} hb_glyph_position_t;


hb_buffer_t *
hb_buffer_create (unsigned int pre_alloc_size);

hb_buffer_t *
hb_buffer_reference (hb_buffer_t *buffer);

unsigned int
hb_buffer_get_reference_count (hb_buffer_t *buffer);

void
hb_buffer_destroy (hb_buffer_t *buffer);


void
hb_buffer_set_direction (hb_buffer_t    *buffer,
			 hb_direction_t  direction);

hb_direction_t
hb_buffer_get_direction (hb_buffer_t *buffer);


void
hb_buffer_clear (hb_buffer_t *buffer);

void
hb_buffer_clear_positions (hb_buffer_t *buffer);

void
hb_buffer_ensure (hb_buffer_t  *buffer,
		  unsigned int  size);

void
hb_buffer_reverse (hb_buffer_t *buffer);


/* Filling the buffer in */

void
hb_buffer_add_glyph (hb_buffer_t    *buffer,
		     hb_codepoint_t  codepoint,
		     hb_mask_t       mask,
		     unsigned int    cluster);

void
hb_buffer_add_utf8 (hb_buffer_t  *buffer,
		    const char   *text,
		    unsigned int  text_length,
		    unsigned int  item_offset,
		    unsigned int  item_length);

void
hb_buffer_add_utf16 (hb_buffer_t    *buffer,
		     const uint16_t *text,
		     unsigned int    text_length,
		     unsigned int    item_offset,
		     unsigned int    item_length);

void
hb_buffer_add_utf32 (hb_buffer_t    *buffer,
		     const uint32_t *text,
		     unsigned int    text_length,
		     unsigned int    item_offset,
		     unsigned int    item_length);


/* Getting glyphs out of the buffer */

/* Return value valid as long as buffer not modified */
unsigned int
hb_buffer_get_len (hb_buffer_t *buffer);

/* Return value valid as long as buffer not modified */
hb_glyph_info_t *
hb_buffer_get_glyph_infos (hb_buffer_t *buffer);

/* Return value valid as long as buffer not modified */
hb_glyph_position_t *
hb_buffer_get_glyph_positions (hb_buffer_t *buffer);


HB_END_DECLS

#endif /* HB_BUFFER_H */
