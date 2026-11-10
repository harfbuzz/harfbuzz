/*
 * Copyright © 2026  Behdad Esfahbod
 *
 * This is part of HarfBuzz, a text shaping library.
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
#include "hb-subset-test.h"

#include <hb-ot.h>

#ifndef HB_NO_BEYOND_64K

static hb_face_t *
create_face (const char *tag, const char *table_data, unsigned table_length)
{
  static const char MAXP_data[] = "\x00\x00\x50\x00\x01\x00\x02";
  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "MAXP", MAXP_data);

  hb_blob_t *blob = hb_blob_create_or_fail (table_data, table_length,
					     HB_MEMORY_MODE_READONLY,
					     NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_TAG_CHAR4 (tag), blob));
  hb_blob_destroy (blob);
  return face;
}

static hb_face_t *
create_colr_face (void)
{
  static const char COLR_data[] = {
    0, 1,				/* version */
    0, 0,				/* numBaseGlyphs */
    0, 0, 0, 0,			/* baseGlyphsZ */
    0, 0, 0, 0,			/* layersZ */
    0, 0,				/* numLayers */
    0, 0, 0, 34,			/* baseGlyphList */
    0, 0, 0, 0,			/* layerList */
    0, 0, 0, 0,			/* clipList */
    0, 0, 0, 0,			/* varIdxMap */
    0, 0, 0, 0,			/* varStore */
    0, 0, 0, 1,			/* BaseGlyphList count */
    0, 0, 0, 0, 0, 10,		/* glyph 0, paint offset */
    33, 0, 0, 7, 1, 0, 1,		/* PaintGlyph2, glyph 0x10001 */
    2, 0xFF, 0xFF, 0x40, 0,		/* PaintSolid foreground */
  };
  return create_face ("COLR", COLR_data, sizeof (COLR_data));
}

typedef struct {
  hb_codepoint_t clipped_glyph;
  unsigned color_count;
} paint_result_t;

static void
push_clip_glyph (hb_paint_funcs_t *funcs HB_UNUSED,
		 void *paint_data,
		 hb_codepoint_t glyph,
		 hb_font_t *font HB_UNUSED,
		 void *user_data HB_UNUSED)
{
  paint_result_t *result = paint_data;
  result->clipped_glyph = glyph;
}

static void
paint_color (hb_paint_funcs_t *funcs HB_UNUSED,
	     void *paint_data,
	     hb_bool_t is_foreground HB_UNUSED,
	     hb_color_t color HB_UNUSED,
	     void *user_data HB_UNUSED)
{
  paint_result_t *result = paint_data;
  result->color_count++;
}

static void
check_colr_paint (hb_face_t *face, hb_codepoint_t expected_glyph)
{
  hb_font_t *font = hb_font_create (face);
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  hb_paint_funcs_set_push_clip_glyph_func (funcs, push_clip_glyph, NULL, NULL);
  hb_paint_funcs_set_color_func (funcs, paint_color, NULL, NULL);

  paint_result_t result = {HB_CODEPOINT_INVALID, 0};
  hb_font_paint_glyph (font, 0, funcs, &result, 0, HB_COLOR (0, 0, 0, 255));
  g_assert_cmpuint (result.clipped_glyph, ==, expected_glyph);
  g_assert_cmpuint (result.color_count, ==, 1);

  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
}

