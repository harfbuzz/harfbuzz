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

#include <stdbool.h>

#include "hb-test.h"

static char *
read_file (const char *path,
           size_t     *length)
{
  FILE *fp = fopen (path, "rb");

  size_t file_length = 0;
  char *buffer = NULL;
  if (fp && fseek (fp, 0, SEEK_END) == 0) {
    file_length = ftell(fp);
    rewind (fp);
  }

  if (file_length > 0) {
    buffer = (char *) calloc (file_length + 1, sizeof(char));
    if (fread (buffer, 1, file_length, fp) == file_length) {
      *length = file_length;
    } else {
      free (buffer);
      buffer = NULL;
    }
  }

  if (fp)
    fclose(fp);

  return buffer;
}

static hb_face_t *
open_font (const char *font_path)
{
#if GLIB_CHECK_VERSION(2,37,2)
  gchar* path = g_test_build_filename(G_TEST_DIST, font_path, NULL);
#else
  gchar* path = g_strdup(fontFile);
#endif

  size_t length;
  char *font_data = read_file(path, &length);

  if (font_data != NULL) {
    hb_blob_t *blob = hb_blob_create (font_data,
                                      length,
                                      HB_MEMORY_MODE_READONLY,
                                      font_data,
                                      free);
    hb_face_t *face = hb_face_create (blob, 0);
    hb_blob_destroy (blob);
    return face;
  }

  return NULL;
}


/* Unit tests for hb-subset-glyf.h */

static void
test_subset_glyf (void)
{
  hb_face_t *face_abc = open_font("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face_ac = open_font("fonts/Roboto-Regular.ac.ttf");
  hb_face_t *face_abc_subset;
  hb_blob_t *glyf_expected_blob;
  hb_blob_t *glyf_actual_blob;
  hb_set_t *codepoints = hb_set_create();
  hb_subset_profile_t *profile = hb_subset_profile_create();
  hb_subset_input_t *input = hb_subset_input_create (codepoints);

  g_assert (face_abc);
  g_assert (face_ac);

  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 99);

  face_abc_subset = hb_subset (face_abc, profile, input);
  g_assert (face_abc_subset);

  glyf_expected_blob = hb_face_reference_table (face_ac, HB_TAG('g','l','y','f'));
  glyf_actual_blob = hb_face_reference_table (face_abc_subset, HB_TAG('g','l','y','f'));

  // TODO(grieger): Uncomment below once this actually works.
  //int expected_length, actual_length;
  // g_assert_cmpmem(hb_blob_get_data (glyf_expected_blob, &expected_length), expected_length,
  //   hb_blob_get_data (glyf_actual_blob, &actual_length), actual_length);

  hb_blob_destroy (glyf_actual_blob);
  hb_blob_destroy (glyf_expected_blob);

  hb_subset_input_destroy (input);
  hb_subset_profile_destroy (profile);
  hb_set_destroy (codepoints);
  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_glyf);

  return hb_test_run();
}
