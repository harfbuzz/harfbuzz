/*
 * Copyright © 2019 Adobe Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */

#include "hb-test.h"
#include <hb-ot.h>

/* Unit tests for glyph advance widths and extents of TrueType variable fonts */

static void
test_extents_tt_var (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansVariable-Roman-nohvar-41,C1.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  hb_glyph_extents_t  extents;
  hb_bool_t result = hb_font_get_glyph_extents (font, 2, &extents);
  g_assert_true (result);

  g_assert_cmpint (extents.x_bearing, ==, 10);
  g_assert_cmpint (extents.y_bearing, ==, 846);
  g_assert_cmpint (extents.width, ==, 500);
  g_assert_cmpint (extents.height, ==, -846);

  float coords[1] = { 500.0f };
  hb_font_set_var_coords_design (font, coords, 1);
  result = hb_font_get_glyph_extents (font, 2, &extents);
  g_assert_true (result);

  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 874);
  g_assert_cmpint (extents.width, ==, 551);
  g_assert_cmpint (extents.height, ==, -874);

  hb_font_destroy (font);
}

static void
test_advance_tt_var_nohvar (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansVariable-Roman-nohvar-41,C1.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  hb_position_t x, y;
  hb_font_get_glyph_advance_for_direction(font, 2, HB_DIRECTION_LTR, &x, &y);

  g_assert_cmpint (x, ==, 520);
  g_assert_cmpint (y, ==, 0);

  hb_font_get_glyph_advance_for_direction(font, 2, HB_DIRECTION_TTB, &x, &y);

  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, -1257);

  float coords[1] = { 500.0f };
  hb_font_set_var_coords_design (font, coords, 1);
  hb_font_get_glyph_advance_for_direction(font, 2, HB_DIRECTION_LTR, &x, &y);

  g_assert_cmpint (x, ==, 551);
  g_assert_cmpint (y, ==, 0);

  hb_font_get_glyph_advance_for_direction(font, 2, HB_DIRECTION_TTB, &x, &y);

  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, -1257);

  hb_font_destroy (font);
}

static void
test_advance_tt_var_hvarvvar (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSerifVariable-Roman-VVAR.abc.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  hb_position_t x, y;
  hb_font_get_glyph_advance_for_direction(font, 1, HB_DIRECTION_LTR, &x, &y);

  g_assert_cmpint (x, ==, 508);
  g_assert_cmpint (y, ==, 0);

  hb_font_get_glyph_advance_for_direction(font, 1, HB_DIRECTION_TTB, &x, &y);

  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, -1000);

  float coords[1] = { 700.0f };
  hb_font_set_var_coords_design (font, coords, 1);
  hb_font_get_glyph_advance_for_direction(font, 1, HB_DIRECTION_LTR, &x, &y);

  g_assert_cmpint (x, ==, 531);
  g_assert_cmpint (y, ==, 0);

  hb_font_get_glyph_advance_for_direction(font, 1, HB_DIRECTION_TTB, &x, &y);

  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, -1012);

  hb_font_destroy (font);
}

static void
test_advance_tt_var_anchor (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.anchor.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  hb_glyph_extents_t  extents;
  hb_bool_t result = hb_font_get_glyph_extents (font, 2, &extents);
  g_assert_true (result);

  g_assert_cmpint (extents.x_bearing, ==, 56);
  g_assert_cmpint (extents.y_bearing, ==, 672);
  g_assert_cmpint (extents.width, ==, 556);
  g_assert_cmpint (extents.height, ==, -684);

  float coords[1] = { 500.0f };
  hb_font_set_var_coords_design (font, coords, 1);
  result = hb_font_get_glyph_extents (font, 2, &extents);
  g_assert_true (result);

  g_assert_cmpint (extents.x_bearing, ==, 50);
  g_assert_cmpint (extents.y_bearing, ==, 667);
  g_assert_cmpint (extents.width, ==, 592);
  g_assert_cmpint (extents.height, ==, -679);

  hb_font_destroy (font);
}

