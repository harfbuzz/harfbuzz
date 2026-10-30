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
#include <hb-subset.h>

/* Differential tests for avar2 partial instancing.
 *
 * A partial instance of an avar2 font must produce the same final
 * normalized coordinates (fvar normalization + avar v1 + avar v2) as the
 * original font at every retained user-space location; the subsetter
 * encodes the difference between the old and the new intermediate
 * coordinate spaces as avar2 deltas ("offset compensation").
 *
 * TestAvar2Instance.ttf axes: wght 100/400/900, wdth 50/100/200,
 * GRAD -200/0/150, XOPQ 10/88/160, opsz 8/14/144. avar v1 has interior
 * breakpoints on wght (at ±0.5), XOPQ and opsz (at -0.45). The avar2
 * VarStore drives wght's final coordinate from wdth, and GRAD's and
 * XOPQ's from wght through a single SHARED delta row (the VarIdxMap maps
 * GRAD and XOPQ to the same varIdx); wdth and opsz map to
 * NO_VARIATION_INDEX. opsz's default lies near its minimum, so a
 * far-moved default makes the retained avar v1 segment steep in the new
 * space -- the regime where offset compensation is quantization-limited.
 */

#define WGHT HB_TAG ('w','g','h','t')
#define WDTH HB_TAG ('w','d','t','h')
#define GRAD HB_TAG ('G','R','A','D')
#define XOPQ HB_TAG ('X','O','P','Q')
#define OPSZ HB_TAG ('o','p','s','z')

/* Compare final normalized coords of two faces at the same user location,
 * in F2Dot14 units. Compared by axis tag: self-contained pinned axes are
 * removed from the instance entirely (their contribution is baked into the
 * variation tables, which check_same_rendering validates). */
static void
check_same_coords (hb_face_t *orig_face, hb_face_t *inst_face,
		   const hb_variation_t *vars, unsigned n_vars,
		   int tolerance)
{
  hb_font_t *orig_font = hb_font_create (orig_face);
  hb_font_t *inst_font = hb_font_create (inst_face);
  hb_font_set_variations (orig_font, vars, n_vars);
  hb_font_set_variations (inst_font, vars, n_vars);

  unsigned orig_len = 0, inst_len = 0;
  const int *orig_coords = hb_font_get_var_coords_normalized (orig_font, &orig_len);
  const int *inst_coords = hb_font_get_var_coords_normalized (inst_font, &inst_len);

  hb_ot_var_axis_info_t orig_axes[8], inst_axes[8];
  unsigned orig_axis_count = 8, inst_axis_count = 8;
  hb_ot_var_get_axis_infos (orig_face, 0, &orig_axis_count, orig_axes);
  hb_ot_var_get_axis_infos (inst_face, 0, &inst_axis_count, inst_axes);

  g_assert_cmpuint (orig_len, ==, 6); /* not vacuous */
  g_assert_cmpuint (orig_len, ==, orig_axis_count);
  g_assert_cmpuint (inst_len, ==, inst_axis_count);

  for (unsigned j = 0; j < inst_axis_count; j++)
  {
    unsigned i = 0;
    for (; i < orig_axis_count; i++)
      if (orig_axes[i].tag == inst_axes[j].tag)
	break;
    g_assert_cmpuint (i, <, orig_axis_count); /* tag must exist in original */
    g_assert_cmpint (ABS (orig_coords[i] - inst_coords[j]), <=, tolerance);
  }

  hb_font_destroy (orig_font);
  hb_font_destroy (inst_font);
}

/* Compare rendering-level outputs (advance width and extents of 'A') at
 * the same user location; validates that culled gvar/HVAR variations were
 * truly unreachable. */
