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
static const char *font_file;

hb_face_t *master_face = NULL;

#define HEAD_TAG HB_TAG ('h', 'e', 'a', 'd')
hb_blob_t *master_head = NULL;

static void
test_face (hb_face_t *face)
{
  g_assert (face);

  hb_blob_t *head = hb_face_reference_table (face, HEAD_TAG);

  unsigned int length = hb_blob_get_length (head);
  unsigned int master_length = hb_blob_get_length (master_head);
  g_assert_cmpuint (length, ==, master_length);

  const char *data = hb_blob_get_data (head, NULL);
  const char *master_data = hb_blob_get_data (master_head, NULL);
  g_assert (memcmp (data, master_data, length) == 0);

  hb_blob_destroy (head);
}

static void
test_create_from_file_using (const char *loader)
{
  hb_face_t *face = hb_face_create_from_file_or_fail_using (font_file, 0, loader);
  test_face (face);
  hb_face_destroy (face);
}

static void
test_create_from_file (void)
{
  test_create_from_file_using (NULL);
  const char **loaders = hb_face_list_loaders ();
  for (unsigned i = 0; loaders[i]; i++)
    test_create_from_file_using (loaders[i]);
}

static void
test_create_from_blob_using (const char *loader)
{
  hb_blob_t *blob = hb_blob_create_from_file_or_fail (font_file);
  hb_face_t *face = hb_face_create_or_fail_using (blob, 0, loader);
  hb_blob_destroy (blob);
  test_face (face);
  hb_face_destroy (face);
}

static void
test_create_from_blob (void)
{
  test_create_from_file_using (NULL);
  const char **loaders = hb_face_list_loaders ();
  for (unsigned i = 0; loaders[i]; i++)
    test_create_from_blob_using (loaders[i]);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  font_file = hb_test_resolve_path (FONT_FILE);
  master_face = hb_test_open_font_file (FONT_FILE);
  master_head = hb_face_reference_table (master_face, HEAD_TAG);
  g_assert (hb_blob_get_length (master_head) > 0);

  hb_test_add (test_create_from_file);
  hb_test_add (test_create_from_blob);

  int ret = hb_test_run();

  hb_blob_destroy (master_head);
  hb_face_destroy (master_face);
  g_free ((char *) font_file);

  return ret;
}
