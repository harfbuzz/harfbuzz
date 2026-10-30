/*
 * Copyright © 2018 Adobe Inc.
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
#include "hb-subset-test.h"

/* Unit tests for CFF2 subsetting */

static void
test_subset_cff2_noop (void)
{
  hb_face_t *face_abc = hb_test_open_font_file("fonts/AdobeVFPrototype.abc.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('C','F','F','2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
}

static void
test_subset_cff2 (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F','2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_strip_hints (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_desubr (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.nosubrs.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_desubr_strip_hints (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.nosubrs.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_DESUBROUTINIZE | HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_retaingids (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.retaingids.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_fdselect_format4 (void)
{
  /* Three glyphs and two Font DICTs: glyphs 0-1 use FD 0, glyph 2 uses FD 1 and
   * draws one line through FD 1's local subr 0.  The two tables differ only in
   * the FDSelect encoding: format 3, or the CFF2-only format 4. */
  static const char maxp_data[] = {0, 0, 0x50, 0, 0, 3};
  static const char cff2_fdselect3_data[] = {
    0x02, 0, 0x05, 0, 0x14, /* CFF2 header */
    0x1D, 0, 0, 0, 0x1D, 0x11, 0x1D, 0, 0, 0, 0x6D, 0x0C, 0x24, 0x1D, 0, 0, 0, 0x62, 0x0C, 0x25, /* Top DICT: CharStrings, FDArray, FDSelect */
    0, 0, 0, 0, /* Global Subrs INDEX (empty) */
    0, 0, 0, 0x03, 0x01, 0x01, 0x16, 0x2B, 0x3D, /* CharStrings INDEX */
    0x1C, 0, 0, 0x1C, 0, 0, 0x15, 0x1C, 0, 0x0A, 0x1C, 0, 0, 0x05, 0x1C, 0, 0, 0x1C, 0, 0x0A, 0x05, /*   .notdef */
    0x1C, 0, 0x32, 0x1C, 0, 0, 0x15, 0x1C, 0x01, (char) 0xC2, 0x1C, 0, 0, 0x05, 0x1C, 0, 0, 0x1C, 0x02, (char) 0xBC, 0x05, /*   glyph 1 */
    0x1C, 0, 0x64, 0x1C, (char) 0xFF, 0x38, 0x15, 0x1C, (char) 0xFF, (char) 0x95, 0x0A, 0x1C, 0, 0, 0x1C, 0x01, 0x2C, 0x05, /*   glyph 2: calls FD 1's local subr 0 */
    0x03, 0, 0x02, 0, 0, 0, 0, 0x02, 0x01, 0, 0x03, /* FDSelect format 3: glyphs 0-1 -> FD 0, glyph 2 -> FD 1 */
    0, 0, 0, 0x02, 0x01, 0x01, 0x0A, 0x13, /* FDArray INDEX */
    0x1C, 0, 0x04, 0x1D, 0, 0, 0, (char) 0x87, 0x12, /*   Font DICT 0 */
    0x1C, 0, 0x0A, 0x1D, 0, 0, 0, (char) 0x8B, 0x12, /*   Font DICT 1 */
    0x1C, 0, 0x32, 0x0A, /* Private DICT 0 */
    0x1C, 0, 0x50, 0x0A, 0x1D, 0, 0, 0, 0x0A, 0x13, /* Private DICT 1: Subrs */
    0, 0, 0, 0x01, 0x01, 0x01, 0x08, 0x1C, 0, 0x64, 0x1C, 0, 0x64, 0x05, /* FD 1 local Subrs INDEX */
  };
  static const char cff2_fdselect4_data[] = {
    0x02, 0, 0x05, 0, 0x14, /* CFF2 header */
    0x1D, 0, 0, 0, 0x1D, 0x11, 0x1D, 0, 0, 0, 0x77, 0x0C, 0x24, 0x1D, 0, 0, 0, 0x62, 0x0C, 0x25, /* Top DICT: CharStrings, FDArray, FDSelect */
    0, 0, 0, 0, /* Global Subrs INDEX (empty) */
    0, 0, 0, 0x03, 0x01, 0x01, 0x16, 0x2B, 0x3D, /* CharStrings INDEX */
    0x1C, 0, 0, 0x1C, 0, 0, 0x15, 0x1C, 0, 0x0A, 0x1C, 0, 0, 0x05, 0x1C, 0, 0, 0x1C, 0, 0x0A, 0x05, /*   .notdef */
    0x1C, 0, 0x32, 0x1C, 0, 0, 0x15, 0x1C, 0x01, (char) 0xC2, 0x1C, 0, 0, 0x05, 0x1C, 0, 0, 0x1C, 0x02, (char) 0xBC, 0x05, /*   glyph 1 */
    0x1C, 0, 0x64, 0x1C, (char) 0xFF, 0x38, 0x15, 0x1C, (char) 0xFF, (char) 0x95, 0x0A, 0x1C, 0, 0, 0x1C, 0x01, 0x2C, 0x05, /*   glyph 2: calls FD 1's local subr 0 */
    0x04, 0, 0, 0, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x02, 0, 0x01, 0, 0, 0, 0x03, /* FDSelect format 4: glyphs 0-1 -> FD 0, glyph 2 -> FD 1 */
    0, 0, 0, 0x02, 0x01, 0x01, 0x0A, 0x13, /* FDArray INDEX */
    0x1C, 0, 0x04, 0x1D, 0, 0, 0, (char) 0x91, 0x12, /*   Font DICT 0 */
    0x1C, 0, 0x0A, 0x1D, 0, 0, 0, (char) 0x95, 0x12, /*   Font DICT 1 */
    0x1C, 0, 0x32, 0x0A, /* Private DICT 0 */
    0x1C, 0, 0x50, 0x0A, 0x1D, 0, 0, 0, 0x0A, 0x13, /* Private DICT 1: Subrs */
    0, 0, 0, 0x01, 0x01, 0x01, 0x08, 0x1C, 0, 0x64, 0x1C, 0, 0x64, 0x05, /* FD 1 local Subrs INDEX */
  };

  hb_face_t *face_fdselect3 = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face_fdselect3, "maxp", maxp_data);
  HB_FACE_ADD_TABLE (face_fdselect3, "CFF2", cff2_fdselect3_data);

  hb_face_t *face_fdselect4 = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face_fdselect4, "maxp", maxp_data);
  HB_FACE_ADD_TABLE (face_fdselect4, "CFF2", cff2_fdselect4_data);

  hb_set_t *glyphs = hb_set_create ();
  hb_face_t *face_fdselect3_subset, *face_fdselect4_subset;
  hb_set_add_range (glyphs, 0, 2);
  face_fdselect3_subset = hb_subset_test_create_subset (face_fdselect3, hb_subset_test_create_input_from_glyphs (glyphs));
  face_fdselect4_subset = hb_subset_test_create_subset (face_fdselect4, hb_subset_test_create_input_from_glyphs (glyphs));
  hb_set_destroy (glyphs);

  /* Both retain FD 1; the format-4 source must not have been planned as FD 0 only. */
  hb_subset_test_check (face_fdselect3_subset, face_fdselect4_subset, HB_TAG ('C','F','F','2'));

  hb_font_t *font = hb_font_create (face_fdselect4_subset);
  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 2, &extents));
  g_assert_cmpint (extents.x_bearing, ==, 100);
  g_assert_cmpint (extents.y_bearing, ==, 200);
  g_assert_cmpint (extents.width, ==, 100);
  g_assert_cmpint (extents.height, ==, -400);
  hb_font_destroy (font);

  hb_face_destroy (face_fdselect4_subset);
  hb_face_destroy (face_fdselect3_subset);
  hb_face_destroy (face_fdselect4);
  hb_face_destroy (face_fdselect3);
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_cff2_iftb_requirements (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_long_off = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.long_off.otf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_IFTB_REQUIREMENTS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_long_off, face_abc_subset, HB_TAG ('C','F','F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_long_off);

}
#endif

/* Caller-dependent subroutine numbers cannot be remapped at a shared offset. */
static void
test_subset_cff2_caller_supplied_subr (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2_caller_subr.otf");
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'B');
  for (unsigned i = 0; i < 2; i++)
  {
    hb_subset_input_set_flags (input, i ? HB_SUBSET_FLAGS_NO_HINTING : HB_SUBSET_FLAGS_DEFAULT);
    hb_face_t *subset = hb_subset_or_fail (face, input);
    g_assert_null (subset);
  }
  /* Explicit flattening does not remap caller-supplied operands. */
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  hb_font_t *font = hb_font_create (subset);
  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 1, &extents));
  g_assert_cmpint (extents.width, ==, 10);
  g_assert_true (hb_font_get_glyph_extents (font, 2, &extents));
  g_assert_cmpint (extents.width, ==, 20);
  hb_font_destroy (font);
  hb_face_destroy (subset);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

static void
test_subset_cff2_long_private_blends (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2_long_private_blends.otf");
  hb_blob_t *source = hb_face_reference_table (face, HB_TAG ('C','F','F','2'));
  unsigned source_length;
  const char *source_data = hb_blob_get_data (source, &source_length);
  /* This fixture's Private DICT occupies [395, 974), including its final
   * BlueValues operator. Its live stack never exceeds 513 operands. */
  const unsigned private_offset = 395, private_length = 579;
  g_assert_cmpuint (private_offset + private_length, <=, source_length);
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'B');
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  hb_blob_t *result = hb_face_reference_table (subset, HB_TAG ('C','F','F','2'));
  unsigned length;
  const char *data = hb_blob_get_data (result, &length);
  gboolean found = FALSE;
  for (unsigned i = 0; i + private_length <= length; i++)
    if (!memcmp (source_data + private_offset, data + i, private_length))
    {
      found = TRUE;
      break;
    }
  g_assert_true (found);
  hb_blob_destroy (result);
  hb_face_destroy (subset);
  hb_subset_input_destroy (input);
  hb_blob_destroy (source);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_cff2_caller_supplied_subr);
  hb_test_add (test_subset_cff2_long_private_blends);

  hb_test_add (test_subset_cff2_noop);
  hb_test_add (test_subset_cff2);
  hb_test_add (test_subset_cff2_strip_hints);
  hb_test_add (test_subset_cff2_desubr);
  hb_test_add (test_subset_cff2_desubr_strip_hints);
  hb_test_add (test_subset_cff2_retaingids);
  hb_test_add (test_subset_cff2_fdselect_format4);

#ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_subset_cff2_iftb_requirements);
#endif

  return hb_test_run ();
}
