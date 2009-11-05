/*
 * Copyright (C) 2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#include "hb-private.h"

#include "hb-shape.h"

#include "hb-buffer-private.h"

static inline hb_bool_t
is_variation_selector (hb_codepoint_t unicode)
{
  return (unicode >=  0xFE00 && unicode <=  0xFE0F) ||
	 (unicode >= 0xE0100 && unicode <= 0xE01EF);
}

static void
hb_map_glyphs (hb_font_t    *font,
	       hb_face_t    *face,
	       hb_buffer_t  *buffer)
{
  unsigned int count;

  count = buffer->in_length - 1;

  for (buffer->in_pos = 0; buffer->in_pos < count; buffer->in_pos++) {
    if (HB_UNLIKELY (is_variation_selector (IN_NEXTGLYPH()))) {
      IN_CURGLYPH() = hb_font_get_glyph (font, face, IN_CURGLYPH(), IN_NEXTGLYPH());
      buffer->in_pos++;
    } else {
      IN_CURGLYPH() = hb_font_get_glyph (font, face, IN_CURGLYPH(), 0);
    }
  }
  IN_CURGLYPH() = hb_font_get_glyph (font, face, IN_CURGLYPH(), 0);
}

static void
hb_position_default (hb_font_t    *font,
		     hb_face_t    *face,
		     hb_buffer_t  *buffer)
{
  unsigned int count;

  hb_buffer_clear_positions (buffer);

  count = buffer->in_length;

  for (buffer->in_pos = 0; buffer->in_pos < count; buffer->in_pos++) {
    hb_glyph_metrics_t metrics;
    hb_font_get_glyph_metrics (font, face, IN_CURGLYPH(), &metrics);
    CURPOSITION()->x_advance = metrics.x_advance;
    CURPOSITION()->y_advance = metrics.y_advance;
  }
}


void
hb_shape (hb_font_t    *font,
	  hb_face_t    *face,
	  hb_buffer_t  *buffer,
	  hb_feature_t *features,
	  unsigned int  num_features)
{
  /* form_clusters (buffer); */
  /* do_mirroring (buffer); */
  /* natural direction analysis */
  /* OT preprocess */

  hb_map_glyphs (font, face, buffer);

  /* GSUB */
  hb_position_default (font, face, buffer);
  /* Default positioning */
  /* GPOS / kern */
}
