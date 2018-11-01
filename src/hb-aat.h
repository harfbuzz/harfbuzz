/*
 * Copyright © 2018  Ebrahim Byagowi
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
 */

#ifndef HB_AAT_H
#define HB_AAT_H

#include "hb.h"
#include "hb-ot.h"

HB_BEGIN_DECLS

/*
 * Since: REPLACEME
 */
typedef enum
{
  HB_AAT_LAYOUT_FEATURE_TYPE_LIGATURES               = 1,
  HB_AAT_LAYOUT_FEATURE_TYPE_LETTER_CASE             = 3,
  HB_AAT_LAYOUT_FEATURE_TYPE_VERTICAL_SUBSTITUTION   = 4,
  HB_AAT_LAYOUT_FEATURE_TYPE_NUMBER_SPACING          = 6,
  HB_AAT_LAYOUT_FEATURE_TYPE_VERTICAL_POSITION       = 10,
  HB_AAT_LAYOUT_FEATURE_TYPE_FRACTIONS               = 11,
  HB_AAT_LAYOUT_FEATURE_TYPE_TYPOGRAPHIC_EXTRAS      = 14,
  HB_AAT_LAYOUT_FEATURE_TYPE_CHARACTER_ALTERNATIVES  = 17,
  HB_AAT_LAYOUT_FEATURE_TYPE_MATHEMATICAL_EXTRAS     = 15,
  HB_AAT_LAYOUT_FEATURE_TYPE_STYLE_OPTIONS           = 19,
  HB_AAT_LAYOUT_FEATURE_TYPE_CHARACTER_SHAPE         = 20,
  HB_AAT_LAYOUT_FEATURE_TYPE_NUMBER_CASE             = 21,
  HB_AAT_LAYOUT_FEATURE_TYPE_TEXT_SPACING            = 22,
  HB_AAT_LAYOUT_FEATURE_TYPE_TRANSLITERATION         = 23,
  HB_AAT_LAYOUT_FEATURE_TYPE_RUBYKANA                = 28,
  HB_AAT_LAYOUT_FEATURE_TYPE_ITALIC_CJK_ROMAN        = 32,
  HB_AAT_LAYOUT_FEATURE_TYPE_CASE_SENSITIVE_LAYOUT   = 33,
  HB_AAT_LAYOUT_FEATURE_TYPE_ALTERNATE_KANA          = 34,
  HB_AAT_LAYOUT_FEATURE_TYPE_STYLISTIC_ALTERNATIVES  = 35,
  HB_AAT_LAYOUT_FEATURE_TYPE_CONTEXTUAL_ALTERNATIVES = 36,
  HB_AAT_LAYOUT_FEATURE_TYPE_LOWER_CASE              = 37,
  HB_AAT_LAYOUT_FEATURE_TYPE_UPPER_CASE              = 38,
  HB_AAT_LAYOUT_FEATURE_TYPE_UNDEFINED               = 0xFFFF
} hb_aat_layout_feature_type_t;

/**
 * hb_aat_feature_t:
 *
 * Feature value
 *
 * Since: REPLACEME
 */
typedef unsigned int hb_aat_layout_feature_setting_t;

/**
 * hb_aat_layout_feature_type_selector_t:
 *
 * Feature type record
 *
 * Since: REPLACEME
 **/
typedef struct hb_aat_layout_feature_type_selector_t
{
  hb_aat_layout_feature_setting_t setting;
  hb_ot_name_id_t name_id;
} hb_aat_layout_feature_type_selector_t;

HB_EXTERN unsigned int
hb_aat_layout_get_feature_settings (hb_face_t                             *face,
				    hb_aat_layout_feature_type_t           type,
				    hb_aat_layout_feature_setting_t       *default_setting, /* OUT.     May be NULL. */
				    unsigned int                           start_offset,
				    unsigned int                          *selectors_count, /* IN/OUT.  May be NULL. */
				    hb_aat_layout_feature_type_selector_t *selectors_buffer /* OUT.     May be NULL. */);

HB_END_DECLS

#endif /* HB_AAT_H */
