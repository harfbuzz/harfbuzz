/*
 * Copyright © 2018  Google, Inc.
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

/* Unit tests for hb-subset-glyf.h */

static void check_maxp_field (uint8_t *raw_maxp, unsigned int offset, uint16_t expected_value)
{
  uint16_t actual_value = (raw_maxp[offset] << 8) + raw_maxp[offset + 1];
  g_assert_cmpuint(expected_value, ==, actual_value);
}

static void check_maxp_num_glyphs (hb_face_t *face, uint16_t expected_num_glyphs, bool hints)
{
  hb_blob_t *maxp_blob = hb_face_reference_table (face, HB_TAG ('m','a','x', 'p'));

  unsigned int maxp_len;
  uint8_t *raw_maxp = (uint8_t *) hb_blob_get_data(maxp_blob, &maxp_len);

  check_maxp_field (raw_maxp, 4, expected_num_glyphs); // numGlyphs
  if (!hints)
  {
    check_maxp_field (raw_maxp, 14, 1); // maxZones
    check_maxp_field (raw_maxp, 16, 0); // maxTwilightPoints
    check_maxp_field (raw_maxp, 18, 0); // maxStorage
    check_maxp_field (raw_maxp, 20, 0); // maxFunctionDefs
    check_maxp_field (raw_maxp, 22, 0); // maxInstructionDefs
    check_maxp_field (raw_maxp, 24, 0); // maxStackElements
    check_maxp_field (raw_maxp, 26, 0); // maxSizeOfInstructions
  }

  hb_blob_destroy (maxp_blob);
}

#ifndef HB_NO_BEYOND_64K
static void
test_subset_GLYF (void)
{
  static const char maxp_data[] =
    "\x00\x00\x50\x00" /* version */
    "\x00\x01" /* numGlyphs */
    ;
  static const char head_data[54] =
    "\x00\x01\x00\x00" /* version */
    "\x00\x00\x00\x00" /* fontRevision */
    "\x00\x00\x00\x00" /* checkSumAdjustment */
    "\x5F\x0F\x3C\xF5" /* magicNumber */
    "\x00\x00" /* flags */
    "\x03\xE8" /* unitsPerEm */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* created */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* modified */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* bounds */
    "\x00\x00" /* macStyle */
    "\x00\x00" /* lowestRecPPEM */
    "\x00\x00" /* fontDirectionHint */
    "\x00\x01" /* indexToLocFormat */
    "\x00\x00" /* glyphDataFormat */
    ;
  static const char loca_data[8] =
    "\x00\x00\x00\x00"
    "\x00\x00\x00\x01"
    ;
  static const char glyf_data[] = "\0";

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "maxp", maxp_data);
  HB_FACE_ADD_TABLE (face, "head", head_data);
  HB_FACE_ADD_TABLE (face, "glyf", glyf_data);
  HB_FACE_ADD_TABLE (face, "loca", loca_data);
  HB_FACE_ADD_TABLE (face, "GLYF", glyf_data);
  HB_FACE_ADD_TABLE (face, "LOCA", loca_data);

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add (glyphs, 0);
  hb_subset_input_t *input = hb_subset_test_create_input_from_glyphs (glyphs);
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG),
	      HB_TAG ('g','l','y','f'));
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG),
	      HB_TAG ('l','o','c','a'));
  hb_face_t *subset = hb_subset_test_create_subset (face,
						    input);
  hb_set_destroy (glyphs);

  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('G','L','Y','F'));
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  hb_blob_destroy (blob);
  blob = hb_face_reference_table (subset, HB_TAG ('L','O','C','A'));
  g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
  hb_blob_destroy (blob);
  blob = hb_face_reference_table (subset, HB_TAG ('g','l','y','f'));
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);
  blob = hb_face_reference_table (subset, HB_TAG ('l','o','c','a'));
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);
  blob = hb_face_reference_table (subset, HB_TAG ('h','e','a','d'));
  unsigned length;
  const char *data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, >=, 52);
  g_assert_cmpuint ((unsigned char) data[50], ==, 0);
  g_assert_cmpuint ((unsigned char) data[51], ==, 0);
  hb_blob_destroy (blob);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_GLYF_high_gid (void)
{
  static const char MAXP_data[] =
    "\x00\x00\x50\x00" /* version */
    "\x01\x00\x02" /* numGlyphs */
    ;
  static const char head_data[54] =
    "\x00\x01\x00\x00" /* version */
    "\x00\x00\x00\x00" /* fontRevision */
    "\x00\x00\x00\x00" /* checkSumAdjustment */
    "\x5F\x0F\x3C\xF5" /* magicNumber */
    "\x00\x00" /* flags */
    "\x03\xE8" /* unitsPerEm */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* created */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* modified */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* bounds */
    "\x00\x00" /* macStyle */
    "\x00\x00" /* lowestRecPPEM */
    "\x00\x00" /* fontDirectionHint */
    "\x00\x01" /* indexToLocFormat */
    "\x00\x00" /* glyphDataFormat */
    ;
  static const char GLYF_data[21] =
    "\x00\x01" /* numberOfContours */
    "\x00\x01\x00\x02\x00\x04\x00\x06" /* bounds */
    "\x00\x02" /* endPtsOfContours */
    "\x00\x00" /* instructionLength */
    "\x31\xB6\x96" /* flags */
    "\x0A\x0A" /* xCoordinates */
    "\x0A\x0A" /* yCoordinates */
    ;

  const unsigned num_glyphs = 0x10002;
  const unsigned loca_length = (num_glyphs + 1) * 4;
  char *LOCA_data = g_malloc0 (loca_length);
  LOCA_data[loca_length - 1] = sizeof (GLYF_data);

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "MAXP", MAXP_data);
  HB_FACE_ADD_TABLE (face, "head", head_data);
  HB_FACE_ADD_TABLE (face, "GLYF", GLYF_data);
  hb_blob_t *blob = hb_blob_create (LOCA_data, loca_length,
				    HB_MEMORY_MODE_WRITABLE, LOCA_data, g_free);
  g_assert_true (hb_face_builder_add_table (face, HB_TAG ('L','O','C','A'), blob));
  hb_blob_destroy (blob);

  g_assert_cmpuint (hb_face_get_glyph_count (face), ==, num_glyphs);
  hb_font_t *font = hb_font_create (face);
  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 0x10001, &extents));
  g_assert_cmpint (extents.width, ==, 3);
  hb_font_destroy (font);

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add (glyphs, 0x10001);
  hb_face_t *subset = hb_subset_test_create_subset (
      face, hb_subset_test_create_input_from_glyphs (glyphs));
  hb_set_destroy (glyphs);

  g_assert_cmpuint (hb_face_get_glyph_count (subset), ==, 2);
  font = hb_font_create (subset);
  g_assert_true (hb_font_get_glyph_extents (font, 1, &extents));
  g_assert_cmpint (extents.width, ==, 3);
  hb_font_destroy (font);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}
