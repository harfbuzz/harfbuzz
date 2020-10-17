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

#ifndef HB_OT_SHAPE_COMPLEX_USE_CATEGORY_HH
#define HB_OT_SHAPE_COMPLEX_USE_CATEGORY_HH


/* Categories used in the Universal Shaping Engine spec:
 * https://docs.microsoft.com/en-us/typography/script-development/use
 */
#define DEFINE_USE(category) USE_##category
enum use_category_t {
  DEFINE_USE (O		= 0),	/* OTHER */

  DEFINE_USE (B		= 1),	/* BASE */
  DEFINE_USE (N		= 4),	/* BASE_NUM */
  DEFINE_USE (GB	= 5),	/* BASE_OTHER */
  DEFINE_USE (SUB	= 11),	/* CONS_SUB */
  DEFINE_USE (H		= 12),	/* HALANT */

  DEFINE_USE (HN	= 13),	/* HALANT_NUM */
  DEFINE_USE (ZWNJ	= 14),	/* Zero width non-joiner */
  DEFINE_USE (R		= 18),	/* REPHA */
  DEFINE_USE (S		= 19),	/* SYM */
  DEFINE_USE (CS	= 43),	/* CONS_WITH_STACKER */

  /* https://github.com/harfbuzz/harfbuzz/issues/1102 */
  DEFINE_USE (HVM	= 44),	/* HALANT_OR_VOWEL_MODIFIER */

  DEFINE_USE (Sk	= 48),	/* SAKOT */
  DEFINE_USE (G		= 49),	/* HIEROGLYPH */
  DEFINE_USE (J		= 50),	/* HIEROGLYPH_JOINER */
  DEFINE_USE (SB	= 51),	/* HIEROGLYPH_SEGMENT_BEGIN */
  DEFINE_USE (SE	= 52),	/* HIEROGLYPH_SEGMENT_END */

  DEFINE_USE (FAbv	= 24),	/* CONS_FINAL_ABOVE */
  DEFINE_USE (FBlw	= 25),	/* CONS_FINAL_BELOW */
  DEFINE_USE (FPst	= 26),	/* CONS_FINAL_POST */
  DEFINE_USE (MAbv	= 27),	/* CONS_MED_ABOVE */
  DEFINE_USE (MBlw	= 28),	/* CONS_MED_BELOW */
  DEFINE_USE (MPst	= 29),	/* CONS_MED_POST */
  DEFINE_USE (MPre	= 30),	/* CONS_MED_PRE */
  DEFINE_USE (CMAbv	= 31),	/* CONS_MOD_ABOVE */
  DEFINE_USE (CMBlw	= 32),	/* CONS_MOD_BELOW */
  DEFINE_USE (VAbv	= 33),	/* VOWEL_ABOVE / VOWEL_ABOVE_BELOW / VOWEL_ABOVE_BELOW_POST / VOWEL_ABOVE_POST */
  DEFINE_USE (VBlw	= 34),	/* VOWEL_BELOW / VOWEL_BELOW_POST */
  DEFINE_USE (VPst	= 35),	/* VOWEL_POST	UIPC = Right */
  DEFINE_USE (VPre	= 22),	/* VOWEL_PRE / VOWEL_PRE_ABOVE / VOWEL_PRE_ABOVE_POST / VOWEL_PRE_POST */
  DEFINE_USE (VMAbv	= 37),	/* VOWEL_MOD_ABOVE */
  DEFINE_USE (VMBlw	= 38),	/* VOWEL_MOD_BELOW */
  DEFINE_USE (VMPst	= 39),	/* VOWEL_MOD_POST */
  DEFINE_USE (VMPre	= 23),	/* VOWEL_MOD_PRE */
  DEFINE_USE (SMAbv	= 41),	/* SYM_MOD_ABOVE */
  DEFINE_USE (SMBlw	= 42),	/* SYM_MOD_BELOW */
  DEFINE_USE (FMAbv	= 45),	/* CONS_FINAL_MOD	UIPC = Top */
  DEFINE_USE (FMBlw	= 46),	/* CONS_FINAL_MOD	UIPC = Bottom */
  DEFINE_USE (FMPst	= 47),	/* CONS_FINAL_MOD	UIPC = Not_Applicable */
};
#undef DEFINE_USE


#endif /* HB_OT_SHAPE_COMPLEX_USE_CATEGORY_HH */
