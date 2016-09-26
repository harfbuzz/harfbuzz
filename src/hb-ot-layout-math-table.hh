/*
 * Copyright © 2016  Igalia S.L.
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
 * Igalia Author(s): Frédéric Wang
 */

#ifndef HB_OT_LAYOUT_MATH_TABLE_HH
#define HB_OT_LAYOUT_MATH_TABLE_HH

#include "hb-open-type-private.hh"
#include "hb-ot-layout-common-private.hh"
#include "hb-ot-math.h"

namespace OT {


struct MathValueRecord
{
  inline hb_position_t get_x_value (hb_font_t *font, const void *base) const
  { return font->em_scale_x (value) + (base+deviceTable).get_x_delta (font); }
  inline hb_position_t get_y_value (hb_font_t *font, const void *base) const
  { return font->em_scale_y (value) + (base+deviceTable).get_y_delta (font); }

  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && deviceTable.sanitize (c, base));
  }

protected:
  SHORT            value;       /* The X or Y value in design units */
  OffsetTo<Device> deviceTable; /* Offset to the device table - from the
                                   beginning of parent table. May be NULL.
                                   Suggested format for device table is 1. */

public:
  DEFINE_SIZE_STATIC (4);
};

struct MathConstants
{
  inline bool sanitize_math_value_records (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    unsigned int count =
      HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE -
      HB_OT_MATH_CONSTANT_MATH_LEADING + 1;
    for (unsigned int i = 0; i < count; i++)
      if (!mathValueRecords[i].sanitize (c, this)) return_trace (false);
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && sanitize_math_value_records(c));
  }

  inline hb_position_t get_value (hb_font_t *font, hb_ot_math_constant_t constant) const
  {
    switch (constant) {
    case HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT:
    case HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT:
      return font->em_scale_y (minHeight[constant - HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT]);

    case HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE:
    case HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE:
    case HB_OT_MATH_CONSTANT_SKEWED_FRACTION_HORIZONTAL_GAP:
    case HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT:
      return mathValueRecords[constant - HB_OT_MATH_CONSTANT_MATH_LEADING].get_x_value(font, this);

    case HB_OT_MATH_CONSTANT_ACCENT_BASE_HEIGHT:
    case HB_OT_MATH_CONSTANT_AXIS_HEIGHT:
    case HB_OT_MATH_CONSTANT_FLATTENED_ACCENT_BASE_HEIGHT:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_DISPLAY_STYLE_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOM_DISPLAY_STYLE_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_DISPLAY_STYLE_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_FRACTION_NUM_DISPLAY_STYLE_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_LOWER_LIMIT_BASELINE_DROP_MIN:
    case HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN:
    case HB_OT_MATH_CONSTANT_MATH_LEADING:
    case HB_OT_MATH_CONSTANT_OVERBAR_EXTRA_ASCENDER:
    case HB_OT_MATH_CONSTANT_OVERBAR_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_OVERBAR_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_RADICAL_DISPLAY_STYLE_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_RADICAL_EXTRA_ASCENDER:
    case HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_SKEWED_FRACTION_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_STACK_BOTTOM_DISPLAY_STYLE_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_STACK_BOTTOM_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_STACK_DISPLAY_STYLE_GAP_MIN:
    case HB_OT_MATH_CONSTANT_STACK_GAP_MIN:
    case HB_OT_MATH_CONSTANT_STACK_TOP_DISPLAY_STYLE_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_STACK_TOP_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_BOTTOM_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_ABOVE_MIN:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_BELOW_MIN:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_TOP_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN:
    case HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_SUBSCRIPT_TOP_MAX:
    case HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MIN:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP_CRAMPED:
    case HB_OT_MATH_CONSTANT_UNDERBAR_EXTRA_DESCENDER:
    case HB_OT_MATH_CONSTANT_UNDERBAR_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_UNDERBAR_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_UPPER_LIMIT_BASELINE_RISE_MIN:
    case HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN:
      return mathValueRecords[constant - HB_OT_MATH_CONSTANT_MATH_LEADING].get_y_value(font, this);

    case HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN:
    case HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN:
      return percentScaleDown[constant - HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN];

    case HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT:
      return radicalDegreeBottomRaisePercent;
    default:
      return 0;
    }
  }

protected:
  SHORT percentScaleDown[HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN - HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN + 1];
  USHORT minHeight[HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT - HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT + 1];
  MathValueRecord mathValueRecords[HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE - HB_OT_MATH_CONSTANT_MATH_LEADING + 1];
  SHORT radicalDegreeBottomRaisePercent;

public:
  DEFINE_SIZE_STATIC (2 * (HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT - HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN + 1) +
                      4 * (HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE - HB_OT_MATH_CONSTANT_MATH_LEADING + 1) +
                      2);
};

/*
 * MATH -- The MATH Table
 */

struct MATH
{
  static const hb_tag_t tableTag	= HB_OT_TAG_MATH;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
                  likely (version.major == 1) &&
                  mathConstants.sanitize (c, this));
  }

  inline bool has_math_constants (void) const { return mathConstants != 0; }
  inline const MathConstants &get_math_constants (void) const {
    return this+mathConstants;
  }

protected:
  FixedVersion<>version;                 /* Version of the MATH table
                                          * initially set to 0x00010000u */
  OffsetTo<MathConstants> mathConstants; /* MathConstants table */

public:
  DEFINE_SIZE_STATIC (6);
};

} /* mathspace OT */


#endif /* HB_OT_LAYOUT_MATH_TABLE_HH */
