/*
 * Copyright © 2009,2010  Red Hat, Inc.
 * Copyright © 2010,2011,2013  Google, Inc.
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
#include "hb-ot-layout-private.hh"



#ifndef HB_DEBUG_MAP
#define HB_DEBUG_MAP (HB_DEBUG+0)
#endif


#include "hb-ot-layout-private.hh"


void
hb_ot_map_t::add_lookups (hb_face_t    *face,
			  unsigned int  table_index,
			  unsigned int  feature_index,
			  hb_mask_t     mask,
			  bool          auto_zwj)
{
  unsigned int lookup_indices[32];
  unsigned int offset, len;

  offset = 0;
  do {
    len = ARRAY_LENGTH (lookup_indices);
    hb_ot_layout_feature_get_lookups (face,
				      table_tags[table_index],
				      feature_index,
				      offset, &len,
				      lookup_indices);

    for (unsigned int i = 0; i < len; i++) {
      hb_ot_map_t::lookup_map_t *lookup = lookups[table_index].push ();
      if (unlikely (!lookup))
        return;
      lookup->mask = mask;
      lookup->index = lookup_indices[i];
      lookup->auto_zwj = auto_zwj;
    }

    offset += len;
  } while (len == ARRAY_LENGTH (lookup_indices));
}

hb_ot_map_builder_t::hb_ot_map_builder_t (hb_face_t *face_,
					  const hb_segment_properties_t *props_)
{
  memset (this, 0, sizeof (*this));

  face = face_;
  props = *props_;


  /* Fetch script/language indices for GSUB/GPOS.  We need these later to skip
   * features not available in either table and not waste precious bits for them. */

  hb_tag_t script_tags[3] = {HB_TAG_NONE, HB_TAG_NONE, HB_TAG_NONE};
  hb_tag_t language_tag;

  hb_ot_tags_from_script (props.script, &script_tags[0], &script_tags[1]);
  language_tag = hb_ot_tag_from_language (props.language);

  for (unsigned int table_index = 0; table_index < 2; table_index++) {
    hb_tag_t table_tag = table_tags[table_index];
    found_script[table_index] = hb_ot_layout_table_choose_script (face, table_tag, script_tags, &script_index[table_index], &chosen_script[table_index]);
    hb_ot_layout_script_find_language (face, table_tag, script_index[table_index], language_tag, &language_index[table_index]);
  }
}

void hb_ot_map_builder_t::add_feature (hb_tag_t tag, unsigned int value,
				       hb_ot_map_feature_flags_t flags)
{
  feature_info_t *info = feature_infos.push();
  if (unlikely (!info)) return;
  info->tag = tag;
  info->seq = feature_infos.len;
  info->max_value = value;
  info->flags = flags;
  info->default_value = (flags & F_GLOBAL) ? value : 0;
  info->stage[0] = current_stage[0];
  info->stage[1] = current_stage[1];
}


static inline bool
may_skip (const hb_glyph_info_t &info, hb_set_t *coverage)
{
  return !(info.glyph_props() & HB_OT_LAYOUT_GLYPH_PROPS_MARK) &&
	 !coverage->has (info.codepoint);
}

