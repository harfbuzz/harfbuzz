/*
 * Copyright © 2015  Mozilla Foundation.
 * Copyright © 2015  Google, Inc.
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
 * Mozilla Author(s): Jonathan Kew
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-ot-shape-complex-use-private.hh"

/* buffer var allocations */
#define use_category() complex_var_u8_0()


/*
 * Universal Shaping Engine.
 * https://www.microsoft.com/typography/OpenTypeDev/USE/intro.htm
 */

static const hb_tag_t
basic_features[] =
{
  /*
   * Basic features.
   * These features are applied all at once, before reordering.
   */
  HB_TAG('r','k','r','f'),
  HB_TAG('a','b','v','f'),
  HB_TAG('b','l','w','f'),
  HB_TAG('h','a','l','f'),
  HB_TAG('p','s','t','f'),
  HB_TAG('v','a','t','u'),
  HB_TAG('c','j','c','t'),
};
static const hb_tag_t
other_features[] =
{
  /*
   * Other features.
   * These features are applied all at once, after reordering.
   */
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('h','a','l','n'),
  HB_TAG('p','r','e','s'),
  HB_TAG('p','s','t','s'),
  /* Positioning features, though we don't care about the types. */
  HB_TAG('d','i','s','t'),
  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
};

static void
setup_syllables (const hb_ot_shape_plan_t *plan,
		 hb_font_t *font,
		 hb_buffer_t *buffer);
static void
clear_substitution_flags (const hb_ot_shape_plan_t *plan,
			  hb_font_t *font,
			  hb_buffer_t *buffer);
static void
record_rphf (const hb_ot_shape_plan_t *plan,
	     hb_font_t *font,
	     hb_buffer_t *buffer);
static void
record_pref (const hb_ot_shape_plan_t *plan,
	     hb_font_t *font,
	     hb_buffer_t *buffer);
static void
reorder (const hb_ot_shape_plan_t *plan,
	 hb_font_t *font,
	 hb_buffer_t *buffer);

static void
collect_features_use (hb_ot_shape_planner_t *plan)
{
  hb_ot_map_builder_t *map = &plan->map;

  /* Do this before any lookups have been applied. */
  map->add_gsub_pause (setup_syllables);

  /* "Default glyph pre-processing group" */
  map->add_global_bool_feature (HB_TAG('l','o','c','l'));
  map->add_global_bool_feature (HB_TAG('c','c','m','p'));
  map->add_global_bool_feature (HB_TAG('n','u','k','t'));
  map->add_global_bool_feature (HB_TAG('a','k','h','n'));

  /* "Reordering group" */
  map->add_gsub_pause (clear_substitution_flags);
  map->add_feature (HB_TAG('r','p','h','f'), 1, F_MANUAL_ZWJ);
  map->add_gsub_pause (record_rphf);
  map->add_feature (HB_TAG('p','r','e','f'), 1, F_GLOBAL | F_MANUAL_ZWJ);
  map->add_gsub_pause (record_pref);

  /* "Orthographic unit shaping group" */
  for (unsigned int i = 0; i < ARRAY_LENGTH (basic_features); i++)
    map->add_feature (basic_features[i], 1, F_GLOBAL | F_MANUAL_ZWJ);

  map->add_gsub_pause (reorder);

  /* "Topographical features" */
  // TODO isol/init/medi/fina

  /* "Standard typographic presentation" and "Positional feature application" */
  for (unsigned int i = 0; i < ARRAY_LENGTH (other_features); i++)
    map->add_feature (other_features[i], 1, F_GLOBAL | F_MANUAL_ZWJ);
}

struct use_shape_plan_t
{
  ASSERT_POD ();

  hb_mask_t rphf_mask;
};

static void *
data_create_use (const hb_ot_shape_plan_t *plan)
{
  use_shape_plan_t *use_plan = (use_shape_plan_t *) calloc (1, sizeof (use_shape_plan_t));
  if (unlikely (!use_plan))
    return NULL;

  use_plan->rphf_mask = plan->map.get_1_mask (HB_TAG('r','p','h','f'));

  return use_plan;
}

static void
data_destroy_use (void *data)
{
  free (data);
}

enum syllable_type_t {
  independent_cluster,
  virama_terminated_cluster,
  consonant_cluster,
  vowel_cluster,
  number_joiner_terminated_cluster,
  numeral_cluster,
  symbol_cluster,
};

#include "hb-ot-shape-complex-use-machine.hh"


static inline void
set_use_properties (hb_glyph_info_t &info)
{
  hb_codepoint_t u = info.codepoint;
  info.use_category() = hb_use_get_categories (u);
}


