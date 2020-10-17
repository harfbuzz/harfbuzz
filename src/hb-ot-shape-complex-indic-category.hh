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

#ifndef HB_OT_SHAPE_COMPLEX_INDIC_CATEGORY_HH
#define HB_OT_SHAPE_COMPLEX_INDIC_CATEGORY_HH


/* Categories used in the OpenType specs:
 * https://docs.microsoft.com/en-us/typography/script-development/devanagari
 * https://docs.microsoft.com/en-us/typography/script-development/khmer
 * https://docs.microsoft.com/en-us/typography/script-development/myanmar
 */
#define DEFINE_OT(category) OT_##category
enum indic_category_t {
  DEFINE_OT (X = 0),
  DEFINE_OT (C = 1),
  DEFINE_OT (V = 2),
  DEFINE_OT (N = 3),
  DEFINE_OT (H = 4),
  DEFINE_OT (ZWNJ = 5),
  DEFINE_OT (ZWJ = 6),
  DEFINE_OT (M = 7),
  DEFINE_OT (SM = 8),
  /* DEFINE_OT (VD = 9), UNUSED; we use OT_A instead. */
  DEFINE_OT (A = 10),
  DEFINE_OT (PLACEHOLDER = 11),
  DEFINE_OT (DOTTEDCIRCLE = 12),
  DEFINE_OT (RS = 13), /* Register Shifter, used in Khmer OT spec. */
  DEFINE_OT (Coeng = 14), /* Khmer-style Virama. */
  DEFINE_OT (Repha = 15), /* Atomically-encoded logical or visual repha. */
  DEFINE_OT (Ra = 16),
  DEFINE_OT (CM = 17), /* Consonant-Medial. */
  DEFINE_OT (Symbol = 18), /* Avagraha, etc that take marks (SM,A,VD). */
  DEFINE_OT (CS = 19),

  /* Khmer */
  DEFINE_OT (Robatic = 20),
  DEFINE_OT (Xgroup  = 21),
  DEFINE_OT (Ygroup  = 22),

  /* The following are used by Khmer & Myanmar shapers. */
  DEFINE_OT (VAbv    = 26),
  DEFINE_OT (VBlw    = 27),
  DEFINE_OT (VPre    = 28),
  DEFINE_OT (VPst    = 29),

  /* Myanmar */
  DEFINE_OT (IV  = 2), /* Independent vowel */
  DEFINE_OT (DB  = 3), /* Dot below */
  DEFINE_OT (VST = 8), /* Visarga and Shan tones */
  DEFINE_OT (GB  = 11), /* Generic base */
  DEFINE_OT (As  = 18), /* Asat */
  DEFINE_OT (D0  = 20), /* Digit zero */
  DEFINE_OT (MH  = 21), /* Various consonant medial types */
  DEFINE_OT (MR  = 22), /* Various consonant medial types */
  DEFINE_OT (MW  = 23), /* Various consonant medial types */
  DEFINE_OT (MY  = 24), /* Various consonant medial types */
  DEFINE_OT (PT  = 25), /* Pwo and other tones */
  DEFINE_OT (VS  = 30), /* Variation selectors */
  DEFINE_OT (P   = 31), /* Punctuation */
  DEFINE_OT (D   = 32), /* Digits except zero */
};
#undef DEFINE_OT


#endif /* HB_OT_SHAPE_COMPLEX_INDIC_CATEGORY_HH */
