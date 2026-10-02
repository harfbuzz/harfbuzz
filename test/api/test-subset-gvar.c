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
#include "hb-subset-test.h"

/* Unit tests for gvar subsetting */

static void
test_subset_gvar_noop (void)
{
  hb_face_t *face_abc = hb_test_open_font_file("fonts/SourceSansVariable-Roman.abc.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('g','v','a','r'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
}

static void
test_subset_gvar (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.ac.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','v','a','r'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_gvar_retaingids (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.ac.retaingids.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','v','a','r'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

#ifndef HB_NO_BEYOND_64K
static hb_face_t *
create_GVAR_face (const char *GVAR_data, unsigned GVAR_length)
{
  hb_face_t *source = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.abc.ttf");
  hb_face_t *face = hb_face_builder_create ();

  unsigned count = hb_face_get_table_tags (source, 0, NULL, NULL);
  hb_tag_t *tags = g_new (hb_tag_t, count);
  unsigned total = count;
  g_assert_cmpuint (hb_face_get_table_tags (source, 0, &count, tags), ==, total);
  for (unsigned i = 0; i < count; i++)
  {
    hb_blob_t *blob = hb_face_reference_table (source, tags[i]);
    g_assert_true (hb_face_builder_add_table (face, tags[i], blob));
    hb_blob_destroy (blob);
  }
  g_free (tags);
  hb_face_destroy (source);

  hb_blob_t *blob = hb_blob_create_or_fail (GVAR_data, GVAR_length,
					     HB_MEMORY_MODE_READONLY,
					     NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_TAG ('G','V','A','R'), blob));
  hb_blob_destroy (blob);

  return face;
}

static void
test_subset_GVAR_header (void)
{
  static const char GVAR_data[] = {
    0, 1, 0, 0,                 /* version 1.0 */
    0, 2,                       /* axisCount */
    0, 0,                       /* sharedTupleCount */
    0, 0, 0, 31,                /* sharedTuplesOffset */
    0, 0, 4,                    /* glyphCount */
    0, 0,                       /* flags */
    0, 0, 0, 31,                /* glyphVariationDataArrayOffset */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* glyphVariationDataOffsets */
  };
  hb_face_t *face = create_GVAR_face (GVAR_data, sizeof (GVAR_data));

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');
  hb_face_t *subset = hb_subset_test_create_subset (
      face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('G','V','A','R'));
  unsigned length;
  const char *data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, sizeof (GVAR_data));
  g_assert_cmpmem (data, length, GVAR_data, sizeof (GVAR_data));
  hb_blob_destroy (blob);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_GVAR_all_axes_pinned (void)
{
  static const char GVAR_data[] = "\0";
  hb_face_t *face = create_GVAR_face (GVAR_data, sizeof (GVAR_data));

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  g_assert_true (hb_subset_input_pin_all_axes_to_default (input, face));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);

  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('G','V','A','R'));
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);

  hb_face_destroy (subset);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}
#endif

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_gvar_noop);
  hb_test_add (test_subset_gvar);
  hb_test_add (test_subset_gvar_retaingids);
#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_subset_GVAR_header);
  hb_test_add (test_subset_GVAR_all_axes_pinned);
#endif

  return hb_test_run ();
}
