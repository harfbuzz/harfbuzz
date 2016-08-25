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

#include "hb-ft.h"
#include "hb-ot.h"

/* Unit tests for hb-ot-layout.h - OpenType MATH table  */

static FT_Library ft_library;
static FT_Face ft_face;
static hb_font_t *hb_font;
static hb_face_t *hb_face;

static inline void
initFreeType (void)
{
  FT_Error ft_error;
  if ((ft_error = FT_Init_FreeType (&ft_library)))
    abort();
}

static inline void
cleanupFreeType (void)
{
  FT_Done_FreeType (ft_library);
}

static void
openFont(const char* fontFile)
{
  FT_Error ft_error;
  if ((ft_error = FT_New_Face (ft_library, fontFile, 0, &ft_face)))
    abort();

#define fontSize 1000

  if ((ft_error = FT_Set_Char_Size (ft_face, fontSize, fontSize, 0, 0)))
    abort();
  hb_font = hb_ft_font_create (ft_face, NULL);
  hb_face = hb_ft_face_create_cached(ft_face);
}

static inline void
closeFont (void)
{
  hb_font_destroy (hb_font);
  FT_Done_Face (ft_face);
}

static void
test_has_math_data (void)
{
  initFreeType();

  openFont("fonts/MathTestFontNone.otf");
  g_assert(!hb_ot_layout_has_math_data (hb_face)); // MATH table not available
  closeFont();

  openFont("fonts/MathTestFontEmpty.otf");
  g_assert(hb_ot_layout_has_math_data (hb_face)); // MATH table available
  closeFont();

  cleanupFreeType();
}

static void
test_get_math_constant (void)
{
  initFreeType();

  openFont("fonts/MathTestFontEmpty.otf");
  g_assert_cmpint(hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT), ==, 0); // MathConstants not available
  closeFont();

  openFont("fonts/MathTestFontFull.otf");
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT)), ==, 100);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT)), ==, 200);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_MATH_LEADING)), ==, 300);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_AXIS_HEIGHT)), ==, 400);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_ACCENT_BASE_HEIGHT)), ==, 500);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FLATTENED_ACCENT_BASE_HEIGHT)), ==, 600);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN)), ==, 700);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUBSCRIPT_TOP_MAX)), ==, 800);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN)), ==, 900);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP)), ==, 1100);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP_CRAMPED)), ==, 1200);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MIN)), ==, 1300);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX)), ==, 1400);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN)), ==, 1500);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT)), ==, 1600);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT)), ==, 1700);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN)), ==, 1800);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_UPPER_LIMIT_BASELINE_RISE_MIN)), ==, 1900);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN)), ==, 2200);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_LOWER_LIMIT_BASELINE_DROP_MIN)), ==, 2300);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STACK_TOP_SHIFT_UP)), ==, 2400);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STACK_TOP_DISPLAY_STYLE_SHIFT_UP)), ==, 2500);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STACK_BOTTOM_SHIFT_DOWN)), ==, 2600);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STACK_BOTTOM_DISPLAY_STYLE_SHIFT_DOWN)), ==, 2700);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STACK_GAP_MIN)), ==, 2800);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STACK_DISPLAY_STYLE_GAP_MIN)), ==, 2900);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STRETCH_STACK_TOP_SHIFT_UP)), ==, 3000);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STRETCH_STACK_BOTTOM_SHIFT_DOWN)), ==, 3100);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_ABOVE_MIN)), ==, 3200);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_BELOW_MIN)), ==, 3300);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_SHIFT_UP)), ==, 3400);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_DISPLAY_STYLE_SHIFT_UP)), ==, 3500);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_SHIFT_DOWN)), ==, 3600);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_DISPLAY_STYLE_SHIFT_DOWN)), ==, 3700);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN)), ==, 3800);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_NUM_DISPLAY_STYLE_GAP_MIN)), ==, 3900);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS)), ==, 4000);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_GAP_MIN)), ==, 4100);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_FRACTION_DENOM_DISPLAY_STYLE_GAP_MIN)), ==, 4200);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SKEWED_FRACTION_HORIZONTAL_GAP)), ==, 4300);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SKEWED_FRACTION_VERTICAL_GAP)), ==, 4400);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_OVERBAR_VERTICAL_GAP)), ==, 4500);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_OVERBAR_RULE_THICKNESS)), ==, 4600);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_OVERBAR_EXTRA_ASCENDER)), ==, 4700);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_UNDERBAR_VERTICAL_GAP)), ==, 4800);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_UNDERBAR_RULE_THICKNESS)), ==, 4900);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_UNDERBAR_EXTRA_DESCENDER)), ==, 5000);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP)), ==, 5100);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_RADICAL_DISPLAY_STYLE_VERTICAL_GAP)), ==, 5200);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS)), ==, 5300);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_RADICAL_EXTRA_ASCENDER)), ==, 5400);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE)), ==, 5500);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE)), ==, 5600);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN)), ==, 87);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN)), ==, 76);
  g_assert_cmpint((hb_ot_layout_get_math_constant (hb_font, HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT)), ==, 65);
  closeFont();

  cleanupFreeType();
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_has_math_data);
  hb_test_add (test_get_math_constant);

  return hb_test_run();
}
