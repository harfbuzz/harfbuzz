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
  inline hb_position_t get_value (hb_font_t *font, bool horizontal,
                                  const void *base) const
  {
    return horizontal ?
      font->em_scale_x (value) + (base+deviceTable).get_x_delta (font) :
      font->em_scale_y (value) + (base+deviceTable).get_y_delta (font);
  }

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
  DEFINE_SIZE_STATIC (2 * 2);
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
      return mathValueRecords[constant - HB_OT_MATH_CONSTANT_MATH_LEADING].get_value(font, true, this);

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
      return mathValueRecords[constant - HB_OT_MATH_CONSTANT_MATH_LEADING].get_value(font, false, this);

    case HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN:
    case HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN:
      return percentScaleDown[constant - HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN];

    case HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT:
      return radicalDegreeBottomRaisePercent;
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

struct MathItalicsCorrectionInfo
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  coverage.sanitize (c, this) &&
                  italicsCorrection.sanitize (c, this));
  }

  inline bool get_value (hb_font_t *font, hb_codepoint_t glyph,
                         hb_position_t &value) const
  {
    unsigned int index = (this+coverage).get_coverage (glyph);
    if (likely (index == NOT_COVERED)) return false;
    if (unlikely (index >= italicsCorrection.len)) return false;
    value = italicsCorrection[index].get_value(font, true, this);
    return true;
  }

protected:
  OffsetTo<Coverage>       coverage;          /* Offset to Coverage table -
                                                 from the beginning of
                                                 MathItalicsCorrectionInfo
                                                 table. */
  ArrayOf<MathValueRecord> italicsCorrection; /* Array of MathValueRecords
                                                 defining italics correction
                                                 values for each
                                                 covered glyph. */

public:
  DEFINE_SIZE_ARRAY (2 + 2, italicsCorrection);
};

struct MathTopAccentAttachment
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  topAccentCoverage.sanitize (c, this) &&
                  topAccentAttachment.sanitize (c, this));
  }

  inline bool get_value (hb_font_t *font, hb_codepoint_t glyph,
                         hb_position_t &value) const
  {
    unsigned int index = (this+topAccentCoverage).get_coverage (glyph);
    if (likely (index == NOT_COVERED)) return false;
    if (unlikely (index >= topAccentAttachment.len)) return false;
    value = topAccentAttachment[index].get_value(font, true, this);
    return true;
  }

protected:
  OffsetTo<Coverage>       topAccentCoverage;   /* Offset to Coverage table -
                                                   from the beginning of
                                                   MathTopAccentAttachment
                                                   table. */
  ArrayOf<MathValueRecord> topAccentAttachment; /* Array of MathValueRecords
                                                   defining top accent
                                                   attachment points for each
                                                   covered glyph. */

public:
  DEFINE_SIZE_ARRAY (2 + 2, topAccentAttachment);
};

struct MathKern
{
  inline bool sanitize_math_value_records (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    unsigned int count = 2 * heightCount + 1;
    for (unsigned int i = 0; i < count; i++)
      if (!mathValueRecords[i].sanitize (c, this)) return_trace (false);
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  c->check_array (mathValueRecords,
                                  mathValueRecords[0].static_size,
                                  2 * heightCount + 1) &&
                  sanitize_math_value_records (c));
  }

  inline hb_position_t get_value (hb_font_t *font,
                                  hb_position_t &correction_height) const
  {
    const MathValueRecord* correctionHeight = mathValueRecords;
    const MathValueRecord* kernValue = mathValueRecords + heightCount;
    // The description of the MathKern table is a ambiguous, but interpreting
    // "between the two heights found at those indexes" for 0 < i < len as
    //
    //   correctionHeight[i-1] < correction_height <= correctionHeight[i]
    //
    // makes the result consistent with the limit cases and we can just use the
    // binary search algorithm of std::upper_bound:
    unsigned int count = heightCount;
    unsigned int i = 0;
    while (count > 0) {
      unsigned int half = count / 2;
      hb_position_t height =
        correctionHeight[i + half].get_value(font, false, this);
      if (height < correction_height) {
        i += half + 1;
        count -= half + 1;
      } else
        count = half;
    }
    return kernValue[i].get_value(font, true, this);
  }

protected:
  USHORT          heightCount;
  MathValueRecord mathValueRecords[VAR]; /* Array of correction heights at
                                            which the kern value changes.
                                            Sorted by the height value in
                                            design units. */
                                         /* Array of kern values corresponding
                                            to heights. */