static void
setup_masks_use (const hb_ot_shape_plan_t *plan HB_UNUSED,
		 hb_buffer_t              *buffer,
		 hb_font_t                *font HB_UNUSED)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, use_category);

  /* We cannot setup masks here.  We save information about characters
   * and setup masks later on in a pause-callback. */

  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < count; i++)
    info[i].use_category() = hb_use_get_categories (info[i].codepoint);
}

static void
setup_syllable (const use_shape_plan_t *use_plan,
		hb_glyph_info_t *info,
		unsigned int start, unsigned int end)
{
  unsigned int limit = info[start].use_category() == USE_R ? 1 : MIN (3u, end - start);
  for (unsigned int i = start; i < start + limit; i++)
    info[i].mask |= use_plan->rphf_mask;
}

static void
setup_syllables (const hb_ot_shape_plan_t *plan,
		 hb_font_t *font HB_UNUSED,
		 hb_buffer_t *buffer)
{
  find_syllables (buffer);

  /* Setup masks for 'rphf' and 'pref'. */
  const use_shape_plan_t *use_plan = (const use_shape_plan_t *) plan->data;
  if (!use_plan->rphf_mask) return;

  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  if (unlikely (!count)) return;
  unsigned int last = 0;
  unsigned int last_syllable = info[0].syllable();
  for (unsigned int i = 1; i < count; i++)
    if (last_syllable != info[i].syllable()) {
      setup_syllable (use_plan, info, last, i);
      last = i;
      last_syllable = info[last].syllable();
    }
  setup_syllable (use_plan, info, last, count);
}

static void
clear_substitution_flags (const hb_ot_shape_plan_t *plan,
			  hb_font_t *font HB_UNUSED,
			  hb_buffer_t *buffer)
{
  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    _hb_glyph_info_clear_substituted_and_ligated_and_multiplied (&info[i]);
}

static void
record_rphf_syllable (hb_glyph_info_t *info,
		      unsigned int start, unsigned int end,
		      hb_mask_t mask)
{
  /* Mark a substituted repha as USE_R. */
  for (unsigned int i = start; i < end && (info[i].mask & mask); i++)
    if (_hb_glyph_info_substituted (&info[i]))
    {
      /* Found the one.  Mark it as Repha.  */
      info[i].use_category() = USE_R;
      return;
    }
}

static void
record_rphf (const hb_ot_shape_plan_t *plan,
	     hb_font_t *font,
	     hb_buffer_t *buffer)
{
  const use_shape_plan_t *use_plan = (const use_shape_plan_t *) plan->data;
  hb_mask_t mask = use_plan->rphf_mask;

  if (!mask) return;
  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  if (unlikely (!count)) return;
  unsigned int last = 0;
  unsigned int last_syllable = info[0].syllable();
  for (unsigned int i = 1; i < count; i++)
    if (last_syllable != info[i].syllable()) {
      record_rphf_syllable (info, last, i, mask);
      last = i;
      last_syllable = info[last].syllable();
    }
  record_rphf_syllable (info, last, count, mask);
}

static void
record_pref_syllable (hb_glyph_info_t *info,
		      unsigned int start, unsigned int end)
{
  /* Mark a substituted pref as VPre, as they behave the same way. */
  for (unsigned int i = start; i < end; i++)
    if (_hb_glyph_info_substituted (&info[i]))
    {
      /* Found the one.  Mark it as Repha.  */
      info[i].use_category() = USE_VPre;
      return;
    }
}

static void
record_pref (const hb_ot_shape_plan_t *plan,
	     hb_font_t *font,
	     hb_buffer_t *buffer)
{
  const use_shape_plan_t *use_plan = (const use_shape_plan_t *) plan->data;

  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  if (unlikely (!count)) return;
  unsigned int last = 0;
  unsigned int last_syllable = info[0].syllable();
  for (unsigned int i = 1; i < count; i++)
    if (last_syllable != info[i].syllable()) {
      record_pref_syllable (info, last, i);
      last = i;
      last_syllable = info[last].syllable();
    }
  record_pref_syllable (info, last, count);
}

static void
reorder_virama_terminated_cluster (const hb_ot_shape_plan_t *plan,
				   hb_face_t *face,
				   hb_buffer_t *buffer,
				   unsigned int start, unsigned int end)
{
}