inline void hb_ot_map_t::apply (unsigned int table_index,
				const hb_ot_shape_plan_t *plan,
				hb_font_t *font,
				hb_buffer_t *buffer) const
{
  unsigned int i = 0;
  unsigned int b = 0;

  for (unsigned int stage_index = 0; stage_index < stages[table_index].len; stage_index++)
  {
    const stage_map_t stage = stages[table_index][stage_index];

    for (; b < batches[table_index].len && batches[table_index][b].last_lookup <= stage.last_lookup; b++)
    {
      const batch_map_t batch = batches[table_index][b];

      if (!batch.coverage)
      {
        switch (table_index)
	{
	  case 0:
	    for (; i < batch.last_lookup; i++)
	    {
	      buffer->clear_output ();
	      hb_ot_layout_substitute_lookup (font, buffer, lookups[table_index][i].index,
					      0, (unsigned int) -1,
					      lookups[table_index][i].mask,
					      lookups[table_index][i].auto_zwj);
	      if (buffer->have_output)
		buffer->swap_buffers ();
	    }
	    break;
	  case 1:
	    for (; i < batch.last_lookup; i++)
	      hb_ot_layout_position_lookup (font, buffer, lookups[table_index][i].index,
					    0, (unsigned int) -1,
					    lookups[table_index][i].mask,
					    lookups[table_index][i].auto_zwj);
	    break;
	}
      }
      else
      {
	unsigned int start = 0, end = 0;
	if (table_index == 0)
	  buffer->clear_output ();
	for (;;)
	{
	  while (start < buffer->len && may_skip (buffer->info[start], batch.coverage))
	    start++;
	  if (start >= buffer->len)
	    break;
	  end = start + 1;
	  while (end < buffer->len && !may_skip (buffer->info[end], batch.coverage))
	    end++;

	  switch (table_index)
	  {
	    case 0:
	      for (unsigned int j = i; j < batch.last_lookup; j++)
		hb_ot_layout_substitute_lookup (font, buffer, lookups[table_index][j].index,
						start, end,
						lookups[table_index][j].mask,
						lookups[table_index][j].auto_zwj);
	      break;
	    case 1:
	      for (unsigned int j = i; j < batch.last_lookup; j++)
		hb_ot_layout_position_lookup (font, buffer, lookups[table_index][j].index,
					      start, end,
					      lookups[table_index][j].mask,
					      lookups[table_index][j].auto_zwj);
	      break;
	  }

	  start = end + 1;
	}

	assert (!buffer->has_separate_output ());
	i = batch.last_lookup;
      }
    }

    if (stage.pause_func)
      stage.pause_func (plan, font, buffer);
  }
}

void hb_ot_map_t::substitute (const hb_ot_shape_plan_t *plan, hb_font_t *font, hb_buffer_t *buffer) const
{
  apply (0, plan, font, buffer);
}

void hb_ot_map_t::position (const hb_ot_shape_plan_t *plan, hb_font_t *font, hb_buffer_t *buffer) const
{
  apply (1, plan, font, buffer);
}


void hb_ot_map_t::collect_lookups (unsigned int table_index, hb_set_t *lookups_out) const
{
  for (unsigned int i = 0; i < lookups[table_index].len; i++)
    hb_set_add (lookups_out, lookups[table_index][i].index);
}

void hb_ot_map_builder_t::add_pause (unsigned int table_index, hb_ot_map_t::pause_func_t pause_func)
{
  stage_info_t *s = stages[table_index].push ();
  if (likely (s)) {
    s->index = current_stage[table_index];
    s->pause_func = pause_func;
  }

  current_stage[table_index]++;
}