public:
  DEFINE_SIZE_ARRAY (2, mathValueRecords);
};

struct MathKernInfoRecord
{
  inline bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  mathKern[HB_OT_MATH_KERN_TOP_RIGHT].sanitize (c, base) &&
                  mathKern[HB_OT_MATH_KERN_TOP_LEFT].sanitize (c, base) &&
                  mathKern[HB_OT_MATH_KERN_BOTTOM_RIGHT].sanitize (c, base) &&
                  mathKern[HB_OT_MATH_KERN_BOTTOM_LEFT].sanitize (c, base));
  }

  inline bool has_math_kern (hb_ot_math_kern_t kern) const {
    return mathKern[kern] != 0;
  }
  inline const MathKern &get_math_kern (hb_ot_math_kern_t kern,
                                        const void *base) const {
    return base+mathKern[kern];
  }

protected:
  /* Offset to MathKern table for each corner -
     from the beginning of MathKernInfo table. May be NULL. */
  OffsetTo<MathKern> mathKern[HB_OT_MATH_KERN_BOTTOM_LEFT -
                              HB_OT_MATH_KERN_TOP_RIGHT + 1];

public:
  DEFINE_SIZE_STATIC (2 * (HB_OT_MATH_KERN_BOTTOM_LEFT -
                           HB_OT_MATH_KERN_TOP_RIGHT + 1));
};

struct MathKernInfo
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  mathKernCoverage.sanitize (c, this) &&
                  mathKernInfoRecords.sanitize (c, this));
  }

  inline bool
  get_math_kern_info_record (hb_codepoint_t glyph,
                             const MathKernInfoRecord *&record) const
  {
    unsigned int index = (this+mathKernCoverage).get_coverage (glyph);
    if (likely (index == NOT_COVERED)) return false;
    if (unlikely (index >= mathKernInfoRecords.len)) return false;
    record = &mathKernInfoRecords[index];
    return true;
  }

protected:
  OffsetTo<Coverage>          mathKernCoverage;    /* Offset to Coverage table -
                                                      from the beginning of the
                                                      MathKernInfo table. */
  ArrayOf<MathKernInfoRecord> mathKernInfoRecords; /* Array of
                                                      MathKernInfoRecords,
                                                      per-glyph information for
                                                      mathematical positioning
                                                      of subscripts and
                                                      superscripts. */

public:
  DEFINE_SIZE_ARRAY (2 + 2, mathKernInfoRecords);
};

struct MathGlyphInfo
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  mathItalicsCorrectionInfo.sanitize (c, this) &&
                  mathTopAccentAttachment.sanitize (c, this) &&
                  extendedShapeCoverage.sanitize (c, this) &&
                  mathKernInfo.sanitize(c, this));
  }

  inline bool has_math_italics_correction_info (void) const {
    return mathItalicsCorrectionInfo != 0;
  }
  inline const MathItalicsCorrectionInfo&
  get_math_italics_correction_info (void) const {
    return this+mathItalicsCorrectionInfo;
  }

  inline bool has_math_top_accent_attachment (void) const {
    return mathTopAccentAttachment != 0;
  }
  inline const MathTopAccentAttachment&
  get_math_top_accent_attachment (void) const {
    return this+mathTopAccentAttachment;
  }

  inline bool is_extended_shape (hb_codepoint_t glyph) const
  {
    if (likely (extendedShapeCoverage == 0)) return false;
    unsigned int index = (this+extendedShapeCoverage).get_coverage (glyph);
    if (likely (index == NOT_COVERED)) return false;
    return true;
  }

  inline bool has_math_kern_info (void) const { return mathKernInfo != 0; }
  inline const MathKernInfo &get_math_kern_info (void) const {
    return this+mathKernInfo;
  }

protected:
  /* Offset to MathItalicsCorrectionInfo table -
     from the beginning of MathGlyphInfo table. */
  OffsetTo<MathItalicsCorrectionInfo> mathItalicsCorrectionInfo;

  /* Offset to MathTopAccentAttachment table -
     from the beginning of MathGlyphInfo table. */
  OffsetTo<MathTopAccentAttachment> mathTopAccentAttachment;

  /* Offset to coverage table for Extended Shape glyphs -
     from the beginning of MathGlyphInfo table. When the left or right glyph of
     a box is an extended shape variant, the (ink) box (and not the default
     position defined by values in MathConstants table) should be used for
     vertical positioning purposes. May be NULL.. */
  OffsetTo<Coverage> extendedShapeCoverage;

   /* Offset to MathKernInfo table -
      from the beginning of MathGlyphInfo table. */
  OffsetTo<MathKernInfo> mathKernInfo;

