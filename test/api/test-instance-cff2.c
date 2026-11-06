/*
 * Copyright © 2022  Google, Inc.
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
 * Google Author(s): Garret Rieger
 */

#include "hb-test.h"
#include "hb-subset-test.h"


static void
test_instance_cff2 (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *expected = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.static.otf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);

  hb_subset_input_t* input = hb_subset_test_create_input (codepoints);
  hb_subset_input_pin_axis_location (input,
                                     face_abc,
                                     HB_TAG ('w', 'g', 'h', 't'),
                                     250);
  hb_subset_input_pin_axis_location (input,
                                     face_abc,
                                     HB_TAG ('C', 'N', 'T', 'R'),
                                     0);

  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (expected, face_abc_subset, HB_TAG ('C','F','F', '2'));
  g_assert_true (hb_face_reference_table (face_abc_subset, HB_TAG ('f', 'v', 'a', 'r'))
		 == hb_blob_get_empty ());

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (expected);
}


static void
test_instance_cff2_private_blend_overflow (void)
{
  /* StdVW blend delta of 1e12; partial instancing must not convert the
   * transformed delta to int out of range. */
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.huge-blend.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 97);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  g_assert_true (hb_subset_input_set_axis_range (input, face,
						 HB_TAG ('w', 'g', 'h', 't'),
						 300, 600, 300));

  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_nonnull (subset);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_instance_cff2_vardata_budget (void)
{
  /* 40 VarData offsets aliasing one 800-region VarData no blend uses:
   * partial instancing must not build a k×k region transform for each
   * of them.  Over budget, create() fails, so the partial instance fails
   * safely like any other CFF2 planning failure instead of exhausting
   * memory. */
  hb_face_t *face = hb_test_open_font_file ("fonts/CFF2.vardata-budget.otf");

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add_range (glyphs, 0, 1);

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_union (hb_subset_input_glyph_set (input), glyphs);
  hb_set_destroy (glyphs);
  g_assert_true (hb_subset_input_set_axis_range (input, face,
						 HB_TAG ('w', 'g', 'h', 't'),
						 200, 600, 400));

  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_null (subset);

  hb_face_destroy (face);
}


static void
test_instance_cff2_empty_bounds (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2-empty.otf");
  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 32);
  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  g_assert_true (hb_subset_input_pin_axis_location (input, face,
                                                   HB_TAG ('w', 'g', 'h', 't'), 0.5f));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_nonnull (subset);

  hb_blob_t *head = hb_face_reference_table (subset, HB_TAG ('h', 'e', 'a', 'd'));
  unsigned length;
  const char *data = hb_blob_get_data (head, &length);
  const char zero_bounds[8] = {0};
  g_assert_cmpuint (length, >=, 44);
  g_assert_cmpmem (data + 36, 8, zero_bounds, 8);
  hb_blob_destroy (head);

  /* Empty outlines must still retain their advance and original bearing. */
  hb_blob_t *hmtx = hb_face_reference_table (subset, HB_TAG ('h', 'm', 't', 'x'));
  data = hb_blob_get_data (hmtx, &length);
  const char metric[4] = {1, (char) 0xF4, 0, 50};
  g_assert_cmpuint (length, >=, 4);
  g_assert_cmpmem (data, 4, metric, 4);
  hb_blob_destroy (hmtx);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}


static void
test_instance_cff2_large_store (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2-large-store.otf");
  hb_set_t *codepoints = hb_set_create ();
  hb_set_add_range (codepoints, 65, 264);
  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  g_assert_true (hb_subset_input_set_axis_range (input, face,
                                                HB_TAG ('w', 'g', 'h', 't'),
                                                0, 1, 0.1f));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_nonnull (subset);

  /* All 200 regions are used, so compaction cannot hide the large store. */
  hb_blob_t *cff2 = hb_face_reference_table (subset, HB_TAG ('C', 'F', 'F', '2'));
  g_assert_cmpuint (hb_blob_get_length (cff2), >, 65535);
  hb_blob_destroy (cff2);
  hb_font_t *font = hb_font_create (subset);
  hb_glyph_extents_t extents;
  hb_codepoint_t glyph;
  g_assert_true (hb_font_get_nominal_glyph (font, 65, &glyph));
  for (unsigned i = 0; i < 3; i++)
  {
    hb_variation_t variation = {HB_TAG ('w', 'g', 'h', 't'), i * 0.5f};
    hb_font_set_variations (font, &variation, 1);
    g_assert_true (hb_font_get_glyph_extents (font, glyph, &extents));
    g_assert_cmpint (extents.width, >, 0);
  }
  hb_font_destroy (font);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}


typedef struct {
  char op;
  unsigned count;
  float points[6];
} rounding_command_t;