#endif

static void
test_subset_glyf (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 99);
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 3, true);
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_glyf_set_overlaps_flag (void)
{
  hb_face_t *face_abcAE = hb_test_open_font_file ("fonts/Roboto-Regular.abcAE.ttf");
  hb_face_t *face_bAE = hb_test_open_font_file ("fonts/Roboto-Regular.bAE.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abcAE_subset;
  hb_set_add (codepoints, 32);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 508);

  hb_subset_input_t* input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_SET_OVERLAPS_FLAG);
  face_abcAE_subset = hb_subset_test_create_subset (face_abcAE, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_bAE, face_abcAE_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_bAE, face_abcAE_subset, HB_TAG ('g','l','y','f'));

  hb_face_destroy (face_abcAE_subset);
  hb_face_destroy (face_abcAE);
  hb_face_destroy (face_bAE);
}

static void
test_subset_glyf_with_input_glyphs (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  hb_set_t *glyphs = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (glyphs, 1);
  hb_set_add (glyphs, 3);
  face_abc_subset =
      hb_subset_test_create_subset (face_abc, hb_subset_test_create_input_from_glyphs (glyphs));
  hb_set_destroy (glyphs);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_abc_subset, 3, true);

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_glyf_with_components (void)
{
  hb_face_t *face_components = hb_test_open_font_file ("fonts/Roboto-Regular.components.ttf");
  hb_face_t *face_subset = hb_test_open_font_file ("fonts/Roboto-Regular.components.subset.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_generated_subset;
  hb_set_add (codepoints, 0x1fc);
  face_generated_subset = hb_subset_test_create_subset (face_components, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_subset, face_generated_subset, HB_TAG ('g','l','y','f'));
  hb_subset_test_check (face_subset, face_generated_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_generated_subset, 4, true);

  hb_face_destroy (face_generated_subset);
  hb_face_destroy (face_subset);
  hb_face_destroy (face_components);
}

static void
test_subset_glyf_with_gsub (void)
{
  hb_face_t *face_fil = hb_test_open_font_file ("fonts/Roboto-Regular.gsub.fil.ttf");
  hb_face_t *face_fi = hb_test_open_font_file ("fonts/Roboto-Regular.gsub.fi.ttf");
  hb_subset_input_t *input;
  hb_face_t *face_subset;

  hb_set_t *codepoints = hb_set_create();
  hb_set_add (codepoints, 102); // f
  hb_set_add (codepoints, 105); // i

  input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  hb_set_del (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'S', 'U', 'B'));
  hb_set_del (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'P', 'O', 'S'));
  hb_set_del (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'D', 'E', 'F'));

  face_subset = hb_subset_test_create_subset (face_fil, input);

  hb_subset_test_check (face_fi, face_subset, HB_TAG ('g','l','y','f'));
  hb_subset_test_check (face_fi, face_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_subset, 5, true);

  hb_face_destroy (face_subset);
  hb_face_destroy (face_fi);
  hb_face_destroy (face_fil);
}

