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


#include "hb-test.h"

#include "hb-ot.h"

/* Unit tests for hb-ot-math.h - OpenType MATH table  */

static void
test_has_data (void)
{
  hb_face_t *face;
  hb_font_t *font;

  face = hb_test_open_font_file ("fonts/MathTestFontNone.otf");
  g_assert_true (!hb_ot_math_has_data (face)); // MATH table not available
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  g_assert_true (hb_ot_math_has_data (face)); // MATH table available
  hb_face_destroy (face);

  face = hb_face_get_empty ();
  font = hb_font_create (face);
  g_assert_true (!hb_ot_math_has_data (face)); // MATH table not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  font = hb_font_get_empty ();
  face = hb_font_get_face (font);
  g_assert_true (!hb_ot_math_has_data (face)); // MATH table not available
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_constant (void)
{
  hb_face_t *face;
  hb_font_t *font;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_cmpint (hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT), ==, 0); // MathConstants not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT)), ==, 100);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT)), ==, 200);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_MATH_LEADING)), ==, 300);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_AXIS_HEIGHT)), ==, 400);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_ACCENT_BASE_HEIGHT)), ==, 500);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FLATTENED_ACCENT_BASE_HEIGHT)), ==, 600);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN)), ==, 700);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUBSCRIPT_TOP_MAX)), ==, 800);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN)), ==, 900);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP)), ==, 1100);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP_CRAMPED)), ==, 1200);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MIN)), ==, 1300);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX)), ==, 1400);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN)), ==, 1500);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT)), ==, 1600);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT)), ==, 3400);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN)), ==, 1800);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_UPPER_LIMIT_BASELINE_RISE_MIN)), ==, 1900);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN)), ==, 2200);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_LOWER_LIMIT_BASELINE_DROP_MIN)), ==, 2300);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STACK_TOP_SHIFT_UP)), ==, 2400);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STACK_TOP_DISPLAY_STYLE_SHIFT_UP)), ==, 2500);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STACK_BOTTOM_SHIFT_DOWN)), ==, 2600);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STACK_BOTTOM_DISPLAY_STYLE_SHIFT_DOWN)), ==, 2700);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STACK_GAP_MIN)), ==, 2800);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STACK_DISPLAY_STYLE_GAP_MIN)), ==, 2900);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STRETCH_STACK_TOP_SHIFT_UP)), ==, 3000);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STRETCH_STACK_BOTTOM_SHIFT_DOWN)), ==, 3100);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_ABOVE_MIN)), ==, 3200);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_BELOW_MIN)), ==, 3300);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_SHIFT_UP)), ==, 3400);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_DISPLAY_STYLE_SHIFT_UP)), ==, 3500);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_SHIFT_DOWN)), ==, 3600);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_DISPLAY_STYLE_SHIFT_DOWN)), ==, 3700);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN)), ==, 3800);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_NUM_DISPLAY_STYLE_GAP_MIN)), ==, 3900);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS)), ==, 4000);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_GAP_MIN)), ==, 4100);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_FRACTION_DENOM_DISPLAY_STYLE_GAP_MIN)), ==, 4200);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SKEWED_FRACTION_HORIZONTAL_GAP)), ==, 8600);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SKEWED_FRACTION_VERTICAL_GAP)), ==, 4400);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_OVERBAR_VERTICAL_GAP)), ==, 4500);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_OVERBAR_RULE_THICKNESS)), ==, 4600);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_OVERBAR_EXTRA_ASCENDER)), ==, 4700);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_UNDERBAR_VERTICAL_GAP)), ==, 4800);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_UNDERBAR_RULE_THICKNESS)), ==, 4900);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_UNDERBAR_EXTRA_DESCENDER)), ==, 5000);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP)), ==, 5100);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_RADICAL_DISPLAY_STYLE_VERTICAL_GAP)), ==, 5200);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS)), ==, 5300);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_RADICAL_EXTRA_ASCENDER)), ==, 5400);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE)), ==, 11000);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE)), ==, 11200);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN)), ==, 87);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN)), ==, 76);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT)), ==, 65);
  hb_font_destroy (font);
  hb_face_destroy (face);

  /* https://github.com/harfbuzz/harfbuzz/pull/5250
   * The test font pretends to be Cambria Math for the purpose of
   * MATH->is_bad_cambria() by having displayOperatorMinHeight set to 2500 and 
   * delimitedSubFormulaMinHeight set to 3000, and the MATH table padded with
   * zeros to be exactly 25722 bytes. When we detect this, we swap the values
   * of the two constants. */
  face = hb_test_open_font_file ("fonts/MathTestFontPretendToBeCambria.ttf");
  font = hb_font_create (face);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT)), ==, 3000);
  g_assert_cmpint ((hb_ot_math_get_constant (font, HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT)), ==, 2500);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_glyph_italics_correction (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_italics_correction (font, glyph), ==, 0); // MathGlyphInfo not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial1.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_italics_correction (font, glyph), ==, 0); // MathGlyphInfo empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial2.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_italics_correction (font, glyph), ==, 0); // MathItalicsCorrectionInfo empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_italics_correction (font, glyph), ==, 0); // Glyph without italic correction.
  g_assert_true (hb_font_get_glyph_from_name (font, "A", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_italics_correction (font, glyph), ==, 394);
  g_assert_true (hb_font_get_glyph_from_name (font, "B", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_italics_correction (font, glyph), ==, 300);
  g_assert_true (hb_font_get_glyph_from_name (font, "C", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_italics_correction (font, glyph), ==, 904);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_glyph_top_accent_attachment (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_top_accent_attachment (font, glyph), ==, 1000); // MathGlyphInfo not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial1.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_top_accent_attachment (font, glyph), ==, 1000); // MathGlyphInfo empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial2.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_top_accent_attachment (font, glyph), ==, 1000); // MathTopAccentAttachment empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_top_accent_attachment (font, glyph), ==, 1000); // Glyph without top accent attachment.
  g_assert_true (hb_font_get_glyph_from_name (font, "D", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_top_accent_attachment (font, glyph), ==, 748);
  g_assert_true (hb_font_get_glyph_from_name (font, "E", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_top_accent_attachment (font, glyph), ==, 692);
  g_assert_true (hb_font_get_glyph_from_name (font, "F", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_top_accent_attachment (font, glyph), ==, 636);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_is_glyph_extended_shape (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_true (!hb_ot_math_is_glyph_extended_shape (face, glyph)); // MathGlyphInfo not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial1.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_true (!hb_ot_math_is_glyph_extended_shape (face, glyph)); // MathGlyphInfo empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "G", -1, &glyph));
  g_assert_true (!hb_ot_math_is_glyph_extended_shape (face, glyph));
  g_assert_true (hb_font_get_glyph_from_name (font, "H", -1, &glyph));
  g_assert_true (hb_ot_math_is_glyph_extended_shape (face, glyph));
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_glyph_kerning (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0), ==, 0); // MathGlyphInfo not available
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0), ==, 0); // MathGlyphInfo not available
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0), ==, 0); // MathGlyphInfo not available
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 0), ==, 0); // MathGlyphInfo not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial2.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0), ==, 0); // MathKernInfo empty
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0), ==, 0); // MathKernInfo empty
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0), ==, 0); // MathKernInfo empty
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 0), ==, 0); // MathKernInfo empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial3.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0), ==, 0); // MathKernInfoRecords empty
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0), ==, 0); // MathKernInfoRecords empty
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0), ==, 0); // MathKernInfoRecords empty
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 0), ==, 0); // MathKernInfoRecords empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "I", -1, &glyph));

  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 7), ==, 62); // lower than min height
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 14), ==, 104); // equal to min height
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 20), ==, 104);
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 23), ==, 146);
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 31), ==, 146);
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 32), ==, 188);
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 86), ==, 440); // equal to max height
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 91), ==, 440); // larger than max height
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 96), ==, 440); // larger than max height

  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 39), ==, 188); // top right
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 39), ==, 110); // top left
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 39), ==, 44); // bottom right
  g_assert_cmpint (hb_ot_math_get_glyph_kerning (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 39), ==, 100); // bottom left

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_glyph_kernings (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;
  hb_ot_math_kern_entry_t entries[20];
  const unsigned entries_size = sizeof (entries) / sizeof (entries[0]);
  unsigned int count;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0, NULL, NULL), ==, 0); // MathGlyphInfo not available
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0, NULL, NULL), ==, 0); // MathGlyphInfo not available
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0, NULL, NULL), ==, 0); // MathGlyphInfo not available
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 0, NULL, NULL), ==, 0); // MathGlyphInfo not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial2.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0, NULL, NULL), ==, 0); // MathKernInfo empty
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0, NULL, NULL), ==, 0); // MathKernInfo empty
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0, NULL, NULL), ==, 0); // MathKernInfo empty
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 0, NULL, NULL), ==, 0); // MathKernInfo empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial3.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0, NULL, NULL), ==, 0); // MathKernInfoRecords empty
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0, NULL, NULL), ==, 0); // MathKernInfoRecords empty
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0, NULL, NULL), ==, 0); // MathKernInfoRecords empty
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 0, NULL, NULL), ==, 0); // MathKernInfoRecords empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "I", -1, &glyph));

  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0, NULL, NULL), ==, 10);
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0, NULL, NULL), ==, 3);
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0, NULL, NULL), ==, 9);
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_BOTTOM_LEFT, 0, NULL, NULL), ==, 7);

  count = entries_size;
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0, &count, entries), ==, 10);
  g_assert_cmpint (count, ==, 10);
  g_assert_cmpint (entries[0].max_correction_height, ==, 14);
  g_assert_cmpint (entries[0].kern_value, ==, 62);
  g_assert_cmpint (entries[1].max_correction_height, ==, 23);
  g_assert_cmpint (entries[1].kern_value, ==, 104);
  g_assert_cmpint (entries[2].max_correction_height, ==, 32);
  g_assert_cmpint (entries[2].kern_value, ==, 146);
  g_assert_cmpint (entries[3].max_correction_height, ==, 41);
  g_assert_cmpint (entries[3].kern_value, ==, 188);
  g_assert_cmpint (entries[4].max_correction_height, ==, 50);
  g_assert_cmpint (entries[4].kern_value, ==, 230);
  g_assert_cmpint (entries[5].max_correction_height, ==, 59);
  g_assert_cmpint (entries[5].kern_value, ==, 272);
  g_assert_cmpint (entries[6].max_correction_height, ==, 68);
  g_assert_cmpint (entries[6].kern_value, ==, 314);
  g_assert_cmpint (entries[7].max_correction_height, ==, 77);
  g_assert_cmpint (entries[7].kern_value, ==, 356);
  g_assert_cmpint (entries[8].max_correction_height, ==, 86);
  g_assert_cmpint (entries[8].kern_value, ==, 398);
  g_assert_cmpint (entries[9].max_correction_height, ==, INT32_MAX);
  g_assert_cmpint (entries[9].kern_value, ==, 440);

  count = entries_size;
  g_assert_cmpint (hb_ot_math_get_glyph_kernings (font, glyph, HB_OT_MATH_KERN_TOP_LEFT, 0, &count, entries), ==, 3);
  g_assert_cmpint (count, ==, 3);
  g_assert_cmpint (entries[0].max_correction_height, ==, 20);
  g_assert_cmpint (entries[0].kern_value, ==, 50);
  g_assert_cmpint (entries[1].max_correction_height, ==, 35);
  g_assert_cmpint (entries[1].kern_value, ==, 80);
  g_assert_cmpint (entries[2].max_correction_height, ==, INT32_MAX);
  g_assert_cmpint (entries[2].kern_value, ==, 110);

  hb_font_destroy (font);
  hb_face_destroy (face);
}


