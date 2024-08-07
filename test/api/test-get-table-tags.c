/*
 * Copyright © 2024  Google, Inc.
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
 * Author(s): Behdad Esfahbod
 */

#include "hb-test.h"

#include <hb.h>
#ifdef HAVE_FREETYPE
#include <hb-ft.h>
#endif
#ifdef HAVE_CORETEXT
#include "hb-coretext.h"
#endif

static void
_test_get_table_tags (hb_face_t *face)
{
  g_assert_cmpuint (15u, ==, hb_face_get_table_tags (face, 2, NULL, NULL));

  unsigned count;
  hb_tag_t tags[3];

  count = sizeof (tags) / sizeof (tags[0]);
  g_assert_cmpuint (15u, ==, hb_face_get_table_tags (face, 2, &count, tags));

  g_assert_cmpuint (3u, ==, count);
  g_assert_cmpuint (tags[0], ==, HB_TAG ('c', 'v', 't', ' '));
  g_assert_cmpuint (tags[1], ==, HB_TAG ('f', 'p', 'g', 'm'));
  g_assert_cmpuint (tags[2], ==, HB_TAG ('g', 'a', 's', 'p'));

  count = sizeof (tags) / sizeof (tags[0]);
  g_assert_cmpuint (15u, ==, hb_face_get_table_tags (face, 14, &count, tags));
  g_assert_cmpuint (1u, ==, count);
  g_assert_cmpuint (tags[0], ==, HB_TAG ('p', 'r', 'e', 'p'));
}

static void
test_get_table_tags_default (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  _test_get_table_tags (face);

  hb_face_destroy (face);
}

static void
test_get_table_tags_builder (void)
{
  hb_face_t *source = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_face_t *face = hb_face_builder_create ();

  hb_tag_t tags[4];
  unsigned total_count = hb_face_get_table_tags (source, 0, NULL, NULL);
  for (unsigned start_offset = 0;
       start_offset < total_count;
       start_offset += sizeof (tags) / sizeof (tags[0]))
  {
    unsigned count = sizeof (tags) / sizeof (tags[0]);
    unsigned new_total_count = hb_face_get_table_tags (source, start_offset, &count, tags);
    g_assert_cmpuint (total_count, ==, new_total_count);

    for (unsigned i = 0; i < count; i++)
    {
      hb_blob_t *blob = hb_face_reference_table (source, tags[i]);
      hb_face_builder_add_table (face, tags[i], blob);
      hb_blob_destroy (blob);
    }
  }

  hb_face_destroy (source);

  _test_get_table_tags (face);

  hb_face_destroy (face);
}

#ifdef HAVE_FREETYPE
static void
test_get_table_tags_ft (void)
{
  hb_face_t *source = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_blob_t *blob = hb_face_reference_blob (source);
  hb_face_destroy (source);

  FT_Error error;

  FT_Library ft_library;
  error = FT_Init_FreeType (&ft_library);
  g_assert_cmpint (0, ==, error);

  FT_Face ft_face;
  error = FT_New_Memory_Face (ft_library,
			      (const FT_Byte *) hb_blob_get_data (blob, NULL),
			      hb_blob_get_length (blob),
			      0,
			      &ft_face);
  g_assert_cmpint (0, ==, error);

  hb_face_t *face = hb_ft_face_create (ft_face, NULL);

  _test_get_table_tags (face);

  hb_face_destroy (face);
  FT_Done_Face (ft_face);
  FT_Done_FreeType (ft_library);
  hb_blob_destroy (blob);
}
#endif

#ifdef HAVE_CORETEXT
static void
test_get_table_tags_ct (void)
{
  hb_face_t *source = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_blob_t *blob = hb_face_reference_blob (source);
  hb_face_destroy (source);

  CGDataProviderRef provider = CGDataProviderCreateWithData (blob,
							     hb_blob_get_data (blob, NULL),
							     hb_blob_get_length (blob),
							     NULL);
  assert (provider);

  CGFontRef cg_font = CGFontCreateWithDataProvider (provider);
  assert (cg_font);
  CGDataProviderRelease (provider);

  hb_face_t *face = hb_coretext_face_create (cg_font);

  _test_get_table_tags (face);

  hb_face_destroy (face);
  CGFontRelease (cg_font);
  hb_blob_destroy (blob);
}
#endif

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_get_table_tags_default);
  hb_test_add (test_get_table_tags_builder);
#ifdef HAVE_FREETYPE
  hb_test_add (test_get_table_tags_ft);
#endif
#ifdef HAVE_CORETEXT
  hb_test_add (test_get_table_tags_ct);
#endif

  return hb_test_run();
}