static void
test_subset_glyf_without_gsub (void)
{
  hb_face_t *face_fil = hb_test_open_font_file ("fonts/Roboto-Regular.gsub.fil.ttf");
  hb_face_t *face_fi = hb_test_open_font_file ("fonts/Roboto-Regular.nogsub.fi.ttf");
  hb_subset_input_t *input;
  hb_face_t *face_subset;

  hb_set_t *codepoints = hb_set_create();
  hb_set_add (codepoints, 102); // f
  hb_set_add (codepoints, 105); // i

  input = hb_subset_test_create_input (codepoints);
  hb_set_destroy (codepoints);
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'S', 'U', 'B'));
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'P', 'O', 'S'));
  hb_set_add (hb_subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'D', 'E', 'F'));

  face_subset = hb_subset_test_create_subset (face_fil, input);

  hb_subset_test_check (face_fi, face_subset, HB_TAG ('g','l','y','f'));
  hb_subset_test_check (face_fi, face_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_subset, 3, true);

  hb_face_destroy (face_subset);
  hb_face_destroy (face_fi);
  hb_face_destroy (face_fil);
}

static void
test_subset_glyf_noop (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 4, true);
  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('g','l','y','f'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
}

static void
test_subset_glyf_strip_hints_simple (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.nohints.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 3, false);
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_glyf_strip_hints_composite (void)
{
  hb_face_t *face_components = hb_test_open_font_file ("fonts/Roboto-Regular.components.ttf");
  hb_face_t *face_subset = hb_test_open_font_file ("fonts/Roboto-Regular.components.1fc.nohints.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_subset_input_t *input;
  hb_face_t *face_generated_subset;
  hb_set_add (codepoints, 0x1fc);
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);

  face_generated_subset = hb_subset_test_create_subset (face_components, input);
  hb_set_destroy (codepoints);

  check_maxp_num_glyphs(face_generated_subset, 4, false);
  hb_subset_test_check (face_subset, face_generated_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_subset, face_generated_subset, HB_TAG ('g','l','y','f'));

  hb_face_destroy (face_generated_subset);
  hb_face_destroy (face_subset);
  hb_face_destroy (face_components);
}

