/*
 * Copyright (C) 2009,2010  Red Hat, Inc.
 * Copyright (C) 2010  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
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

#define MAX_FEATURES 100 /* FIXME */
#define MAX_LOOKUPS 1000 /* FIXME */

static const hb_tag_t table_tags[2] = {HB_OT_TAG_GSUB, HB_OT_TAG_GPOS};


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
    hb_tag_t tag; /* should be first for our bsearch to work */
    unsigned int index[2]; /* GSUB, GPOS */
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

  struct lookup_map_t {
    unsigned int index;
    hb_mask_t mask;

    static int
    cmp (const void *p1, const void *p2)
    {
      const lookup_map_t *a = reinterpret_cast<const lookup_map_t *>(p1);
      const lookup_map_t *b = reinterpret_cast<const lookup_map_t *>(p2);

      return a->index < b->index ? -1 : a->index > b->index ? 1 : 0;
    }
  };


  void
  add_lookups (hb_ot_shape_context_t *c,
	       unsigned int  table_index,
	       unsigned int  feature_index,
	       hb_mask_t     mask)
  {
    unsigned int i = MAX_LOOKUPS - lookup_count[table_index];
    lookup_map_t *lookups = lookup_maps[table_index] + lookup_count[table_index];

    unsigned int *lookup_indices = (unsigned int *) lookups;

    hb_ot_layout_feature_get_lookup_indexes (c->face,
					     table_tags[table_index],
					     feature_index,
					     0, &i,
					     lookup_indices);

    lookup_count[table_index] += i;

    while (i--) {
      lookups[i].mask = mask;
      lookups[i].index = lookup_indices[i];
    }
  }




  hb_mask_allocator_t (void) : feature_count (0) {}

  void add_feature (hb_tag_t tag,
		    unsigned int value,
		    bool global)
  {
    feature_info_t *info = &feature_infos[feature_count++];
    info->tag = tag;
    info->value = value;
    info->seq = feature_count;
    info->global = global;
  }

  void compile (hb_ot_shape_context_t *c)
  {
   global_mask = 0;
   lookup_count[0] = lookup_count[1] = 0;

    if (!feature_count)
      return;


    /* Fetch script/language indices for GSUB/GPOS.  We need these later to skip
     * features not available in either table and not waste precious bits for them. */

    const hb_tag_t *script_tags;
    hb_tag_t language_tag;

    script_tags = hb_ot_tags_from_script (c->buffer->props.script);
    language_tag = hb_ot_tag_from_language (c->buffer->props.language);

    unsigned int script_index[2], language_index[2];
    for (unsigned int table_index = 0; table_index < 2; table_index++) {
      hb_tag_t table_tag = table_tags[table_index];
      hb_ot_layout_table_choose_script (c->face, table_tag, script_tags, &script_index[table_index]);
      hb_ot_layout_script_find_language (c->face, table_tag, script_index[table_index], language_tag, &language_index[table_index]);
    }


    /* Sort the features so we can bsearch later */
    qsort (feature_infos, feature_count, sizeof (feature_infos[0]), feature_info_t::cmp);

    /* Remove dups, let later-occurring features override earlier ones. */
    unsigned int j = 0;
    for (unsigned int i = 1; i < feature_count; i++)
      if (feature_infos[i].tag != feature_infos[j].tag)
	feature_infos[++j] = feature_infos[i];
      else {
	if (feature_infos[i].global)
	  feature_infos[j] = feature_infos[i];
	else {
	  feature_infos[j].global = feature_infos[j].global && (feature_infos[j].value == feature_infos[i].value);
	  feature_infos[j].value = MAX (feature_infos[j].value, feature_infos[i].value);
	}
      }
    feature_count = j + 1;


    /* Allocate bits now */
    unsigned int next_bit = 1;
    j = 0;
    for (unsigned int i = 0; i < feature_count; i++) {
      const feature_info_t *info = &feature_infos[i];

      unsigned int bits_needed;

      if (info->global && info->value == 1)
        /* Uses the global bit */
        bits_needed = 0;
      else
        bits_needed = _hb_bit_storage (info->value);

      if (!info->value || next_bit + bits_needed > 8 * sizeof (hb_mask_t))
        continue; /* Feature disabled, or not enough bits. */


      bool found = false;
      unsigned int feature_index[2];
      for (unsigned int table_index = 0; table_index < 2; table_index++)
        found |= hb_ot_layout_language_find_feature (c->face,
						     table_tags[table_index],
						     script_index[table_index],
						     language_index[table_index],
						     info->tag,
						     &feature_index[table_index]);
      if (!found)
        continue;


      feature_map_t *map = &feature_maps[j++];

      map->tag = info->tag;
      map->index[0] = feature_index[0];
      map->index[1] = feature_index[1];
      if (info->global && info->value == 1) {
        /* Uses the global bit */
        map->shift = 0;
	map->mask = 1;
      } else {
	map->shift = next_bit;
	map->mask = (1 << (next_bit + bits_needed)) - (1 << next_bit);
	next_bit += bits_needed;
	if (info->global)
	  global_mask |= map->mask;
      }

    }
    feature_count = j;


    for (unsigned int table_index = 0; table_index < 2; table_index++) {
      hb_tag_t table_tag = table_tags[table_index];

      /* Collect lookup indices for features */

      unsigned int required_feature_index;
      if (hb_ot_layout_language_get_required_feature_index (c->face,
							    table_tag,
							    script_index[table_index],
							    language_index[table_index],
							    &required_feature_index))
	add_lookups (c, table_index, required_feature_index, 1);

      for (unsigned i = 0; i < feature_count; i++)
	add_lookups (c, table_index, feature_maps[i].index[table_index], feature_maps[i].mask);

      /* Sort lookups and merge duplicates */

      qsort (lookup_maps[table_index], lookup_count[table_index], sizeof (lookup_maps[table_index][0]), lookup_map_t::cmp);

      if (lookup_count[table_index])
      {
	unsigned int j = 0;
	for (unsigned int i = 1; i < lookup_count[table_index]; i++)
	  if (lookup_maps[table_index][i].index != lookup_maps[table_index][j].index)
	    lookup_maps[table_index][++j] = lookup_maps[table_index][i];
	  else
	    lookup_maps[table_index][j].mask |= lookup_maps[table_index][i].mask;
	j++;
	lookup_count[table_index] = j;
      }
    }
  }

  hb_mask_t get_global_mask (void) { return global_mask; }

  const feature_map_t *find_feature (hb_tag_t tag) const {
    static const feature_map_t off_map = { HB_TAG_NONE, {Index::NOT_FOUND_INDEX,Index::NOT_FOUND_INDEX}, 0, 0 };
    const feature_map_t *map = (const feature_map_t *) bsearch (&tag, feature_maps, feature_count, sizeof (feature_maps[0]), feature_map_t::cmp);
    return map ? map : &off_map;
  }

  void substitute (hb_ot_shape_context_t *c) const {
    for (unsigned int i = 0; i < lookup_count[0]; i++)
      hb_ot_layout_substitute_lookup (c->face, c->buffer, lookup_maps[0][i].index, lookup_maps[0][i].mask);
  }

  void position (hb_ot_shape_context_t *c) const {
    for (unsigned int i = 0; i < lookup_count[1]; i++)
      hb_ot_layout_position_lookup (c->font, c->face, c->buffer, lookup_maps[1][i].index, lookup_maps[1][i].mask);
  }

  private:

  hb_mask_t global_mask;

  unsigned int feature_count;
  feature_info_t feature_infos[MAX_FEATURES]; /* used before compile() only */
  feature_map_t feature_maps[MAX_FEATURES];

  lookup_map_t lookup_maps[2][MAX_LOOKUPS]; /* GSUB/GPOS */
  unsigned int lookup_count[2];
};

