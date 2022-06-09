/*
 * Copyright © 2012  Google, Inc.
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

#ifndef HB_OT_SHAPER_INDIC_HH
#define HB_OT_SHAPER_INDIC_HH

#include "hb.hh"

#include "hb-ot-shaper-syllabic.hh"


/* buffer var allocations */
#define indic_category() ot_shaper_var_u8_category() /* indic_category_t */
#define indic_position() ot_shaper_var_u8_auxiliary() /* indic_position_t */


/* Cateories used in the OpenType spec:
 * https://docs.microsoft.com/en-us/typography/script-development/devanagari
 */
/* Note: This enum is duplicated in the -machine.rl source file.
 * Not sure how to avoid duplication. */
enum ot_category_t {
  OT_X = 0,
  OT_C = 1,
  OT_V = 2,
  OT_N = 3,
  OT_H = 4,
  OT_ZWNJ = 5,
  OT_ZWJ = 6,
  OT_M = 7,
  OT_SM = 8,
  OT_A = 9,
  OT_VD = OT_A,
  OT_PLACEHOLDER = 10,
  OT_DOTTEDCIRCLE = 11,
  OT_RS = 12, /* Register Shifter, used in Khmer OT spec. */
  OT_Repha = 14, /* Atomically-encoded logical or visual repha. */
  OT_Ra = 15,
  OT_CM = 16,  /* Consonant-Medial. */
  OT_Symbol = 17, /* Avagraha, etc that take marks (SM,A,VD). */
  OT_CS = 18,

  /* Khmer & Myanmar shapers. */
  OT_VAbv    = 20,
  OT_VBlw    = 21,
  OT_VPre    = 22,
  OT_VPst    = 23,

  /* Khmer. */
  OT_Coeng   = 24,
  OT_Robatic = 25,
  OT_Xgroup  = 26,
  OT_Ygroup  = 27,

  /* Myanmar */
  OT_As      = 32,	// Asat
  OT_D       = 33,	// Digits except zero
  OT_D0      = 34,	// Digit zero
  OT_MH      = 35,	// Medial
  OT_MR      = 36,	// Medial
  OT_MW      = 37,	// Medial
  OT_MY      = 38,	// Medial
  OT_PT      = 39,	// Pwo and other tones
  OT_VS      = 40,	// Variation selectors
  OT_P       = 41,	// Punctuation
  OT_ML      = 42,	// Consonant medials
};

#define MEDIAL_FLAGS (FLAG (OT_CM))

/* Note:
 *
 * We treat Vowels and placeholders as if they were consonants.  This is safe because Vowels
 * cannot happen in a consonant syllable.  The plus side however is, we can call the
 * consonant syllable logic from the vowel syllable function and get it all right!
 *
 * Keep in sync with consonant_categories in the generator. */
#define CONSONANT_FLAGS (FLAG (OT_C) | FLAG (OT_CS) | FLAG (OT_Ra) | MEDIAL_FLAGS | FLAG (OT_V) | FLAG (OT_PLACEHOLDER) | FLAG (OT_DOTTEDCIRCLE))
#define JOINER_FLAGS (FLAG (OT_ZWJ) | FLAG (OT_ZWNJ))


/* Visual positions in a syllable from left to right. */
enum ot_position_t {
  POS_START = 0,

  POS_RA_TO_BECOME_REPH = 1,
  POS_PRE_M = 2,
  POS_PRE_C = 3,

  POS_BASE_C = 4,
  POS_AFTER_MAIN = 5,

  POS_ABOVE_C = 6,

  POS_BEFORE_SUB = 7,
  POS_BELOW_C = 8,
  POS_AFTER_SUB = 9,

  POS_BEFORE_POST = 10,
  POS_POST_C = 11,
  POS_AFTER_POST = 12,

  POS_FINAL_C = 13,
  POS_SMVD = 14,

  POS_END = 15
};


#define INDIC_COMBINE_CATEGORIES(S,M) ((S) | ((M) << 8))

HB_INTERNAL uint16_t
hb_indic_get_categories (hb_codepoint_t u);


static inline bool
is_one_of (const hb_glyph_info_t &info, unsigned int flags)
{
  /* If it ligated, all bets are off. */
  if (_hb_glyph_info_ligated (&info)) return false;
  return !!(FLAG_UNSAFE (info.indic_category()) & flags);
}

static inline bool
is_joiner (const hb_glyph_info_t &info)
{
  return is_one_of (info, JOINER_FLAGS);
}

static inline bool
is_consonant (const hb_glyph_info_t &info)
{
  return is_one_of (info, CONSONANT_FLAGS);
}

static inline bool
is_halant (const hb_glyph_info_t &info)
{
  return is_one_of (info, FLAG (OT_H));
}


#endif /* HB_OT_SHAPER_INDIC_HH */
