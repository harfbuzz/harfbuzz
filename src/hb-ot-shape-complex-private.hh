/*
 * Copyright © 2010,2011  Google, Inc.
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

#ifndef HB_OT_SHAPE_COMPLEX_PRIVATE_HH
#define HB_OT_SHAPE_COMPLEX_PRIVATE_HH

#include "hb-private.hh"

#include "hb-ot-map-private.hh"

HB_BEGIN_DECLS


/* buffer var allocations, used by all shapers */
#define general_category() var1.u8[0] /* unicode general_category (hb_unicode_general_category_t) */
#define combining_class() var1.u8[1] /* unicode combining_class (uint8_t) */


enum hb_ot_complex_shaper_t {
  hb_ot_complex_shaper_default,
  hb_ot_complex_shaper_arabic,
  hb_ot_complex_shaper_indic
};

static inline hb_ot_complex_shaper_t
hb_ot_shape_complex_categorize (const hb_segment_properties_t *props)
{
  switch ((int) props->script)
  {
    case HB_SCRIPT_ARABIC:
    case HB_SCRIPT_MANDAIC:
    case HB_SCRIPT_MONGOLIAN:
    case HB_SCRIPT_NKO:
    case HB_SCRIPT_SYRIAC:
      return hb_ot_complex_shaper_arabic;

    /* TODO: These are all the scripts that the ucd/IndicSyllabicCategory.txt covers.
     * Quite possibly many of these need no shaping, and some other are encoded visually.
     * Needs to be refined.
     */
    case HB_SCRIPT_BALINESE:
    case HB_SCRIPT_BATAK:
    case HB_SCRIPT_BENGALI:
    case HB_SCRIPT_BRAHMI:
    case HB_SCRIPT_BUGINESE:
    case HB_SCRIPT_BUHID:
    case HB_SCRIPT_CHAM:
    case HB_SCRIPT_DEVANAGARI:
    case HB_SCRIPT_GUJARATI:
    case HB_SCRIPT_GURMUKHI:
    case HB_SCRIPT_HANUNOO:
    case HB_SCRIPT_JAVANESE:
    case HB_SCRIPT_KAITHI:
    case HB_SCRIPT_KANNADA:
    case HB_SCRIPT_KAYAH_LI:
    case HB_SCRIPT_KHAROSHTHI:
    case HB_SCRIPT_KHMER:
    case HB_SCRIPT_LAO:
    case HB_SCRIPT_LEPCHA:
    case HB_SCRIPT_LIMBU:
    case HB_SCRIPT_MALAYALAM:
    case HB_SCRIPT_MEETEI_MAYEK:
    case HB_SCRIPT_MYANMAR:
    case HB_SCRIPT_NEW_TAI_LUE:
    case HB_SCRIPT_ORIYA:
    case HB_SCRIPT_PHAGS_PA:
    case HB_SCRIPT_REJANG:
    case HB_SCRIPT_SAURASHTRA:
    case HB_SCRIPT_SINHALA:
    case HB_SCRIPT_SUNDANESE:
    case HB_SCRIPT_SYLOTI_NAGRI:
    case HB_SCRIPT_TAGALOG:
    case HB_SCRIPT_TAGBANWA:
    case HB_SCRIPT_TAI_LE:
    case HB_SCRIPT_TAI_THAM:
    case HB_SCRIPT_TAI_VIET:
    case HB_SCRIPT_TAMIL:
    case HB_SCRIPT_TELUGU:
    case HB_SCRIPT_THAI:
    case HB_SCRIPT_TIBETAN:
      return hb_ot_complex_shaper_indic;

    default:
      return hb_ot_complex_shaper_default;
  }
}



/*
 * collect_features()
 *
 * Called during shape_plan().
 *
 * Shapers should use map to add their features and callbacks.
 */

typedef void hb_ot_shape_complex_collect_features_func_t (hb_ot_map_builder_t *map, const hb_segment_properties_t  *props);
HB_INTERNAL hb_ot_shape_complex_collect_features_func_t _hb_ot_shape_complex_collect_features_default;
HB_INTERNAL hb_ot_shape_complex_collect_features_func_t _hb_ot_shape_complex_collect_features_arabic;
HB_INTERNAL hb_ot_shape_complex_collect_features_func_t _hb_ot_shape_complex_collect_features_indic;

static inline void
hb_ot_shape_complex_collect_features (hb_ot_complex_shaper_t shaper,
				      hb_ot_map_builder_t *map,
				      const hb_segment_properties_t  *props)
{
  switch (shaper) {
    default:
    case hb_ot_complex_shaper_default:	_hb_ot_shape_complex_collect_features_default	(map, props);	return;
    case hb_ot_complex_shaper_arabic:	_hb_ot_shape_complex_collect_features_arabic	(map, props);	return;
    case hb_ot_complex_shaper_indic:	_hb_ot_shape_complex_collect_features_indic	(map, props);	return;
  }
}


/* setup_masks()
 *
 * Called during shape_execute().
 *
 * Shapers should use map to get feature masks and set on buffer.
 */

typedef void hb_ot_shape_complex_setup_masks_func_t (hb_ot_map_t *map, hb_buffer_t *buffer);
HB_INTERNAL hb_ot_shape_complex_setup_masks_func_t _hb_ot_shape_complex_setup_masks_default;
HB_INTERNAL hb_ot_shape_complex_setup_masks_func_t _hb_ot_shape_complex_setup_masks_arabic;
HB_INTERNAL hb_ot_shape_complex_setup_masks_func_t _hb_ot_shape_complex_setup_masks_indic;

static inline void
hb_ot_shape_complex_setup_masks (hb_ot_complex_shaper_t shaper,
				 hb_ot_map_t *map,
				 hb_buffer_t *buffer)
{
  switch (shaper) {
    default:
    case hb_ot_complex_shaper_default:	_hb_ot_shape_complex_setup_masks_default(map, buffer);	return;
    case hb_ot_complex_shaper_arabic:	_hb_ot_shape_complex_setup_masks_arabic	(map, buffer);	return;
    case hb_ot_complex_shaper_indic:	_hb_ot_shape_complex_setup_masks_indic	(map, buffer);	return;
  }
}


HB_END_DECLS

#endif /* HB_OT_SHAPE_COMPLEX_PRIVATE_HH */
