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

#ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_subset_cff2_iftb_requirements);
#endif

  return hb_test_run ();
}
