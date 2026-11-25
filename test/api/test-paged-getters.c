/*
 * Copyright © 2026  Behdad Esfahbod
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

#include "hb-test.h"

#include <hb-ot.h>
#include <hb-aat.h>

/* A paged getter takes the capacity of a caller-supplied array in a count,
 * writes at most that many items, stores the number it wrote back in the
 * count, and returns the total number of items there are.
 *
 * Passing a count together with a NULL array is not a conforming call.  When
 * it happens anyway, the getter writes nothing and says so: the count comes
 * back zero.  The return value is the same total that passing NULL for both
 * gets.
 *
 * Each test makes that call where the getter has something to return, and
 * again where it has nothing. */

#define SENTINEL 12345u

/* For getters whose last two parameters are the count and the array. */
#define TEST_PAGED_GETTER(cmp, getter, ...) \
  G_STMT_START { \
    unsigned int total_ = getter (__VA_ARGS__, NULL, NULL); \
    unsigned int count_ = SENTINEL; \
    g_assert_cmpuint (total_, cmp, 0); \
    g_assert_cmpuint (getter (__VA_ARGS__, &count_, NULL), ==, total_); \
    g_assert_cmpuint (count_, ==, 0); \
  } G_STMT_END
#define TEST_POPULATED(getter, ...) TEST_PAGED_GETTER (>, getter, __VA_ARGS__)
#define TEST_EMPTY(getter, ...) TEST_PAGED_GETTER (==, getter, __VA_ARGS__)


