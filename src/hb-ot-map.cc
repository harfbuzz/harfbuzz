/*
 * Copyright © 2009,2010  Red Hat, Inc.
 * Copyright © 2010  Google, Inc.
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

#include "hb-ot-map-private.hh"

#include "hb-ot-shape-private.hh"

HB_BEGIN_DECLS


void
hb_ot_map_t::add_lookups (hb_face_t    *face,
			  unsigned int  table_index,
			  unsigned int  feature_index,
			  hb_mask_t     mask)
{
  unsigned int lookup_indices[32];
  unsigned int offset, len;

  offset = 0;
  do {
    len = ARRAY_LENGTH (lookup_indices);
    hb_ot_layout_feature_get_lookup_indexes (face,
					     table_tags[table_index],
					     feature_index,
					     offset, &len,
					     lookup_indices);

    for (unsigned int i = 0; i < len; i++) {
      lookup_map_t *lookup = lookup_maps[table_index].push ();
      if (unlikely (!lookup))
        return;
      lookup->mask = mask;
      lookup->index = lookup_indices[i];
    }

    offset += len;
  } while (len == ARRAY_LENGTH (lookup_indices));
}


void
hb_ot_map_t::compile (hb_face_t *face,
		      const hb_segment_properties_t *props)
{
 global_mask = 1;

  if (!feature_infos.len)
    return;


  /* Fetch script/language indices for GSUB/GPOS.  We need these later to skip
   * features not available in either table and not waste precious bits for them. */

  hb_tag_t script_tags[3] = {HB_TAG_NONE};
  hb_tag_t language_tag;

  hb_ot_tags_from_script (props->script, &script_tags[0], &script_tags[1]);
  language_tag = hb_ot_tag_from_language (props->language);

  unsigned int script_index[2], language_index[2];
  for (unsigned int table_index = 0; table_index < 2; table_index++) {
    hb_tag_t table_tag = table_tags[table_index];
    hb_ot_layout_table_choose_script (face, table_tag, script_tags, &script_index[table_index]);
    hb_ot_layout_script_find_language (face, table_tag, script_index[table_index], language_tag, &language_index[table_index]);
  }


  /* Sort features and merge duplicates */
  feature_infos.sort ();
  unsigned int j = 0;
  for (unsigned int i = 1; i < feature_infos.len; i++)
    if (feature_infos[i].tag != feature_infos[j].tag)
      feature_infos[++j] = feature_infos[i];
    else {
      if (feature_infos[i].global)
	feature_infos[j] = feature_infos[i];
      else {
	feature_infos[j].global = false;
	feature_infos[j].max_value = MAX (feature_infos[j].max_value, feature_infos[i].max_value);
	/* Inherit default_value from j */
      }
    }
  feature_infos.shrink (j + 1);


  /* Allocate bits now */
  unsigned int next_bit = 1;
  for (unsigned int i = 0; i < feature_infos.len; i++) {
    const feature_info_t *info = &feature_infos[i];

    unsigned int bits_needed;

    if (info->global && info->max_value == 1)
      /* Uses the global bit */
      bits_needed = 0;
    else
      bits_needed = _hb_bit_storage (info->max_value);

    if (!info->max_value || next_bit + bits_needed > 8 * sizeof (hb_mask_t))
      continue; /* Feature disabled, or not enough bits. */


    bool found = false;
    unsigned int feature_index[2];
    for (unsigned int table_index = 0; table_index < 2; table_index++)
      found |= hb_ot_layout_language_find_feature (face,
						   table_tags[table_index],
						   script_index[table_index],
						   language_index[table_index],
						   info->tag,
						   &feature_index[table_index]);
    if (!found)
      continue;


    feature_map_t *map = feature_maps.push ();
    if (unlikely (!map))
      break;

    map->tag = info->tag;
    map->index[0] = feature_index[0];
    map->index[1] = feature_index[1];
    if (info->global && info->max_value == 1) {
      /* Uses the global bit */
      map->shift = 0;
      map->mask = 1;
    } else {
      map->shift = next_bit;
      map->mask = (1 << (next_bit + bits_needed)) - (1 << next_bit);
      next_bit += bits_needed;
      if (info->global)
	global_mask |= (info->default_value << map->shift) & map->mask;
    }
    map->_1_mask = (1 << map->shift) & map->mask;

  }
  feature_infos.shrink (0); /* Done with these */


  for (unsigned int table_index = 0; table_index < 2; table_index++) {
    hb_tag_t table_tag = table_tags[table_index];

    /* Collect lookup indices for features */

    unsigned int required_feature_index;
    if (hb_ot_layout_language_get_required_feature_index (face,
							  table_tag,
							  script_index[table_index],
							  language_index[table_index],
							  &required_feature_index))
      add_lookups (face, table_index, required_feature_index, 1);

    for (unsigned i = 0; i < feature_maps.len; i++)
      add_lookups (face, table_index, feature_maps[i].index[table_index], feature_maps[i].mask);

    /* Sort lookups and merge duplicates */
    lookup_maps[table_index].sort ();
    if (lookup_maps[table_index].len)
    {
      unsigned int j = 0;
      for (unsigned int i = 1; i < lookup_maps[table_index].len; i++)
	if (lookup_maps[table_index][i].index != lookup_maps[table_index][j].index)
	  lookup_maps[table_index][++j] = lookup_maps[table_index][i];
	else
	  lookup_maps[table_index][j].mask |= lookup_maps[table_index][i].mask;
      lookup_maps[table_index].shrink (j + 1);
    }
  }
}


HB_END_DECLS
