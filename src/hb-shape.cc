/*
 * Copyright © 2009  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
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

#include "hb-private.hh"

#include "hb-shape.h"

#include "hb-buffer-private.hh"

#include "hb-ot-shape.h"

#ifdef HAVE_GRAPHITE
#include "hb-graphite.h"
#endif

HB_BEGIN_DECLS


static void
hb_shape_internal (hb_font_t          *font,
		   hb_buffer_t        *buffer,
		   const hb_feature_t *features,
		   unsigned int        num_features)
{
  hb_ot_shape (font, buffer, features, num_features);
}

void
hb_shape (hb_font_t          *font,
	  hb_buffer_t        *buffer,
	  const hb_feature_t *features,
	  unsigned int        num_features)
{
  hb_segment_properties_t orig_props;

  orig_props = buffer->props;

  /* If script is set to INVALID, guess from buffer contents */
  if (buffer->props.script == HB_SCRIPT_INVALID) {
    hb_unicode_funcs_t *unicode = buffer->unicode;
    unsigned int count = buffer->len;
    for (unsigned int i = 0; i < count; i++) {
      hb_script_t script = unicode->get_script (buffer->info[i].codepoint);
      if (likely (script != HB_SCRIPT_COMMON &&
		  script != HB_SCRIPT_INHERITED &&
		  script != HB_SCRIPT_UNKNOWN)) {
        buffer->props.script = script;
        break;
      }
    }
  }

  /* If direction is set to INVALID, guess from script */
  if (buffer->props.direction == HB_DIRECTION_INVALID) {
    buffer->props.direction = hb_script_get_horizontal_direction (buffer->props.script);
  }

  /* If language is not set, use default language from locale */
  if (buffer->props.language == NULL) {
    /* TODO get_default_for_script? using $LANGUAGE */
    //buffer->props.language = hb_language_get_default ();
  }

  hb_shape_internal (font, buffer, features, num_features);

  buffer->props = orig_props;
}


HB_END_DECLS
