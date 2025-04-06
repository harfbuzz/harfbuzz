/*
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
 * Author(s): Behdad Esfahbod
 */

#include "hb-test.h"

/* Unit tests for hb-face.h */

#define FONT_FILE "fonts/Roboto-Regular.ac.ttf"
static const char *font_file = NULL;
static unsigned int face_index = 0;

hb_face_t *master_face = NULL;

#define HEAD_TAG HB_TAG ('h', 'e', 'a', 'd')
hb_blob_t *master_head = NULL;

static void
test_face (hb_face_t *face)
{
  if (!face)
  {
    g_test_skip ("Failed to create face");
    return;
  }

  hb_blob_t *head = hb_face_reference_table (face, HEAD_TAG);

  unsigned int length;
  unsigned int master_length;
  const char *data = hb_blob_get_data (head, &length);
  const char *master_data = hb_blob_get_data (master_head, &master_length);
  g_assert_cmpmem (data, length, master_data, master_length);

  hb_blob_destroy (head);
}

static void
test_create_from_file_using (const void *user_data)
{
  const char *loader = user_data;
  hb_face_t *face = hb_face_create_from_file_or_fail_using (font_file, face_index, loader);
  test_face (face);
  hb_face_destroy (face);
}

static void
test_create_from_blob_using (const void *user_data)
{
  const char *loader = user_data;
  hb_blob_t *blob = hb_blob_create_from_file_or_fail (font_file);
  hb_face_t *face = hb_face_create_or_fail_using (blob, face_index, loader);
  hb_blob_destroy (blob);
  test_face (face);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  font_file = FONT_FILE;
  if (argc > 1)
    font_file = argv[1];
  if (argc > 2)
    face_index = atoi (argv[2]);

  master_face = hb_test_open_font_file_with_index (font_file, face_index);
  master_head = hb_face_reference_table (master_face, HEAD_TAG);
  g_assert_true (hb_blob_get_length (master_head) > 0);

  font_file = hb_test_resolve_path (font_file);

  hb_test_add_flavor ("", test_create_from_file_using);
  hb_test_add_flavor ("", test_create_from_blob_using);
  for (const char **loaders = hb_face_list_loaders (); *loaders; loaders++)
  {
    hb_test_add_flavor (*loaders, test_create_from_file_using);
    hb_test_add_flavor (*loaders, test_create_from_blob_using);
  }

  int ret = hb_test_run();

  hb_blob_destroy (master_head);
  hb_face_destroy (master_face);
  g_free ((char *) font_file);

  return ret;
}