static void
test_colr_paint_glyph2 (void)
{
  hb_face_t *face = create_colr_face ();
  check_colr_paint (face, 0x10001);

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add (glyphs, 0);
  hb_face_t *subset = hb_subset_test_create_subset (
      face, hb_subset_test_create_input_from_glyphs (glyphs));
  hb_set_destroy (glyphs);

  check_colr_paint (subset, 1);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static hb_face_t *
create_vorg_face (void)
{
  static const char VORG_data[] = {
    0, 2, 0, 0,			/* version 2.0 */
    0, 123,				/* defaultVertOriginY */
    0, 0, 1,				/* numVertOriginYMetrics */
    1, 0, 1, 1, 65,			/* glyph 0x10001, origin 321 */
  };
  return create_face ("VORG", VORG_data, sizeof (VORG_data));
}

static void
test_vorg_versions (void)
{
  static const char VORG1_data[] = {
    0, 1, 0, 0,			/* version 1.0 */
    0, 123,				/* defaultVertOriginY */
    0, 1,				/* numVertOriginYMetrics */
    0, 1, 1, 65,			/* glyph 1, origin 321 */
  };
  hb_face_t *face = create_face ("VORG", VORG1_data, sizeof (VORG1_data));
  hb_font_t *font = hb_font_create (face);
  hb_position_t x, y;
  g_assert_true (hb_font_get_glyph_v_origin (font, 1, &x, &y));
  g_assert_cmpint (y, ==, 321);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = create_vorg_face ();
  font = hb_font_create (face);
  g_assert_true (hb_font_get_glyph_v_origin (font, 0x10001, &x, &y));
  g_assert_cmpint (y, ==, 321);
  g_assert_true (hb_font_get_glyph_v_origin (font, 1, &x, &y));
  g_assert_cmpint (y, ==, 123);
  hb_font_destroy (font);

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add (glyphs, 0x10001);
  hb_face_t *subset = hb_subset_test_create_subset (
      face, hb_subset_test_create_input_from_glyphs (glyphs));
  hb_set_destroy (glyphs);

  font = hb_font_create (subset);
  g_assert_true (hb_font_get_glyph_v_origin (font, 1, &x, &y));
  g_assert_cmpint (y, ==, 321);
  hb_font_destroy (font);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

#endif

static hb_face_t *
create_gsub_v1_2_face (void)
{
  /* A version 1.2 GSUB that keeps its lists behind the 16-bit offsets
   * and has a FeatureVariations table. */
  static const char GSUB_data[] = {
    0, 1, 0, 2,				/* version 1.2 */
    0, 26,				/* scriptList */
    0, 46,				/* featureList */
    0, 62,				/* lookupList */
    0, 0, 0, 108,			/* featureVars */
    0, 0, 0, 0,				/* scriptList2 */
    0, 0, 0, 0,				/* featureList2 */
    0, 0, 0, 0,				/* lookupList2 */
    /* ScriptList */
    0, 1, 'D', 'F', 'L', 'T', 0, 8,	/* one script at 8 */
    0, 4, 0, 0,				/* defaultLangSys at 4, no LangSys records */
    0, 0, 0xFF, 0xFF, 0, 1, 0, 0,	/* no required feature, feature 0 */
    /* FeatureList */
    0, 1, 'c', 'c', 'm', 'p', 0, 8,	/* one feature at 8 */
    0, 0, 0, 2, 0, 0, 0, 1,		/* lookups 0 and 1 */
    /* LookupList */
    0, 2, 0, 6, 0, 26,			/* two 16-bit lookup offsets */
    0, 1, 0, 0, 0, 1, 0, 8,		/* SingleSubst, one subtable at 8 */
    0, 1, 0, 6, 0, 1,			/* format 1, coverage at 6, delta +1 */
    0, 1, 0, 1, 0, 1,			/* coverage: glyph 1 */
    0, 1, 0, 0, 0, 1, 0, 8,		/* SingleSubst, one subtable at 8 */
    0, 1, 0, 6, 0, 2,			/* format 1, coverage at 6, delta +2 */
    0, 1, 0, 1, 0, 2,			/* coverage: glyph 2 */
    /* FeatureVariations */
    0, 1, 0, 0,				/* version 1.0 */
    0, 0, 0, 1,				/* one record */
    0, 0, 0, 16, 0, 0, 0, 30,		/* conditionSet at 16, substitutions at 30 */
    0, 1, 0, 0, 0, 6,			/* one condition at 6 */
    0, 1, 0, 0, 0x20, 0, 0x40, 0,	/* axis 0 in [0.5, 1.0] */
    0, 1, 0, 0, 0, 1,			/* one substitution record */
    0, 0, 0, 0, 0, 12,			/* feature 0, alternate Feature at 12 */
    0, 0, 0, 1, 0, 0,			/* lookup 0 only */
  };
  static const char FVAR_data[] = {
    0, 1, 0, 0,				/* version 1.0 */
    0, 16, 0, 2, 0, 1, 0, 20,		/* one axis at 16, axisSize 20 */
    0, 0, 0, 8,				/* no instances */
    'w', 'g', 'h', 't',
    0, 100, 0, 0,			/* minValue 100 */
    1, 0x90, 0, 0,			/* defaultValue 400 */
    3, 0x84, 0, 0,			/* maxValue 900 */
    0, 0, 1, 0,				/* flags, axisNameID */
  };
  static const char MAXP_data[] = {
    0, 0, 0x50, 0,			/* version 0.5 */
    0, 5,				/* numGlyphs */
  };
  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "GSUB", GSUB_data);
  HB_FACE_ADD_TABLE (face, "fvar", FVAR_data);
  HB_FACE_ADD_TABLE (face, "maxp", MAXP_data);
  return face;
}

static void
check_gsub_v1_2_lookup (hb_face_t *face, unsigned lookup_index,
			hb_codepoint_t glyph, hb_codepoint_t expected)
{
  unsigned count = 1;
  hb_codepoint_t alternate = HB_CODEPOINT_INVALID;
  g_assert_cmpuint (hb_ot_layout_lookup_get_glyph_alternates (face, lookup_index, glyph,
							      0, &count, &alternate),
		    ==, 1);
  g_assert_cmpuint (alternate, ==, expected);
}

static void
check_gsub_v1_2 (hb_face_t *face, hb_codepoint_t last_glyph, hb_bool_t has_feature_variations)
{
  int coords[] = {0x4000};		/* wght 900 */
  unsigned index;
  g_assert_cmpuint (hb_ot_layout_table_get_lookup_count (face, HB_OT_TAG_GSUB), ==, 2);
  check_gsub_v1_2_lookup (face, 0, 1, 2);
  check_gsub_v1_2_lookup (face, 1, 2, last_glyph);
  g_assert_cmpint (hb_ot_layout_table_find_feature_variations (face, HB_OT_TAG_GSUB,
								coords, 1, &index),
		   ==, has_feature_variations);
}

static void
test_gsub_v1_2_subset (void)
{
  hb_face_t *face = create_gsub_v1_2_face ();
  check_gsub_v1_2 (face, 4, TRUE);

  /* Glyph 1 closes over 2 and 4; 4 is renumbered to 3. */
  hb_set_t *glyphs = hb_set_create ();
  hb_set_add (glyphs, 1);
  hb_face_t *subset = hb_subset_test_create_subset (
      face, hb_subset_test_create_input_from_glyphs (glyphs));
  check_gsub_v1_2 (subset, 3, TRUE);
  hb_face_destroy (subset);

  /* Pinning the only axis drops the FeatureVariations. */
  hb_subset_input_t *input = hb_subset_test_create_input_from_glyphs (glyphs);
  g_assert_true (hb_subset_input_pin_axis_to_default (input, face, HB_TAG ('w','g','h','t')));
  subset = hb_subset_test_create_subset (face, input);
  check_gsub_v1_2 (subset, 3, FALSE);
  hb_face_destroy (subset);

  hb_set_destroy (glyphs);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_colr_paint_glyph2);
  hb_test_add (test_vorg_versions);
#endif
  hb_test_add (test_gsub_v1_2_subset);

  return hb_test_run ();
}
