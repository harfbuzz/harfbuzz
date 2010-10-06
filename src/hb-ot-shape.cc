/*
 * Copyright (C) 2009,2010  Red Hat, Inc.
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

#include "hb-ot-shape-private.h"

#include "hb-open-type-private.hh"

#include "hb-ot-layout.h"

HB_BEGIN_DECLS


/* XXX vertical */
hb_tag_t default_features[] = {
  HB_TAG('c','a','l','t'),
  HB_TAG('c','c','m','p'),
  HB_TAG('c','l','i','g'),
  HB_TAG('c','s','w','h'),
  HB_TAG('c','u','r','s'),
  HB_TAG('k','e','r','n'),
  HB_TAG('l','i','g','a'),
  HB_TAG('l','o','c','l'),
  HB_TAG('m','a','r','k'),
  HB_TAG('m','k','m','k'),
  HB_TAG('r','l','i','g')
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

static int
cmp_lookups (const void *p1, const void *p2)
{
  const lookup_map *a = (const lookup_map *) p1;
  const lookup_map *b = (const lookup_map *) p2;

  return a->index - b->index;
}


#define MAX_FEATURES 100 /* FIXME */

struct hb_mask_allocator_t {

  struct feature_info_t {
    hb_tag_t tag;
    unsigned int value;
    unsigned int seq;
    bool global;

    static int
    cmp (const void *p1, const void *p2)
    {
      const feature_info_t *a = reinterpret_cast<const feature_info_t *>(p1);
      const feature_info_t *b = reinterpret_cast<const feature_info_t *>(p2);

      if (a->tag != b->tag)
        return a->tag < b->tag ? -1 : 1;

      return a->seq < b->seq ? -1 : 1;
    }
  };

  struct feature_map_t {
    hb_tag_t tag; /* should be first */
    unsigned int index;
    unsigned int shift;
    hb_mask_t mask;

    static int
    cmp (const void *p1, const void *p2)
    {
      const feature_map_t *a = reinterpret_cast<const feature_map_t *>(p1);
      const feature_map_t *b = reinterpret_cast<const feature_map_t *>(p2);

      return a->tag < b->tag ? -1 : a->tag > b->tag ? 1 : 0;
    }
  };

  hb_mask_allocator_t (void) : count (0) {}

  void add_feature (hb_tag_t tag,
		    unsigned int value,
		    bool global)
  {
    feature_info_t *info = &infos[count++];
    info->tag = tag;
    info->value = value;
    info->seq = count;
    info->global = global;
  }

  void compile (hb_face_t *face,
		hb_tag_t table_tag,
		unsigned int script_index,
		unsigned int language_index)
  {
    global_mask = 0;
    unsigned int next_bit = 1;

    if (!count)
      return;

    qsort (infos, count, sizeof (infos[0]), feature_info_t::cmp);

    unsigned int j = 0;
    for (unsigned int i = 1; i < count; i++)
      if (infos[i].tag != infos[j].tag)
	infos[++j] = infos[i];
      else {
	if (infos[i].global)
	  infos[j] = infos[i];
	else {
	  infos[j].global = infos[j].global && (infos[j].value == infos[i].value);
	  infos[j].value = MAX (infos[j].value, infos[i].value);
	}
      }
    count = j + 1;

    /* Allocate bits now */
    j = 0;
    for (unsigned int i = 0; i < count; i++) {
      const feature_info_t *info = &infos[i];

      unsigned int bits_needed;

      if (info->global && info->value == 1)
        /* Uses the global bit */
        bits_needed = 0;
      else
        bits_needed = _hb_bit_storage (info->value);

      if (!info->value || next_bit + bits_needed > 8 * sizeof (hb_mask_t))
        continue; /* Feature disabled, or not enough bits. */

      unsigned int feature_index;
      if (!hb_ot_layout_language_find_feature (face, table_tag, script_index, language_index,
					       info->tag, &feature_index))
        continue;

      feature_map_t *map = &maps[j++];

      map->tag = info->tag;
      map->index = feature_index;
      if (info->global && info->value == 1) {
        /* Use the global bit */
        map->shift = 0;
	map->mask = 1;
      } else {
	map->shift = next_bit;
	map->mask = (1 << (next_bit + bits_needed)) - (1 << next_bit);
	next_bit += bits_needed;
      }

      if (info->global && map->mask != 1)
        global_mask |= map->mask;
    }
    count = j;
  }

  hb_mask_t get_global_mask (void) { return global_mask; }
  const feature_map_t *find_feature (hb_tag_t tag) const {
    static const feature_map_t off_map = { HB_TAG_NONE, Index::NOT_FOUND_INDEX, 0, 0 };
    const feature_map_t *map = (const feature_map_t *) bsearch (&tag, maps, count, sizeof (maps[0]), feature_map_t::cmp);
    return map ? map : &off_map;
  }


  private:

  unsigned int count;
  feature_info_t infos[MAX_FEATURES];

  feature_map_t maps[MAX_FEATURES];
  hb_mask_t global_mask;
};

static void
setup_lookups (hb_ot_shape_context_t *c,
	       hb_tag_t               table_tag,
	       lookup_map            *lookups,
	       unsigned int          *num_lookups)
{
  unsigned int i, j, script_index, language_index, feature_index, room_lookups;

  room_lookups = *num_lookups;
  *num_lookups = 0;

  hb_ot_layout_table_choose_script (c->face, table_tag,
				    hb_ot_tags_from_script (c->buffer->props.script),
				    &script_index);
  hb_ot_layout_script_find_language (c->face, table_tag, script_index,
				     hb_ot_tag_from_language (c->buffer->props.language),
				     &language_index);


  hb_mask_allocator_t allocator;

  switch (c->original_direction) {
    case HB_DIRECTION_LTR:
      allocator.add_feature (HB_TAG ('l','t','r','a'), 1, true);
      allocator.add_feature (HB_TAG ('l','t','r','m'), 1, true);
      break;
    case HB_DIRECTION_RTL:
      allocator.add_feature (HB_TAG ('r','t','l','a'), 1, true);
      allocator.add_feature (HB_TAG ('r','t','l','m'), 1, false);
      break;
    case HB_DIRECTION_TTB:
    case HB_DIRECTION_BTT:
    default:
      break;
  }

  for (i = 0; i < ARRAY_LENGTH (default_features); i++)
    allocator.add_feature (default_features[i], 1, true);

  /* XXX complex-shaper features go here */

  for (unsigned int i = 0; i < c->num_features; i++) {
    const hb_feature_t *feature = &c->features[i];
    allocator.add_feature (feature->tag, feature->value, (feature->start == 0 && feature->end == (unsigned int) -1));
  }


  /* Compile features */
  allocator.compile (c->face, table_tag, script_index, language_index);


  /* Gather lookup indices for features and set buffer masks at the same time */

  if (hb_ot_layout_language_get_required_feature_index (c->face, table_tag, script_index, language_index,
							&feature_index))
    add_feature (c->face, table_tag, feature_index, 1, lookups, num_lookups, room_lookups);

  const hb_mask_allocator_t::feature_map_t *map;

  hb_mask_t global_mask = allocator.get_global_mask ();
  if (global_mask)
    c->buffer->set_masks (global_mask, global_mask, 0, (unsigned int) -1);

  switch (c->original_direction) {
    case HB_DIRECTION_LTR:
      map = allocator.find_feature (HB_TAG ('l','t','r','a'));
      add_feature (c->face, table_tag, map->index, map->mask, lookups, num_lookups, room_lookups);
      map = allocator.find_feature (HB_TAG ('l','t','r','m'));
      add_feature (c->face, table_tag, map->index, map->mask, lookups, num_lookups, room_lookups);
      break;
    case HB_DIRECTION_RTL:
      map = allocator.find_feature (HB_TAG ('r','t','l','a'));
      add_feature (c->face, table_tag, map->index, map->mask, lookups, num_lookups, room_lookups);
      map = allocator.find_feature (HB_TAG ('r','t','l','m'));
      add_feature (c->face, table_tag, map->index, map->mask, lookups, num_lookups, room_lookups);
      break;
    case HB_DIRECTION_TTB:
    case HB_DIRECTION_BTT:
    default:
      break;
  }

  for (i = 0; i < ARRAY_LENGTH (default_features); i++)
  {
    map = allocator.find_feature (default_features[i]);
    add_feature (c->face, table_tag, map->index, map->mask, lookups, num_lookups, room_lookups);
  }

  for (i = 0; i < c->num_features; i++)
  {
    hb_feature_t *feature = &c->features[i];
    map = allocator.find_feature (feature->tag);
    add_feature (c->face, table_tag, map->index, map->mask, lookups, num_lookups, room_lookups);
    if (!(feature->start == 0 && feature->end == (unsigned int)-1))
      c->buffer->set_masks (feature->value << map->shift, map->mask, feature->start, feature->end);
  }


  /* Sort lookups and merge duplicates */

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


static void
hb_ot_substitute_complex (hb_ot_shape_context_t *c)
{
  lookup_map lookups[1000]; /* FIXME */
  unsigned int num_lookups = ARRAY_LENGTH (lookups);
  unsigned int i;

  if (!hb_ot_layout_has_substitution (c->face))
    return;

  setup_lookups (c, HB_OT_TAG_GSUB, lookups, &num_lookups);

  for (i = 0; i < num_lookups; i++)
    hb_ot_layout_substitute_lookup (c->face, c->buffer, lookups[i].index, lookups[i].mask);

  c->applied_substitute_complex = TRUE;
  return;
}

static void
hb_ot_position_complex (hb_ot_shape_context_t *c)
{
  lookup_map lookups[1000]; /* FIXME */
  unsigned int num_lookups = ARRAY_LENGTH (lookups);
  unsigned int i;

  if (!hb_ot_layout_has_positioning (c->face))
    return;

  setup_lookups (c, HB_OT_TAG_GPOS, lookups, &num_lookups);

  for (i = 0; i < num_lookups; i++)
    hb_ot_layout_position_lookup (c->font, c->face, c->buffer, lookups[i].index, lookups[i].mask);

  hb_ot_layout_position_finish (c->font, c->face, c->buffer);

  c->applied_position_complex = TRUE;
  return;
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
    if (buffer->unicode->v.get_general_category (buffer->info[i].codepoint) == HB_CATEGORY_NON_SPACING_MARK)
      buffer->info[i].cluster = buffer->info[i - 1].cluster;
}

static void
hb_ensure_native_direction (hb_buffer_t *buffer)
{
  hb_direction_t direction = buffer->props.direction;

  /* TODO vertical */
  if (HB_DIRECTION_IS_HORIZONTAL (direction) &&
      direction != _hb_script_get_horizontal_direction (buffer->props.script))
  {
    hb_buffer_reverse_clusters (buffer);
    buffer->props.direction = HB_DIRECTION_REVERSE (buffer->props.direction);
  }
}


/* Substitute */

static void
hb_mirror_chars (hb_buffer_t *buffer)
{
  hb_unicode_get_mirroring_func_t get_mirroring = buffer->unicode->v.get_mirroring;

  if (HB_DIRECTION_IS_FORWARD (buffer->props.direction))
    return;

//  map = allocator.find_feature (HB_TAG ('r','t','l','m'));

  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++) {
    hb_codepoint_t codepoint = get_mirroring (buffer->info[i].codepoint);
    if (likely (codepoint == buffer->info[i].codepoint))
;//      buffer->info[i].mask |= map->mask;
    else
      buffer->info[i].codepoint = codepoint;
  }
}

static void
hb_map_glyphs (hb_font_t    *font,
	       hb_face_t    *face,
	       hb_buffer_t  *buffer)
{
  if (unlikely (!buffer->len))
    return;

  buffer->clear_output ();
  unsigned int count = buffer->len - 1;
  for (buffer->i = 0; buffer->i < count;) {
    if (unlikely (is_variation_selector (buffer->info[buffer->i + 1].codepoint))) {
      buffer->add_output_glyph (hb_font_get_glyph (font, face, buffer->info[buffer->i].codepoint, buffer->info[buffer->i + 1].codepoint));
      buffer->i++;
    } else {
      buffer->add_output_glyph (hb_font_get_glyph (font, face, buffer->info[buffer->i].codepoint, 0));
    }
  }
  if (likely (buffer->i < buffer->len))
    buffer->add_output_glyph (hb_font_get_glyph (font, face, buffer->info[buffer->i].codepoint, 0));
  buffer->swap ();
}

static void
hb_substitute_default (hb_ot_shape_context_t *c)
{
  hb_map_glyphs (c->font, c->face, c->buffer);
}

static void
hb_substitute_complex_fallback (hb_ot_shape_context_t *c HB_UNUSED)
{
  /* TODO Arabic */
}


/* Position */

static void
hb_position_default (hb_ot_shape_context_t *c)
{
  hb_buffer_clear_positions (c->buffer);

  unsigned int count = c->buffer->len;
  for (unsigned int i = 0; i < count; i++) {
    hb_glyph_metrics_t metrics;
    hb_font_get_glyph_metrics (c->font, c->face, c->buffer->info[i].codepoint, &metrics);
    c->buffer->pos[i].x_advance = metrics.x_advance;
    c->buffer->pos[i].y_advance = metrics.y_advance;
  }
}

static void
hb_position_complex_fallback (hb_ot_shape_context_t *c HB_UNUSED)
{
  /* TODO Mark pos */
}

static void
hb_truetype_kern (hb_ot_shape_context_t *c)
{
  /* TODO Check for kern=0 */
  unsigned int count = c->buffer->len;
  for (unsigned int i = 1; i < count; i++) {
    hb_position_t kern, kern1, kern2;
    kern = hb_font_get_kerning (c->font, c->face, c->buffer->info[i - 1].codepoint, c->buffer->info[i].codepoint);
    kern1 = kern >> 1;
    kern2 = kern - kern1;
    c->buffer->pos[i - 1].x_advance += kern1;
    c->buffer->pos[i].x_advance += kern2;
    c->buffer->pos[i].x_offset += kern2;
  }
}

static void
hb_position_complex_fallback_visual (hb_ot_shape_context_t *c)
{
  hb_truetype_kern (c);
}


/* Do it! */

static void
hb_ot_shape_internal (hb_ot_shape_context_t *c)
{
  hb_form_clusters (c->buffer);

  /* SUBSTITUTE */
  {
    c->buffer->clear_masks ();

    /* Mirroring needs to see the original direction */
    hb_mirror_chars (c->buffer);

    hb_ensure_native_direction (c->buffer);

    hb_substitute_default (c);

    hb_ot_substitute_complex (c);

    if (!c->applied_substitute_complex)
      hb_substitute_complex_fallback (c);
  }

  /* POSITION */
  {
    c->buffer->clear_masks ();

    hb_position_default (c);

    hb_ot_position_complex (c);

    hb_bool_t position_fallback = !c->applied_position_complex;
    if (position_fallback)
      hb_position_complex_fallback (c);

    if (HB_DIRECTION_IS_BACKWARD (c->buffer->props.direction))
      hb_buffer_reverse (c->buffer);

    if (position_fallback)
      hb_position_complex_fallback_visual (c);
  }

  c->buffer->props.direction = c->original_direction;
}

void
hb_ot_shape (hb_font_t    *font,
	     hb_face_t    *face,
	     hb_buffer_t  *buffer,
	     hb_feature_t *features,
	     unsigned int  num_features)
{
  hb_ot_shape_context_t c = {font, face, buffer, features, num_features};

  /* Setup transient context members */
  c.original_direction = buffer->props.direction;

  hb_ot_shape_internal (&c);
}


HB_END_DECLS