static void
test_subset_glyf_strip_hints_invalid (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/oom-ccc61c92d589f895174cdef6ff2e3b20e9999a1a");

  hb_set_t *codepoints = hb_set_create();
  const hb_codepoint_t text[] =
  {
    'A', 'B', 'C', 'D', 'E', 'X', 'Y', 'Z', '1', '2',
    '3', '@', '_', '%', '&', ')', '*', '$', '!'
  };
  unsigned int i;
  hb_subset_input_t *input;
  hb_face_t *face_subset;

  for (i = 0; i < sizeof (text) / sizeof (hb_codepoint_t); i++)
  {
    hb_set_add (codepoints, text[i]);
  }

  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  hb_set_destroy (codepoints);

  face_subset = hb_subset_or_fail (face, input);
  g_assert_true (!face_subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

static void
test_subset_glyf_retain_gids (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/Roboto-Regular.ac.retaingids.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 99);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 4, true);
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_glyf_retain_gids_truncates (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_a = hb_test_open_font_file ("fonts/Roboto-Regular.a.retaingids.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 2, true);
  hb_subset_test_check (face_a, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_a, face_abc_subset, HB_TAG ('g','l','y','f'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_a);
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_glyf_iftb_requirements (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Variable.abc.ttf");
  hb_face_t *face_long_loca = hb_test_open_font_file ("fonts/Roboto-Variable.abc.long_loca.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_IFTB_REQUIREMENTS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_long_loca, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  hb_subset_test_check (face_long_loca, face_abc_subset, HB_TAG ('g','l','y','f'));
  hb_subset_test_check (face_long_loca, face_abc_subset, HB_TAG ('g','v','a','r'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_long_loca);

}
#endif

// TODO(grieger): test for long loca generation.

static void
test_subset_glyf_zero_contour_instructions (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/empty-hinted.ttf");
  /* No instance, partial instance, full instance, and pinning at default. */
  for (unsigned instance = 0; instance < 4; instance++)
    for (unsigned no_hinting = 0; no_hinting < 2; no_hinting++)
    {
      hb_subset_input_t *input = hb_subset_input_create_or_fail ();
      hb_set_add (hb_subset_input_glyph_set (input), 1);
      hb_subset_input_set_flags (input, no_hinting ? HB_SUBSET_FLAGS_NO_HINTING : HB_SUBSET_FLAGS_DEFAULT);
      if (instance)
        g_assert_true (hb_subset_input_pin_axis_location (input, face,
                      HB_TAG ('w','g','h','t'), instance == 3 ? 0.f : 1.f));
      if (instance >= 2)
        g_assert_true (hb_subset_input_pin_axis_location (input, face,
                      HB_TAG ('w','d','t','h'), 0.f));
      hb_face_t *subset = hb_subset_test_create_subset (face, input);
      if (instance)
      {
        hb_blob_t *head = hb_face_reference_table (subset, HB_TAG ('h','e','a','d'));
        unsigned length;
        const char *data = hb_blob_get_data (head, &length);
        static const char zero_bounds[8] = {0};
        g_assert_cmpuint (length, >=, 44);
        g_assert_cmpmem (data + 36, 8, zero_bounds, 8);
        hb_blob_destroy (head);
      }
      hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('g','l','y','f'));
      unsigned length;
      const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &length);
      if (!no_hinting)
      {
        static const unsigned char instructions[] = {0xb0, 1, 0x21};
        g_assert_cmpuint (length, >=, 15);
        g_assert_cmpuint (data[0], ==, 0);
        g_assert_cmpuint (data[1], ==, 0);
        g_assert_cmpuint (data[10], ==, 0);
        g_assert_cmpuint (data[11], ==, sizeof (instructions));
        g_assert_cmpmem (data + 12, sizeof (instructions), instructions, sizeof (instructions));
      }
      else if (length >= 12)
      {
        g_assert_cmpuint (data[10], ==, 0);
        g_assert_cmpuint (data[11], ==, 0);
      }
      hb_blob_destroy (blob);
      hb_font_t *font = hb_font_create (subset);
      hb_ot_font_set_funcs (font);
      g_assert_cmpint (hb_font_get_glyph_h_advance (font, 1), ==,
                      instance == 1 || instance == 2 ? 550 : 500);
      hb_font_destroy (font);
      hb_face_destroy (subset);
    }
  hb_face_destroy (face);
}

static void
test_subset_glyf_recompute_default_instance_bounds (void)
{
  hb_face_t *source = hb_test_open_font_file ("fonts/glyf-default-instance-bounds.ttf");
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, 2);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  g_assert_true (hb_subset_input_pin_axis_location (input, source,
                  HB_TAG ('w','g','h','t'), 0.5f));
  hb_face_t *partial = hb_subset_test_create_subset (source, input);

  hb_font_t *font = hb_font_create (partial);
  hb_ot_font_set_funcs (font);
  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 2, &extents));
  g_assert_cmpint (extents.x_bearing + extents.width, ==, 11);
  hb_font_destroy (font);

  input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, 2);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  g_assert_true (hb_subset_input_pin_all_axes_to_default (input, partial));
  hb_face_t *full = hb_subset_test_create_subset (partial, input);
  font = hb_font_create (full);
  hb_ot_font_set_funcs (font);
  /* Partial instancing stores the child's xMax 10.5 as 11 and the component
   * offset 0.5 as 1. Recomputing at the new default must therefore give 12. */
  g_assert_true (hb_font_get_glyph_extents (font, 2, &extents));
  g_assert_cmpint (extents.x_bearing + extents.width, ==, 12);
  hb_font_destroy (font);
  hb_face_destroy (full);
  hb_face_destroy (partial);
  hb_face_destroy (source);
}

