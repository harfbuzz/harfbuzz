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

#ifndef HB_SUBSET_TEST_H
#define HB_SUBSET_TEST_H

#include <stdio.h>

#include "hb-test.h"
#include "hb-subset.h"


HB_BEGIN_DECLS

static inline char *
hb_subset_test_read_file (const char *path,
                          size_t     *length /* OUT */)
{
  FILE *fp = fopen (path, "rb");

  long file_length = 0;
  char *buffer = NULL;
  if (fp && fseek (fp, 0, SEEK_END) == 0)
  {
    file_length = ftell(fp);
    rewind (fp);
  }

  if (file_length > 0)
  {
    buffer = (char *) calloc (file_length + 1, sizeof (char));
    if (buffer && fread (buffer, 1, file_length, fp) == (size_t) file_length)
    {
      *length = file_length;
    }
    else
    {
      free (buffer);
      buffer = NULL;
    }
  }

  if (fp)
    fclose(fp);

  return buffer;
}

static inline hb_face_t *
hb_subset_test_open_font (const char *font_path)
{
#if GLIB_CHECK_VERSION(2,37,2)
  gchar* path = g_test_build_filename(G_TEST_DIST, font_path, NULL);
#else
  gchar* path = g_strdup(fontFile);
#endif

  size_t length;
  char *font_data = hb_subset_test_read_file(path, &length);

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
  g_assert (false);
  return NULL; /* Shut up, compiler! */
}

static inline hb_subset_input_t *
hb_subset_test_create_input(const hb_set_t  *codepoints)
{
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_t * input_codepoints = hb_subset_input_unicode_set (input);
  hb_set_union (input_codepoints, codepoints);
  return input;
}

static inline hb_face_t *
hb_subset_test_create_subset (hb_face_t *source,
                              hb_subset_input_t *input)
{
  hb_subset_profile_t *profile = hb_subset_profile_create();
  hb_face_t *subset = hb_subset (source, profile, input);
  g_assert (subset);

  hb_subset_profile_destroy (profile);
  hb_subset_input_destroy (input);
  return subset;
}

static inline void
hb_subset_test_check (hb_face_t *expected,
                      hb_face_t *actual,
                      hb_tag_t   table)
{
  fprintf(stderr, "compare %c%c%c%c\n", HB_UNTAG(table));
  hb_blob_t *expected_blob = hb_face_reference_table (expected, table);
  hb_blob_t *actual_blob = hb_face_reference_table (actual, table);
  hb_test_assert_blobs_equal (expected_blob, actual_blob);
  hb_blob_destroy (expected_blob);
  hb_blob_destroy (actual_blob);
}


HB_END_DECLS

#endif /* HB_SUBSET_TEST_H */