static void
check_same_rendering (hb_face_t *orig_face, hb_face_t *inst_face,
		      const hb_variation_t *vars, unsigned n_vars)
{
  hb_font_t *orig_font = hb_font_create (orig_face);
  hb_font_t *inst_font = hb_font_create (inst_face);
  hb_font_set_variations (orig_font, vars, n_vars);
  hb_font_set_variations (inst_font, vars, n_vars);

  hb_codepoint_t orig_gid = 0, inst_gid = 0;
  g_assert_true (hb_font_get_nominal_glyph (orig_font, 'A', &orig_gid));
  g_assert_true (hb_font_get_nominal_glyph (inst_font, 'A', &inst_gid));

  hb_position_t orig_adv = hb_font_get_glyph_h_advance (orig_font, orig_gid);
  hb_position_t inst_adv = hb_font_get_glyph_h_advance (inst_font, inst_gid);
  g_assert_cmpint (ABS (orig_adv - inst_adv), <=, 1);

  hb_glyph_extents_t orig_ext, inst_ext;
  g_assert_true (hb_font_get_glyph_extents (orig_font, orig_gid, &orig_ext));
  g_assert_true (hb_font_get_glyph_extents (inst_font, inst_gid, &inst_ext));
  g_assert_cmpint (ABS (orig_ext.x_bearing - inst_ext.x_bearing), <=, 1);
  g_assert_cmpint (ABS (orig_ext.y_bearing - inst_ext.y_bearing), <=, 1);
  g_assert_cmpint (ABS (orig_ext.width - inst_ext.width), <=, 1);
  g_assert_cmpint (ABS (orig_ext.height - inst_ext.height), <=, 1);

  /* MVAR-driven metric (underline offset varies with wght). */
  hb_position_t orig_undo = 0, inst_undo = 0;
  g_assert_true (hb_ot_metrics_get_position (orig_font, HB_OT_METRICS_TAG_UNDERLINE_OFFSET, &orig_undo));
  g_assert_true (hb_ot_metrics_get_position (inst_font, HB_OT_METRICS_TAG_UNDERLINE_OFFSET, &inst_undo));
  g_assert_cmpint (ABS (orig_undo - inst_undo), <=, 1);

  hb_font_destroy (orig_font);
  hb_font_destroy (inst_font);
}

static hb_face_t *
open_original (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/TestAvar2Instance.ttf");
  g_assert_cmpuint (hb_ot_var_get_axis_count (face), ==, 6);
  return face;
}

static unsigned
read_uint16_be (const unsigned char *p)
{
  return ((unsigned) p[0] << 8) | p[1];
}

static unsigned
read_uint32_be (const unsigned char *p)
{
  return ((unsigned) p[0] << 24) | ((unsigned) p[1] << 16) |
	 ((unsigned) p[2] << 8) | p[3];
}

/* Replace the test font's avar2 VarIdxMap with the implicit map and empty
 * its non-NULL VarStore. The resulting implicit delta-set indices are out
 * of range and must contribute zero, matching runtime evaluation. */
static hb_blob_t *
reference_table_with_empty_avar2_store (hb_face_t *face HB_UNUSED,
					hb_tag_t tag,
					void *user_data)
{
  hb_blob_t *table = hb_face_reference_table ((hb_face_t *) user_data, tag);
  if (tag != HB_TAG ('a','v','a','r'))
    return table;

  hb_blob_t *copy = hb_blob_copy_writable_or_fail (table);
  hb_blob_destroy (table);
  g_assert_nonnull (copy);

  unsigned length;
  unsigned char *data = (unsigned char *) hb_blob_get_data_writable (copy, &length);
  g_assert_nonnull (data);
  g_assert_cmpuint (length, >=, 8);
  g_assert_cmpuint (read_uint16_be (data), ==, 2); /* major version */

  unsigned offset = 8;
  unsigned axis_count = read_uint16_be (data + 6);
  for (unsigned i = 0; i < axis_count; i++)
  {
    g_assert_cmpuint (offset + 2, <=, length);
    unsigned map_count = read_uint16_be (data + offset);
    g_assert_cmpuint (map_count, <=, (length - offset - 2) / 4);
    offset += 2 + map_count * 4;
  }

  g_assert_cmpuint (offset + 8, <=, length);
  unsigned var_store_offset = read_uint32_be (data + offset + 4);
  g_assert_cmpuint (var_store_offset, <=, length - 8);
  g_assert_cmpuint (read_uint16_be (data + var_store_offset), ==, 1);
  g_assert_cmpuint (read_uint16_be (data + var_store_offset + 6), >, 0);

  data[offset + 0] = data[offset + 1] = 0;
  data[offset + 2] = data[offset + 3] = 0; /* NULL VarIdxMap: implicit map */
  data[var_store_offset + 6] = 0;
  data[var_store_offset + 7] = 0; /* VarDataCount = 0 */
  return copy;
}