static void
rounding_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED, void *data,
		 hb_draw_state_t *st HB_UNUSED, float x, float y,
		 void *user_data HB_UNUSED)
{
  rounding_command_t command = {'M', 2, {x, y}};
  g_array_append_val ((GArray *) data, command);
}

static void
rounding_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED, void *data,
		 hb_draw_state_t *st HB_UNUSED, float x, float y,
		 void *user_data HB_UNUSED)
{
  rounding_command_t command = {'L', 2, {x, y}};
  g_array_append_val ((GArray *) data, command);
}

static void
rounding_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED, void *data,
		  hb_draw_state_t *st HB_UNUSED, float x1, float y1,
		  float x2, float y2, float x, float y,
		  void *user_data HB_UNUSED)
{
  rounding_command_t command = {'C', 6, {x1, y1, x2, y2, x, y}};
  g_array_append_val ((GArray *) data, command);
}

static void
rounding_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED, void *data,
		     hb_draw_state_t *st HB_UNUSED, void *user_data HB_UNUSED)
{
  rounding_command_t command = {'Z', 0, {0}};
  g_array_append_val ((GArray *) data, command);
}

static void
check_rounding (hb_face_t *face, hb_face_t *subset, float weight, float slant,
		float tolerance)
{
  hb_variation_t location[] = {{HB_TAG ('w','g','h','t'), weight},
			     {HB_TAG ('s','l','n','t'), slant}};
  hb_font_t *original = hb_font_create (face);
  hb_font_t *instanced = hb_font_create (subset);
  hb_font_set_variations (original, location, 2);
  hb_font_set_variations (instanced, location, 2);
  hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, rounding_move_to, NULL, NULL);
  hb_draw_funcs_set_line_to_func (funcs, rounding_line_to, NULL, NULL);
  hb_draw_funcs_set_cubic_to_func (funcs, rounding_cubic_to, NULL, NULL);
  hb_draw_funcs_set_close_path_func (funcs, rounding_close_path, NULL, NULL);

  for (hb_codepoint_t glyph = 1; glyph < hb_face_get_glyph_count (face); glyph++)
  {
    char name[64];
    g_assert_true (hb_font_get_glyph_name (original, glyph, name, sizeof (name)));
    GArray *expected = g_array_new (FALSE, FALSE, sizeof (rounding_command_t));
    GArray *actual = g_array_new (FALSE, FALSE, sizeof (rounding_command_t));
    g_assert_true (hb_font_draw_glyph_or_fail (original, glyph, funcs, expected));
    g_assert_true (hb_font_draw_glyph_or_fail (instanced, glyph, funcs, actual));
    g_test_message ("glyph %u, wght %g, slnt %g", glyph, weight, slant);
    if (!strcmp (name, "flex1"))
    {
      /* flex1 deliberately retains relative rounding for the entire glyph.
       * At wght=570 its first dx/dy round to 24/15, after a 101/103 moveto. */
      if (weight == 570)
      {
	rounding_command_t *curve = &g_array_index (actual, rounding_command_t, 1);
	g_assert_cmpint (curve->op, ==, 'C');
	g_assert_cmpfloat (curve->points[0], ==, 125);
	g_assert_cmpfloat (curve->points[1], ==, 118);
      }
      g_array_free (actual, TRUE);
      g_array_free (expected, TRUE);
      continue;
    }
    if (!strcmp (name, "fractional.blended") &&
	hb_ot_var_get_axis_count (subset) == 0)
    {
      rounding_command_t *line1 = &g_array_index (actual, rounding_command_t, 1);
      rounding_command_t *line2 = &g_array_index (actual, rounding_command_t, 2);
      g_assert_cmpint (line1->op, ==, 'L');
      g_assert_cmpfloat (line1->points[0], ==, 111);
      g_assert_cmpint (line2->op, ==, 'L');
      g_assert_cmpfloat (line2->points[1], ==, 121);
    }
    g_assert_cmpuint (expected->len, ==, actual->len);
    for (unsigned i = 0; i < expected->len; i++)
    {
      rounding_command_t *a = &g_array_index (actual, rounding_command_t, i);
      rounding_command_t *e = &g_array_index (expected, rounding_command_t, i);
      g_assert_cmpint (a->op, ==, e->op);
      for (unsigned j = 0; j < e->count; j++)
	g_assert_cmpfloat_with_epsilon (a->points[j], e->points[j], tolerance);
      /* The rings' final cubic endpoint must equal their moveto, without
       * relying on the implicit line supplied by close_path. */
      if ((!strcmp (name, "ring") || !strcmp (name, "hinted")) && a->op == 'Z')
      {
	rounding_command_t *end = &g_array_index (actual, rounding_command_t, i - 1);
	unsigned start = i - 1;
	while (g_array_index (actual, rounding_command_t, start).op != 'M') start--;
	rounding_command_t *move = &g_array_index (actual, rounding_command_t, start);
	g_assert_cmpint (end->op, ==, 'C');
	g_assert_cmpfloat_with_epsilon (end->points[4], move->points[0], .001);
	g_assert_cmpfloat_with_epsilon (end->points[5], move->points[1], .001);
      }
    }
    g_array_free (actual, TRUE);
    g_array_free (expected, TRUE);
  }
  hb_draw_funcs_destroy (funcs);
  hb_font_destroy (original);
  hb_font_destroy (instanced);
}

