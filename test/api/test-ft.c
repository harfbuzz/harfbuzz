/*
 * Copyright © 2022 Red Hat, Inc.
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
 * Author: Matthias Clasen
 */


#include "hb-test.h"

#include "hb-ft.h"

#include FT_FONT_FORMATS_H

static FT_Library ft_library;

static void
init_freetype (void)
{
  FT_Error ft_error;
  if ((ft_error = FT_Init_FreeType (&ft_library)))
    abort ();
}

static void
cleanup_freetype (void)
{
  FT_Done_FreeType (ft_library);
}

static FT_Face
get_ft_face (const char *file)
{
  FT_Face ft_face;

#if GLIB_CHECK_VERSION(2,37,2)
  char* path = g_test_build_filename (G_TEST_DIST, file, NULL);
#else
  char* path = g_strdup (file);
#endif

  FT_Error ft_error;
  if ((ft_error = FT_New_Face (ft_library, path, 0, &ft_face))) {
    g_free (path);
    abort();
  }
  g_free (path);

  if ((ft_error = FT_Set_Char_Size (ft_face, 2000, 1000, 0, 0)))
    abort ();

  return ft_face;
}

static void
test_native_ft_basic (void)
{
  FT_Face ft_face;
  hb_font_t *font;
  FT_Face ft_face2;

  init_freetype ();

  ft_face = get_ft_face ("fonts/adwaita.ttf");

  g_assert_nonnull (ft_face);
  g_assert_nonnull (FT_Get_Font_Format (ft_face));

  font = hb_ft_font_create_referenced (ft_face);

  ft_face2 = hb_ft_font_get_ft_face (font);

  g_assert_true (ft_face2 == ft_face);

  ft_face2 = hb_ft_font_lock_face (font);

  g_assert_true (ft_face2 == ft_face);

  hb_ft_font_unlock_face (font);

  hb_ft_font_set_load_flags (font, FT_LOAD_NO_SCALE | FT_LOAD_NO_AUTOHINT);
  int load_flags = hb_ft_font_get_load_flags (font);

  g_assert_true (load_flags == (FT_LOAD_NO_SCALE | FT_LOAD_NO_AUTOHINT));

  hb_font_destroy (font);

  FT_Done_Face (ft_face);

  cleanup_freetype ();
}

static void
test_native_ft_set_funcs_preserves_load_flags (void)
{
  FT_Face ft_face;
  hb_font_t *font;
  const int requested_load_flags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL;

  init_freetype ();

  ft_face = get_ft_face ("fonts/Cantarell.A.otf");
  g_assert_nonnull (ft_face);

  font = hb_ft_font_create_referenced (ft_face);
  g_assert_nonnull (font);

  hb_ft_font_set_load_flags (font, requested_load_flags);
  hb_ft_font_set_funcs (font);

  g_assert_cmpint (hb_ft_font_get_load_flags (font), ==, requested_load_flags);

  hb_font_destroy (font);
  FT_Done_Face (ft_face);

  cleanup_freetype ();
}

static void
test_native_ft_glyph_name_zero_size_probe (void)
{
  static const char *files[] = {
    "fonts/adwaita.ttf",
    "fonts/SourceSansPro-Regular.otf",
    "fonts/Cantarell.A.otf",
  };
  FT_Face ft_face = NULL;
  hb_font_t *font = NULL;
  hb_bool_t ret;
  hb_codepoint_t glyph = 0;
  char name[64];
  char guard[4] = { 0x7f, 0x7f, 0x7f, 0x7f };
  unsigned int i;

  init_freetype ();

  for (i = 0; i < G_N_ELEMENTS (files); i++)
  {
    unsigned int gid;

    ft_face = get_ft_face (files[i]);
    g_assert_nonnull (ft_face);

    font = hb_ft_font_create_referenced (ft_face);
    g_assert_nonnull (font);
    hb_ft_font_set_funcs (font);

    for (gid = 0; gid < (unsigned int) ft_face->num_glyphs; gid++)
      if (hb_font_get_glyph_name (font, gid, name, sizeof (name)))
      {
        glyph = gid;
        goto found;
      }

    hb_font_destroy (font);
    font = NULL;
    FT_Done_Face (ft_face);
    ft_face = NULL;
  }

  g_test_skip ("No FreeType glyph-name test font available");
  cleanup_freetype ();
  return;

found:

  ret = hb_font_get_glyph_name (font, glyph, name, sizeof (name));
  g_assert_true (ret);
  g_assert_true (*name);

  ret = hb_font_get_glyph_name (font, glyph, NULL, 0);
  g_assert_true (ret);

  ret = hb_font_get_glyph_name (font, glyph, guard, 0);
  g_assert_true (ret);
  g_assert_cmpint (guard[0], ==, 0x7f);
  g_assert_cmpint (guard[1], ==, 0x7f);
  g_assert_cmpint (guard[2], ==, 0x7f);
  g_assert_cmpint (guard[3], ==, 0x7f);

  ret = hb_font_get_glyph_name (font, 0xFFFFFF, NULL, 0);
  g_assert_false (ret);

  hb_font_destroy (font);
  FT_Done_Face (ft_face);

  cleanup_freetype ();
}