/* Point the test font's gvar offsets of glyphs 0 and 2 at glyph 1's
 * variation data (and glyph 1 at nothing), so that the retained glyphs'
 * data add up to more than the table. */
static hb_blob_t *
reference_table_with_aliased_gvar_data (hb_face_t *face HB_UNUSED,
					hb_tag_t tag,
					void *user_data)
{
  hb_blob_t *table = hb_face_reference_table ((hb_face_t *) user_data, tag);
  if (tag != HB_TAG ('g','v','a','r'))
    return table;

  hb_blob_t *copy = hb_blob_copy_writable_or_fail (table);
  hb_blob_destroy (table);
  g_assert_nonnull (copy);

  unsigned length;
  unsigned char *data = (unsigned char *) hb_blob_get_data_writable (copy, &length);
  g_assert_nonnull (data);
  g_assert_cmpuint (length, >=, 28);
  g_assert_cmpuint (read_uint16_be (data + 12), ==, 3); /* glyphCount */
  g_assert_cmpuint (read_uint16_be (data + 14) & 1, ==, 0); /* short offsets */
  g_assert_cmpuint (read_uint16_be (data + 24), >, read_uint16_be (data + 22)); /* glyph 1 has data */

  unsigned char start[2] = {data[22], data[23]};
  unsigned char end[2] = {data[24], data[25]};
  data[20] = start[0]; data[21] = start[1]; /* glyph 0: [start, end) */
  data[22] = end[0];   data[23] = end[1];   /* glyph 1: [end, start): nothing */
  data[24] = start[0]; data[25] = start[1]; /* glyph 2: [start, end) */
  data[26] = end[0];   data[27] = end[1];
  return copy;
}

static unsigned char *
write_uint16_be (unsigned char *p, unsigned v)
{
  *p++ = (v >> 8) & 0xFF;
  *p++ = v & 0xFF;
  return p;
}

/* Replace the test font's avar table with one that gives wght hundreds of
 * avar v1 breakpoints and, through the implicit VarIdxMap, keeps the axes'
 * delta sets in a VarData of 65535 rows but no regions: rows that take no
 * bytes.  Few enough breakpoints that the quadratic estimator budget is not
 * the one that trips. */
static hb_blob_t *
reference_table_with_avar2_empty_rows (hb_face_t *face HB_UNUSED,
				       hb_tag_t tag,
				       void *user_data)
{
  if (tag != HB_TAG ('a','v','a','r'))
    return hb_face_reference_table ((hb_face_t *) user_data, tag);

  const unsigned axis_count = 6, map_count = 513;
  unsigned tail = 8 + 2 * axis_count + 4 * map_count;
  unsigned length = tail + 8 + 12 + 4 + 6;
  unsigned char *data = (unsigned char *) calloc (length, 1);
  g_assert_nonnull (data);

  unsigned char *p = data;
  p = write_uint16_be (p, 2); /* majorVersion; minorVersion, reserved */
  p = write_uint16_be (p + 4, axis_count);
  p = write_uint16_be (p, map_count); /* wght: identity breakpoints */
  for (int v = -16384; v <= 16384; v += 64)
  {
    p = write_uint16_be (p, (unsigned) v);
    p = write_uint16_be (p, (unsigned) v);
  }
  g_assert_true (p + 2 * (axis_count - 1) == data + tail); /* no maps */

  p = write_uint16_be (data + tail + 6, tail + 8); /* NULL VarIdxMap; VarStore */
  p = write_uint16_be (p, 1); /* format */
  p = write_uint16_be (p + 2, 12); /* VarRegionList */
  p = write_uint16_be (p, 1); /* VarDataCount */
  p = write_uint16_be (p + 2, 12 + 4); /* VarData */
  p = write_uint16_be (p, axis_count); /* no regions */
  p = write_uint16_be (p + 2, 65535); /* itemCount; no region indices */
  g_assert_true (p + 4 == data + length);

  return hb_blob_create ((const char *) data, length,
			 HB_MEMORY_MODE_WRITABLE, data, free);
}