static void
test_extents_tt_var_comp (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.modcomp.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  hb_glyph_extents_t  extents;
  float coords[1] = { 800.0f };
  hb_font_set_var_coords_design (font, coords, 1);

  hb_bool_t result;
  result = hb_font_get_glyph_extents (font, 2, &extents);	/* Ccedilla, cedilla y-scaled by 0.8, with unscaled component offset */
  g_assert_true (result);

  g_assert_cmpint (extents.x_bearing, ==, 19);
  g_assert_cmpint (extents.y_bearing, ==, 663);
  g_assert_cmpint (extents.width, ==, 519);
  g_assert_cmpint (extents.height, ==, -895);

  result = hb_font_get_glyph_extents (font, 3, &extents);	/* Cacute, acute y-scaled by 0.8, with unscaled component offset (default) */
  g_assert_true (result);

  g_assert_cmpint (extents.x_bearing, ==, 19);
  g_assert_cmpint (extents.y_bearing, ==, 909);
  g_assert_cmpint (extents.width, ==, 519);
  g_assert_cmpint (extents.height, ==, -921);

  result = hb_font_get_glyph_extents (font, 4, &extents);	/* Ccaron, caron y-scaled by 0.8, with scaled component offset */
  g_assert_true (result);

  g_assert_cmpint (extents.x_bearing, ==, 19);
  g_assert_cmpint (extents.y_bearing, ==, 866);
  g_assert_cmpint (extents.width, ==, 519);
  g_assert_cmpint (extents.height, ==, -878);

  hb_font_destroy (font);
}

static void
test_advance_tt_var_comp_v (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.modcomp.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  float coords[1] = { 800.0f };
  hb_font_set_var_coords_design (font, coords, 1);

  hb_position_t x, y;
  hb_font_get_glyph_advance_for_direction(font, 2, HB_DIRECTION_TTB, &x, &y);	/* No VVAR; 'C' in composite Ccedilla determines metrics */

  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, -991);

  hb_font_get_glyph_origin_for_direction(font, 2, HB_DIRECTION_TTB, &x, &y);

  g_assert_cmpint (x, ==, 292);
  g_assert_cmpint (y, ==, 1013);

  hb_font_destroy (font);
}

static void
test_advance_tt_var_gvar_infer (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/TestGVAREight.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  int coords[6] = {100};
  hb_font_set_var_coords_normalized (font, coords, 6);

  hb_glyph_extents_t extents = {0};
  g_assert_true (hb_font_get_glyph_extents (font, 4, &extents));

  hb_font_destroy (font);
}

static void
test_advance_tt_var_hvar_long_deltas (void)
{
  /* hvar-long-deltas.ttf has an HVAR whose item-variation store uses
   * long-word (int32) advance deltas across three regions that all peak at
   * the axis maximum.  At that location the summed advance delta is about
   * 6e9, so advance + roundf (delta) exceeds the unsigned range; converting
   * that float to unsigned in get_advance_with_var_unscaled was undefined
   * behavior.  The value is now saturated instead. */
  hb_face_t *face = hb_test_open_font_file ("fonts/hvar-long-deltas.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  /* At the default location the delta is zero. */
  g_assert_cmpint (hb_font_get_glyph_h_advance (font, 1), ==, 500);

  /* At the axis extreme the summed delta overflows; the result is clamped. */
  float coords[1] = { 1.0f };
  hb_font_set_var_coords_design (font, coords, 1);
  g_assert_cmpint (hb_font_get_glyph_h_advance (font, 1), ==, -1);

  hb_font_destroy (font);
}

static void
test_advance_tt_var_comp_use_my_metrics (void)
{
  /* gid 0 is a composite of four USE_MY_METRICS copies of gid 1, a simple
   * glyph declaring 65536 points (and no coordinate data; only the point
   * count is read when fetching an advance).  Without HVAR the advance at a
   * non-default location comes from the phantom points alone, so the
   * components' unloaded contour points must not be transformed or kept
   * around, let alone add up against HB_GLYF_MAX_POINTS. */
  const char glyf_data[] =
  {
    /* gid 0: composite */
    (char) 0xFF, (char) 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, /* numberOfContours = -1, bbox */
    0x02, 0x22, 0, 1, 0, 0, /* ARGS_ARE_XY_VALUES | MORE_COMPONENTS | USE_MY_METRICS, gid 1 */
    0x02, 0x22, 0, 1, 0, 0,
    0x02, 0x22, 0, 1, 0, 0,
    0x02, 0x02, 0, 1, 0, 0,
    /* gid 1: simple */
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, /* numberOfContours = 1, bbox */
    (char) 0xFF, (char) 0xFF, /* endPtsOfContours[0] = 65535 */
    0, 0, /* instructionLength */
  };
  const char loca_data[] = {0, 0, 0, 17, 0, 24}; /* short offsets: 0, 34, 48 */
  char hhea_data[36] = {0, 1};
  hhea_data[35] = 2; /* numberOfHMetrics */
  const char hmtx_data[] = {0x02, 0x58, 0, 0, 0x02, 0x58, 0, 0}; /* advance 600 */
  const char fvar_data[] =
  {
    0, 1, 0, 0, 0, 16, 0, 2, 0, 1, 0, 20, 0, 0, 0, 8,
    'w', 'g', 'h', 't', 0, 100, 0, 0, 0x01, (char) 0x90, 0, 0, 0x03, (char) 0x84, 0, 0, 0, 0, 0x01, 0,
  };
  const char gvar_data[] =
  {
    0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 26, 0, 2, 0, 0, 0, 0, 0, 26,
    0, 0, 0, 0, 0, 0, /* no variation data for either glyph */
  };

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "glyf", glyf_data);
  HB_FACE_ADD_TABLE (face, "loca", loca_data);
  HB_FACE_ADD_TABLE (face, "hhea", hhea_data);
  HB_FACE_ADD_TABLE (face, "hmtx", hmtx_data);
  HB_FACE_ADD_TABLE (face, "fvar", fvar_data);
  HB_FACE_ADD_TABLE (face, "gvar", gvar_data);
  hb_face_set_glyph_count (face, 2);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);

  g_assert_cmpint (hb_font_get_glyph_h_advance (font, 0), ==, 600);

  float coords[1] = { 500.0f };
  hb_font_set_var_coords_design (font, coords, 1);
  g_assert_cmpint (hb_font_get_glyph_h_advance (font, 0), ==, 600);

  hb_font_destroy (font);
}