/* A minimal custom FT_Stream, so that FT_Open_Face() takes the
 * hb_face_create_for_tables() path in hb_ft_face_create() (ie.
 * ft_face->stream->read != NULL), instead of wrapping FT_Face's
 * mmap'd bytes directly in a shared blob. */
typedef struct
{
  const FT_Byte *data;
  FT_ULong       length;
} memory_stream_data_t;

static unsigned long
memory_stream_read (FT_Stream      stream,
		    unsigned long  offset,
		    unsigned char *buffer,
		    unsigned long  count)
{
  memory_stream_data_t *sd = (memory_stream_data_t *) stream->descriptor.pointer;

  if (offset > sd->length || count > sd->length - offset)
    return count ? 0 : 1;

  if (count)
    memcpy (buffer, sd->data + offset, count);

  return count;
}

static void
memory_stream_close (FT_Stream stream HB_UNUSED)
{
}

static void
test_native_ft_set_funcs_stream_face_no_deadlock (void)
{
  init_freetype ();

#if GLIB_CHECK_VERSION(2,37,2)
  char *path = g_test_build_filename (G_TEST_DIST, "fonts/adwaita.ttf", NULL);
#else
  char *path = g_strdup ("fonts/adwaita.ttf");
#endif

  gchar *contents;
  gsize length;
  GError *error = NULL;
  g_assert_true (g_file_get_contents (path, &contents, &length, &error));
  g_free (path);

  memory_stream_data_t stream_data = { (const FT_Byte *) contents, (FT_ULong) length };

  FT_StreamRec stream_rec;
  memset (&stream_rec, 0, sizeof (stream_rec));
  stream_rec.size = (unsigned long) length;
  stream_rec.descriptor.pointer = &stream_data;
  stream_rec.read = memory_stream_read;
  stream_rec.close = memory_stream_close;

  FT_Open_Args args;
  memset (&args, 0, sizeof (args));
  args.flags = FT_OPEN_STREAM;
  args.stream = &stream_rec;

  FT_Face ft_face;
  FT_Error ft_error = FT_Open_Face (ft_library, &args, 0, &ft_face);
  g_assert_cmpint (ft_error, ==, 0);

  g_assert_cmpint (FT_Set_Char_Size (ft_face, 2000, 1000, 0, 0), ==, 0);

  /* hb_ft_font_create() followed by hb_ft_font_set_funcs() on a face
   * opened from a custom stream used to self-deadlock in
   * hb_font_destroy(); https://github.com/harfbuzz/harfbuzz/issues/6276 */
  hb_font_t *font = hb_ft_font_create (ft_face, NULL);
  g_assert_nonnull (font);
  hb_ft_font_set_funcs (font);

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, "Hello", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, NULL, 0);
  hb_buffer_destroy (buffer);

  hb_font_destroy (font);

  FT_Done_Face (ft_face);
  g_free (contents);

  cleanup_freetype ();
}

static void
test_native_ft_create_invalid_blob_no_leak (void)
{
  /* The error path of hb_ft_face_create_from_blob_or_fail() must release the
   * FT_Library reference it took from reference_ft_library(); otherwise the
   * static library is pinned and leaks. Drive the failing path repeatedly so
   * LeakSanitizer flags any unbalanced reference. */
  static const char junk[] = "this is not a valid font blob at all";

  for (unsigned int i = 0; i < 16; i++)
  {
    hb_blob_t *blob = hb_blob_create (junk, sizeof (junk),
				      HB_MEMORY_MODE_READONLY, NULL, NULL);
    hb_face_t *face = hb_ft_face_create_from_blob_or_fail (blob, 0);
    g_assert_null (face);
    hb_blob_destroy (blob);
  }
}

static gpointer
create_static_ft_faces (gpointer data)
{
  hb_blob_t *blob = (hb_blob_t *) data;

  for (unsigned int i = 0; i < 100; i++)
  {
    hb_face_t *face = hb_ft_face_create_from_blob_or_fail (blob, 0);
    g_assert_nonnull (face);

    hb_font_t *font = hb_font_create (face);
    hb_ft_font_set_funcs (font);

    hb_font_destroy (font);
    hb_face_destroy (face);
  }

  return NULL;
}

static void
test_static_ft_library_multithreaded (void)
{
  hb_blob_t *blob;
  GThread *threads[8];

#if GLIB_CHECK_VERSION(2,37,2)
  char *path = g_test_build_filename (G_TEST_DIST, "fonts/adwaita.ttf", NULL);
#else
  char *path = g_strdup ("fonts/adwaita.ttf");
#endif

  blob = hb_blob_create_from_file_or_fail (path);
  g_assert_nonnull (blob);
  g_free (path);

  for (unsigned int i = 0; i < G_N_ELEMENTS (threads); i++)
    threads[i] = g_thread_new (NULL, create_static_ft_faces, blob);

  for (unsigned int i = 0; i < G_N_ELEMENTS (threads); i++)
    g_thread_join (threads[i]);

  hb_blob_destroy (blob);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_native_ft_basic);
  hb_test_add (test_native_ft_set_funcs_preserves_load_flags);
  hb_test_add (test_native_ft_set_funcs_stream_face_no_deadlock);
  hb_test_add (test_native_ft_glyph_name_zero_size_probe);
  hb_test_add (test_native_ft_create_invalid_blob_no_leak);
  hb_test_add (test_static_ft_library_multithreaded);

  return hb_test_run ();
}
