/*
 * Copyright © 2018 Adobe Systems Incorporated.
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
  hb_face_t *face_abc = hb_subset_test_open_font("fonts/SourceSansPro-Regular.abc.otf");

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
  hb_face_t *face_abc = hb_subset_test_open_font ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_subset_test_open_font ("fonts/SourceSansPro-Regular.ac.otf");

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
  hb_face_t *face_abc = hb_subset_test_open_font ("fonts/SourceSansPro-Regular.abc.otf");
  hb_face_t *face_ac = hb_subset_test_open_font ("fonts/SourceSansPro-Regular.ac.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_drop_hints (input, true);
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
  hb_face_t *face_41_3041_4e9d = hb_subset_test_open_font ("fonts/SourceHanSans-Regular.41,3041,4E9D.otf");
  hb_face_t *face_41_4e9d = hb_subset_test_open_font ("fonts/SourceHanSans-Regular.41,4E9D.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_41_3041_4e9d_subset;
  hb_set_add (codepoints, 0x41);
  hb_set_add (codepoints, 0x4E9D);
  face_41_3041_4e9d_subset = hb_subset_test_create_subset (face_41_3041_4e9d, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_41_4e9d, face_41_3041_4e9d_subset, HB_TAG ('C','F','F',' '));

  hb_face_destroy (face_41_3041_4e9d_subset);
  hb_face_destroy (face_41_3041_4e9d);
  hb_face_destroy (face_41_4e9d);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_cff1_noop);
  hb_test_add (test_subset_cff1);
  hb_test_add (test_subset_cff1_strip_hints);
  hb_test_add (test_subset_cff1_j);

  return hb_test_run ();
}