static hb_face_t *
instance_n (hb_face_t *face, hb_subset_input_t *input, unsigned expected_axes)
{
  hb_face_t *inst = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  g_assert_nonnull (inst);
  /* All axes are kept (pinned ones as hidden), except self-contained
   * pinned axes, which are removed entirely. */
  g_assert_cmpuint (hb_ot_var_get_axis_count (inst), ==, expected_axes);
  return inst;
}

static hb_face_t *
instance (hb_face_t *face, hb_subset_input_t *input)
{
  return instance_n (face, input, 6);
}

static hb_subset_input_t *
create_input (void)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  g_assert_nonnull (input);
  hb_subset_input_keep_everything (input);
  return input;
}

static float
lerp (float lo, float hi, unsigned i, unsigned n)
{
  return lo + (hi - lo) * i / (float) (n - 1);
}

/* Restrict wght without moving its default: the classic two-tent offset
 * compensation. */
static void
test_avar2_restrict_same_default (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 200.f, 700.f, 400.f));
  hb_face_t *inst = instance (face, input);

  static const float wdths[] = {50.f, 100.f, 150.f, 200.f};
  for (unsigned i = 0; i < 9; i++)
    for (unsigned j = 0; j < G_N_ELEMENTS (wdths); j++)
    {
      hb_variation_t vars[2] = {{WGHT, lerp (200.f, 700.f, i, 9)},
				{WDTH, wdths[j]}};
      check_same_coords (face, inst, vars, 2, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Restrict wght MOVING its default: the offset function kinks where the
 * old default lands in the new space; a plain two-tent encoding gets
 * interior coordinates wrong. */
static void
test_avar2_restrict_moved_default (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 190.f, 900.f, 550.f));
  hb_face_t *inst = instance (face, input);

  /* Include the exact kink locations: the avar v1 breakpoints (user 250
   * and 650) and the old default (user 400, the z_old kink). */
  static const float wdths[] = {50.f, 100.f, 200.f};
  static const float kinks[] = {250.f, 400.f, 650.f};
  for (unsigned i = 0; i < 9 + G_N_ELEMENTS (kinks); i++)
    for (unsigned j = 0; j < G_N_ELEMENTS (wdths); j++)
    {
      float w = i < 9 ? lerp (190.f, 900.f, i, 9) : kinks[i - 9];
      hb_variation_t vars[2] = {{WGHT, w},
				{WDTH, wdths[j]}};
      check_same_coords (face, inst, vars, 2, 2);
      check_same_rendering (face, inst, vars, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Restrict wght moving its default the other way: the old default lands
 * ABOVE the new one, putting the z_old kink on the positive side. */
static void
test_avar2_restrict_moved_default_positive (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 100.f, 500.f, 250.f));
  hb_face_t *inst = instance (face, input);

  static const float wdths[] = {50.f, 100.f, 200.f};
  for (unsigned i = 0; i < 9; i++)
    for (unsigned j = 0; j < G_N_ELEMENTS (wdths); j++)
    {
      hb_variation_t vars[2] = {{WGHT, lerp (100.f, 500.f, i, 9)},
				{WDTH, wdths[j]}};
      check_same_coords (face, inst, vars, 2, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Restrict wdth, which has no avar2 delta row (NO_VARIATION_INDEX): its
 * offset compensation needs a freshly created VarData. Moved default. */
static void
test_avar2_restrict_no_varidx_axis (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WDTH, 60.f, 200.f, 120.f));
  hb_face_t *inst = instance (face, input);

  static const float wghts[] = {100.f, 400.f, 900.f};
  for (unsigned i = 0; i < 9; i++)
    for (unsigned k = 0; k < G_N_ELEMENTS (wghts); k++)
    {
      hb_variation_t vars[2] = {{WDTH, lerp (60.f, 200.f, i, 9)},
				{WGHT, wghts[k]}};
      check_same_coords (face, inst, vars, 2, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Restrict XOPQ (moved default; interior avar v1 breakpoint in range).
 * XOPQ shares its avar2 delta row with GRAD: without privatizing the
 * shared row, XOPQ's offset-compensation deltas corrupt free GRAD. */
static void
test_avar2_restrict_shared_row (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, XOPQ, 20.f, 160.f, 120.f));
  hb_face_t *inst = instance (face, input);

  /* Include the exact kink locations: the avar v1 breakpoint (user 52.9)
   * and the old default (user 88, the z_old kink). */
  static const float grads[] = {-200.f, -50.f, 0.f, 100.f, 150.f};
  static const float wghts[] = {100.f, 400.f, 900.f};
  static const float kinks[] = {52.9f, 88.f};
  for (unsigned i = 0; i < 9 + G_N_ELEMENTS (kinks); i++)
    for (unsigned j = 0; j < G_N_ELEMENTS (grads); j++)
      for (unsigned k = 0; k < G_N_ELEMENTS (wghts); k++)
      {
	float x = i < 9 ? lerp (20.f, 160.f, i, 9) : kinks[i - 9];
	hb_variation_t vars[3] = {{XOPQ, x},
				  {GRAD, grads[j]},
				  {WGHT, wghts[k]}};
	check_same_coords (face, inst, vars, 3, 2);
      }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Pin XOPQ off-default: only its bias is written; free GRAD, sharing the
 * row, must stay uncorrupted. */
static void
test_avar2_pin_shared_row (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_pin_axis_location (input, face, XOPQ, 120.f));
  hb_face_t *inst = instance (face, input);

  static const float grads[] = {-200.f, -50.f, 0.f, 100.f, 150.f};
  static const float wghts[] = {100.f, 400.f, 700.f, 900.f};
  for (unsigned j = 0; j < G_N_ELEMENTS (grads); j++)
    for (unsigned k = 0; k < G_N_ELEMENTS (wghts); k++)
    {
      /* Evaluate the original at the pinned XOPQ value. */
      hb_variation_t vars[3] = {{XOPQ, 120.f},
				{GRAD, grads[j]},
				{WGHT, wghts[k]}};
      check_same_coords (face, inst, vars, 3, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Pin wght off-default: its final coordinate still varies with free wdth
 * through the (rebased) avar2 delta row. */
static void
test_avar2_pin_driven_axis (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_pin_axis_location (input, face, WGHT, 600.f));
  hb_face_t *inst = instance (face, input);

  static const float wdths[] = {50.f, 100.f, 150.f, 200.f};
  static const float grads[] = {0.f, 100.f};
  for (unsigned j = 0; j < G_N_ELEMENTS (wdths); j++)
    for (unsigned k = 0; k < G_N_ELEMENTS (grads); k++)
    {
      hb_variation_t vars[3] = {{WGHT, 600.f},
				{WDTH, wdths[j]},
				{GRAD, grads[k]}};
      check_same_coords (face, inst, vars, 3, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Pin axes AT their old defaults (the most common instancing operation):
 * d_i = 0 and the bias vanishes; the axes just become hidden. XOPQ covers
 * the shared-row path, wdth the NO_VARIATION_INDEX path. */
static void
test_avar2_pin_at_default (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_pin_axis_location (input, face, XOPQ, 88.f));
  g_assert_true (hb_subset_input_pin_axis_location (input, face, WDTH, 100.f));
  /* wdth has no avar2 row: pinning it is self-contained, so it is removed
   * from fvar entirely. XOPQ's row varies with wght, so it stays hidden. */
  hb_face_t *inst = instance_n (face, input, 5);

  static const float grads[] = {-200.f, -50.f, 0.f, 100.f, 150.f};
  static const float wghts[] = {100.f, 250.f, 400.f, 650.f, 900.f};
  for (unsigned j = 0; j < G_N_ELEMENTS (grads); j++)
    for (unsigned k = 0; k < G_N_ELEMENTS (wghts); k++)
    {
      hb_variation_t vars[4] = {{XOPQ, 88.f},
				{WDTH, 100.f},
				{GRAD, grads[j]},
				{WGHT, wghts[k]}};
      check_same_coords (face, inst, vars, 4, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Restrict both axes sharing the delta row: each needs a private copy. */
static void
test_avar2_restrict_both_shared (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, GRAD, -100.f, 100.f, 0.f));
  g_assert_true (hb_subset_input_set_axis_range (input, face, XOPQ, 30.f, 140.f, 88.f));
  hb_face_t *inst = instance (face, input);

  static const float wghts[] = {100.f, 400.f, 900.f};
  for (unsigned i = 0; i < 7; i++)
    for (unsigned j = 0; j < 7; j++)
      for (unsigned k = 0; k < G_N_ELEMENTS (wghts); k++)
      {
	hb_variation_t vars[3] = {{GRAD, lerp (-100.f, 100.f, i, 7)},
				  {XOPQ, lerp (30.f, 140.f, j, 7)},
				  {WGHT, wghts[k]}};
	check_same_coords (face, inst, vars, 3, 2);
      }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Move XOPQ's default near the axis edge: the retained avar v1 segment
 * becomes steep in the new space; interior-breakpoint collection keeps
 * the residual within quantization noise. */
static void
test_avar2_restrict_steep (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, XOPQ, 10.f, 160.f, 140.f));
  hb_face_t *inst = instance (face, input);

  static const float wghts[] = {100.f, 400.f, 900.f};
  static const float kinks[] = {52.9f, 88.f};
  for (unsigned i = 0; i < 17 + G_N_ELEMENTS (kinks); i++)
    for (unsigned k = 0; k < G_N_ELEMENTS (wghts); k++)
    {
      float x = i < 17 ? lerp (10.f, 160.f, i, 17) : kinks[i - 17];
      hb_variation_t vars[2] = {{XOPQ, x},
				{WGHT, wghts[k]}};
      check_same_coords (face, inst, vars, 2, 2);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Pin opsz (NO_VARIATION_INDEX) off its default: self-contained, so the
 * axis is removed from fvar entirely and its constant final coordinate is
 * baked into the variation tables by standard instancing. */
static void
test_avar2_self_contained_pin (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_pin_axis_location (input, face, OPSZ, 40.f));
  hb_face_t *inst = instance_n (face, input, 5);

  static const float grads[] = {-200.f, 0.f, 150.f};
  for (unsigned i = 0; i < 9; i++)
    for (unsigned j = 0; j < G_N_ELEMENTS (grads); j++)
    {
      hb_variation_t vars[3] = {{OPSZ, 40.f},
				{WGHT, lerp (100.f, 900.f, i, 9)},
				{GRAD, grads[j]}};
      check_same_coords (face, inst, vars, 3, 2);
      check_same_rendering (face, inst, vars, 3);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Self-contained pin combined with a range restriction: wdth folds into
 * the tables (including its constant push on wght's final coordinate,
 * re-added as a free-axis bias) while wght keeps offset compensation. */
static void
test_avar2_self_contained_plus_range (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_pin_axis_location (input, face, WDTH, 150.f));
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 250.f, 700.f, 400.f));
  hb_face_t *inst = instance_n (face, input, 5);

  static const float grads[] = {-200.f, 0.f, 150.f};
  for (unsigned i = 0; i < 9; i++)
    for (unsigned j = 0; j < G_N_ELEMENTS (grads); j++)
    {
      hb_variation_t vars[3] = {{WDTH, 150.f},
				{WGHT, lerp (250.f, 700.f, i, 9)},
				{GRAD, grads[j]}};
      check_same_coords (face, inst, vars, 3, 2);
      check_same_rendering (face, inst, vars, 3);
    }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Culling: restricting wght narrows the reachable range of the hidden
 * XTRA axis (driven by wght through avar2), killing the gvar/HVAR
 * variations that peak outside it. Rendering must be unchanged over the
 * retained space, and the instance's variation tables must shrink. */
static void
test_avar2_culling (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 300.f, 500.f, 400.f));
  hb_face_t *inst = instance (face, input);

  static const float grads[] = {-200.f, 0.f, 150.f};
  static const float wdths[] = {50.f, 100.f, 200.f};
  for (unsigned i = 0; i < 9; i++)
    for (unsigned j = 0; j < G_N_ELEMENTS (grads); j++)
      for (unsigned k = 0; k < G_N_ELEMENTS (wdths); k++)
      {
	hb_variation_t vars[3] = {{WGHT, lerp (300.f, 500.f, i, 9)},
				  {GRAD, grads[j]},
				  {WDTH, wdths[k]}};
	check_same_coords (face, inst, vars, 3, 2);
	check_same_rendering (face, inst, vars, 3);
      }

  /* Unreachable gvar/HVAR variations must be gone. */
  hb_blob_t *orig_gvar = hb_face_reference_table (face, HB_TAG ('g','v','a','r'));
  hb_blob_t *inst_gvar = hb_face_reference_table (inst, HB_TAG ('g','v','a','r'));
  g_assert_cmpuint (hb_blob_get_length (inst_gvar), <, hb_blob_get_length (orig_gvar));
  hb_blob_destroy (orig_gvar);
  hb_blob_destroy (inst_gvar);

  hb_blob_t *orig_hvar = hb_face_reference_table (face, HB_TAG ('H','V','A','R'));
  hb_blob_t *inst_hvar = hb_face_reference_table (inst, HB_TAG ('H','V','A','R'));
  g_assert_cmpuint (hb_blob_get_length (inst_hvar), <, hb_blob_get_length (orig_hvar));
  hb_blob_destroy (orig_hvar);
  hb_blob_destroy (inst_hvar);

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

static void
test_avar2_empty_varstore (void)
{
  hb_face_t *source = open_original ();
  hb_face_t *face = hb_face_create_for_tables (reference_table_with_empty_avar2_store,
					       source, NULL);
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 300.f, 500.f, 400.f));
  hb_face_t *inst = instance (face, input);

  for (unsigned i = 0; i < 9; i++)
  {
    hb_variation_t vars[1] = {{WGHT, lerp (300.f, 500.f, i, 9)}};
    check_same_coords (face, inst, vars, 1, 2);
  }

  hb_face_destroy (inst);
  hb_face_destroy (face);
  hb_face_destroy (source);
}

/* Culling parses every retained glyph's variation data; a table whose
 * offsets point many glyphs at the same data must be rejected, not
 * parsed and copied once per glyph. */
static void
test_avar2_culling_aliased_gvar_data (void)
{
  hb_face_t *source = open_original ();
  hb_face_t *face = hb_face_create_for_tables (reference_table_with_aliased_gvar_data,
					       source, NULL);
  hb_subset_input_t *input = create_input ();
  /* Keep glyph 0's data in the pass too: with glyph 2 it exceeds the table. */
  hb_subset_input_set_flags (input, hb_subset_input_get_flags (input) | HB_SUBSET_FLAGS_NOTDEF_OUTLINE);
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 300.f, 500.f, 400.f));
  g_assert_null (hb_subset_or_fail (face, input));
  hb_subset_input_destroy (input);

  hb_face_destroy (face);
  hb_face_destroy (source);
}

/* Move opsz's default from near the axis minimum (14) far up the axis:
 * the retained negative-side avar v1 segment becomes very steep in the
 * new space and offset compensation is quantization-limited. The
 * tolerance sits one F2Dot14 unit above the residual achieved WITH
 * interior-breakpoint collection; dropping the collection (or the whole
 * knot machinery) exceeds it. */
static void
test_avar2_restrict_steep_no_varidx (void)
{
  hb_face_t *face = open_original ();
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, OPSZ, 8.f, 144.f, 120.f));
  hb_face_t *inst = instance (face, input);

  for (unsigned i = 0; i < 33; i++)
  {
    hb_variation_t vars[1] = {{OPSZ, lerp (8.f, 144.f, i, 33)}};
    check_same_coords (face, inst, vars, 1, 5);
  }

  hb_face_destroy (inst);
  hb_face_destroy (face);
}

/* Replace the test font's avar with a version 2 table that has no VarIdxMap
 * and no VarStore, and gives the first axis, wght, an identity mapping with
 * a breakpoint every step F2Dot14 units. */
typedef struct
{
  hb_face_t *source;
  unsigned step;
} breakpoints_t;

static hb_blob_t *
reference_table_with_breakpoints (hb_face_t *face HB_UNUSED,
				  hb_tag_t tag,
				  void *user_data)
{
  const breakpoints_t *breakpoints = (const breakpoints_t *) user_data;
  if (tag != HB_TAG ('a','v','a','r'))
    return hb_face_reference_table (breakpoints->source, tag);

  unsigned map_count = 2 * 16384 / breakpoints->step + 1;
  unsigned length = 8 + (2 + map_count * 4) + 5 * 2 + 8;
  unsigned char *data = (unsigned char *) calloc (length, 1);
  g_assert_nonnull (data);

  data[1] = 2; /* major version */
  data[7] = 6; /* axisCount */
  data[8] = map_count >> 8;
  data[9] = map_count & 0xFF;
  for (unsigned i = 0; i < map_count; i++)
  {
    unsigned char *p = data + 10 + i * 4;
    unsigned coord = (unsigned) ((int) (i * breakpoints->step) - 16384);
    p[0] = p[2] = (coord >> 8) & 0xFF;
    p[1] = p[3] = coord & 0xFF;
  }

  return hb_blob_create ((const char *) data, length,
			 HB_MEMORY_MODE_WRITABLE, data, free);
}

/* Offset compensation takes time quadratic in the breakpoints of the
 * restricted axis. It must cope with hundreds of them, but give up on tens
 * of thousands rather than spend a minute. */
static void
test_avar2_restrict_many_breakpoints (void)
{
  hb_face_t *source = open_original ();

  breakpoints_t many = {source, 64};
  hb_face_t *face = hb_face_create_for_tables (reference_table_with_breakpoints,
					       &many, NULL);
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 200.f, 700.f, 400.f));
  hb_face_t *inst = instance (face, input);

  for (unsigned i = 0; i < 9; i++)
  {
    hb_variation_t vars[1] = {{WGHT, lerp (200.f, 700.f, i, 9)}};
    check_same_coords (face, inst, vars, 1, 2);
  }

  hb_face_destroy (inst);
  hb_face_destroy (face);

  breakpoints_t too_many = {source, 1};
  face = hb_face_create_for_tables (reference_table_with_breakpoints,
				    &too_many, NULL);
  input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 200.f, 700.f, 400.f));
  g_assert_null (hb_subset_or_fail (face, input));

  hb_subset_input_destroy (input);
  hb_face_destroy (face);
  hb_face_destroy (source);
}

