/*
 * Copyright © 2011  Google, Inc.
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
#define indic_category() complex_var_persistent_u8_0() /* indic_category_t */
#define indic_position() complex_var_persistent_u8_1() /* indic_matra_category_t */

#define INDIC_TABLE_ELEMENT_TYPE uint8_t

/* Cateories used in the OpenType spec:
 * https://www.microsoft.com/typography/otfntdev/devanot/shaping.aspx
 */
/* Note: This enum is duplicated in the -machine.rl source file.
 * Not sure how to avoid duplication. */
enum indic_category_t {
  OT_X = 0,
  OT_C,
  OT_Ra, /* Not explicitly listed in the OT spec, but used in the grammar. */
  OT_V,
  OT_N,
  OT_H,
  OT_ZWNJ,
  OT_ZWJ,
  OT_M,
  OT_SM,
  OT_VD,
  OT_A,
  OT_NBSP
};

/* Visual positions in a syllable from left to right. */
enum indic_position_t {
  POS_PRE,
  POS_BASE,
  POS_ABOVE,
  POS_BELOW,
  POS_POST,

  POS_INHERIT /* For Halant, Nukta, ZWJ, ZWNJ */
};

/* Categories used in IndicSyllabicCategory.txt from UCD */
/* The assignments are guesswork */
enum indic_syllabic_category_t {
  INDIC_SYLLABIC_CATEGORY_OTHER			= OT_X,

  INDIC_SYLLABIC_CATEGORY_AVAGRAHA		= OT_X,
  INDIC_SYLLABIC_CATEGORY_BINDU			= OT_SM,
  INDIC_SYLLABIC_CATEGORY_CONSONANT		= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_DEAD	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_FINAL	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_HEAD_LETTER	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_MEDIAL	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_PLACEHOLDER	= OT_NBSP,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_SUBJOINED	= OT_C,
  INDIC_SYLLABIC_CATEGORY_CONSONANT_REPHA	= OT_C,
  INDIC_SYLLABIC_CATEGORY_MODIFYING_LETTER	= OT_X,
  INDIC_SYLLABIC_CATEGORY_NUKTA			= OT_N,
  INDIC_SYLLABIC_CATEGORY_REGISTER_SHIFTER	= OT_X,
  INDIC_SYLLABIC_CATEGORY_TONE_LETTER		= OT_X,
  INDIC_SYLLABIC_CATEGORY_TONE_MARK		= OT_X,
  INDIC_SYLLABIC_CATEGORY_VIRAMA		= OT_H,
  INDIC_SYLLABIC_CATEGORY_VISARGA		= OT_SM,
  INDIC_SYLLABIC_CATEGORY_VOWEL			= OT_V,
  INDIC_SYLLABIC_CATEGORY_VOWEL_DEPENDENT	= OT_M,
  INDIC_SYLLABIC_CATEGORY_VOWEL_INDEPENDENT	= OT_V
};

/* Categories used in IndicSMatraCategory.txt from UCD */
enum indic_matra_category_t {
  INDIC_MATRA_CATEGORY_NOT_APPLICABLE		= POS_BASE,

  INDIC_MATRA_CATEGORY_LEFT			= POS_PRE,
  INDIC_MATRA_CATEGORY_TOP			= POS_ABOVE,
  INDIC_MATRA_CATEGORY_BOTTOM			= POS_BELOW,
  INDIC_MATRA_CATEGORY_RIGHT			= POS_POST,

  /* We don't really care much about these since we decompose them
   * in the generic pre-shaping layer.  They will only be used if
   * the font does not cover the decomposition.  In which case, we
   * define these as aliases to the place we want the split-matra
   * glyph to show up.  Quite arbitrary. */
  INDIC_MATRA_CATEGORY_BOTTOM_AND_RIGHT		= INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_LEFT_AND_RIGHT		= INDIC_MATRA_CATEGORY_LEFT,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM		= INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM_AND_RIGHT	= INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT		= INDIC_MATRA_CATEGORY_LEFT,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT_AND_RIGHT	= INDIC_MATRA_CATEGORY_LEFT,
  INDIC_MATRA_CATEGORY_TOP_AND_RIGHT		= INDIC_MATRA_CATEGORY_RIGHT,

