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

#include "hb-ot-shape.h"

#include "hb-buffer-private.hh"

#include "hb-ot-layout.h"

hb_tag_t default_features[] = {
  /* GSUB */
  HB_TAG('c','c','m','p'),
  HB_TAG('l','o','c','l'),
  HB_TAG('l','i','g','a'),
  HB_TAG('c','l','i','g'),
  /* GPOS */
  HB_TAG('k','e','r','n'),
  HB_TAG('m','a','r','k'),
  HB_TAG('m','k','m','k'),
};

struct lookup_map {
  unsigned int index;
  hb_mask_t mask;
};


static void
add_feature (hb_face_t    *face,
	     hb_tag_t      table_tag,
	     unsigned int  feature_index,
	     hb_mask_t     mask,
	     lookup_map   *lookups,
	     unsigned int *num_lookups,
	     unsigned int  room_lookups)
{
  unsigned int i = room_lookups - *num_lookups;
  lookups += *num_lookups;

  unsigned int *lookup_indices = (unsigned int *) lookups;

  hb_ot_layout_feature_get_lookup_indexes (face, table_tag, feature_index, 0,
					   &i,
					   lookup_indices);

  *num_lookups += i;

  while (i--) {
    lookups[i].mask = mask;
    lookups[i].index = lookup_indices[i];
  }
}

static hb_bool_t
maybe_add_feature (hb_face_t    *face,
		   hb_tag_t      table_tag,
		   unsigned int  script_index,
		   unsigned int  language_index,
		   hb_tag_t      feature_tag,
		   hb_mask_t     mask,
		   lookup_map   *lookups,
		   unsigned int *num_lookups,
		   unsigned int  room_lookups)
{
  unsigned int feature_index;
  if (hb_ot_layout_language_find_feature (face, table_tag, script_index, language_index,
					  feature_tag,
					  &feature_index))
  {
    add_feature (face, table_tag, feature_index, mask, lookups, num_lookups, room_lookups);
    return TRUE;
  }
  return FALSE;
}

static int
cmp_lookups (const void *p1, const void *p2)
{
  const lookup_map *a = (const lookup_map *) p1;
  const lookup_map *b = (const lookup_map *) p2;

  return a->index - b->index;
}

static void
setup_lookups (hb_face_t    *face,
	       hb_buffer_t  *buffer,
	       hb_feature_t *features,
	       unsigned int  num_features,
	       hb_tag_t      table_tag,
	       lookup_map   *lookups,
	       unsigned int *num_lookups,
	       hb_direction_t original_direction)
{
  unsigned int i, j, script_index, language_index, feature_index, room_lookups;

  room_lookups = *num_lookups;
  *num_lookups = 0;

  hb_ot_layout_table_choose_script (face, table_tag,
				    hb_ot_tags_from_script (buffer->script),
				    &script_index);
  hb_ot_layout_script_find_language (face, table_tag, script_index,
				     hb_ot_tag_from_language (buffer->language),
				     &language_index);

  if (hb_ot_layout_language_get_required_feature_index (face, table_tag, script_index, language_index,
							&feature_index))
    add_feature (face, table_tag, feature_index, 1, lookups, num_lookups, room_lookups);

  for (i = 0; i < ARRAY_LENGTH (default_features); i++)
    maybe_add_feature (face, table_tag, script_index, language_index, default_features[i], 1, lookups, num_lookups, room_lookups);

  switch (original_direction) {
    case HB_DIRECTION_LTR:
      maybe_add_feature (face, table_tag, script_index, language_index, HB_TAG ('l','t','r','a'), 1, lookups, num_lookups, room_lookups);
      maybe_add_feature (face, table_tag, script_index, language_index, HB_TAG ('l','t','r','m'), 1, lookups, num_lookups, room_lookups);
      break;
    case HB_DIRECTION_RTL:
      maybe_add_feature (face, table_tag, script_index, language_index, HB_TAG ('r','t','l','a'), 1, lookups, num_lookups, room_lookups);
      break;
    case HB_DIRECTION_TTB:
    case HB_DIRECTION_BTT:
    default:
      break;
  }

  /* Clear buffer masks. */
  buffer->clear_masks ();

  unsigned int next_bit = 1;
  hb_mask_t global_mask = 0;
  for (i = 0; i < num_features; i++)
  {
    hb_feature_t *feature = &features[i];
    if (!hb_ot_layout_language_find_feature (face, table_tag, script_index, language_index,
					     feature->tag,
					     &feature_index))
      continue;

    if (feature->value == 1 && feature->start == 0 && feature->end == (unsigned int) -1) {
      add_feature (face, table_tag, feature_index, 1, lookups, num_lookups, room_lookups);
      continue;
    }

    /* Allocate bits for the features */

    unsigned int bits_needed = _hb_bit_storage (feature->value);
    if (!bits_needed)
      continue; /* Feature disabled */

    if (next_bit + bits_needed > 8 * sizeof (hb_mask_t))
      continue; /* Oh well... */

    unsigned int mask = (1 << (next_bit + bits_needed)) - (1 << next_bit);
    unsigned int value = feature->value << next_bit;
    next_bit += bits_needed;

    add_feature (face, table_tag, feature_index, mask, lookups, num_lookups, room_lookups);

    if (feature->start == 0 && feature->end == (unsigned int) -1)
      global_mask |= value;
    else
      buffer->or_masks (mask, feature->start, feature->end);
  }

  if (global_mask)
    buffer->or_masks (global_mask, 0, (unsigned int) -1);

  qsort (lookups, *num_lookups, sizeof (lookups[0]), cmp_lookups);

  if (*num_lookups)
  {
    for (i = 1, j = 0; i < *num_lookups; i++)
      if (lookups[i].index != lookups[j].index)
	lookups[++j] = lookups[i];
      else
        lookups[j].mask |= lookups[i].mask;
    j++;
    *num_lookups = j;
  }
}