static hb_position_t
get_glyph_assembly_italics_correction (hb_font_t *font,
				       hb_codepoint_t glyph,
				       hb_bool_t horizontal)
{
  hb_position_t corr;
  hb_ot_math_get_glyph_assembly (font, glyph,
				 horizontal ? HB_DIRECTION_LTR : HB_DIRECTION_TTB,
				 0, NULL, NULL,
				 &corr);
  return corr;
}

static void
test_get_glyph_assembly_italics_correction (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, TRUE), ==, 0); // MathVariants not available
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, FALSE), ==, 0); // MathVariants not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial1.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, TRUE), ==, 0); // VertGlyphCoverage and HorizGlyphCoverage absent
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, FALSE), ==, 0); // VertGlyphCoverage and HorizGlyphCoverage absent
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial2.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, TRUE), ==, 0); // VertGlyphCoverage and HorizGlyphCoverage empty
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, FALSE), ==, 0); // VertGlyphCoverage and HorizGlyphCoverage empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial3.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, TRUE), ==, 0); // HorizGlyphConstruction and VertGlyphConstruction empty
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, FALSE), ==, 0);  // HorizGlyphConstruction and VertGlyphConstruction empty
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial4.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, TRUE), ==, 0);
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, FALSE), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_true (hb_font_get_glyph_from_name (font, "arrowleft", -1, &glyph));
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, TRUE), ==, 248);
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, FALSE), ==, 0);
  g_assert_true (hb_font_get_glyph_from_name (font, "arrowup", -1, &glyph));
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, TRUE), ==, 0);
  g_assert_cmpint (get_glyph_assembly_italics_correction (font, glyph, FALSE), ==, 662);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_min_connector_overlap (void)
{
  hb_face_t *face;
  hb_font_t *font;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_cmpint (hb_ot_math_get_min_connector_overlap (font, HB_DIRECTION_LTR), ==, 0); // MathVariants not available
  g_assert_cmpint (hb_ot_math_get_min_connector_overlap (font, HB_DIRECTION_TTB), ==, 0); // MathVariants not available
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial1.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);
  g_assert_cmpint (hb_ot_math_get_min_connector_overlap (font, HB_DIRECTION_LTR), ==, 108);
  g_assert_cmpint (hb_ot_math_get_min_connector_overlap (font, HB_DIRECTION_TTB), ==, 54);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_glyph_variants (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;
  hb_ot_math_glyph_variant_t variants[20];
  unsigned variantsSize = sizeof (variants) / sizeof (variants[0]);
  unsigned int count;
  unsigned int offset = 0;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial1.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial2.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial3.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial4.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowleft", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font,
						 glyph,
						 HB_DIRECTION_BTT,
						 0,
						 NULL,
						 NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font,
						 glyph,
						 HB_DIRECTION_RTL,
						 0,
						 NULL,
						 NULL), ==, 3);

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowup", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font,
						 glyph,
						 HB_DIRECTION_BTT,
						 0,
						 NULL,
						 NULL), ==, 4);
  g_assert_cmpint (hb_ot_math_get_glyph_variants (font,
						 glyph,
						 HB_DIRECTION_RTL,
						 0,
						 NULL,
						 NULL), ==, 0);

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowleft", -1, &glyph));
  do {
    count = variantsSize;
    hb_ot_math_get_glyph_variants (font,
				   glyph,
				   HB_DIRECTION_RTL,
				   offset,
				   &count,
				   variants);
    offset += count;
  } while (count == variantsSize);
  g_assert_cmpint (offset, ==, 3);
  g_assert_true (hb_font_get_glyph_from_name (font, "uni2190_size2", -1, &glyph));
  g_assert_cmpint (variants[0].glyph, ==, glyph);
  g_assert_cmpint (variants[0].advance, ==, 4302);
  g_assert_true (hb_font_get_glyph_from_name (font, "uni2190_size3", -1, &glyph));
  g_assert_cmpint (variants[1].glyph, ==, glyph);
  g_assert_cmpint (variants[1].advance, ==, 4802);
  g_assert_true (hb_font_get_glyph_from_name (font, "uni2190_size4", -1, &glyph));
  g_assert_cmpint (variants[2].glyph, ==, glyph);
  g_assert_cmpint (variants[2].advance, ==, 5802);

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowup", -1, &glyph));
  offset = 0;
  do {
    count = variantsSize;
    hb_ot_math_get_glyph_variants (font,
				   glyph,
				   HB_DIRECTION_BTT,
				   offset,
				   &count,
				   variants);
    offset += count;
  } while (count == variantsSize);
  g_assert_cmpint (offset, ==, 4);
  g_assert_true (hb_font_get_glyph_from_name (font, "uni2191_size2", -1, &glyph));
  g_assert_cmpint (variants[0].glyph, ==, glyph);
  g_assert_cmpint (variants[0].advance, ==, 2251);
  g_assert_true (hb_font_get_glyph_from_name (font, "uni2191_size3", -1, &glyph));
  g_assert_cmpint (variants[1].glyph, ==, glyph);
  g_assert_cmpint (variants[1].advance, ==, 2501);
  g_assert_true (hb_font_get_glyph_from_name (font, "uni2191_size4", -1, &glyph));
  g_assert_cmpint (variants[2].glyph, ==, glyph);
  g_assert_cmpint (variants[2].advance, ==, 3001);
  g_assert_true (hb_font_get_glyph_from_name (font, "uni2191_size5", -1, &glyph));
  g_assert_cmpint (variants[3].glyph, ==, glyph);
  g_assert_cmpint (variants[3].advance, ==, 3751);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_get_glyph_assembly (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_codepoint_t glyph;
  hb_ot_math_glyph_part_t parts[20];
  unsigned partsSize = sizeof (parts) / sizeof (parts[0]);
  unsigned int count;
  unsigned int offset = 0;

  face = hb_test_open_font_file ("fonts/MathTestFontEmpty.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial1.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial2.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial3.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontPartial4.otf");
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_from_name (font, "space", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_RTL, 0, NULL, NULL, NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_BTT, 0, NULL, NULL, NULL), ==, 0);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  font = hb_font_create (face);
  hb_font_set_scale (font, 2000, 1000);

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowright", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font,
						 glyph,
						 HB_DIRECTION_BTT,
						 0,
						 NULL,
						 NULL,
						 NULL), ==, 0);
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font,
						 glyph,
						 HB_DIRECTION_RTL,
						 0,
						 NULL,
						 NULL,
						 NULL), ==, 3);

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowdown", -1, &glyph));
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font,
						 glyph,
						 HB_DIRECTION_BTT,
						 0,
						 NULL,
						 NULL,
						 NULL), ==, 5);
  g_assert_cmpint (hb_ot_math_get_glyph_assembly (font,
						 glyph,
						 HB_DIRECTION_RTL,
						 0,
						 NULL,
						 NULL,
						 NULL), ==, 0);

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowright", -1, &glyph));
  do {
    count = partsSize;
    hb_ot_math_get_glyph_assembly (font,
				   glyph,
				   HB_DIRECTION_RTL,
				   offset,
				   &count,
				   parts,
				   NULL);
    offset += count;
  } while (count == partsSize);
  g_assert_cmpint (offset, ==, 3);
  g_assert_true (hb_font_get_glyph_from_name (font, "left", -1, &glyph));
  g_assert_cmpint (parts[0].glyph, ==, glyph);
  g_assert_cmpint (parts[0].start_connector_length, ==, 800);
  g_assert_cmpint (parts[0].end_connector_length, ==, 384);
  g_assert_cmpint (parts[0].full_advance, ==, 2000);
  g_assert_true (!(parts[0].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER));
  g_assert_true (hb_font_get_glyph_from_name (font, "horizontal", -1, &glyph));
  g_assert_cmpint (parts[1].glyph, ==, glyph);
  g_assert_cmpint (parts[1].start_connector_length, ==, 524);
  g_assert_cmpint (parts[1].end_connector_length, ==, 800);
  g_assert_cmpint (parts[1].full_advance, ==, 2000);
  g_assert_true (parts[1].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER);
  g_assert_true (hb_font_get_glyph_from_name (font, "right", -1, &glyph));
  g_assert_cmpint (parts[2].glyph, ==, glyph);
  g_assert_cmpint (parts[2].start_connector_length, ==, 316);
  g_assert_cmpint (parts[2].end_connector_length, ==, 454);
  g_assert_cmpint (parts[2].full_advance, ==, 2000);
  g_assert_true (!(parts[2].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER));

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowdown", -1, &glyph));
  offset = 0;
  do {
    count = partsSize;
    hb_ot_math_get_glyph_assembly (font,
				   glyph,
				   HB_DIRECTION_BTT,
				   offset,
				   &count,
				   parts,
				   NULL);
    offset += count;
  } while (count == partsSize);
  g_assert_cmpint (offset, ==, 5);
  g_assert_true (hb_font_get_glyph_from_name (font, "bottom", -1, &glyph));
  g_assert_cmpint (parts[0].glyph, ==, glyph);
  g_assert_cmpint (parts[0].start_connector_length, ==, 365);
  g_assert_cmpint (parts[0].end_connector_length, ==, 158);
  g_assert_cmpint (parts[0].full_advance, ==, 1000);
  g_assert_true (!(parts[0].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER));
  g_assert_true (hb_font_get_glyph_from_name (font, "vertical", -1, &glyph));
  g_assert_cmpint (parts[1].glyph, ==, glyph);
  g_assert_cmpint (parts[1].start_connector_length, ==, 227);
  g_assert_cmpint (parts[1].end_connector_length, ==, 365);
  g_assert_cmpint (parts[1].full_advance, ==, 1000);
  g_assert_true (parts[1].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER);
  g_assert_true (hb_font_get_glyph_from_name (font, "center", -1, &glyph));
  g_assert_cmpint (parts[2].glyph, ==, glyph);
  g_assert_cmpint (parts[2].start_connector_length, ==, 54);
  g_assert_cmpint (parts[2].end_connector_length, ==, 158);
  g_assert_cmpint (parts[2].full_advance, ==, 1000);
  g_assert_true (!(parts[2].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER));
  g_assert_true (hb_font_get_glyph_from_name (font, "vertical", -1, &glyph));
  g_assert_cmpint (parts[3].glyph, ==, glyph);
  g_assert_cmpint (parts[3].start_connector_length, ==, 400);
  g_assert_cmpint (parts[3].end_connector_length, ==, 296);
  g_assert_cmpint (parts[3].full_advance, ==, 1000);
  g_assert_true (parts[1].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER);
  g_assert_true (hb_font_get_glyph_from_name (font, "top", -1, &glyph));
  g_assert_cmpint (parts[4].glyph, ==, glyph);
  g_assert_cmpint (parts[4].start_connector_length, ==, 123);
  g_assert_cmpint (parts[4].end_connector_length, ==, 192);
  g_assert_cmpint (parts[4].full_advance, ==, 1000);
  g_assert_true (!(parts[4].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER));

  hb_font_destroy (font);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_has_data);
  hb_test_add (test_get_constant);
  hb_test_add (test_get_glyph_italics_correction);
  hb_test_add (test_get_glyph_top_accent_attachment);
  hb_test_add (test_is_glyph_extended_shape);
  hb_test_add (test_get_glyph_kerning);
  hb_test_add (test_get_glyph_kernings);
  hb_test_add (test_get_glyph_assembly_italics_correction);
  hb_test_add (test_get_min_connector_overlap);
  hb_test_add (test_get_glyph_variants);
  hb_test_add (test_get_glyph_assembly);

  return hb_test_run ();
}