  INDIC_MATRA_CATEGORY_INVISIBLE		= INDIC_MATRA_CATEGORY_NOT_APPLICABLE,
  INDIC_MATRA_CATEGORY_OVERSTRUCK		= INDIC_MATRA_CATEGORY_NOT_APPLICABLE,
  INDIC_MATRA_CATEGORY_VISUAL_ORDER_LEFT	= INDIC_MATRA_CATEGORY_NOT_APPLICABLE
};

/* Note: We use ASSERT_STATIC_EXPR_ZERO() instead of ASSERT_STATIC_EXPR() and the comma operation
 * because gcc fails to optimize the latter and fills the table in at runtime. */
#define INDIC_COMBINE_CATEGORIES(S,M) \
  (ASSERT_STATIC_EXPR_ZERO (M == INDIC_MATRA_CATEGORY_NOT_APPLICABLE || (S == INDIC_SYLLABIC_CATEGORY_VIRAMA || S == INDIC_SYLLABIC_CATEGORY_VOWEL_DEPENDENT)) + \
   ASSERT_STATIC_EXPR_ZERO (S < 16 && M < 16) + \
   ((M << 4) | S))

#include "hb-ot-shape-complex-indic-table.hh"

/* XXX
 * This is a hack for now.  We should:
 * 1. Move this data into the main Indic table,
 * and/or
 * 2. Probe font lookups to determine consonant positions.
 */
static const struct {
  hb_codepoint_t u;
  indic_position_t position;
} consonant_positions[] = {
  {0x0930, POS_BELOW},
  {0x09AC, POS_BELOW},
  {0x09AF, POS_POST},
  {0x09B0, POS_BELOW},
  {0x09F0, POS_BELOW},
  {0x0A2F, POS_POST},
  {0x0A30, POS_BELOW},
  {0x0A35, POS_BELOW},
  {0x0A39, POS_BELOW},
  {0x0AB0, POS_BELOW},
  {0x0B24, POS_BELOW},
  {0x0B28, POS_BELOW},
  {0x0B2C, POS_BELOW},
  {0x0B2D, POS_BELOW},
  {0x0B2E, POS_BELOW},
  {0x0B2F, POS_POST},
  {0x0B30, POS_BELOW},
  {0x0B32, POS_BELOW},
  {0x0B33, POS_BELOW},
  {0x0B5F, POS_POST},
  {0x0B71, POS_BELOW},
  {0x0C15, POS_BELOW},
  {0x0C16, POS_BELOW},
  {0x0C17, POS_BELOW},
  {0x0C18, POS_BELOW},
  {0x0C19, POS_BELOW},
  {0x0C1A, POS_BELOW},
  {0x0C1B, POS_BELOW},
  {0x0C1C, POS_BELOW},
  {0x0C1D, POS_BELOW},
  {0x0C1E, POS_BELOW},
  {0x0C1F, POS_BELOW},
  {0x0C20, POS_BELOW},
  {0x0C21, POS_BELOW},
  {0x0C22, POS_BELOW},
  {0x0C23, POS_BELOW},
  {0x0C24, POS_BELOW},
  {0x0C25, POS_BELOW},
  {0x0C26, POS_BELOW},
  {0x0C27, POS_BELOW},
  {0x0C28, POS_BELOW},
  {0x0C2A, POS_BELOW},
  {0x0C2B, POS_BELOW},
  {0x0C2C, POS_BELOW},
  {0x0C2D, POS_BELOW},
  {0x0C2E, POS_BELOW},
  {0x0C2F, POS_BELOW},
  {0x0C30, POS_BELOW},
  {0x0C32, POS_BELOW},
  {0x0C33, POS_BELOW},
  {0x0C35, POS_BELOW},
  {0x0C36, POS_BELOW},
  {0x0C37, POS_BELOW},
  {0x0C38, POS_BELOW},
  {0x0C39, POS_BELOW},
  {0x0C95, POS_BELOW},
  {0x0C96, POS_BELOW},
  {0x0C97, POS_BELOW},
  {0x0C98, POS_BELOW},
  {0x0C99, POS_BELOW},
  {0x0C9A, POS_BELOW},
  {0x0C9B, POS_BELOW},
  {0x0C9C, POS_BELOW},
  {0x0C9D, POS_BELOW},
  {0x0C9E, POS_BELOW},
  {0x0C9F, POS_BELOW},
  {0x0CA0, POS_BELOW},
  {0x0CA1, POS_BELOW},
  {0x0CA2, POS_BELOW},
  {0x0CA3, POS_BELOW},
  {0x0CA4, POS_BELOW},
  {0x0CA5, POS_BELOW},
  {0x0CA6, POS_BELOW},
  {0x0CA7, POS_BELOW},
  {0x0CA8, POS_BELOW},
  {0x0CAA, POS_BELOW},
  {0x0CAB, POS_BELOW},
  {0x0CAC, POS_BELOW},
  {0x0CAD, POS_BELOW},
  {0x0CAE, POS_BELOW},
  {0x0CAF, POS_BELOW},
  {0x0CB0, POS_BELOW},
  {0x0CB2, POS_BELOW},
  {0x0CB3, POS_BELOW},
  {0x0CB5, POS_BELOW},
  {0x0CB6, POS_BELOW},
  {0x0CB7, POS_BELOW},
  {0x0CB8, POS_BELOW},
  {0x0CB9, POS_BELOW},
  {0x0CDE, POS_BELOW},
  {0x0D2F, POS_POST},
  {0x0D30, POS_POST},
  {0x0D32, POS_BELOW},
  {0x0D35, POS_POST},
};