static void
hb_ot_shape_collect_features (hb_ot_shape_context_t *c,
			      hb_mask_allocator_t   *allocator)
{
  switch (c->original_direction) {
    case HB_DIRECTION_LTR:
      allocator->add_feature (HB_TAG ('l','t','r','a'), 1, true);
      allocator->add_feature (HB_TAG ('l','t','r','m'), 1, true);
      break;
    case HB_DIRECTION_RTL:
      allocator->add_feature (HB_TAG ('r','t','l','a'), 1, true);
      allocator->add_feature (HB_TAG ('r','t','l','m'), 1, false);
      break;
    case HB_DIRECTION_TTB:
    case HB_DIRECTION_BTT:
    default:
      break;
  }

  for (unsigned int i = 0; i < ARRAY_LENGTH (default_features); i++)
    allocator->add_feature (default_features[i], 1, true);

  /* complex */

  for (unsigned int i = 0; i < c->num_features; i++) {
    const hb_feature_t *feature = &c->features[i];
    allocator->add_feature (feature->tag, feature->value, (feature->start == 0 && feature->end == (unsigned int) -1));
  }
}


static void
hb_ot_shape_setup_lookups (hb_ot_shape_context_t *c,
			   hb_mask_allocator_t   *allocator)
{
  hb_ot_shape_collect_features (c, allocator);

  /* Compile features */
  allocator->compile (c);

  /* Set masks in buffer */

  hb_mask_t global_mask = allocator->get_global_mask ();
  if (global_mask)
    c->buffer->set_masks (global_mask, global_mask, 0, (unsigned int) -1);

  for (unsigned int i = 0; i < c->num_features; i++)
  {
    hb_feature_t *feature = &c->features[i];
    const hb_mask_allocator_t::feature_map_t *map = allocator->find_feature (feature->tag);
    if (!(feature->start == 0 && feature->end == (unsigned int)-1))
      c->buffer->set_masks (feature->value << map->shift, map->mask, feature->start, feature->end);
  }

  /* complex */
}


static void
hb_ot_substitute_complex (hb_ot_shape_context_t *c,
			  const hb_mask_allocator_t *allocator)
{
  if (!hb_ot_layout_has_substitution (c->face))
    return;

  allocator->substitute (c);

  c->applied_substitute_complex = TRUE;
  return;
}

static void
hb_ot_position_complex (hb_ot_shape_context_t *c,
			const hb_mask_allocator_t *allocator)
{

  if (!hb_ot_layout_has_positioning (c->face))
    return;

  allocator->position (c);

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
  hb_mask_allocator_t allocator;

  hb_ot_shape_setup_lookups (c, &allocator);


  hb_form_clusters (c->buffer);

  /* SUBSTITUTE */
  {
    c->buffer->clear_masks ();

    /* Mirroring needs to see the original direction */
    hb_mirror_chars (c->buffer);

    hb_ensure_native_direction (c->buffer);

    hb_substitute_default (c);

    hb_ot_substitute_complex (c, &allocator);

    if (!c->applied_substitute_complex)
      hb_substitute_complex_fallback (c);
  }

  /* POSITION */
  {
    c->buffer->clear_masks ();

    hb_position_default (c);

    hb_ot_position_complex (c, &allocator);

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