static void
test_advance_tt_var_comp_use_my_metrics_last (void)
{
  /* a80ae54f...ttf has composites with several USE_MY_METRICS components;
   * each one overwrites the phantoms of the previous one, so HarfBuzz takes
   * the advance of the last such component.  Regression for
   * ANT-2026-DQGMHCLX, pinned to HarfBuzz's own glyph metrics: this cannot
   * be a shape .tests line because that suite is also run under
   * --font-funcs=fontations, which resolves these advances differently. */
  hb_face_t *face = hb_test_open_font_file ("fonts/a80ae54f8ad6577653d6bd370d6a81b37bdd7e79.ttf");
  g_assert_true (face);
  hb_font_t *font = hb_font_create (face);
  hb_face_destroy (face);
  g_assert_true (font);

  /* Default instance; upem is 1000 so advances are in design units. */
  static const int def_adv[6] = {700, 300, 500, 500, 500, 800};
  for (unsigned gid = 1; gid <= 6; gid++)
    g_assert_cmpint (hb_font_get_glyph_h_advance (font, gid), ==, def_adv[gid - 1]);

  /* wght=900: the last USE_MY_METRICS component wins, and a component that
   * is not descended into must not strip an earlier sibling's points (glyph
   * 5's advance was 0 before the fix). */
  hb_variation_t var = { HB_TAG ('w','g','h','t'), 900.0f };
  hb_font_set_variations (font, &var, 1);
  static const int adv[6] = {700, 300, 300, 700, 700, 800};
  for (unsigned gid = 1; gid <= 6; gid++)
    g_assert_cmpint (hb_font_get_glyph_h_advance (font, gid), ==, adv[gid - 1]);

  hb_font_destroy (font);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_extents_tt_var);
  hb_test_add (test_advance_tt_var_nohvar);
  hb_test_add (test_advance_tt_var_hvarvvar);
  hb_test_add (test_advance_tt_var_anchor);
  hb_test_add (test_extents_tt_var_comp);
  hb_test_add (test_advance_tt_var_comp_v);
  hb_test_add (test_advance_tt_var_gvar_infer);
  hb_test_add (test_advance_tt_var_hvar_long_deltas);
  hb_test_add (test_advance_tt_var_comp_use_my_metrics);
  hb_test_add (test_advance_tt_var_comp_use_my_metrics_last);

  return hb_test_run ();
}
