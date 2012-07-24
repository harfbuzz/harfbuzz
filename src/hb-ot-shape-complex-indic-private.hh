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

#ifndef HB_OT_SHAPE_COMPLEX_INDIC_PRIVATE_HH
#define HB_OT_SHAPE_COMPLEX_INDIC_PRIVATE_HH

#include "hb-private.hh"


#include "hb-ot-shape-complex-private.hh"


/* buffer var allocations */
#define indic_category() complex_var_persistent_u8_0() /* indic_category_t */
#define indic_position() complex_var_persistent_u8_1() /* indic_matra_category_t */



#define IN_HALF_BLOCK(u, Base) (((u) & ~0x7F) == (Base))

#define IS_DEVA(u) (IN_HALF_BLOCK (u, 0x900))
#define IS_BENG(u) (IN_HALF_BLOCK (u, 0x980))
#define IS_GURM(u) (IN_HALF_BLOCK (u, 0xA00))
#define IS_GUJA(u) (IN_HALF_BLOCK (u, 0xA80))
#define IS_ORYA(u) (IN_HALF_BLOCK (u, 0xB00))
#define IS_TAML(u) (IN_HALF_BLOCK (u, 0xB80))
#define IS_TELU(u) (IN_HALF_BLOCK (u, 0xC00))
#define IS_KNDA(u) (IN_HALF_BLOCK (u, 0xC80))
#define IS_MLYM(u) (IN_HALF_BLOCK (u, 0xD00))
#define IS_SINH(u) (IN_HALF_BLOCK (u, 0xD80))



#define INDIC_TABLE_ELEMENT_TYPE uint8_t

/* Cateories used in the OpenType spec:
 * https://www.microsoft.com/typography/otfntdev/devanot/shaping.aspx
 */
/* Note: This enum is duplicated in the -machine.rl source file.
 * Not sure how to avoid duplication. */
enum indic_category_t {
  OT_X = 0,
  OT_C,
  OT_V,
  OT_N,
  OT_H,
  OT_ZWNJ,
  OT_ZWJ,
  OT_M,
  OT_SM,
  OT_VD,
  OT_A,
  OT_NBSP,
  OT_DOTTEDCIRCLE, /* Not in the spec, but special in Uniscribe. /Very very/ special! */
  OT_RS, /* Register Shifter, used in Khmer OT spec */
  OT_Coeng,
  OT_Repha,
  OT_Ra /* Not explicitly listed in the OT spec, but used in the grammar. */
};

/* Visual positions in a syllable from left to right. */
enum indic_position_t {
  POS_START,

  POS_RA_TO_BECOME_REPH,
  POS_PRE_M,
  POS_PRE_C,

  POS_BASE_C,
  POS_AFTER_MAIN,

  POS_ABOVE_C,

  POS_BEFORE_SUB,
  POS_BELOW_C,
  POS_AFTER_SUB,

  POS_BEFORE_POST,
  POS_POST_C,
  POS_AFTER_POST,

  POS_FINAL_C,
  POS_SMVD,

  POS_END
};

/* Categories used in IndicSyllabicCategory.txt from UCD. */
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
  INDIC_SYLLABIC_CATEGORY_CONSONANT_REPHA	= OT_Repha,
  INDIC_SYLLABIC_CATEGORY_MODIFYING_LETTER	= OT_X,
  INDIC_SYLLABIC_CATEGORY_NUKTA			= OT_N,
  INDIC_SYLLABIC_CATEGORY_REGISTER_SHIFTER	= OT_RS,
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
  INDIC_MATRA_CATEGORY_NOT_APPLICABLE		= POS_END,

  INDIC_MATRA_CATEGORY_LEFT			= POS_PRE_C,
  INDIC_MATRA_CATEGORY_TOP			= POS_ABOVE_C,
  INDIC_MATRA_CATEGORY_BOTTOM			= POS_BELOW_C,
  INDIC_MATRA_CATEGORY_RIGHT			= POS_POST_C,

  /* These should resolve to the position of the last part of the split sequence. */
  INDIC_MATRA_CATEGORY_BOTTOM_AND_RIGHT		= INDIC_MATRA_CATEGORY_RIGHT,
  INDIC_MATRA_CATEGORY_LEFT_AND_RIGHT		= INDIC_MATRA_CATEGORY_RIGHT,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM		= INDIC_MATRA_CATEGORY_BOTTOM,
  INDIC_MATRA_CATEGORY_TOP_AND_BOTTOM_AND_RIGHT	= INDIC_MATRA_CATEGORY_RIGHT,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT		= INDIC_MATRA_CATEGORY_TOP,
  INDIC_MATRA_CATEGORY_TOP_AND_LEFT_AND_RIGHT	= INDIC_MATRA_CATEGORY_RIGHT,
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
 * This is a hack for now.  We should move this data into the main Indic table.
 * Or completely remove it and just check in the tables.
 */
static const hb_codepoint_t ra_chars[] = {
  0x0930, /* Devanagari */
  0x09B0, /* Bengali */
  0x09F0, /* Bengali */
  0x0A30, /* Gurmukhi */	/* No Reph */
  0x0AB0, /* Gujarati */
  0x0B30, /* Oriya */
  0x0BB0, /* Tamil */		/* No Reph */
  0x0C30, /* Telugu */		/* Reph formed only with ZWJ */
  0x0CB0, /* Kannada */
  0x0D30, /* Malayalam */	/* No Reph, Logical Repha */

  0x0DBB, /* Sinhala */		/* Reph formed only with ZWJ */

  0x179A, /* Khmer */		/* No Reph, Visual Repha */
};


#endif /* HB_OT_SHAPE_COMPLEX_INDIC_PRIVATE_HH */
