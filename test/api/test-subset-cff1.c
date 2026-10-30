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

/* Unit tests for CFF subsetting */

static void
test_subset_cff1_noop (void)
{
  hb_face_t *face_abc = hb_test_open_font_file("fonts/SourceSansPro-Regular.abc.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
}

static void
test_subset_cff1 (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansPro-Regular.ac.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff1_strip_hints (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansPro-Regular.ac.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', ' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff1_desubr (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansPro-Regular.ac.nosubrs.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff1_desubr_strip_hints (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansPro-Regular.ac.nosubrs.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NO_HINTING | HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', ' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff1_j (void)
{
  hb_face_t *face_41_3041_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  hb_face_t *face_41_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_41_3041_4c2e_subset;
  hb_set_add (codepoints, 0x41);
  hb_set_add (codepoints, 0x4C2E);
  face_41_3041_4c2e_subset = hb_subset_test_create_subset (face_41_3041_4c2e, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_41_3041_4c2e_subset);
  hb_face_destroy (face_41_3041_4c2e);
  hb_face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_j_strip_hints (void)
{
  hb_face_t *face_41_3041_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  hb_face_t *face_41_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_41_3041_4c2e_subset;
  hb_subset_input_t *input;
  hb_set_add (codepoints, 0x41);
  hb_set_add (codepoints, 0x4C2E);
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_41_3041_4c2e_subset = hb_subset_test_create_subset (face_41_3041_4c2e, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_41_3041_4c2e_subset);
  hb_face_destroy (face_41_3041_4c2e);
  hb_face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_j_desubr (void)
{
  hb_face_t *face_41_3041_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  hb_face_t *face_41_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.nosubrs.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_41_3041_4c2e_subset;
  hb_subset_input_t *input;
  hb_set_add (codepoints, 0x41);
  hb_set_add (codepoints, 0x4C2E);
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_41_3041_4c2e_subset = hb_subset_test_create_subset (face_41_3041_4c2e, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_41_3041_4c2e_subset);
  hb_face_destroy (face_41_3041_4c2e);
  hb_face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_j_desubr_strip_hints (void)
{
  hb_face_t *face_41_3041_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  hb_face_t *face_41_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.nosubrs.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_41_3041_4c2e_subset;
  hb_subset_input_t *input;
  hb_set_add (codepoints, 0x41);
  hb_set_add (codepoints, 0x4C2E);
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NO_HINTING | HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_41_3041_4c2e_subset = hb_subset_test_create_subset (face_41_3041_4c2e, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_41_3041_4c2e_subset);
  hb_face_destroy (face_41_3041_4c2e);
  hb_face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_expert (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_expert.otf");
  hb_face_t *face_subset = hb_test_open_font_file ("fonts/cff1_expert.2D,F6E9,FB00.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_test;
  hb_set_add (codepoints, 0x2D);
  hb_set_add (codepoints, 0xF6E9);
  hb_set_add (codepoints, 0xFB00);
  face_test = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_subset, face_test, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_test);
  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cff1_seac (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_seac.otf");
  hb_face_t *face_subset = hb_test_open_font_file ("fonts/cff1_seac.C0.otf");
  hb_face_t *face_test;

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 0xC0);  /* Agrave */
  face_test = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_subset, face_test, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_test);
  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cff1_cid_seac (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_cid_seac.otf");
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add (hb_subset_input_glyph_set (input), 3);

  hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
  g_assert_nonnull (plan);
  const hb_map_t *mapping = hb_subset_plan_old_to_new_glyph_mapping (plan);
  /* The CID charset entries 34 and 125 are not seac components. */
  g_assert_cmpuint (hb_map_get_population (mapping), ==, 2);
  g_assert_true (hb_map_has (mapping, 0));
  g_assert_true (hb_map_has (mapping, 3));
  g_assert_false (hb_map_has (mapping, 1));
  g_assert_false (hb_map_has (mapping, 2));

  hb_subset_plan_destroy (plan);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

static void
test_subset_cff1_seac_precheck (void)
{
  const char *fonts[] = {
    "fonts/cff1_seac_dotsection.otf",
    "fonts/cff1_seac_subr_endchar.otf",
    "fonts/cff1_seac_subr_operands.otf",
  };

  for (unsigned int i = 0; i < G_N_ELEMENTS (fonts); i++)
  {
    hb_face_t *face = hb_test_open_font_file (fonts[i]);
    hb_set_t *codepoints = hb_set_create ();
    hb_set_add (codepoints, 0xC1);  /* Aacute */
    hb_face_t *subset = hb_subset_test_create_subset (face,
                                                       hb_subset_test_create_input (codepoints));
    hb_set_destroy (codepoints);

    /* .notdef, Aacute and both seac components. */
    g_assert_cmpuint (hb_face_get_glyph_count (subset), ==, 4);

    hb_face_destroy (subset);
    hb_face_destroy (face);
  }
}

static void
test_subset_cff1_encoding_supplement (void)
{
  /* Dropping "first" renumbers the custom glyph-name SIDs; the Encoding
   * supplement (code 98 -> "second") must follow them. */
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_encoding_supplement.otf");
  hb_face_t *face_subset = hb_test_open_font_file ("fonts/cff1_encoding_supplement.42,43.otf");
  hb_face_t *face_test;

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 0x42);  /* B */
  hb_set_add (codepoints, 0x43);  /* C */
  face_test = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_subset, face_test, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_test);
  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cff1_dotsection (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_dotsect.otf");
  hb_face_t *face_subset = hb_test_open_font_file ("fonts/cff1_dotsect.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_test;
  hb_set_add (codepoints, 0x69);  /* i */
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_test = hb_subset_test_create_subset (face, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_subset, face_test, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_test);
  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cff1_subr_loop (void)
{
  /* CID-keyed font whose parsed subrs, shared between font dicts, form a
   * call cycle that the charstrings never execute. Dropping hints used to
   * recurse over it without end. */
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_subr_loop.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);
  hb_set_destroy (codepoints);

  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cff1_retaingids (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansPro-Regular.ac.retaingids.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff1_encoding_many_codes_retaingids (void)
{
  /* A format 1 encoding gives codes to 768 glyphs; with retain-gids
   * the subset keeps more than 255 of them, too many for format 0. */
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_encoding_many_codes.otf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/cff1_encoding_many_codes.300.retaingids.otf");

  hb_set_t *glyphs = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_subset;
  hb_set_add (glyphs, 300);
  input = hb_subset_test_create_input_from_glyphs (glyphs);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_subset = hb_subset_test_create_subset (face, input);
  hb_set_destroy (glyphs);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_subset);
  hb_face_destroy (face);
  hb_face_destroy (face_expected);
}

static void
test_subset_cff1_encoding_range_overflow (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_encoding_many_codes.otf");
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t *glyphs = hb_subset_input_glyph_set (input);

  /* Keep 256 codes in 255 ranges, the format 1 range limit. */
  for (unsigned i = 1; i < 511; i += 2)
    hb_set_add (glyphs, i);
  hb_set_add (glyphs, 510);
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  g_assert_cmpuint (hb_face_get_glyph_count (subset), ==, 257);
  hb_blob_t *cff = hb_face_reference_table (subset, HB_TAG ('C','F','F',' '));
  g_assert_cmpuint (hb_blob_get_length (cff), >, 0);
  hb_blob_destroy (cff);
  hb_face_destroy (subset);

  /* A 256th range must fail instead of silently dropping the CFF table. */
  hb_set_del (glyphs, 510);
  hb_set_add (glyphs, 511);
  subset = hb_subset_or_fail (face, input);
  g_assert_null (subset);

  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

static void
test_subset_cff1_j_retaingids (void)
{
  hb_face_t *face_41_3041_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  hb_face_t *face_41_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.retaingids.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_41_3041_4c2e_subset;
  hb_set_add (codepoints, 0x41);
  hb_set_add (codepoints, 0x4C2E);
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_41_3041_4c2e_subset = hb_subset_test_create_subset (face_41_3041_4c2e, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_41_3041_4c2e_subset);
  hb_face_destroy (face_41_3041_4c2e);
  hb_face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_identity_charset (void)
{
  /* The flag has no effect on a name-keyed font, so this is the same output
   * as test_subset_cff1(). */
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansPro-Regular.ac.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_CFF_IDENTITY_CHARSET);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff1_j_identity_charset (void)
{
  hb_face_t *face_41_3041_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  hb_face_t *face_41_4c2e = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.identity.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_41_3041_4c2e_subset;
  hb_set_add (codepoints, 0x41);
  hb_set_add (codepoints, 0x4C2E);
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_CFF_IDENTITY_CHARSET);
  face_41_3041_4c2e_subset = hb_subset_test_create_subset (face_41_3041_4c2e, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_41_3041_4c2e_subset);
  hb_face_destroy (face_41_3041_4c2e);
  hb_face_destroy (face_41_4c2e);
}

/* Subsets a CID-keyed font of 16 Font DICTs, 1000 empty local subroutines and
 * two empty glyphs, and returns whether the CFF table was subsetted. */
static hb_bool_t
subset_cff1_many_font_dicts (hb_bool_t share_private_dict)
{
  static const unsigned char maxp[] = {0x00, 0x00, 0x50, 0x00, 0x00, 0x02};
  static const unsigned char prefix[] = {
    0x01, 0x00, 0x04, 0x04,			/*  0: header */
    0x00, 0x01, 0x01, 0x01, 0x02, 'X',		/*  4: Name INDEX */
    0x00, 0x01, 0x01, 0x01, 0x18,		/* 10: Top DICT INDEX */
    0x8b, 0x8b, 0x8b, 0x0c, 0x1e,		/*     ROS */
    0x1c, 0x00, 0x2a, 0x0f,			/*     charset */
    0x1c, 0x00, 0x32, 0x11,			/*     CharStrings */
    0x1c, 0x00, 0x3e, 0x0c, 0x24,		/*     FDArray */
    0x1c, 0x00, 0x2f, 0x0c, 0x25,		/*     FDSelect */
    0x00, 0x00,					/* 38: String INDEX */
    0x00, 0x00,					/* 40: Global Subr INDEX */
    0x02, 0x00, 0x01, 0x00, 0x00,		/* 42: charset */
    0x00, 0x00, 0x00,				/* 47: FDSelect */
    0x00, 0x02, 0x01, 0x01, 0x02, 0x03, 0x0e, 0x0e, /* 50: CharStrings INDEX */
    0x1c, 0x00, 0x68, 0x13,			/* 58: Private DICT: Subrs */
    0x00, 0x10, 0x01,				/* 62: FDArray INDEX */
  };
  unsigned char cff[1166], *p = cff;

  memcpy (p, prefix, sizeof (prefix));
  p += sizeof (prefix);
  for (unsigned i = 0; i <= 16; i++)
    *p++ = 1 + 5 * i;
  for (unsigned i = 0; i < 16; i++)
  {
    /* Private: size and offset. */
    *p++ = !i || share_private_dict ? 0x8f : 0x8b;
    *p++ = 0x1c; *p++ = 0x00; *p++ = 0x3a;
    *p++ = 0x12;
  }
  /* 162: Local Subr INDEX */
  *p++ = 0x03; *p++ = 0xe8; *p++ = 0x01;
  memset (p, 0x01, 1001);
  p += 1001;
  g_assert_true (p == cff + sizeof (cff));

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create ((const char *) maxp, sizeof (maxp),
				    HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('m','a','x','p'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create ((const char *) cff, sizeof (cff),
			 HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('C','F','F',' '), blob);
  hb_blob_destroy (blob);

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, 1);
  hb_face_t *subset = hb_subset_or_fail (face, input);
  hb_subset_input_destroy (input);

  /* Whether the subset was produced with a (non-empty) CFF table. */
  hb_bool_t ret = 0;
  if (subset)
  {
    blob = hb_face_reference_table (subset, HB_TAG ('C','F','F',' '));
    ret = hb_blob_get_length (blob) != 0;
    hb_blob_destroy (blob);
    hb_face_destroy (subset);
  }

  hb_face_destroy (face);
  return ret;
}

static void
test_subset_cff1_shared_local_subrs (void)
{
  /* Subroutines bounded by the table subset fine. */
  g_assert_true (subset_cff1_many_font_dicts (false));
  /* With every Font DICT sharing the one Subrs INDEX, each would get its
   * own parsed copy, out of proportion to the table; the subset fails
   * instead (a planning failure, as for other unsupported CFF input). */
  g_assert_true (!subset_cff1_many_font_dicts (true));
}

static hb_face_t *
create_shared_subrs_face (unsigned num_glyphs, hb_bool_t endchar_in_subr)
{
  /* Global subrs 1, 2 and 3 call the previous one ten times each, so a call
   * of subr 3 executes a little under HB_CFF_MAX_OPS operators and operands.
   * Every glyph is such a call. */
  static const unsigned char prefix[] = {
    1, 0, 4, 4, /* header */
    0, 1, 1, 1, 2, 'A', /* Name INDEX */
    0, 1, 1, 1, 24, /* Top DICT INDEX */
    29, 0, 0, 1, 62, 15, /* charset */
    29, 0, 0, 1, 67, 17, /* CharStrings */
    29, 0, 0, 0, 0, 29, 0, 0, 0, 4, 18, /* Private, empty */
    0, 0, /* String INDEX */
    0, 5, 2, 0, 1, 0, 198, 0, 219, 0, 240, 1, 5, 1, 8, /* Global Subr INDEX */
  };
  const unsigned char rlineto = 5, return_ = 11, endchar = 14, callgsubr = 29, bias = 139 - 107;
  unsigned charstring_length = endchar_in_subr ? 2 : 3;
  unsigned length = sizeof (prefix) + 263 + 5 + 3 + 2 * (num_glyphs + 1) + charstring_length * num_glyphs;
  unsigned char *cff = (unsigned char *) calloc (length, 1);
  g_assert_nonnull (cff);
  unsigned char *p = cff;

  memcpy (p, prefix, sizeof (prefix));
  p += sizeof (prefix);
  for (unsigned i = 0; i < 4; i++)
  {
    memset (p, 140, 48);
    p += 48;
    *p++ = rlineto;
  }
  *p++ = return_;
  for (unsigned subr = 1; subr <= 3; subr++)
  {
    for (unsigned i = 0; i < 10; i++)
    {
      *p++ = bias + subr - 1;
      *p++ = callgsubr;
    }
    *p++ = return_;
  }
  *p++ = bias + 3;
  *p++ = callgsubr;
  *p++ = endchar;

  /* charset */
  *p++ = 2;
  *p++ = 0;
  *p++ = 1;
  *p++ = (num_glyphs - 2) >> 8;
  *p++ = (num_glyphs - 2) & 0xFF;

  /* CharStrings INDEX */
  *p++ = num_glyphs >> 8;
  *p++ = num_glyphs & 0xFF;
  *p++ = 2;
  for (unsigned i = 0; i <= num_glyphs; i++)
  {
    *p++ = (1 + charstring_length * i) >> 8;
    *p++ = (1 + charstring_length * i) & 0xFF;
  }
  for (unsigned i = 0; i < num_glyphs; i++)
  {
    *p++ = bias + (endchar_in_subr ? 4 : 3);
    *p++ = callgsubr;
    if (!endchar_in_subr)
      *p++ = endchar;
  }
  g_assert_cmpuint (p - cff, ==, length);

  char maxp[] = {0, 0, 0x50, 0, (char) (num_glyphs >> 8), (char) (num_glyphs & 0xFF)};
  hb_face_t *builder = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (builder, "maxp", maxp);
  hb_blob_t *blob = hb_blob_create_or_fail ((const char *) cff, length,
					    HB_MEMORY_MODE_READONLY, cff, free);
  g_assert_nonnull (blob);
  hb_face_builder_add_table (builder, HB_TAG ('C','F','F',' '), blob);
  hb_blob_destroy (blob);

  /* maxp does not outlive this function. */
  blob = hb_face_reference_blob (builder);
  hb_face_destroy (builder);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  return face;
}

static void
test_subset_cff1_shared_subrs (void)
{
  /* The work is limited over all glyphs, not only per glyph.  With endchar
   * in the glyph the seac closure interprets the charstrings as well. */
  for (unsigned endchar_in_subr = 0; endchar_in_subr < 2; endchar_in_subr++)
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_subset_input_keep_everything (input);

    hb_face_t *face = create_shared_subrs_face (4, endchar_in_subr);
    hb_face_t *subset = hb_subset_or_fail (face, input);
    g_assert_nonnull (subset);
    hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('C','F','F',' '));
    g_assert_cmpuint (hb_blob_get_length (blob), >, 0);
    hb_blob_destroy (blob);
    hb_face_destroy (subset);
    hb_face_destroy (face);

    /* Enough of them exhaust the shared work budget; the subset fails
     * instead of interpreting num_glyphs x HB_CFF_MAX_OPS, and so does the
     * dependency builder that walks the same seac closure. */
    face = create_shared_subrs_face (100, endchar_in_subr);
    subset = hb_subset_or_fail (face, input);
    g_assert_null (subset);
#ifndef HB_NO_SUBSET_DEPEND
    g_assert_null (hb_subset_depend_from_face_or_fail (face));
#endif
    hb_face_destroy (face);

    hb_subset_input_destroy (input);
  }
}

static void
test_subset_cff1_preprocess_glyph_map_holes (void)
{
  const char *fonts[] = {
    "fonts/SourceSansPro-Regular.abc.otf",
    "fonts/SourceHanSans-Regular.41,4C2E.otf",
  };
  for (unsigned i = 0; i < G_N_ELEMENTS (fonts); i++)
  {
    hb_face_t *face = hb_test_open_font_file (fonts[i]);
    hb_face_t *preprocessed = hb_subset_preprocess (face);
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add (hb_subset_input_glyph_set (input), 1);
    hb_set_add (hb_subset_input_glyph_set (input), 2);
    hb_map_t *mapping = hb_subset_input_old_to_new_glyph_mapping (input);
    hb_map_set (mapping, 1, 7);
    hb_map_set (mapping, 2, 20);

    hb_face_t *expected = hb_subset_or_fail (face, input);
    hb_face_t *actual = hb_subset_or_fail (preprocessed, input);
    g_assert_nonnull (expected);
    g_assert_nonnull (actual);
    hb_subset_test_check (expected, actual, HB_TAG ('C','F','F',' '));

    hb_face_destroy (actual);
    hb_face_destroy (expected);
    hb_subset_input_destroy (input);
    hb_face_destroy (preprocessed);
    hb_face_destroy (face);
  }
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_cff1_iftb_requirements (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_long_off = hb_test_open_font_file ("fonts/SourceSansPro-Regular.abc.long_off.otf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_IFTB_REQUIREMENTS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_long_off, face_abc_subset, HB_TAG ('C','F','F', ' '));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_long_off);

}
#endif


/* Caller-dependent subroutine numbers cannot be remapped at a shared offset. */
static void
test_subset_cff1_caller_supplied_subr (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff1_caller_subr.otf");
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
test_subset_cff1_index_count_overflow (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.otf");
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add (hb_subset_input_glyph_set (input), 1);
  /* The output CharStrings count must fit in a CFF1 INDEX. */
  hb_map_set (hb_subset_input_old_to_new_glyph_mapping (input), 1, 70000);

  g_assert_null (hb_subset_or_fail (face, input));

  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
  hb_test_add (test_subset_cff1_index_count_overflow);

  hb_test_add (test_subset_cff1_caller_supplied_subr);

  hb_test_add (test_subset_cff1_noop);
  hb_test_add (test_subset_cff1);
  hb_test_add (test_subset_cff1_strip_hints);
  hb_test_add (test_subset_cff1_desubr);
  hb_test_add (test_subset_cff1_desubr_strip_hints);
  hb_test_add (test_subset_cff1_j);
  hb_test_add (test_subset_cff1_j_strip_hints);
  hb_test_add (test_subset_cff1_j_desubr);
  hb_test_add (test_subset_cff1_j_desubr_strip_hints);
  hb_test_add (test_subset_cff1_expert);
  hb_test_add (test_subset_cff1_seac);
  hb_test_add (test_subset_cff1_cid_seac);
  hb_test_add (test_subset_cff1_seac_precheck);
  hb_test_add (test_subset_cff1_encoding_supplement);
  hb_test_add (test_subset_cff1_dotsection);
  hb_test_add (test_subset_cff1_subr_loop);
  hb_test_add (test_subset_cff1_retaingids);
  hb_test_add (test_subset_cff1_j_retaingids);
  hb_test_add (test_subset_cff1_encoding_many_codes_retaingids);
  hb_test_add (test_subset_cff1_encoding_range_overflow);
  hb_test_add (test_subset_cff1_identity_charset);
  hb_test_add (test_subset_cff1_j_identity_charset);
  hb_test_add (test_subset_cff1_shared_local_subrs);
  hb_test_add (test_subset_cff1_shared_subrs);
  hb_test_add (test_subset_cff1_preprocess_glyph_map_holes);

#ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_subset_cff1_iftb_requirements);
#endif

  return hb_test_run ();
}
