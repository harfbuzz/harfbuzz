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

#include "hb-ot-shape-private.hh"

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


static void
add_feature (hb_face_t    *face,
	     hb_tag_t      table_tag,
	     unsigned int  feature_index,
	     unsigned int *lookups,
	     unsigned int *num_lookups,
	     unsigned int  room_lookups)
{
  unsigned int i = room_lookups - *num_lookups;
  hb_ot_layout_feature_get_lookup_indexes (face, table_tag, feature_index, 0,
					   &i,
					   lookups + *num_lookups);
  *num_lookups += i;
}

static int
cmp_lookups (const void *p1, const void *p2)
{
  unsigned int a = * (const unsigned int *) p1;
  unsigned int b = * (const unsigned int *) p2;

  return a - b;
}

static void
setup_lookups (hb_face_t    *face,
	       hb_buffer_t  *buffer,
	       hb_feature_t *features,
	       unsigned int  num_features,
	       hb_tag_t      table_tag,
	       unsigned int *lookups,
	       unsigned int *num_lookups)
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
    add_feature (face, table_tag, feature_index, lookups, num_lookups, room_lookups);

  for (i = 0; i < ARRAY_LENGTH (default_features); i++)
  {
    if (hb_ot_layout_language_find_feature (face, table_tag, script_index, language_index,
					    default_features[i],
					    &feature_index))
      add_feature (face, table_tag, feature_index, lookups, num_lookups, room_lookups);
  }

  qsort (lookups, *num_lookups, sizeof (lookups[0]), cmp_lookups);

  if (*num_lookups)
  {
    for (i = 1, j = 0; i < *num_lookups; i++)
      if (lookups[i] != lookups[j])
	lookups[++j] = lookups[i];
    lookups[j++] = lookups[i - 1];
    *num_lookups = j;
  }
}


hb_bool_t
_hb_ot_substitute_complex (hb_font_t    *font HB_UNUSED,
			   hb_face_t    *face,
			   hb_buffer_t  *buffer,
			   hb_feature_t *features,
			   unsigned int  num_features)
{
  unsigned int lookups[1000];
  unsigned int num_lookups = ARRAY_LENGTH (lookups);
  unsigned int i;

  if (!hb_ot_layout_has_substitution (face))
    return FALSE;

  setup_lookups (face, buffer, features, num_features,
		 HB_OT_TAG_GSUB,
		 lookups, &num_lookups);

  for (i = 0; i < num_lookups; i++)
    hb_ot_layout_substitute_lookup (face, buffer, lookups[i], 0xFFFF);

  return TRUE;
}

hb_bool_t
_hb_ot_position_complex (hb_font_t    *font,
			 hb_face_t    *face,
			 hb_buffer_t  *buffer,
			 hb_feature_t *features,
			 unsigned int  num_features)
{
  unsigned int lookups[1000];
  unsigned int num_lookups = ARRAY_LENGTH (lookups);
  unsigned int i;

  if (!hb_ot_layout_has_positioning (face))
    return FALSE;

  setup_lookups (face, buffer, features, num_features,
		 HB_OT_TAG_GPOS,
		 lookups, &num_lookups);

  for (i = 0; i < num_lookups; i++)
    hb_ot_layout_position_lookup (font, face, buffer, lookups[i], 0xFFFF);

  hb_ot_layout_position_finish (font, face, buffer);

  return TRUE;
}