static hb_bool_t
hb_ot_substitute_complex (hb_font_t    *font HB_UNUSED,
			  hb_face_t    *face,
			  hb_buffer_t  *buffer,
			  hb_feature_t *features,
			  unsigned int  num_features,
			  hb_direction_t original_direction)
{
  lookup_map lookups[1000];
  unsigned int num_lookups = ARRAY_LENGTH (lookups);
  unsigned int i;

  if (!hb_ot_layout_has_substitution (face))
    return FALSE;

  setup_lookups (face, buffer, features, num_features,
		 HB_OT_TAG_GSUB,
		 lookups, &num_lookups,
		 original_direction);

  for (i = 0; i < num_lookups; i++)
    hb_ot_layout_substitute_lookup (face, buffer, lookups[i].index, lookups[i].mask);

  return TRUE;
}

static hb_bool_t
hb_ot_position_complex (hb_font_t    *font,
			hb_face_t    *face,
			hb_buffer_t  *buffer,
			hb_feature_t *features,
			unsigned int  num_features,
			hb_direction_t original_direction)
{
  lookup_map lookups[1000];
  unsigned int num_lookups = ARRAY_LENGTH (lookups);
  unsigned int i;

  if (!hb_ot_layout_has_positioning (face))
    return FALSE;

  setup_lookups (face, buffer, features, num_features,
		 HB_OT_TAG_GPOS,
		 lookups, &num_lookups,
		 original_direction);

  for (i = 0; i < num_lookups; i++)
    hb_ot_layout_position_lookup (font, face, buffer, lookups[i].index, lookups[i].mask);

  hb_ot_layout_position_finish (font, face, buffer);

  return TRUE;
}


/* Main shaper */

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

static void
hb_substitute_complex_fallback (hb_font_t    *font HB_UNUSED,
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

static void
hb_position_complex_fallback (hb_font_t    *font HB_UNUSED,
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
hb_position_complex_fallback_visual (hb_font_t    *font,
				     hb_face_t    *face,
				     hb_buffer_t  *buffer,
				     hb_feature_t *features,
				     unsigned int  num_features)
{
  hb_truetype_kern (font, face, buffer, features, num_features);
}


/* Do it! */

void
hb_ot_shape (hb_font_t    *font,
	     hb_face_t    *face,
	     hb_buffer_t  *buffer,
	     hb_feature_t *features,
	     unsigned int  num_features)
{
  hb_direction_t original_direction;
  hb_bool_t substitute_fallback, position_fallback;

  hb_form_clusters (buffer);

  hb_substitute_default (font, face, buffer, features, num_features);

  /* We do this after substitute_default because mirroring needs to
   * see the original direction. */
  original_direction = hb_ensure_native_direction (buffer);

  substitute_fallback = !hb_ot_substitute_complex (font, face, buffer, features, num_features, original_direction);

  if (substitute_fallback)
    hb_substitute_complex_fallback (font, face, buffer, features, num_features);

  hb_position_default (font, face, buffer, features, num_features);

  position_fallback = !hb_ot_position_complex (font, face, buffer, features, num_features, original_direction);

  if (position_fallback)
    hb_position_complex_fallback (font, face, buffer, features, num_features);

  if (HB_DIRECTION_IS_BACKWARD (buffer->direction))
    hb_buffer_reverse (buffer);

  if (position_fallback)
    hb_position_complex_fallback_visual (font, face, buffer, features, num_features);

  buffer->direction = original_direction;
}
