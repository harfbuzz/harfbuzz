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

#include "hb-ot-shape-private.h"


/* Prepare */

static inline hb_bool_t
is_variation_selector (hb_codepoint_t unicode)
{
  return HB_UNLIKELY ((unicode >=  0x180B && unicode <=  0x180D) || /* MONGOLIAN FREE VARIATION SELECTOR ONE..THREE */
		      (unicode >=  0xFE00 && unicode <=  0xFE0F) || /* VARIATION SELECTOR-1..16 */
		      (unicode >= 0xE0100 && unicode <= 0xE01EF));  /* VARIATION SELECTOR-17..256 */
}

static void
hb_form_clusters (hb_buffer_t *buffer)
{
  unsigned int count;

  count = buffer->in_length;
  for (buffer->in_pos = 1; buffer->in_pos < count; buffer->in_pos++)
    if (buffer->unicode->get_general_category (IN_CURGLYPH()) == HB_CATEGORY_NON_SPACING_MARK)
      IN_CLUSTER (buffer->in_pos) = IN_CLUSTER (buffer->in_pos - 1);
}

static hb_direction_t
hb_ensure_native_direction (hb_buffer_t *buffer)
{
  hb_direction_t original_direction = buffer->direction;

  /* TODO vertical */
  if (HB_DIRECTION_IS_HORIZONTAL (original_direction) &&
      original_direction != _hb_script_get_horizontal_direction (buffer->script))
  {
    hb_buffer_reverse_clusters (buffer);
    buffer->direction ^=  1;
  }

  return original_direction;
}


/* Substitute */

static void
hb_mirror_chars (hb_buffer_t *buffer)
{
  unsigned int count;
  hb_unicode_get_mirroring_func_t get_mirroring = buffer->unicode->get_mirroring;

  if (HB_DIRECTION_IS_FORWARD (buffer->direction))
    return;

  count = buffer->in_length;
  for (buffer->in_pos = 0; buffer->in_pos < count; buffer->in_pos++) {
      IN_CURGLYPH() = get_mirroring (IN_CURGLYPH());
  }
}

static void
hb_map_glyphs (hb_font_t    *font,
	       hb_face_t    *face,
	       hb_buffer_t  *buffer)
{
  unsigned int count;

  if (HB_UNLIKELY (!buffer->in_length))
    return;
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
hb_substitute_default (hb_font_t    *font,
		       hb_face_t    *face,
		       hb_buffer_t  *buffer,
		       hb_feature_t *features,
		       unsigned int  num_features)
{
  hb_mirror_chars (buffer);
  hb_map_glyphs (font, face, buffer);
}

static gboolean
hb_substitute_complex (hb_font_t    *font,
		       hb_face_t    *face,
		       hb_buffer_t  *buffer,
		       hb_feature_t *features,
		       unsigned int  num_features)
{
  return _hb_ot_substitute_complex (font, face, buffer, features, num_features);
}

static void
hb_substitute_fallback (hb_font_t    *font,
			hb_face_t    *face,
			hb_buffer_t  *buffer,
			hb_feature_t *features,
			unsigned int  num_features)
{
  /* TODO Arabic */
}


/* Position */

static void
hb_position_default (hb_font_t    *font,
		     hb_face_t    *face,
		     hb_buffer_t  *buffer,
		     hb_feature_t *features,
		     unsigned int  num_features)
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

static gboolean
hb_position_complex (hb_font_t    *font,
		     hb_face_t    *face,
		     hb_buffer_t  *buffer,
		     hb_feature_t *features,
		     unsigned int  num_features)
{
  return _hb_ot_position_complex (font, face, buffer, features, num_features);
}

static void
hb_position_fallback (hb_font_t    *font,
		      hb_face_t    *face,
		      hb_buffer_t  *buffer,
		      hb_feature_t *features,
		      unsigned int  num_features)
{
  /* TODO Mark pos */
}

static void
hb_truetype_kern (hb_font_t    *font,
		  hb_face_t    *face,
		  hb_buffer_t  *buffer,
		  hb_feature_t *features,
		  unsigned int  num_features)
{
  unsigned int count;

  /* TODO Check for kern=0 */
  count = buffer->in_length;
  for (buffer->in_pos = 1; buffer->in_pos < count; buffer->in_pos++) {
    hb_position_t kern, kern1, kern2;
    kern = hb_font_get_kerning (font, face, IN_GLYPH(buffer->in_pos - 1), IN_CURGLYPH());
    kern1 = kern >> 1;
    kern2 = kern - kern1;
    POSITION(buffer->in_pos - 1)->x_advance += kern1;
    CURPOSITION()->x_advance += kern2;
    CURPOSITION()->x_offset += kern2;
  }
}

static void
hb_position_fallback_visual (hb_font_t    *font,
			     hb_face_t    *face,
			     hb_buffer_t  *buffer,
			     hb_feature_t *features,
			     unsigned int  num_features)
{
  hb_truetype_kern (font, face, buffer, features, num_features);
}


/* Shape */

void
hb_shape (hb_font_t    *font,
	  hb_face_t    *face,
	  hb_buffer_t  *buffer,
	  hb_feature_t *features,
	  unsigned int  num_features)
{
  hb_direction_t original_direction;
  hb_bool_t substitute_fallback, position_fallback;

  hb_form_clusters (buffer);
  original_direction = hb_ensure_native_direction (buffer);

  hb_substitute_default (font, face, buffer, features, num_features);

  substitute_fallback = !hb_substitute_complex (font, face, buffer, features, num_features);

  if (substitute_fallback)
    hb_substitute_fallback (font, face, buffer, features, num_features);

  hb_position_default (font, face, buffer, features, num_features);

  position_fallback = !hb_position_complex (font, face, buffer, features, num_features);

  if (position_fallback)
    hb_position_fallback (font, face, buffer, features, num_features);

  if (HB_DIRECTION_IS_BACKWARD (buffer->direction))
    hb_buffer_reverse (buffer);

  if (position_fallback)
    hb_position_fallback_visual (font, face, buffer, features, num_features);

  buffer->direction = original_direction;
}