/* Every retained breakpoint adds an offset-compensation tuple with a delta
 * per row of the VarData. For this table that is far more than the glyph
 * has points, so instancing must fail instead of allocating it. */
static void
test_avar2_empty_rows (void)
{
  hb_face_t *source = open_original ();
  hb_face_t *face = hb_face_create_for_tables (reference_table_with_avar2_empty_rows,
					       source, NULL);
  hb_subset_input_t *input = create_input ();
  g_assert_true (hb_subset_input_set_axis_range (input, face, WGHT, 200.f, 700.f, 400.f));
  g_assert_null (hb_subset_or_fail (face, input));

  hb_subset_input_destroy (input);
  hb_face_destroy (face);
  hb_face_destroy (source);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_avar2_restrict_same_default);
  hb_test_add (test_avar2_restrict_moved_default);
  hb_test_add (test_avar2_restrict_moved_default_positive);
  hb_test_add (test_avar2_restrict_no_varidx_axis);
  hb_test_add (test_avar2_restrict_shared_row);
  hb_test_add (test_avar2_pin_shared_row);
  hb_test_add (test_avar2_pin_at_default);
  hb_test_add (test_avar2_pin_driven_axis);
  hb_test_add (test_avar2_self_contained_pin);
  hb_test_add (test_avar2_self_contained_plus_range);
  hb_test_add (test_avar2_restrict_both_shared);
  hb_test_add (test_avar2_culling);
  hb_test_add (test_avar2_empty_varstore);
  hb_test_add (test_avar2_culling_aliased_gvar_data);
  hb_test_add (test_avar2_restrict_steep);
  hb_test_add (test_avar2_restrict_steep_no_varidx);
  hb_test_add (test_avar2_restrict_many_breakpoints);
  hb_test_add (test_avar2_empty_rows);

  return hb_test_run ();
}
