/*
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
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-ot-shape-complex-private.hh"

HB_BEGIN_DECLS


/* buffer var allocations */
#define indic_categories() var2.u32 /* indic shaping action */

#define INDIC_TABLE_ELEMENT_TYPE uint8_t

enum indic_syllabic_category_t {
  INDIC_SYLLABIC_CATEGORY_AVAGRAHA,
  INDIC_SYLLABIC_CATEGORY_BINDU,
  INDIC_SYLLABIC_CATEGORY_CONSONANT,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_DEAD,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_FINAL,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_HEAD_LETTER,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_MEDIAL,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_PLACEHOLDER,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_REPHA,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_SUBJOINED,
  INDIC_SYLLABIC_CATEGORY_MODIFYING_LETTER,
  INDIC_SYLLABIC_CATEGORY_NUKTA,
  INDIC_SYLLABIC_CATEGORY_OTHER,
  INDIC_SYLLABIC_CATEGORY_REGISTER_SHIFTER,
  INDIC_SYLLABIC_CATEGORY_TONE_LETTER,
  INDIC_SYLLABIC_CATEGORY_TONE_MARK,
  INDIC_SYLLABIC_CATEGORY_VIRAMA,
  INDIC_SYLLABIC_CATEGORY_VISARGA,
  INDIC_SYLLABIC_CATEGORY_VOWEL,
  INDIC_SYLLABIC_CATEGORY_VOWEL_DEPENDENT,
  INDIC_SYLLABIC_CATEGORY_VOWEL_INDEPENDENT,
};

enum indic_matra_category_t {
  INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_BOTTOM_AND_RIGHT,
  INDIC_MATRA_CATEGORY_INVISIBLE,
  INDIC_MATRA_CATEGORY_LEFT,
  INDIC_MATRA_CATEGORY_LEFT_AND_RIGHT,
  INDIC_MATRA_CATEGORY_NOT_APPLICABLE,
  INDIC_MATRA_CATEGORY_OVERSTRUCK,
  INDIC_MATRA_CATEGORY_RIGHT,
  INDIC_MATRA_CATEGORY_TOP,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM_AND_RIGHT,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT_AND_RIGHT,
  INDIC_MATRA_CATEGORY_TOP_AND_RIGHT,
  INDIC_MATRA_CATEGORY_VISUAL_ORDER_LEFT,
};

#define INDIC_COMBINE_CATEGORIES(S,M) (S)

#include "hb-ot-shape-complex-indic-table.hh"

static const hb_tag_t indic_basic_features[] =
{
  HB_TAG('n','u','k','t'),
  HB_TAG('a','k','h','n'),
  HB_TAG('r','p','h','f'),
  HB_TAG('r','k','r','f'),
  HB_TAG('p','r','e','f'),
  HB_TAG('b','l','w','f'),
  HB_TAG('h','a','l','f'),
  HB_TAG('v','a','t','u'),
  HB_TAG('p','s','t','f'),
  HB_TAG('c','j','c','t'),
};

static const hb_tag_t indic_other_features[] =
{
  HB_TAG('p','r','e','s'),
  HB_TAG('a','b','v','s'),
  HB_TAG('b','l','w','s'),
  HB_TAG('p','s','t','s'),
  HB_TAG('h','a','l','n'),

  HB_TAG('d','i','s','t'),
  HB_TAG('a','b','v','m'),
  HB_TAG('b','l','w','m'),
};



void
_hb_ot_shape_complex_collect_features_indic	(hb_ot_shape_planner_t *planner, const hb_segment_properties_t  *props)
{
  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_basic_features); i++)
    planner->map.add_bool_feature (indic_basic_features[i], false);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_other_features); i++)
    planner->map.add_bool_feature (indic_other_features[i], true);
}

void
_hb_ot_shape_complex_setup_masks_indic	(hb_ot_shape_context_t *c)
{
  unsigned int count = c->buffer->len;

  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int this_type = get_indic_categories (c->buffer->info[i].codepoint);

    c->buffer->info[i].indic_categories() = this_type;
  }

  hb_mask_t mask_array[ARRAY_LENGTH (indic_basic_features)] = {0};
  unsigned int num_masks = ARRAY_LENGTH (indic_basic_features);
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = c->plan->map.get_1_mask (indic_basic_features[i]);
}


HB_END_DECLS