static void
test_instance_cff2_absolute_rounding (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2-rounding.otf");
  for (unsigned mode = 0; mode < 4; mode++)
    for (unsigned no_hinting = 0; no_hinting < 2; no_hinting++)
    {
      hb_subset_input_t *input = hb_subset_input_create_or_fail ();
      hb_set_add_range (hb_subset_input_glyph_set (input), 0, hb_face_get_glyph_count (face) - 1);
      if (no_hinting) hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
      if (mode == 0 || mode == 3)
      {
	g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','g','h','t'), mode == 0 ? 570 : 300));
	g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('s','l','n','t'), mode == 0 ? -5.5 : 0));
      }
      else
      {
	g_assert_true (hb_subset_input_set_axis_range (input, face, HB_TAG ('w','g','h','t'), 300, 900, 400));
	if (mode == 2)
	  g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('s','l','n','t'), -5.5));
      }
      hb_face_t *subset = hb_subset_or_fail (face, input);
      g_assert_nonnull (subset);
      check_rounding (face, subset, mode == 0 ? 570 : mode == 3 ? 300 : 400,
		      mode == 1 || mode == 3 ? 0 : -5.5, .501);
      if (mode == 1 || mode == 2)
      {
	check_rounding (face, subset, 300, mode == 1 ? 0 : -5.5, 1.51);
	check_rounding (face, subset, 600, mode == 1 ? -11 : -5.5, 2.01);
	check_rounding (face, subset, 900, mode == 1 ? -11 : -5.5, 2.01);
      }
      hb_face_destroy (subset);
      hb_subset_input_destroy (input);
    }
  hb_face_destroy (face);
}



static void
test_instance_cff2_nested_delta (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2-nested-delta.otf");
  for (unsigned mode = 0; mode < 5; mode++)
  {
    g_test_message ("nested delta mode %u", mode);
    hb_set_t *codepoints = hb_set_create ();
    hb_set_add (codepoints, mode == 4 ? 65 : 66);
    hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
    hb_set_destroy (codepoints);
    if (mode == 1 || mode == 2 || mode == 4)
      g_assert_true (hb_subset_input_pin_axis_location (input, face,
                                                       HB_TAG ('w', 'g', 'h', 't'), 0.25f));
    if (mode == 2)
      g_assert_true (hb_subset_input_pin_axis_location (input, face,
                                                       HB_TAG ('w', 'd', 't', 'h'), 1));
    if (mode == 3)
      hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);

    hb_face_t *subset = hb_subset_or_fail (face, input);
    hb_subset_input_destroy (input);
    if (mode == 1 || mode == 3)
    {
      /* The flat symbolic representation cannot retain a variable delta.
       * Fail the operation instead of changing the outline or dropping CFF2. */
      g_assert_null (subset);
      continue;
    }
    g_assert_nonnull (subset);
    hb_blob_t *cff2 = hb_face_reference_table (subset, HB_TAG ('C', 'F', 'F', '2'));
    g_assert_cmpuint (hb_blob_get_length (cff2), >, 0);
    hb_blob_destroy (cff2);
    hb_font_t *font = hb_font_create (subset);
    hb_variation_t variations[] = {
      {HB_TAG ('w', 'g', 'h', 't'), 0.25f},
      {HB_TAG ('w', 'd', 't', 'h'), 1}
    };
    hb_font_set_variations (font, variations, G_N_ELEMENTS (variations));
    hb_codepoint_t glyph;
    hb_glyph_extents_t extents;
    g_assert_true (hb_font_get_nominal_glyph (font, mode == 4 ? 65 : 66, &glyph));
    g_assert_true (hb_font_get_glyph_extents (font, glyph, &extents));
    g_assert_cmpint (extents.width, ==, mode == 4 ? 135 : 115);
    hb_font_destroy (font);
    hb_face_destroy (subset);
  }
  hb_face_destroy (face);
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_instance_cff2);
  hb_test_add (test_instance_cff2_private_blend_overflow);
  hb_test_add (test_instance_cff2_vardata_budget);
  hb_test_add (test_instance_cff2_empty_bounds);
  hb_test_add (test_instance_cff2_large_store);
  hb_test_add (test_instance_cff2_absolute_rounding);
  hb_test_add (test_instance_cff2_nested_delta);

  return hb_test_run();
}
