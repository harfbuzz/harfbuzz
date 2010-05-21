/*
 * Copyright (C) 2009  Red Hat, Inc.
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

#include "hb-private.h"

#include "hb-shape.h"

#include "hb-buffer-private.hh"

#include "hb-ot-shape-private.hh"

#ifdef HAVE_GRAPHITE
#include "hb-graphite.h"
#endif

/* Prepare */

static inline hb_bool_t
is_variation_selector (hb_codepoint_t unicode)
{
  return unlikely ((unicode >=  0x180B && unicode <=  0x180D) || /* MONGOLIAN FREE VARIATION SELECTOR ONE..THREE */
		   (unicode >=  0xFE00 && unicode <=  0xFE0F) || /* VARIATION SELECTOR-1..16 */
		   (unicode >= 0xE0100 && unicode <= 0xE01EF));  /* VARIATION SELECTOR-17..256 */
}

static void
hb_form_clusters (hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  for (unsigned int i = 1; i < count; i++)
    if (buffer->unicode->get_general_category (buffer->info[i].codepoint) == HB_CATEGORY_NON_SPACING_MARK)
      buffer->info[i].cluster = buffer->info[i - 1].cluster;
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
    buffer->direction = HB_DIRECTION_REVERSE (buffer->direction);
  }

  return original_direction;
}


/* Substitute */

static void
hb_mirror_chars (hb_buffer_t *buffer)
{
  hb_unicode_get_mirroring_func_t get_mirroring = buffer->unicode->get_mirroring;

  if (HB_DIRECTION_IS_FORWARD (buffer->direction))
    return;

  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++) {
      buffer->info[i].codepoint = get_mirroring (buffer->info[i].codepoint);
  }
}

static void
hb_map_glyphs (hb_font_t    *font,
	       hb_face_t    *face,
	       hb_buffer_t  *buffer)
{
  if (unlikely (!buffer->len))
    return;

  unsigned int count = buffer->len - 1;
  for (unsigned int i = 0; i < count; i++) {
    if (unlikely (is_variation_selector (buffer->info[i + 1].codepoint))) {
      buffer->info[i].codepoint = hb_font_get_glyph (font, face, buffer->info[i].codepoint, buffer->info[i + 1].codepoint);
      i++;
    } else {
      buffer->info[i].codepoint = hb_font_get_glyph (font, face, buffer->info[i].codepoint, 0);
    }
  }
  buffer->info[count].codepoint = hb_font_get_glyph (font, face, buffer->info[count].codepoint, 0);
}

static void
hb_substitute_default (hb_font_t    *font,
		       hb_face_t    *face,
		       hb_buffer_t  *buffer,
		       hb_feature_t *features HB_UNUSED,
		       unsigned int  num_features HB_UNUSED)
{
  hb_mirror_chars (buffer);
  hb_map_glyphs (font, face, buffer);
}

static hb_bool_t
hb_substitute_complex (hb_font_t    *font,
		       hb_face_t    *face,
		       hb_buffer_t  *buffer,
		       hb_feature_t *features,
		       unsigned int  num_features)
{
  return _hb_ot_substitute_complex (font, face, buffer, features, num_features);
}

static void
hb_substitute_fallback (hb_font_t    *font HB_UNUSED,
			hb_face_t    *face HB_UNUSED,
			hb_buffer_t  *buffer HB_UNUSED,
			hb_feature_t *features HB_UNUSED,
			unsigned int  num_features HB_UNUSED)
{
  /* TODO Arabic */
}


/* Position */

static void
hb_position_default (hb_font_t    *font,
		     hb_face_t    *face,
		     hb_buffer_t  *buffer,
		     hb_feature_t *features HB_UNUSED,
		     unsigned int  num_features HB_UNUSED)
{
  hb_buffer_clear_positions (buffer);

  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++) {
    hb_glyph_metrics_t metrics;
    hb_font_get_glyph_metrics (font, face, buffer->info[i].codepoint, &metrics);
    buffer->pos[i].x_advance = metrics.x_advance;
    buffer->pos[i].y_advance = metrics.y_advance;
  }
}

static hb_bool_t
hb_position_complex (hb_font_t    *font,
		     hb_face_t    *face,
		     hb_buffer_t  *buffer,
		     hb_feature_t *features,
		     unsigned int  num_features)
{
  return _hb_ot_position_complex (font, face, buffer, features, num_features);
}

static void
hb_position_fallback (hb_font_t    *font HB_UNUSED,
		      hb_face_t    *face HB_UNUSED,
		      hb_buffer_t  *buffer HB_UNUSED,
		      hb_feature_t *features HB_UNUSED,
		      unsigned int  num_features HB_UNUSED)
{
  /* TODO Mark pos */
}

static void
hb_truetype_kern (hb_font_t    *font,
		  hb_face_t    *face,
		  hb_buffer_t  *buffer,
		  hb_feature_t *features HB_UNUSED,
		  unsigned int  num_features HB_UNUSED)
{
  /* TODO Check for kern=0 */
  unsigned int count = buffer->len;
  for (unsigned int i = 1; i < count; i++) {
    hb_position_t kern, kern1, kern2;
    kern = hb_font_get_kerning (font, face, buffer->info[i - 1].codepoint, buffer->info[i].codepoint);
    kern1 = kern >> 1;
    kern2 = kern - kern1;
    buffer->pos[i - 1].x_advance += kern1;
    buffer->pos[i].x_advance += kern2;
    buffer->pos[i].x_offset += kern2;
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

#ifdef HAVE_GRAPHITE
  hb_blob_t *silf_blob;
  silf_blob = hb_face_get_table (face, HB_GRAPHITE_TAG_Silf);
  if (hb_blob_get_length(silf_blob))
  {
    hb_graphite_shape(font, face, buffer, features, num_features);
    hb_blob_destroy(silf_blob);
    return;
  }
  hb_blob_destroy(silf_blob);
#endif

  hb_form_clusters (buffer);

  hb_substitute_default (font, face, buffer, features, num_features);

  /* We do this after substitute_default because mirroring needs to
   * see the original direction. */
  original_direction = hb_ensure_native_direction (buffer);

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