static void
reorder_consonant_cluster (const hb_ot_shape_plan_t *plan,
			   hb_face_t *face,
			   hb_buffer_t *buffer,
			   unsigned int start, unsigned int end)
{
  hb_glyph_info_t *info = buffer->info;

  /* Reorder! */
#if 0
  unsigned int i = start;
  for (; i < base; i++)
    info[i].use_position() = POS_PRE_C;
  if (i < end)
  {
    info[i].use_position() = POS_BASE_C;
    i++;
  }
  for (; i < end; i++)
  {
    if (info[i].use_category() == OT_MR) /* Pre-base reordering */
    {
      info[i].use_position() = POS_PRE_C;
      continue;
    }
    if (info[i].use_category() == OT_VPre) /* Left matra */
    {
      info[i].use_position() = POS_PRE_M;
      continue;
    }

    info[i].use_position() = POS_AFTER_MAIN;
  }

  buffer->merge_clusters (start, end);
  /* Sit tight, rock 'n roll! */
  hb_bubble_sort (info + start, end - start, compare_use_order);
#endif
}

static void
reorder_vowel_cluster (const hb_ot_shape_plan_t *plan,
		       hb_face_t *face,
		       hb_buffer_t *buffer,
		       unsigned int start, unsigned int end)
{
  reorder_consonant_cluster (plan, face, buffer, start, end);
}

static void
reorder_syllable (const hb_ot_shape_plan_t *plan,
		  hb_face_t *face,
		  hb_buffer_t *buffer,
		  unsigned int start, unsigned int end)
{
  syllable_type_t syllable_type = (syllable_type_t) (buffer->info[start].syllable() & 0x0F);
  switch (syllable_type) {
#define HANDLE(X) case X: reorder_##X (plan, face, buffer, start, end); return
    HANDLE (virama_terminated_cluster);
    HANDLE (consonant_cluster);
    HANDLE (vowel_cluster);
#undef HANDLE
#define HANDLE(X) case X: return
    HANDLE (number_joiner_terminated_cluster);
    HANDLE (numeral_cluster);
    HANDLE (symbol_cluster);
    HANDLE (independent_cluster);
#undef HANDLE
  }
}

static inline void
insert_dotted_circles (const hb_ot_shape_plan_t *plan HB_UNUSED,
		       hb_font_t *font,
		       hb_buffer_t *buffer)
{
#if 0
  /* Note: This loop is extra overhead, but should not be measurable. */
  bool has_broken_syllables = false;
  unsigned int count = buffer->len;
  hb_glyph_info_t *info = buffer->info;
  for (unsigned int i = 0; i < count; i++)
    if ((info[i].syllable() & 0x0F) == broken_cluster)
    {
      has_broken_syllables = true;
      break;
    }
  if (likely (!has_broken_syllables))
    return;


  hb_glyph_info_t dottedcircle = {0};
  if (!font->get_glyph (0x25CCu, 0, &dottedcircle.codepoint))
    return;
  dottedcircle.use_category() = hb_use_get_categories (0x25CC);

  buffer->clear_output ();

  buffer->idx = 0;
  unsigned int last_syllable = 0;
  while (buffer->idx < buffer->len)
  {
    unsigned int syllable = buffer->cur().syllable();
    syllable_type_t syllable_type = (syllable_type_t) (syllable & 0x0F);
    if (unlikely (last_syllable != syllable && syllable_type == broken_cluster))
    {
      last_syllable = syllable;

      hb_glyph_info_t info = dottedcircle;
      info.cluster = buffer->cur().cluster;
      info.mask = buffer->cur().mask;
      info.syllable() = buffer->cur().syllable();

      buffer->output_info (info);
    }
    else
      buffer->next_glyph ();
  }

  buffer->swap_buffers ();
#endif
}

static void
reorder (const hb_ot_shape_plan_t *plan,
	 hb_font_t *font,
	 hb_buffer_t *buffer)
{
  insert_dotted_circles (plan, font, buffer);

  hb_glyph_info_t *info = buffer->info;
  unsigned int count = buffer->len;
  if (unlikely (!count)) return;
  unsigned int last = 0;
  unsigned int last_syllable = info[0].syllable();
  for (unsigned int i = 1; i < count; i++)
    if (last_syllable != info[i].syllable()) {
      reorder_syllable (plan, font->face, buffer, last, i);
      last = i;
      last_syllable = info[last].syllable();
    }
  reorder_syllable (plan, font->face, buffer, last, count);

  /* Zero syllables now... */
  for (unsigned int i = 0; i < count; i++)
    info[i].syllable() = 0;

  HB_BUFFER_DEALLOCATE_VAR (buffer, use_category);
}


const hb_ot_complex_shaper_t _hb_ot_complex_shaper_use =
{
  "use",
  collect_features_use,
  NULL, /* override_features */
  data_create_use,
  data_destroy_use,
  NULL, /* preprocess_text */
  HB_OT_SHAPE_NORMALIZATION_MODE_COMPOSED_DIACRITICS_NO_SHORT_CIRCUIT,
  NULL, /* decompose */
  NULL, /* compose */
  setup_masks_use,
  HB_OT_SHAPE_ZERO_WIDTH_MARKS_NONE,
  false, /* fallback_position */
};