static void
test_subset_glyf_aliased_loca (void)
{
  /* A loca of 0, 16, 0, 16, ...: every even glyph is the same 16 bytes and
   * every odd one starts after its end, so reads as empty. */
  enum { num_glyphs = 101 };
  static const char maxp_data[] = {0x00, 0x00, 0x50, 0x00, 0x00, num_glyphs};
  static const char head_data[54] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5F, 0x0F, 0x3C, (char) 0xF5, 0x00, 0x00, 0x03, (char) 0xE8,
  };
  static const char glyf_data[16] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x31,
  };
  char loca_data[2 * (num_glyphs + 1)] = {0};
  for (unsigned i = 1; i <= num_glyphs; i += 2)
    loca_data[2 * i + 1] = sizeof (glyf_data) / 2;

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "maxp", maxp_data);
  HB_FACE_ADD_TABLE (face, "head", head_data);
  HB_FACE_ADD_TABLE (face, "glyf", glyf_data);
  HB_FACE_ADD_TABLE (face, "loca", loca_data);

  /* A few glyphs sharing the same bytes are each copied out. */
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, 4);
  hb_face_t *subset = hb_subset_test_create_subset (face, input);
  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('g','l','y','f'));
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 2 * sizeof (glyf_data));
  hb_blob_destroy (blob);
  hb_face_destroy (subset);

  /* Enough of them to add up to many times the table are rejected. */
  input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, num_glyphs - 1);
  g_assert_null (hb_subset_plan_create_or_fail (face, input));
  g_assert_null (hb_subset_or_fail (face, input));
  hb_subset_input_destroy (input);

  hb_face_destroy (face);
}

static unsigned
add_composite_glyph (uint8_t *p, unsigned count, unsigned component)
{
  memset (p, 0, 10);
  p[0] = p[1] = 0xFF; /* numberOfContours -1 */
  for (unsigned i = 0; i < count; i++)
  {
    uint8_t *r = p + 10 + i * 6;
    r[0] = 0;
    r[1] = 0x02 | (i + 1 < count ? 0x20 : 0); /* ARGS_ARE_XY_VALUES [| MORE_COMPONENTS] */
    r[2] = component >> 8;
    r[3] = component & 0xFF;
    r[4] = r[5] = 1;
  }
  return 10 + count * 6;
}