void
hb_ot_map_builder_t::compile (hb_ot_map_t &m)
{
  m.global_mask = 1;

  for (unsigned int table_index = 0; table_index < 2; table_index++) {
    m.chosen_script[table_index] = chosen_script[table_index];
    m.found_script[table_index] = found_script[table_index];
  }

  if (!feature_infos.len)
    return;

  /* Sort features and merge duplicates */
  {
    feature_infos.sort ();
    unsigned int j = 0;
    for (unsigned int i = 1; i < feature_infos.len; i++)
      if (feature_infos[i].tag != feature_infos[j].tag)
	feature_infos[++j] = feature_infos[i];
      else {
	if (feature_infos[i].flags & F_GLOBAL) {
	  feature_infos[j].flags |= F_GLOBAL;
	  feature_infos[j].max_value = feature_infos[i].max_value;
	  feature_infos[j].default_value = feature_infos[i].default_value;
	} else {
	  feature_infos[j].flags &= ~F_GLOBAL;
	  feature_infos[j].max_value = MAX (feature_infos[j].max_value, feature_infos[i].max_value);
	  /* Inherit default_value from j */
	}
	feature_infos[j].flags |= (feature_infos[i].flags & F_HAS_FALLBACK);
	feature_infos[j].stage[0] = MIN (feature_infos[j].stage[0], feature_infos[i].stage[0]);
	feature_infos[j].stage[1] = MIN (feature_infos[j].stage[1], feature_infos[i].stage[1]);
      }
    feature_infos.shrink (j + 1);
  }


  /* Allocate bits now */
  unsigned int next_bit = 1;
  for (unsigned int i = 0; i < feature_infos.len; i++) {
    const feature_info_t *info = &feature_infos[i];

    unsigned int bits_needed;

    if ((info->flags & F_GLOBAL) && info->max_value == 1)
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
    if (!found && !(info->flags & F_HAS_FALLBACK))
      continue;


    hb_ot_map_t::feature_map_t *map = m.features.push ();
    if (unlikely (!map))
      break;

    map->tag = info->tag;
    map->index[0] = feature_index[0];
    map->index[1] = feature_index[1];
    map->stage[0] = info->stage[0];
    map->stage[1] = info->stage[1];
    map->auto_zwj = !(info->flags & F_MANUAL_ZWJ);
    if ((info->flags & F_GLOBAL) && info->max_value == 1) {
      /* Uses the global bit */
      map->shift = 0;
      map->mask = 1;
    } else {
      map->shift = next_bit;
      map->mask = (1 << (next_bit + bits_needed)) - (1 << next_bit);
      next_bit += bits_needed;
      m.global_mask |= (info->default_value << map->shift) & map->mask;
    }
    map->_1_mask = (1 << map->shift) & map->mask;
    map->needs_fallback = !found;

  }
  feature_infos.shrink (0); /* Done with these */


  add_gsub_pause (NULL);
  add_gpos_pause (NULL);

  for (unsigned int table_index = 0; table_index < 2; table_index++) {
    hb_tag_t table_tag = table_tags[table_index];

    /* Collect lookup indices for features */

    unsigned int required_feature_index;
    if (hb_ot_layout_language_get_required_feature_index (face,
							  table_tag,
							  script_index[table_index],
							  language_index[table_index],
							  &required_feature_index))
      m.add_lookups (face, table_index, required_feature_index, 1, true);

    unsigned int stage_index = 0;
    unsigned int last_num_lookups = 0;
    for (unsigned stage = 0; stage < current_stage[table_index]; stage++)
    {
      for (unsigned i = 0; i < m.features.len; i++)
        if (m.features[i].stage[table_index] == stage)
	  m.add_lookups (face, table_index,
			 m.features[i].index[table_index],
			 m.features[i].mask,
			 m.features[i].auto_zwj);

      /* Sort lookups and merge duplicates */
      if (last_num_lookups < m.lookups[table_index].len)
      {
	m.lookups[table_index].sort (last_num_lookups, m.lookups[table_index].len);

	unsigned int j = last_num_lookups;
	for (unsigned int i = j + 1; i < m.lookups[table_index].len; i++)
	  if (m.lookups[table_index][i].index != m.lookups[table_index][j].index)
	    m.lookups[table_index][++j] = m.lookups[table_index][i];
	  else
	  {
	    m.lookups[table_index][j].mask |= m.lookups[table_index][i].mask;
	    m.lookups[table_index][j].auto_zwj &= m.lookups[table_index][i].auto_zwj;
	  }
	m.lookups[table_index].shrink (j + 1);
      }

      last_num_lookups = m.lookups[table_index].len;

      if (stage_index < stages[table_index].len && stages[table_index][stage_index].index == stage) {
	hb_ot_map_t::stage_map_t *stage_map = m.stages[table_index].push ();
	if (likely (stage_map)) {
	  stage_map->last_lookup = last_num_lookups;
	  stage_map->pause_func = stages[table_index][stage_index].pause_func;
	}

	stage_index++;
      }
    }
  }

  m.optimize (face);
}


struct optimize_lookups_context_t
{
  inline optimize_lookups_context_t (hb_ot_map_t *map_,
				     hb_face_t *face_,
				     unsigned int table_index_) :
				map (map_),
				face (face_),
				table_index (table_index_),
				start_lookup (0),
				num_lookups (0),
				num_glyphs (hb_face_get_glyph_count (face_)) {}

  inline void add_lookups (unsigned int start_lookup_,
			   unsigned int num_lookups_)
  {
    set_lookups (start_lookup_, num_lookups_);
    solve ();
  }

  private:

  inline void set_lookups (unsigned int start_lookup_,
			   unsigned int num_lookups_)
  {
    start_lookup = start_lookup_;
    num_lookups = num_lookups_;
  }