public:
  DEFINE_SIZE_STATIC (4 * 2);
};

struct MathGlyphVariantRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  hb_codepoint_t get_glyph() const { return variantGlyph; }
  inline hb_position_t get_advance_measurement (hb_font_t *font,
                                                bool horizontal) const
  {
    return horizontal ?
      font->em_scale_x (advanceMeasurement) :
      font->em_scale_y (advanceMeasurement);
  }

protected:
  GlyphID variantGlyph;       /* Glyph ID for the variant. */
  USHORT  advanceMeasurement; /* Advance width/height, in design units, of the
                                 variant, in the direction of requested
                                 glyph extension. */

public:
  DEFINE_SIZE_STATIC (2 + 2);
};

struct PartFlags : USHORT
{
  enum Flags {
    Extender = 0x0001u, /* If set, the part can be skipped or repeated. */
  };

public:
  DEFINE_SIZE_STATIC (2);
};

struct GlyphPartRecord
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  hb_codepoint_t get_glyph() const { return glyph; }

  inline hb_position_t
  get_start_connector_length (hb_font_t *font, bool horizontal) const
  {
    return horizontal ?
      font->em_scale_x (startConnectorLength) :
      font->em_scale_y (startConnectorLength);
  }

  inline hb_position_t
  get_end_connector_length (hb_font_t *font, bool horizontal) const
  {
    return horizontal ?
      font->em_scale_x (endConnectorLength) :
      font->em_scale_y (endConnectorLength);
  }

  inline hb_position_t
  get_full_advance (hb_font_t *font, bool horizontal) const
  {
    return horizontal ?
      font->em_scale_x (fullAdvance) :
      font->em_scale_y (fullAdvance);
  }

  inline bool is_extender() const {
    return partFlags & PartFlags::Flags::Extender;
  }

protected:
  GlyphID   glyph;                /* Glyph ID for the part. */
  USHORT    startConnectorLength; /* Advance width/ height of the straight bar
                                     connector material, in design units, is at
                                     the beginning of the glyph, in the
                                     direction of the extension. */
  USHORT    endConnectorLength;   /* Advance width/ height of the straight bar
                                     connector material, in design units, is at
                                     the end of the glyph, in the direction of
                                     the extension. */
  USHORT    fullAdvance;          /* Full advance width/height for this part,
                                     in the direction of the extension.
                                     In design units. */
  PartFlags partFlags;            /* Part qualifiers. */

public:
  DEFINE_SIZE_STATIC (5 * 2);
};

struct GlyphAssembly
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  italicsCorrection.sanitize(c, this) &&
                  partRecords.sanitize(c));
  }

  inline hb_position_t get_italic_correction (hb_font_t *font) const
  {
    return italicsCorrection.get_value(font, true, this);
  }

  inline unsigned int part_record_count() const { return partRecords.len; }
  inline const GlyphPartRecord &get_part_record(unsigned int i) const {
    assert(i < partRecords.len);
    return partRecords[i];
  }

protected:
  MathValueRecord          italicsCorrection; /* Italics correction of this
                                                 GlyphAssembly. Should not
                                                 depend on the assembly size. */
  ArrayOf<GlyphPartRecord> partRecords;       /* Array of part records, from
                                                 left to right and bottom to
                                                 top. */

public:
  DEFINE_SIZE_ARRAY (4 + 2, partRecords);
};

struct MathGlyphConstruction
{
  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  glyphAssembly.sanitize(c, this) &&
                   mathGlyphVariantRecord.sanitize(c));
  }

  inline bool has_glyph_assembly (void) const { return glyphAssembly != 0; }
  inline const GlyphAssembly &get_glyph_assembly (void) const {
    return this+glyphAssembly;
  }

  inline unsigned int glyph_variant_count() const {
    return mathGlyphVariantRecord.len;
  }
  inline const MathGlyphVariantRecord &get_glyph_variant(unsigned int i) const {
    assert(i < mathGlyphVariantRecord.len);
    return mathGlyphVariantRecord[i];
  }