static void
test_subset_glyf_instancing_budget (void)
{
  /* gid 1 is a simple glyph of 65536 points (or 4 for the control), gid 2
   * uses it three times, gid 3..64 each wrap the previous glyph, and 100
   * top-level glyphs wrap gid 64.  No glyph exceeds a per-glyph limit, but
   * instancing loads the shared chain again for every top-level glyph, so
   * the table as a whole must be bounded by one work budget. */
  static const char maxp_data[] =
    "\x00\x00\x50\x00" /* version */
    "\x00\xA5" /* numGlyphs */
    ;
  static const char head_data[54] =
    "\x00\x01\x00\x00" /* version */
    "\x00\x00\x00\x00" /* fontRevision */
    "\x00\x00\x00\x00" /* checkSumAdjustment */
    "\x5F\x0F\x3C\xF5" /* magicNumber */
    "\x00\x00" /* flags */
    "\x03\xE8" /* unitsPerEm */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* created */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* modified */
    "\x00\x00\x00\x00\x00\x00\x00\x00" /* bounds */
    "\x00\x00" /* macStyle */
    "\x00\x00" /* lowestRecPPEM */
    "\x00\x00" /* fontDirectionHint */
    "\x00\x01" /* indexToLocFormat */
    "\x00\x00" /* glyphDataFormat */
    ;
  static const char fvar_data[] =
    "\x00\x01\x00\x00" /* version */
    "\x00\x10" /* axesArrayOffset */
    "\x00\x02" /* reserved */
    "\x00\x01" /* axisCount */
    "\x00\x14" /* axisSize */
    "\x00\x00" /* instanceCount */
    "\x00\x08" /* instanceSize */
    "wght" "\x00\x64\x00\x00" "\x01\x90\x00\x00" "\x03\x84\x00\x00" "\x00\x00" "\x01\x00"
    ;
  const unsigned num_glyphs = 165;

  for (unsigned heavy = 0; heavy < 2; heavy++)
  {
    unsigned flag_pairs = heavy ? 256 : 1;
    uint8_t *glyf_data = calloc (14 + 2 * flag_pairs + 28 + 62 * 16 + 100 * 16, 1);
    uint8_t *loca_data = calloc (4 * (num_glyphs + 1), 1);
    unsigned offset = 0;
    for (unsigned gid = 0; gid < num_glyphs; gid++)
    {
      uint8_t *p = glyf_data + offset;
      loca_data[gid * 4 + 2] = offset >> 8;
      loca_data[gid * 4 + 3] = offset & 0xFF;
      if (gid == 1)
      {
	p[1] = 1; /* numberOfContours */
	p[10] = (flag_pairs * 256 - 1) >> 8; /* endPtsOfContours[0] */
	p[11] = heavy ? 0xFF : 0x03;
	for (unsigned i = 0; i < flag_pairs; i++)
	{
	  p[14 + i * 2] = 0x39; /* ON_CURVE | X_SAME | Y_SAME | REPEAT */
	  p[15 + i * 2] = heavy ? 0xFF : 0x03;
	}
	offset += 14 + 2 * flag_pairs;
      }
      else if (gid == 2)
	offset += add_composite_glyph (p, 3, 1);
      else if (gid >= 3)
	offset += add_composite_glyph (p, 1, gid <= 64 ? gid - 1 : 64);
    }
    loca_data[num_glyphs * 4 + 1] = offset >> 16;
    loca_data[num_glyphs * 4 + 2] = offset >> 8;
    loca_data[num_glyphs * 4 + 3] = offset & 0xFF;

    hb_face_t *face = hb_face_builder_create ();
    HB_FACE_ADD_TABLE (face, "maxp", maxp_data);
    HB_FACE_ADD_TABLE (face, "head", head_data);
    HB_FACE_ADD_TABLE (face, "fvar", fvar_data);
    hb_blob_t *blob = hb_blob_create_or_fail ((const char *) glyf_data, offset,
					      HB_MEMORY_MODE_WRITABLE, glyf_data, free);
    hb_face_builder_add_table (face, HB_TAG ('g','l','y','f'), blob);
    hb_blob_destroy (blob);
    blob = hb_blob_create_or_fail ((const char *) loca_data, 4 * (num_glyphs + 1),
				   HB_MEMORY_MODE_WRITABLE, loca_data, free);
    hb_face_builder_add_table (face, HB_TAG ('l','o','c','a'), blob);
    hb_blob_destroy (blob);

    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add_range (hb_subset_input_glyph_set (input), 0, num_glyphs - 1);
    g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','g','h','t'), 500.f));
    hb_face_t *subset = hb_subset_test_create_subset (face, input);

    /* Exhausting the budget drops glyf from the instance. */
    blob = hb_face_reference_table (subset, HB_TAG ('g','l','y','f'));
    if (heavy)
      g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
    else
      g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
    hb_blob_destroy (blob);

    hb_face_destroy (subset);
    hb_face_destroy (face);
  }
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_subset_GLYF);
  hb_test_add (test_subset_GLYF_high_gid);
#endif
  hb_test_add (test_subset_glyf_noop);
  hb_test_add (test_subset_glyf);
  hb_test_add (test_subset_glyf_set_overlaps_flag);
  hb_test_add (test_subset_glyf_with_input_glyphs);
  hb_test_add (test_subset_glyf_strip_hints_simple);
  hb_test_add (test_subset_glyf_strip_hints_composite);
  hb_test_add (test_subset_glyf_strip_hints_invalid);
  hb_test_add (test_subset_glyf_with_components);
  hb_test_add (test_subset_glyf_with_gsub);
  hb_test_add (test_subset_glyf_without_gsub);
  hb_test_add (test_subset_glyf_retain_gids);
  hb_test_add (test_subset_glyf_retain_gids_truncates);
  hb_test_add (test_subset_glyf_zero_contour_instructions);
  hb_test_add (test_subset_glyf_recompute_default_instance_bounds);
  hb_test_add (test_subset_glyf_aliased_loca);
  hb_test_add (test_subset_glyf_instancing_budget);

#ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_subset_glyf_iftb_requirements);
#endif

  return hb_test_run();
}