#ifndef HB_NO_AAT
static void
test_paged_getters_aat_feature_types (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/aat-feat.ttf");

  TEST_POPULATED (hb_aat_layout_get_feature_types, face, 0);
  TEST_EMPTY (hb_aat_layout_get_feature_types, hb_face_get_empty (), 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_aat_selector_infos (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/aat-feat.ttf");
  hb_aat_layout_feature_type_t type = HB_AAT_LAYOUT_FEATURE_TYPE_DESIGN_COMPLEXITY_TYPE;
  unsigned int total, count, default_index;

  total = hb_aat_layout_feature_type_get_selector_infos (face, type, 0, NULL, NULL, NULL);
  g_assert_cmpuint (total, >, 0);
  count = default_index = SENTINEL;
  g_assert_cmpuint (hb_aat_layout_feature_type_get_selector_infos (face, type, 0,
								   &count, NULL,
								   &default_index), ==, total);
  g_assert_cmpuint (count, ==, 0);
  /* The other output is still filled in. */
  g_assert_cmpuint (default_index, ==, 0);

  count = SENTINEL;
  g_assert_cmpuint (hb_aat_layout_feature_type_get_selector_infos (hb_face_get_empty (), type, 0,
								   &count, NULL, NULL), ==, 0);
  g_assert_cmpuint (count, ==, 0);

  hb_face_destroy (face);
}
#endif

static void
test_paged_getters_face_table_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  TEST_POPULATED (hb_face_get_table_tags, face, 0);
  TEST_EMPTY (hb_face_get_table_tags, hb_face_get_empty (), 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_face_builder_table_tags (void)
{
  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create ("test", 4, HB_MEMORY_MODE_READONLY, NULL, NULL);

  TEST_EMPTY (hb_face_get_table_tags, face, 0);
  g_assert_true (hb_face_builder_add_table (face, HB_TAG ('h','e','a','d'), blob));
  g_assert_true (hb_face_builder_add_table (face, HB_TAG ('m','a','x','p'), blob));
  TEST_POPULATED (hb_face_get_table_tags, face, 0);

  hb_blob_destroy (blob);
  hb_face_destroy (face);
}

/* The getter hands the call to the face's callback, which can be the
 * caller's own, and need not expect a NULL array. */
static unsigned int
get_table_tags (const hb_face_t *face HB_UNUSED,
		unsigned int start_offset,
		unsigned int *table_count,
		hb_tag_t *table_tags,
		void *user_data HB_UNUSED)
{
  if (table_count)
  {
    unsigned int n = 0;
    for (unsigned int i = start_offset; i < 2 && n < *table_count; i++, n++)
      table_tags[n] = HB_TAG ('t','a','g','0' + i);
    *table_count = n;
  }
  return 2;
}

static hb_blob_t *
reference_table (hb_face_t *face HB_UNUSED,
		 hb_tag_t tag HB_UNUSED,
		 void *user_data HB_UNUSED)
{
  return NULL;
}

static void
test_paged_getters_face_callback_table_tags (void)
{
  hb_face_t *face = hb_face_create_for_tables (reference_table, NULL, NULL);

  hb_face_set_get_table_tags_func (face, get_table_tags, NULL, NULL);
  TEST_POPULATED (hb_face_get_table_tags, face, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_color_palette_colors (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cpal-v0.ttf");

  TEST_POPULATED (hb_ot_color_palette_get_colors, face, 0, 0);
  TEST_EMPTY (hb_ot_color_palette_get_colors, hb_face_get_empty (), 0, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_color_glyph_layers (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cpal-v1.ttf");

  TEST_POPULATED (hb_ot_color_glyph_get_layers, face, 2, 0);
  TEST_EMPTY (hb_ot_color_glyph_get_layers, hb_face_get_empty (), 2, 0);

  hb_face_destroy (face);
}

static void
check_color_stops (hb_paint_funcs_t *funcs HB_UNUSED,
		   void *paint_data,
		   hb_color_line_t *color_line,
		   float x0 HB_UNUSED, float y0 HB_UNUSED,
		   float x1 HB_UNUSED, float y1 HB_UNUSED,
		   float x2 HB_UNUSED, float y2 HB_UNUSED,
		   void *user_data HB_UNUSED)
{
  unsigned int *gradients = (unsigned int *) paint_data;

  TEST_POPULATED (hb_color_line_get_color_stops, color_line, 0);
  (*gradients)++;
}

static void
test_paged_getters_color_line_color_stops (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/noto_handwriting-cff2_colr_1.otf");
  hb_font_t *font = hb_font_create (face);
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  unsigned int gradients = 0;

  hb_paint_funcs_set_linear_gradient_func (funcs, check_color_stops, NULL, NULL);
  hb_font_paint_glyph (font, 10, funcs, &gradients, 0, HB_COLOR (0, 0, 0, 255));
  g_assert_cmpuint (gradients, >, 0);

  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

/* Likewise for a color line: the callback can be the caller's own. */
static unsigned int
get_color_stops (hb_color_line_t *color_line HB_UNUSED,
		 void *color_line_data HB_UNUSED,
		 unsigned int start,
		 unsigned int *count,
		 hb_color_stop_t *color_stops,
		 void *user_data HB_UNUSED)
{
  if (count)
  {
    unsigned int n = 0;
    for (unsigned int i = start; i < 2 && n < *count; i++, n++)
    {
      color_stops[n].offset = (float) i;
      color_stops[n].is_foreground = false;
      color_stops[n].color = HB_COLOR (0, 0, 0, 255);
    }
    *count = n;
  }
  return 2;
}

static void
test_paged_getters_color_line_callback_color_stops (void)
{
  hb_color_line_t color_line;

  memset (&color_line, 0, sizeof (color_line));
  color_line.get_color_stops = get_color_stops;
  TEST_POPULATED (hb_color_line_get_color_stops, &color_line, 0);
}

#ifndef HB_DISABLE_DEPRECATED
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable: 4996)
#endif
static void
test_paged_getters_var_axes (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");

  TEST_POPULATED (hb_ot_var_get_axes, face, 0);
  TEST_EMPTY (hb_ot_var_get_axes, hb_face_get_empty (), 0);

  hb_face_destroy (face);
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning (pop)
#endif
#endif

static void
test_paged_getters_var_axis_infos (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");

  TEST_POPULATED (hb_ot_var_get_axis_infos, face, 0);
  TEST_EMPTY (hb_ot_var_get_axis_infos, hb_face_get_empty (), 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_var_named_instance_design_coords (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");

  TEST_POPULATED (hb_ot_var_named_instance_get_design_coords, face, 0);
  TEST_EMPTY (hb_ot_var_named_instance_get_design_coords, hb_face_get_empty (), 0);

  hb_face_destroy (face);
}

/* Returns nothing, and has two count/array pairs: each is on its own. */
static void
test_paged_getters_tags_from_script_and_language (void)
{
  hb_script_t script = HB_SCRIPT_DEVANAGARI;
  hb_language_t language = hb_language_from_string ("en", -1);
  hb_tag_t script_tags[HB_OT_MAX_TAGS_PER_SCRIPT];
  hb_tag_t language_tags[HB_OT_MAX_TAGS_PER_LANGUAGE];
  unsigned int script_count, language_count;

  script_count = HB_OT_MAX_TAGS_PER_SCRIPT;
  language_count = HB_OT_MAX_TAGS_PER_LANGUAGE;
  hb_ot_tags_from_script_and_language (script, language,
				       &script_count, script_tags,
				       &language_count, language_tags);
  g_assert_cmpuint (script_count, ==, 3);
  g_assert_cmpuint (language_count, ==, 1);

  script_count = language_count = SENTINEL;
  hb_ot_tags_from_script_and_language (script, language,
				       &script_count, NULL,
				       &language_count, NULL);
  g_assert_cmpuint (script_count, ==, 0);
  g_assert_cmpuint (language_count, ==, 0);

  script_count = SENTINEL;
  language_count = HB_OT_MAX_TAGS_PER_LANGUAGE;
  language_tags[0] = HB_TAG_NONE;
  hb_ot_tags_from_script_and_language (script, language,
				       &script_count, NULL,
				       &language_count, language_tags);
  g_assert_cmpuint (script_count, ==, 0);
  g_assert_cmpuint (language_count, ==, 1);
  g_assert_cmpuint (language_tags[0], ==, HB_TAG ('E','N','G',' '));

  script_count = HB_OT_MAX_TAGS_PER_SCRIPT;
  language_count = SENTINEL;
  script_tags[0] = HB_TAG_NONE;
  hb_ot_tags_from_script_and_language (script, language,
				       &script_count, script_tags,
				       &language_count, NULL);
  g_assert_cmpuint (script_count, ==, 3);
  g_assert_cmpuint (script_tags[0], ==, HB_TAG ('d','e','v','3'));
  g_assert_cmpuint (language_count, ==, 0);

  /* Tags given in private-use subtags. */
  script_count = language_count = SENTINEL;
  hb_ot_tags_from_script_and_language (script,
				       hb_language_from_string ("x-hbscabcd-hbotefgh", -1),
				       &script_count, NULL,
				       &language_count, NULL);
  g_assert_cmpuint (script_count, ==, 0);
  g_assert_cmpuint (language_count, ==, 0);

  /* Nothing to return. */
  script_count = language_count = SENTINEL;
  hb_ot_tags_from_script_and_language (HB_SCRIPT_INVALID, HB_LANGUAGE_INVALID,
				       &script_count, NULL,
				       &language_count, NULL);
  g_assert_cmpuint (script_count, ==, 0);
  g_assert_cmpuint (language_count, ==, 0);
}

static void
test_paged_getters_layout_attach_points (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  TEST_POPULATED (hb_ot_layout_get_attach_points, face, 15, 0);
  TEST_EMPTY (hb_ot_layout_get_attach_points, hb_face_get_empty (), 15, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_ligature_carets (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");
  hb_font_t *font = hb_font_create (face);

  TEST_POPULATED (hb_ot_layout_get_ligature_carets, font, HB_DIRECTION_LTR, 1020, 0);
  TEST_EMPTY (hb_ot_layout_get_ligature_carets, hb_font_get_empty (), HB_DIRECTION_LTR, 1020, 0);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_paged_getters_layout_script_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  TEST_POPULATED (hb_ot_layout_table_get_script_tags, face, HB_OT_TAG_GSUB, 0);
  TEST_EMPTY (hb_ot_layout_table_get_script_tags, hb_face_get_empty (), HB_OT_TAG_GSUB, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_feature_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  TEST_POPULATED (hb_ot_layout_table_get_feature_tags, face, HB_OT_TAG_GSUB, 0);
  TEST_EMPTY (hb_ot_layout_table_get_feature_tags, hb_face_get_empty (), HB_OT_TAG_GSUB, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_language_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  TEST_POPULATED (hb_ot_layout_script_get_language_tags, face, HB_OT_TAG_GSUB, 0, 0);
  TEST_EMPTY (hb_ot_layout_script_get_language_tags, hb_face_get_empty (), HB_OT_TAG_GSUB, 0, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_language_feature_indexes (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  TEST_POPULATED (hb_ot_layout_language_get_feature_indexes, face, HB_OT_TAG_GSUB,
		  0, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX, 0);
  TEST_EMPTY (hb_ot_layout_language_get_feature_indexes, hb_face_get_empty (), HB_OT_TAG_GSUB,
	      0, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_language_feature_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  TEST_POPULATED (hb_ot_layout_language_get_feature_tags, face, HB_OT_TAG_GSUB,
		  0, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX, 0);
  TEST_EMPTY (hb_ot_layout_language_get_feature_tags, hb_face_get_empty (), HB_OT_TAG_GSUB,
	      0, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_feature_lookups (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  TEST_POPULATED (hb_ot_layout_feature_get_lookups, face, HB_OT_TAG_GSUB, 0, 0);
  TEST_EMPTY (hb_ot_layout_feature_get_lookups, hb_face_get_empty (), HB_OT_TAG_GSUB, 0, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_feature_with_variations_lookups (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/feature-variation-conditions.ttf");

  /* The first FeatureVariations record replaces the first feature. */
  TEST_POPULATED (hb_ot_layout_feature_with_variations_get_lookups, face, HB_OT_TAG_GSUB,
		  0, 0, 0);
  TEST_POPULATED (hb_ot_layout_feature_with_variations_get_lookups, face, HB_OT_TAG_GSUB,
		  0, HB_OT_LAYOUT_NO_VARIATIONS_INDEX, 0);
  TEST_EMPTY (hb_ot_layout_feature_with_variations_get_lookups, hb_face_get_empty (), HB_OT_TAG_GSUB,
	      0, 0, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_glyph_alternates (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansPro-Regular.otf");

  /* An AlternateSubst lookup, and a SingleSubst one. */
  TEST_POPULATED (hb_ot_layout_lookup_get_glyph_alternates, face, 1, 1091, 0);
  TEST_POPULATED (hb_ot_layout_lookup_get_glyph_alternates, face, 0, 54, 0);
  TEST_EMPTY (hb_ot_layout_lookup_get_glyph_alternates, face, 1, 0, 0);
  TEST_EMPTY (hb_ot_layout_lookup_get_glyph_alternates, hb_face_get_empty (), 1, 1091, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_feature_characters (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cv01.otf");
  unsigned int feature_index;

  g_assert_true (hb_ot_layout_language_find_feature (face, HB_OT_TAG_GSUB, 0,
						     HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
						     HB_TAG ('c','v','0','1'), &feature_index));
  TEST_POPULATED (hb_ot_layout_feature_get_characters, face, HB_OT_TAG_GSUB, feature_index, 0);
  TEST_EMPTY (hb_ot_layout_feature_get_characters, hb_face_get_empty (), HB_OT_TAG_GSUB, feature_index, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_layout_extender_glyphs (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/jstf.ttf");

  TEST_POPULATED (hb_ot_layout_get_extender_glyphs, face, HB_SCRIPT_ARABIC, HB_LANGUAGE_INVALID, 0);
  TEST_EMPTY (hb_ot_layout_get_extender_glyphs, hb_face_get_empty (), HB_SCRIPT_ARABIC, HB_LANGUAGE_INVALID, 0);

  hb_face_destroy (face);
}

static void
test_paged_getters_math_glyph_kernings (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  hb_font_t *font = hb_font_create (face);
  hb_codepoint_t glyph;

  g_assert_true (hb_font_get_glyph_from_name (font, "I", -1, &glyph));
  TEST_POPULATED (hb_ot_math_get_glyph_kernings, font, glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0);
  TEST_EMPTY (hb_ot_math_get_glyph_kernings, hb_font_get_empty (), glyph, HB_OT_MATH_KERN_TOP_RIGHT, 0);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_paged_getters_math_glyph_variants (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  hb_font_t *font = hb_font_create (face);
  hb_codepoint_t glyph;

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowup", -1, &glyph));
  TEST_POPULATED (hb_ot_math_get_glyph_variants, font, glyph, HB_DIRECTION_BTT, 0);
  TEST_EMPTY (hb_ot_math_get_glyph_variants, hb_font_get_empty (), glyph, HB_DIRECTION_BTT, 0);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_paged_getters_math_glyph_assembly (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/MathTestFontFull.otf");
  hb_font_t *font = hb_font_create (face);
  hb_codepoint_t glyph;
  unsigned int total, count;
  hb_position_t expected_italics_correction, italics_correction;

  g_assert_true (hb_font_get_glyph_from_name (font, "arrowup", -1, &glyph));
  total = hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_BTT, 0,
					 NULL, NULL, &expected_italics_correction);
  g_assert_cmpuint (total, >, 0);
  g_assert_cmpint (expected_italics_correction, !=, 0);
  count = SENTINEL;
  italics_correction = 0;
  g_assert_cmpuint (hb_ot_math_get_glyph_assembly (font, glyph, HB_DIRECTION_BTT, 0,
						   &count, NULL,
						   &italics_correction), ==, total);
  g_assert_cmpuint (count, ==, 0);
  /* The other output is still filled in. */
  g_assert_cmpint (italics_correction, ==, expected_italics_correction);

  count = SENTINEL;
  g_assert_cmpuint (hb_ot_math_get_glyph_assembly (hb_font_get_empty (), glyph, HB_DIRECTION_BTT, 0,
						   &count, NULL, NULL), ==, 0);
  g_assert_cmpuint (count, ==, 0);

  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_paged_getters_meta_entry_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/meta.ttf");

  TEST_POPULATED (hb_ot_meta_get_entry_tags, face, 0);
  TEST_EMPTY (hb_ot_meta_get_entry_tags, hb_face_get_empty (), 0);

  hb_face_destroy (face);
}

/* The count is the size of the text buffer, and the return value the full
 * length of the name. */
static void
test_paged_getters_name_utf8 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cv01.otf");
  hb_language_t language = hb_language_from_string ("en", -1);

  TEST_POPULATED (hb_ot_name_get_utf8, face, HB_OT_NAME_ID_FONT_FAMILY, language);
  TEST_EMPTY (hb_ot_name_get_utf8, hb_face_get_empty (), HB_OT_NAME_ID_FONT_FAMILY, language);

  hb_face_destroy (face);
}

static void
test_paged_getters_name_utf16 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cv01.otf");
  hb_language_t language = hb_language_from_string ("en", -1);

  TEST_POPULATED (hb_ot_name_get_utf16, face, HB_OT_NAME_ID_FONT_FAMILY, language);
  TEST_EMPTY (hb_ot_name_get_utf16, hb_face_get_empty (), HB_OT_NAME_ID_FONT_FAMILY, language);

  hb_face_destroy (face);
}

static void
test_paged_getters_name_utf32 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cv01.otf");
  hb_language_t language = hb_language_from_string ("en", -1);

  TEST_POPULATED (hb_ot_name_get_utf32, face, HB_OT_NAME_ID_FONT_FAMILY, language);
  TEST_EMPTY (hb_ot_name_get_utf32, hb_face_get_empty (), HB_OT_NAME_ID_FONT_FAMILY, language);

  hb_face_destroy (face);
}

static void
test_paged_getters_shape_plan_feature_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_segment_properties_t props = HB_SEGMENT_PROPERTIES_DEFAULT;
  hb_shape_plan_t *shape_plan;

  props.script = HB_SCRIPT_ARABIC;
  props.direction = HB_DIRECTION_RTL;
  shape_plan = hb_shape_plan_create (face, &props, NULL, 0, NULL);

  TEST_POPULATED (hb_ot_shape_plan_get_feature_tags, shape_plan, 0);
  TEST_EMPTY (hb_ot_shape_plan_get_feature_tags, hb_shape_plan_get_empty (), 0);

  hb_shape_plan_destroy (shape_plan);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

#ifndef HB_NO_AAT
  hb_test_add (test_paged_getters_aat_feature_types);
  hb_test_add (test_paged_getters_aat_selector_infos);
#endif
  hb_test_add (test_paged_getters_face_table_tags);
  hb_test_add (test_paged_getters_face_builder_table_tags);
  hb_test_add (test_paged_getters_face_callback_table_tags);
  hb_test_add (test_paged_getters_color_palette_colors);
  hb_test_add (test_paged_getters_color_glyph_layers);
  hb_test_add (test_paged_getters_color_line_color_stops);
  hb_test_add (test_paged_getters_color_line_callback_color_stops);
#ifndef HB_DISABLE_DEPRECATED
  hb_test_add (test_paged_getters_var_axes);
#endif
  hb_test_add (test_paged_getters_var_axis_infos);
  hb_test_add (test_paged_getters_var_named_instance_design_coords);
  hb_test_add (test_paged_getters_tags_from_script_and_language);
  hb_test_add (test_paged_getters_layout_attach_points);
  hb_test_add (test_paged_getters_layout_ligature_carets);
  hb_test_add (test_paged_getters_layout_script_tags);
  hb_test_add (test_paged_getters_layout_feature_tags);
  hb_test_add (test_paged_getters_layout_language_tags);
  hb_test_add (test_paged_getters_layout_language_feature_indexes);
  hb_test_add (test_paged_getters_layout_language_feature_tags);
  hb_test_add (test_paged_getters_layout_feature_lookups);
  hb_test_add (test_paged_getters_layout_feature_with_variations_lookups);
  hb_test_add (test_paged_getters_layout_glyph_alternates);
  hb_test_add (test_paged_getters_layout_feature_characters);
  hb_test_add (test_paged_getters_layout_extender_glyphs);
  hb_test_add (test_paged_getters_math_glyph_kernings);
  hb_test_add (test_paged_getters_math_glyph_variants);
  hb_test_add (test_paged_getters_math_glyph_assembly);
  hb_test_add (test_paged_getters_meta_entry_tags);
  hb_test_add (test_paged_getters_name_utf8);
  hb_test_add (test_paged_getters_name_utf16);
  hb_test_add (test_paged_getters_name_utf32);
  hb_test_add (test_paged_getters_shape_plan_feature_tags);

  return hb_test_run ();
}
