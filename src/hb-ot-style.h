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

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_STYLE_H
#define HB_OT_STYLE_H

#include "hb.h"

HB_BEGIN_DECLS

/**
 * hb_ot_style_t
 *
 * Defined by:
 * * https://docs.microsoft.com/en-us/typography/opentype/spec/dvaraxisreg
 * * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6fdsc.html
 *
 * Since: REPLACEME
 */
typedef enum
{
  /**
   * Used to vary between non-italic and italic.
   *
   * Valid numeric range: Values must be in the range 0 to 1.
   *
   * Scale interpretation: A value of 0 can be interpreted
   * as “Roman” (non-italic); a value of 1 can be interpreted
   * as (fully) italic.
   *
   * Recommended or required “Regular” value: 0 is required.
   *
   * https://docs.microsoft.com/en-us/typography/opentype/spec/dvaraxistag_ital
   */
  HB_OT_STYLE_ITALIC		= HB_TAG ('i','t','a','l'),

  /**
   * Used to vary design to suit different text sizes.
   *
   * Valid numeric range: Values must be strictly greater than zero.
   *
   * Scale interpretation: Values can be interpreted as text size,
   * in points.
   *
   * Recommended or required “Regular” value: A value in the range
   * 9 to 13 is recommended.
   *
   * Suggested programmatic interactions: Applications may choose to select
   * an optical-size variant automatically based on the text size.
   *
   * https://docs.microsoft.com/en-us/typography/opentype/spec/dvaraxistag_opsz
   */
  HB_OT_STYLE_OPTICAL_SIZE	= HB_TAG ('o','p','s','z'),

  /**
   * Used to vary between upright and slanted text.
   *
   * Valid numeric range: Values must be greater than -90 and less
   * than +90.
   *
   * Scale interpretation: Values can be interpreted as the angle, in
   * counter-clockwise degrees, of oblique slant from whatever the
   * designer considers to be upright for that font design.
   *
   * Recommended or required “Regular” value: 0 is required.
   *
   * https://docs.microsoft.com/en-us/typography/opentype/spec/dvaraxistag_slnt
   */
  HB_OT_STYLE_SLANT		= HB_TAG ('s','l','n','t'),

  /**
   * Used to vary width of text from narrower to wider.
   *
   * Valid numeric range: Values must be strictly greater than zero.
   *
   * Scale interpretation: Values can be interpreted as a percentage
   * of whatever the font designer considers “normal width” for that font design.
   *
   * Recommended or required “Regular” value: 100 is required.
   *
   * Suggested programmatic interactions: Applications may choose to select
   * a width variant in a variable font automatically in order to fit a span
   * of text into a target width.
   *
   * https://docs.microsoft.com/en-us/typography/opentype/spec/dvaraxistag_wdth
   */
  HB_OT_STYLE_WIDTH		= HB_TAG ('w','d','t','h'),

  /**
   * Used to vary stroke thicknesses or other design details
   * to give variation from lighter to blacker.
   *
   * Valid numeric range: Values must be in the range 1 to 1000.
   *
   * Scale interpretation: Values can be interpreted in direct
   * comparison to values for usWeightClass in the OS/2 table,
   * or the CSS font-weight property.
   *
   * Recommended or required “Regular” value: 400 is required.
   *
   * https://docs.microsoft.com/en-us/typography/opentype/spec/dvaraxistag_wght
   */
  HB_OT_STYLE_WEIGHT		= HB_TAG ('w','g','h','t')
} hb_ot_style_t;


HB_EXTERN float
hb_ot_style_get (hb_face_t *face, hb_ot_style_t tag);


HB_END_DECLS

#endif /* HB_OT_STYLE_H */