  inline void
  solve (void)
  {
    if (!num_lookups)
      return;

    DEBUG_MSG_FUNC (MAP, map, "%d lookups", num_lookups);

    hb_set_t *cov = hb_set_create ();

    int best_move[num_lookups];
    float best_cost[num_lookups];
    best_move[0] = -1;
    best_cost[0] = single_lookup_cost (0);
    for (unsigned int i = 1; i < num_lookups; i++)
    {
      cov->clear ();
      add_coverage (i, cov);
      float this_cost = single_lookup_cost (i);
      best_cost[i] = 1e20;
      for (unsigned int j = i - 1; (int) j >= 0; j--)
      {
	if (best_cost[i] > best_cost[j] + this_cost)
	{
	  best_cost[i] = best_cost[j] + this_cost;
	  best_move[i] = j;
	}
	add_coverage (j, cov);
	this_cost = lookup_batch_cost (cov, i - j + 1);
        if (this_cost > best_cost[i])
	  break; /* No chance */
      }
      if (best_cost[i] > this_cost)
      {
	best_cost[i] = this_cost;
	best_move[i] = -1;
      }
    }

    DEBUG_MSG_FUNC (MAP, map, "optimal solution costs %f", best_cost[num_lookups - 1]);
    for (int i = num_lookups - 1; i >= 0; i = best_move[i])
    {
      unsigned int batch_num_lookups = i - best_move[i];
      if (DEBUG_LEVEL_ENABLED (MAP, 1))
	DEBUG_MSG_FUNC (MAP, map, "batch has %d lookups; costs %f",
			batch_num_lookups,
			best_cost[i] - (best_move[i] == -1 ? 0 : best_cost[best_move[i]]));

      hb_ot_map_t::batch_map_t *batch = map->batches[table_index].push ();
      if (batch)
      {
	batch->last_lookup = MAX (lookup_offset (i), lookup_offset (best_move[i] + 1)) + 1;
	batch->coverage = batch_num_lookups == 1 ? NULL : hb_set_create ();

	for (int j = i; j > best_move[i]; j--)
	{
	  if (batch->coverage)
	    add_coverage (j, batch->coverage);
	  if (DEBUG_LEVEL_ENABLED (MAP, 2))
	  {
	    cov->clear ();
	    add_coverage (j, cov);
	    DEBUG_MSG_FUNC (MAP, map, "lookup %d (lookup-index %d) popcount %d",
			    lookup_offset (j),
			    lookup_index (j),
			    cov->get_population ());
	  }
	}
      }
    }

    hb_set_destroy (cov);
  }

  inline unsigned int lookup_offset (unsigned int i)
  {
    assert (i < num_lookups);
    return start_lookup + num_lookups - 1 - i;
  }

  inline unsigned int lookup_index (unsigned int i)
  {
    return map->lookups[table_index][lookup_offset (i)].index;
  }

  inline void add_coverage (unsigned int i, hb_set_t *coverage)
  {
    hb_ot_layout_lookup_get_coverage (face,
				      table_tags[table_index],
				      lookup_index (i),
				      coverage);
  }

  /* Parameters */

  inline float single_lookup_cost (unsigned int lookup_index HB_UNUSED)
  {
    return 1.0;
  }
  inline float
  lookup_batch_cost (hb_set_t *cov, unsigned int n_lookups)
  {
    return .1 + probability (cov) * n_lookups;
  }
  inline float
  probability (hb_set_t *cov)
  {
    /* Biggest hack: assume uniform glyph probability. */
    return cov->get_population () / (float) num_glyphs;
  }

  private:

  hb_ot_map_t *map;
  hb_face_t *face;
  unsigned int table_index;
  unsigned int start_lookup;
  unsigned int num_lookups;

  unsigned int num_glyphs;
};

void
hb_ot_map_t::optimize (hb_face_t *face)
{
  DEBUG_MSG_FUNC (MAP, this, "face %p", face);
  for (unsigned int table_index = 0; table_index < 2; table_index++)
  {
    DEBUG_MSG_FUNC (MAP, this, "table %c%c%c%c", HB_UNTAG (table_tags[table_index]));

    unsigned int i = 0;

    for (unsigned int stage_index = 0; stage_index < stages[table_index].len; stage_index++)
    {
      stage_map_t *stage = &stages[table_index][stage_index];
      unsigned int num_lookups = stage->last_lookup - i;
      DEBUG_MSG_FUNC (MAP, this, "stage %d: %d lookups", stage_index, num_lookups);

      optimize_lookups_context_t c (this, face, table_index);
      unsigned int start = i;
      for (; i < num_lookups; i++)
	if (!hb_ot_layout_lookup_is_inplace (face, table_tags[table_index],
					     lookups[table_index][i].index))
	{
	  DEBUG_MSG_FUNC (MAP, this, "lookup %d (lookup-index %d) NOT inplace",
			  i, lookups[table_index][i].index);

	  c.add_lookups (start, i - start);
	  c.add_lookups (i, 1);
	  start = i + 1;
	}
      c.add_lookups (start, i - start);
    }
  }
}
