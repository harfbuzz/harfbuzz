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

/* XXX  Hide structs? */

typedef struct _hb_glyph_info_t {
  hb_codepoint_t gindex;
  unsigned int   properties;
  unsigned int   cluster;
  unsigned short component;
  unsigned short ligID;
  unsigned short gproperty;
} hb_glyph_info_t;

typedef struct _hb_glyph_position_t {
  hb_position_t  x_pos;
  hb_position_t  y_pos;
  hb_position_t  x_advance;
  hb_position_t  y_advance;
  unsigned short back;		/* number of glyphs to go back
				   for drawing current glyph */
  hb_bool_t      new_advance;	/* if set, the advance width values are
				   absolute, i.e., they won't be
				   added to the original glyph's value
				   but rather replace them */
  short          cursive_chain; /* character to which this connects,
				   may be positive or negative; used
				   only internally */
} hb_glyph_position_t;


typedef struct _hb_buffer_t {
  unsigned int allocated;

  unsigned int in_length;
  unsigned int out_length;
  unsigned int in_pos;
  unsigned int out_pos;

  hb_glyph_info_t     *in_string;
  hb_glyph_info_t     *out_string;
  hb_glyph_info_t     *alt_string;
  hb_glyph_position_t *positions;
  unsigned int         max_lig_id;
} hb_buffer_t;

hb_buffer_t *
hb_buffer_new (void);

void
hb_buffer_free (hb_buffer_t *buffer);

void
hb_buffer_clear (hb_buffer_t *buffer);

void
hb_buffer_add_glyph (hb_buffer_t    *buffer,
		     hb_codepoint_t  glyph_index,
		     unsigned int    properties,
		     unsigned int    cluster);

HB_END_DECLS

#endif /* HB_BUFFER_H */