static const struct {
  hb_tag_t tag;
  hb_bool_t is_global;
} indic_basic_features[] =
{
  {HB_TAG('n','u','k','t'), true},
  {HB_TAG('a','k','h','n'), false},
  {HB_TAG('r','p','h','f'), false},
  {HB_TAG('r','k','r','f'), false},
  {HB_TAG('p','r','e','f'), false},
  {HB_TAG('b','l','w','f'), false},
  {HB_TAG('h','a','l','f'), false},
  {HB_TAG('v','a','t','u'), true},
  {HB_TAG('p','s','t','f'), false},
  {HB_TAG('c','j','c','t'), true},
};

/* Same order as the indic_basic_features array */
enum {
  _NUKT,
  AKHN,
  RPHF,
  RKRF,
  PREF,
  BLWF,
  HALF,
  _VATU,
  PSTF,
  _CJCT,
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

static void
final_reordering (const hb_ot_map_t *map,
		  hb_face_t *face,
		  hb_buffer_t *buffer,
		  void *user_data HB_UNUSED)
{

  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_position);
}

void
_hb_ot_shape_complex_collect_features_indic (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props)
{
  map->add_bool_feature (HB_TAG('l','o','c','l'));
  /* The Indic specs do not require ccmp, but we apply it here since if
   * there is a use of it, it's typically at the beginning. */
  map->add_bool_feature (HB_TAG('c','c','m','p'));

  map->add_gsub_pause (NULL, NULL);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_basic_features); i++)
    map->add_bool_feature (indic_basic_features[i].tag, indic_basic_features[i].is_global);

  map->add_gsub_pause (final_reordering, NULL);

  for (unsigned int i = 0; i < ARRAY_LENGTH (indic_other_features); i++)
    map->add_bool_feature (indic_other_features[i], true);
}


bool
_hb_ot_shape_complex_prefer_decomposed_indic (void)
{
  /* We want split matras decomposed by the common shaping logic. */
  return TRUE;
}

static void
found_syllable (hb_ot_map_t *map, hb_buffer_t *buffer,
		unsigned int start, unsigned int end)
{
  //fprintf (stderr, "%d %d\n", start, end);
}

#include "hb-ot-shape-complex-indic-machine.hh"


void
_hb_ot_shape_complex_setup_masks_indic (hb_ot_map_t *map, hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;

  HB_BUFFER_ALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_position);

  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int type = get_indic_categories (buffer->info[i].codepoint);

    buffer->info[i].indic_category() = type & 0x0F;
    buffer->info[i].indic_position() = type >> 4;
  }

  find_syllables (map, buffer);

  hb_mask_t mask_array[ARRAY_LENGTH (indic_basic_features)] = {0};
  unsigned int num_masks = ARRAY_LENGTH (indic_basic_features);
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = map->get_1_mask (indic_basic_features[i].tag);
}


HB_END_DECLS
