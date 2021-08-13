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

static void
test_subset_32_tables (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/oom-6ef8c96d3710262511bcc730dce9c00e722cb653");

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);
  hb_face_t *subset;

  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');

  subset = hb_subset_or_fail (face, input);
  g_assert (subset);
  g_assert (subset != hb_face_get_empty ());

  hb_subset_input_destroy (input);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_no_inf_loop (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/clusterfuzz-testcase-minimized-hb-subset-fuzzer-5521982557782016");

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);
  hb_face_t *subset;

  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');

  subset = hb_subset_or_fail (face, input);
  g_assert (!subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_crash (void)
{
  hb_face_t *face = hb_test_open_font_file ("../fuzzing/fonts/crash-4b60576767ee4d9fe1cc10959d89baf73d4e8249");

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *codepoints = hb_subset_input_unicode_set (input);
  hb_face_t *subset;

  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');

  subset = hb_subset_or_fail (face, input);
  g_assert (!subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_set_flags (void)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();

  g_assert (hb_subset_input_get_flags (input) == HB_SUBSET_FLAGS_DEFAULT);

  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NAME_LEGACY |
                             HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
                             HB_SUBSET_FLAGS_GLYPH_NAMES);

  g_assert (hb_subset_input_get_flags (input) ==
            (hb_subset_flags_t) (
            HB_SUBSET_FLAGS_NAME_LEGACY |
            HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
            HB_SUBSET_FLAGS_GLYPH_NAMES));

  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NAME_LEGACY |
                             HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
                             HB_SUBSET_FLAGS_RETAIN_ALL_FEATURES);

  g_assert (hb_subset_input_get_flags (input) ==
            (hb_subset_flags_t) (
            HB_SUBSET_FLAGS_NAME_LEGACY |
            HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
            HB_SUBSET_FLAGS_RETAIN_ALL_FEATURES));


  hb_subset_input_destroy (input);
}


static void
test_subset_legacy_api (void)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();

  g_assert (hb_subset_input_get_flags (input) == HB_SUBSET_FLAGS_DEFAULT);

  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NAME_LEGACY |
                             HB_SUBSET_FLAGS_NOTDEF_OUTLINE);

  g_assert (hb_subset_input_get_name_legacy (input));
  g_assert (hb_subset_input_get_notdef_outline (input));
  g_assert (!hb_subset_input_get_desubroutinize (input));
  g_assert (!hb_subset_input_get_drop_hints (input));

  hb_subset_input_set_drop_hints (input, true);
  hb_subset_input_set_name_legacy (input, false);
  g_assert (!hb_subset_input_get_name_legacy (input));
  g_assert (hb_subset_input_get_notdef_outline (input));
  g_assert (!hb_subset_input_get_desubroutinize (input));
  g_assert (hb_subset_input_get_drop_hints (input));


  hb_face_t *face_abc = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  hb_set_t *codepoints = hb_set_create();
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 99);
  hb_set_union (hb_subset_input_unicode_set (input), codepoints);
  hb_set_destroy (codepoints);

  hb_face_t *face_abc_subset_new = hb_subset (face_abc, input);
  hb_face_t *face_abc_subset_legacy = hb_subset_or_fail (face_abc, input);


  hb_blob_t* a = hb_face_reference_blob (face_abc_subset_new);
  hb_blob_t* b = hb_face_reference_blob (face_abc_subset_new);
  hb_test_assert_blobs_equal (a, b);

  hb_blob_destroy (a);
  hb_blob_destroy (b);
  hb_face_destroy (face_abc_subset_new);
  hb_face_destroy (face_abc_subset_legacy);
  hb_face_destroy (face_abc);

  hb_subset_input_destroy (input);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_32_tables);
  hb_test_add (test_subset_no_inf_loop);
  hb_test_add (test_subset_crash);
  hb_test_add (test_subset_set_flags);
  hb_test_add (test_subset_legacy_api);

  return hb_test_run();
}