protected:
  /* Offset to GlyphAssembly table for this shape - from the beginning of
     MathGlyphConstruction table. May be NULL. */
  OffsetTo<GlyphAssembly>         glyphAssembly;

  /* MathGlyphVariantRecords for alternative variants of the glyphs. */
  ArrayOf<MathGlyphVariantRecord> mathGlyphVariantRecord;

public:
  DEFINE_SIZE_ARRAY (2 + 2, mathGlyphVariantRecord);
};

struct MathVariants
{
  inline bool sanitize_offsets (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    unsigned int count = vertGlyphCount + horizGlyphCount;
    for (unsigned int i = 0; i < count; i++)
      if (!glyphConstruction[i].sanitize (c, this)) return_trace (false);
    return_trace (true);
  }

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  vertGlyphCoverage.sanitize (c, this) &&
                  horizGlyphCoverage.sanitize (c, this) &&
                  c->check_array (glyphConstruction,
                                  glyphConstruction[0].static_size,
                                  vertGlyphCount + horizGlyphCount) &&
                  sanitize_offsets (c));
  }

  inline hb_position_t get_min_connector_overlap (hb_font_t *font,
                                                  bool horizontal) const
  {
    return horizontal ?
      font->em_scale_x (minConnectorOverlap) :
      font->em_scale_y (minConnectorOverlap);
  }

  inline bool
  get_glyph_construction (hb_codepoint_t glyph,
                          hb_bool_t horizontal,
                          const MathGlyphConstruction *&glyph_construction) const {
    unsigned int index =
      horizontal ?
      (this+horizGlyphCoverage).get_coverage (glyph) :
      (this+vertGlyphCoverage).get_coverage (glyph);
    if (likely (index == NOT_COVERED)) return false;

    USHORT glyphCount = horizontal ? horizGlyphCount : vertGlyphCount;
    if (unlikely (index >= glyphCount)) return false;

    if (horizontal)
      index += vertGlyphCount;

    glyph_construction = &(this + glyphConstruction[index]);
    return true;
  }

protected:
  USHORT             minConnectorOverlap; /* Minimum overlap of connecting
                                             glyphs during glyph construction,
                                             in design units. */
  OffsetTo<Coverage> vertGlyphCoverage;   /* Offset to Coverage table -
                                             from the beginning of MathVariants
                                             table. */
  OffsetTo<Coverage> horizGlyphCoverage;  /* Offset to Coverage table -
                                             from the beginning of MathVariants
                                             table. */
  USHORT             vertGlyphCount;      /* Number of glyphs for which
                                             information is provided for
                                             vertically growing variants. */
  USHORT             horizGlyphCount;     /* Number of glyphs for which
                                             information is provided for
                                             horizontally growing variants. */

  /* Array of offsets to MathGlyphConstruction tables - from the beginning of
     the MathVariants table, for shapes growing in vertical/horizontal
     direction. */
  OffsetTo<MathGlyphConstruction>       glyphConstruction[VAR];

public:
  DEFINE_SIZE_ARRAY (5 * 2, glyphConstruction);
};

/*
 * MATH -- The MATH Table
 */

struct MATH
{
  static const hb_tag_t tableTag        = HB_OT_TAG_MATH;

  inline bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
                  likely (version.major == 1) &&
                  mathConstants.sanitize (c, this) &&
                  mathGlyphInfo.sanitize (c, this) &&
                  mathVariants.sanitize (c, this));
  }

  inline bool has_math_constants (void) const { return mathConstants != 0; }
  inline const MathConstants &get_math_constants (void) const {
    return this+mathConstants;
  }

  inline bool has_math_glyph_info (void) const { return mathGlyphInfo != 0; }
  inline const MathGlyphInfo &get_math_glyph_info (void) const {
    return this+mathGlyphInfo;
  }

  inline bool has_math_variants (void) const { return mathVariants != 0; }
  inline const MathVariants &get_math_variants (void) const {
    return this+mathVariants;
  }

protected:
  FixedVersion<>version;                 /* Version of the MATH table
                                            initially set to 0x00010000u */
  OffsetTo<MathConstants> mathConstants; /* MathConstants table */
  OffsetTo<MathGlyphInfo> mathGlyphInfo; /* MathGlyphInfo table */
  OffsetTo<MathVariants>  mathVariants;  /* MathVariants table */

public:
  DEFINE_SIZE_STATIC (4 + 3 * 2);
};

} /* mathspace OT */


#endif /* HB_OT_LAYOUT_MATH_TABLE_HH */
