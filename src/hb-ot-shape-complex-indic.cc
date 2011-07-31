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
static const struct consonant_position_t {
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

/* XXX
 * This is a hack for now.  We should move this data into the main Indic table.
 */
static const hb_codepoint_t ra_chars[] = {
  0x0930, /* Devanagari */
  0x09B0, /* Bengali */
  0x09F0, /* Bengali */
//0x09F1, /* Bengali */
//0x0A30, /* Gurmukhi */
  0x0AB0, /* Gujarati */
  0x0B30, /* Oriya */
//0x0BB0, /* Tamil */
//0x0C30, /* Telugu */
  0x0CB0, /* Kannada */
//0x0D30, /* Malayalam */
};

static int
compare_codepoint (const void *pa, const void *pb)
{
  hb_codepoint_t a = * (hb_codepoint_t *) pa;
  hb_codepoint_t b = * (hb_codepoint_t *) pb;

  return a < b ? -1 : a == b ? 0 : +1;
}

static indic_position_t
consonant_position (hb_codepoint_t u)
{
  consonant_position_t *record;

  record = (consonant_position_t *) bsearch (&u, consonant_positions,
					     ARRAY_LENGTH (consonant_positions),
					     sizeof (consonant_positions[0]),
					     compare_codepoint);

  return record ? record->position : POS_BASE;
}

static bool
is_ra (hb_codepoint_t u)
{
  return !!bsearch (&u, ra_chars,
		    ARRAY_LENGTH (ra_chars),
		    sizeof (ra_chars[0]),
		    compare_codepoint);
}


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
initial_reordering (const hb_ot_map_t *map,
		    hb_face_t *face,
		    hb_buffer_t *buffer,
		    void *user_data HB_UNUSED);
static void
final_reordering (const hb_ot_map_t *map,
		  hb_face_t *face,
		  hb_buffer_t *buffer,
		  void *user_data HB_UNUSED);

void
_hb_ot_shape_complex_collect_features_indic (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props)
{
  map->add_bool_feature (HB_TAG('l','o','c','l'));
  /* The Indic specs do not require ccmp, but we apply it here since if
   * there is a use of it, it's typically at the beginning. */
  map->add_bool_feature (HB_TAG('c','c','m','p'));

  map->add_gsub_pause (initial_reordering, NULL);

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


void
_hb_ot_shape_complex_setup_masks_indic (hb_ot_map_t *map, hb_buffer_t *buffer)
{
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_ALLOCATE_VAR (buffer, indic_position);

  /* We cannot setup masks here.  We save information about characters
   * and setup masks later on in a pause-callback. */

  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
  {
    unsigned int type = get_indic_categories (buffer->info[i].codepoint);

    buffer->info[i].indic_category() = type & 0x0F;
    buffer->info[i].indic_position() = type >> 4;

    if (buffer->info[i].indic_category() == OT_C) {
      buffer->info[i].indic_position() = consonant_position (buffer->info[i].codepoint);
      if (is_ra (buffer->info[i].codepoint))
	buffer->info[i].indic_category() = OT_Ra;
    }
  }
}

static int
compare_indic_order (const hb_glyph_info_t *pa, const hb_glyph_info_t *pb)
{
  int a = pa->indic_position();
  int b = pb->indic_position();

  return a < b ? -1 : a == b ? 0 : +1;
}

static void
found_consonant_syllable (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
			  unsigned int start, unsigned int end)
{
  unsigned int i;
  hb_glyph_info_t *info = buffer->info;

  /* Comments from:
   * https://www.microsoft.com/typography/otfntdev/devanot/shaping.aspx */

  /* 1. Find base consonant:
   *
   * The shaping engine finds the base consonant of the syllable, using the
   * following algorithm: starting from the end of the syllable, move backwards
   * until a consonant is found that does not have a below-base or post-base
   * form (post-base forms have to follow below-base forms), or that is not a
   * pre-base reordering Ra, or arrive at the first consonant. The consonant
   * stopped at will be the base.
   *
   *   o If the syllable starts with Ra + Halant (in a script that has Reph)
   *     and has more than one consonant, Ra is excluded from candidates for
   *     base consonants.
   */

  unsigned int base = 0;

  /* -> starting from the end of the syllable, move backwards */
  i = end;
  do {
    i--;
    /* -> until a consonant is found */
    if (info[i].indic_category() == OT_C)
    //if ((FLAG (info[i].indic_category()) & (FLAG (OT_C) | FLAG (OT_Ra))))
    {
      /* -> that does not have a below-base or post-base form
       * (post-base forms have to follow below-base forms), */
      if (info[i].indic_position() != POS_BELOW &&
	  info[i].indic_position() != POS_POST)
      {
        base = i;
	break;
      }

      /* TODO: or that is not a pre-base reordering Ra, */

      /* -> or arrive at the first consonant. The consonant stopped at will be the base. */
      base = i;
    }
  } while (i > start);
  if (base < start)
    base = start; /* Just in case... */

  /* TODO
   * If the syllable starts with Ra + Halant (in a script that has Reph)
   * and has more than one consonant, Ra is excluded from candidates for
   * base consonants. */


  /* 2. Decompose and reorder Matras:
   *
   * Each matra and any syllable modifier sign in the cluster are moved to the
   * appropriate position relative to the consonant(s) in the cluster. The
   * shaping engine decomposes two- or three-part matras into their constituent
   * parts before any repositioning. Matra characters are classified by which
   * consonant in a conjunct they have affinity for and are reordered to the
   * following positions:
   *
   *   o Before first half form in the syllable
   *   o After subjoined consonants
   *   o After post-form consonant
   *   o After main consonant (for above marks)
   *
   * IMPLEMENTATION NOTES:
   *
   * The normalize() routine has already decomposed matras for us, so we don't
   * need to worry about that.
   */


  /* 3.  Reorder marks to canonical order:
   *
   * Adjacent nukta and halant or nukta and vedic sign are always repositioned
   * if necessary, so that the nukta is first.
   *
   * IMPLEMENTATION NOTES:
   *
   * We don't need to do this: the normalize() routine already did this for us.
   */


  /* Reorder characters */

  for (i = start; i < base; i++)
    info[i].indic_position() = POS_PRE;
  info[base].indic_position() = POS_BASE;


  /* Handle beginning Ra */
  if (start + 2 <= end &&
      info[start].indic_category() == OT_Ra &&
      info[start + 1].indic_category() == OT_H)
   {
    info[start].indic_position() = POS_POST;
    info[start].mask = mask_array[RPHF];
   }

  /* For old-style Indic script tags, move the first post-base Halant after
   * last consonant. */
  if ((map->get_chosen_script (0) & 0x000000FF) != '2') {
    /* We should only do this for Indic scripts which have a version two I guess. */
    for (i = base + 1; i < end; i++)
      if (info[i].indic_category() == OT_H) {
        unsigned int j;
        for (j = end - 1; j > i; j--)
	  if ((FLAG (info[j].indic_category()) & (FLAG (OT_C) | FLAG (OT_Ra))))
	    break;
	if (j > i) {
	  /* Move Halant to after last consonant. */
	  hb_glyph_info_t t = info[i];
	  memmove (&info[i], &info[i + 1], (j - i) * sizeof (info[0]));
	  info[j] = t;
	}
        break;
      }
  }

  /* Attach ZWJ, ZWNJ, nukta, and halant to previous char to move with them. */
  for (i = start + 1; i < end; i++)
    if ((FLAG (info[i].indic_category()) &
	 (FLAG (OT_ZWNJ) | FLAG (OT_ZWJ) | FLAG (OT_N) | FLAG (OT_H))))
      info[i].indic_position() = info[i - 1].indic_position();

  /* We do bubble-sort, skip malicious clusters attempts */
  if (end - start > 20)
    return;

  /* Sit tight, rock 'n roll! */
  hb_bubble_sort (info + start, end - start, compare_indic_order);

  /* Setup masks now */

  /* Pre-base */
  for (i = start; i < base; i++)
    info[i].mask  |= mask_array[HALF] | mask_array[AKHN];
  /* Base */
  info[base].mask |= mask_array[AKHN];
  /* Post-base */
  for (i = base + 1; i < end; i++)
    info[i].mask  |= mask_array[BLWF] | mask_array[PSTF];
}


static void
found_vowel_syllable (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
		      unsigned int start, unsigned int end)
{
  /* TODO
   * Not clear to me how this should work.  Do the matras move to before the
   * independent vowel?  No idea.
   */
}

static void
found_standalone_cluster (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
			  unsigned int start, unsigned int end)
{
  /* TODO
   * Easiest thing to do here is to convert the NBSP to consonant and
   * call found_consonant_syllable.
   */
}

static void
found_non_indic (const hb_ot_map_t *map, hb_buffer_t *buffer, hb_mask_t *mask_array,
		 unsigned int start, unsigned int end)
{
  /* Nothing to do right now.  If we ever switch to using the output
   * buffer in the reordering process, we'd need to next_glyph() here. */
}

#include "hb-ot-shape-complex-indic-machine.hh"

static void
initial_reordering (const hb_ot_map_t *map,
		    hb_face_t *face,
		    hb_buffer_t *buffer,
		    void *user_data HB_UNUSED)
{
  hb_mask_t mask_array[ARRAY_LENGTH (indic_basic_features)] = {0};
  unsigned int num_masks = ARRAY_LENGTH (indic_basic_features);
  for (unsigned int i = 0; i < num_masks; i++)
    mask_array[i] = map->get_1_mask (indic_basic_features[i].tag);

  find_syllables (map, buffer, mask_array);
}

static void
final_reordering (const hb_ot_map_t *map,
		  hb_face_t *face,
		  hb_buffer_t *buffer,
		  void *user_data HB_UNUSED)
{
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_category);
  HB_BUFFER_DEALLOCATE_VAR (buffer, indic_position);
}



HB_END_DECLS
