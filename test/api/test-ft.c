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
test_native_ft_set_funcs_immutable (void)
{
  FT_Face ft_face;
  hb_font_t *font;
  hb_glyph_extents_t extents;

  init_freetype ();

  ft_face = get_ft_face ("fonts/Cantarell.A.otf");
  g_assert_nonnull (ft_face);

  font = hb_ft_font_create_referenced (ft_face);
  g_assert_nonnull (font);

  /* Setters must fail silently on an immutable font; this used to
   * create a new FT_Face, free it, and then use it. */
  hb_font_make_immutable (font);
  hb_ft_font_set_funcs (font);

  g_assert_true (hb_ft_font_get_ft_face (font) == ft_face);
  g_assert_true (hb_font_get_glyph_extents (font, 1, &extents));

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

static unsigned int ft_face_destroy_count;

static void
count_ft_face_destroy (void *data HB_UNUSED)
{
  ft_face_destroy_count++;
}

static void
test_native_ft_create_unsanitizable_font (void)
{
  /* A font that FreeType opens but HarfBuzz fails to sanitize: numTables
   * claims more table records than the file has room for.  hb_face_create()
   * drops the blob then, but the destroy callback handed to hb_ft_face_create()
   * must still not run before the hb_face_t / hb_font_t is destroyed. */
  init_freetype ();

#if GLIB_CHECK_VERSION(2,37,2)
  char *path = g_test_build_filename (G_TEST_DIST, "fonts/Roboto-Regular.abc.ttf", NULL);
#else
  char *path = g_strdup ("fonts/Roboto-Regular.abc.ttf");
#endif

  gchar *contents;
  gsize length;
  GError *error = NULL;
  g_assert_true (g_file_get_contents (path, &contents, &length, &error));
  g_free (path);

  contents[4] = contents[5] = (gchar) 0xFF;

  FT_Face ft_face;
  g_assert_cmpint (FT_New_Memory_Face (ft_library, (const FT_Byte *) contents, length, 0, &ft_face), ==, 0);
  g_assert_cmpint (FT_Set_Char_Size (ft_face, 2000, 1000, 0, 0), ==, 0);

  ft_face_destroy_count = 0;
  hb_face_t *face = hb_ft_face_create (ft_face, count_ft_face_destroy);
  g_assert_cmpuint (ft_face_destroy_count, ==, 0);
  hb_face_destroy (face);
  g_assert_cmpuint (ft_face_destroy_count, ==, 1);

  ft_face_destroy_count = 0;
  hb_font_t *font = hb_ft_font_create (ft_face, count_ft_face_destroy);
  g_assert_cmpuint (ft_face_destroy_count, ==, 0);
  hb_font_destroy (font);
  g_assert_cmpuint (ft_face_destroy_count, ==, 1);

  /* The font holds its own FT_Face reference; releasing ours must be fine. */
  font = hb_ft_font_create_referenced (ft_face);
  FT_Done_Face (ft_face);

  hb_font_extents_t extents;
  hb_codepoint_t glyph;
  g_assert_true (hb_font_get_h_extents (font, &extents));
  g_assert_true (hb_font_get_nominal_glyph (font, 'a', &glyph));

  hb_font_destroy (font);
  g_free (contents);

  cleanup_freetype ();
}

static unsigned int ft_blob_destroy_count;

static void
count_ft_blob_destroy (void *data)
{
  ft_blob_destroy_count++;
  g_free (data);
}

static void
test_native_ft_create_from_blob_table_outlives_face (void)
{
  /* Table blobs of a face created by hb_ft_face_create_from_blob_or_fail()
   * point into the caller's blob data; they must keep that data alive on
   * their own, after both the face and the caller's blob reference are gone. */
#if GLIB_CHECK_VERSION(2,37,2)
  char *path = g_test_build_filename (G_TEST_DIST, "fonts/Roboto-Regular.abc.ttf", NULL);
#else
  char *path = g_strdup ("fonts/Roboto-Regular.abc.ttf");
#endif

  gchar *contents;
  gsize length;
  GError *error = NULL;
  g_assert_true (g_file_get_contents (path, &contents, &length, &error));
  g_free (path);

  ft_blob_destroy_count = 0;
  hb_blob_t *blob = hb_blob_create (contents, length, HB_MEMORY_MODE_READONLY,
				    contents, count_ft_blob_destroy);
  hb_face_t *face = hb_ft_face_create_from_blob_or_fail (blob, 0);
  g_assert_nonnull (face);
  hb_blob_destroy (blob);

  hb_blob_t *head = hb_face_reference_table (face, HB_TAG ('h','e','a','d'));
  hb_face_destroy (face);
  g_assert_cmpuint (ft_blob_destroy_count, ==, 0);

  unsigned int head_length;
  const char *head_data = hb_blob_get_data (head, &head_length);
  g_assert_cmpuint (head_length, ==, 54);
  /* head.magicNumber */
  g_assert_cmpmem (head_data + 12, 4, "\x5F\x0F\x3C\xF5", 4);

  hb_blob_destroy (head);
  g_assert_cmpuint (ft_blob_destroy_count, ==, 1);
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

#if (FREETYPE_MAJOR*10000 + FREETYPE_MINOR*100 + FREETYPE_PATCH) >= 21300
typedef struct
{
  hb_font_t *font;
  hb_paint_funcs_t *funcs;
  hb_codepoint_t glyph;
  unsigned int images;
} bitmap_paint_data_t;

static hb_bool_t
bitmap_paint_image (hb_paint_funcs_t *funcs HB_UNUSED,
                    void *paint_data,
                    hb_blob_t *blob,
                    unsigned int width,
                    unsigned int height,
                    hb_tag_t format,
                    float slant HB_UNUSED,
                    hb_glyph_extents_t *extents HB_UNUSED,
                    void *user_data HB_UNUSED)
{
  bitmap_paint_data_t *data = paint_data;
  hb_glyph_extents_t other_extents;

  g_assert_cmpuint (format, ==, HB_PAINT_IMAGE_FORMAT_BGRA);
  g_assert_cmpuint (width, >, 0);
  g_assert_cmpuint (height, >, 0);
  g_assert_cmpuint (hb_blob_get_length (blob), ==, width * height * 4);

  /* Image callbacks must be able to query the same font. */
  g_assert_true (hb_font_get_glyph_extents (data->font, 3 - data->glyph, &other_extents));
  data->images++;
  return TRUE;
}

static gpointer
paint_bitmaps (gpointer user_data)
{
  bitmap_paint_data_t *data = user_data;

  for (unsigned int i = 0; i < 100; i++)
    g_assert_true (hb_font_paint_glyph_or_fail (data->font, data->glyph,
                                              data->funcs, data, 0,
                                              HB_COLOR (0, 0, 0, 255)));
  return NULL;
}
#endif

static void
test_ft_paint_bitmap_multithreaded (void)
{
#if (FREETYPE_MAJOR*10000 + FREETYPE_MINOR*100 + FREETYPE_PATCH) >= 21300
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();
  GThread *threads[4];
  bitmap_paint_data_t data[4];

  hb_ft_font_set_funcs (font);
  FT_Face ft_face = hb_ft_font_lock_face (font);
  hb_bool_t have_bitmaps = TRUE;
  for (unsigned int glyph = 1; glyph <= 2; glyph++)
    if (FT_Load_Glyph (ft_face, glyph, FT_LOAD_COLOR) ||
        ft_face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA)
      have_bitmaps = FALSE;
  hb_ft_font_unlock_face (font);
  if (!have_bitmaps)
  {
    g_test_skip ("FreeType color bitmap support not present");
    goto out;
  }

  hb_font_make_immutable (font);
  hb_paint_funcs_set_image_func (funcs, bitmap_paint_image, NULL, NULL);
  hb_paint_funcs_make_immutable (funcs);

  /* The two color bitmap glyphs share a FreeType glyph slot. */
  for (unsigned int i = 0; i < G_N_ELEMENTS (threads); i++)
  {
    data[i] = (bitmap_paint_data_t) {font, funcs, 1 + i % 2, 0};
    threads[i] = g_thread_new (NULL, paint_bitmaps, &data[i]);
  }

  for (unsigned int i = 0; i < G_N_ELEMENTS (threads); i++)
  {
    g_thread_join (threads[i]);
    g_assert_cmpuint (data[i].images, ==, 100);
  }

out:
  hb_paint_funcs_destroy (funcs);
  hb_font_destroy (font);
  hb_face_destroy (face);
#else
  g_test_skip ("FreeType bitmap paint support not present");
#endif
}


int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_native_ft_basic);
  hb_test_add (test_ft_paint_bitmap_multithreaded);
  hb_test_add (test_native_ft_set_funcs_preserves_load_flags);
  hb_test_add (test_native_ft_set_funcs_immutable);
  hb_test_add (test_native_ft_set_funcs_stream_face_no_deadlock);
  hb_test_add (test_native_ft_glyph_name_zero_size_probe);
  hb_test_add (test_native_ft_create_invalid_blob_no_leak);
  hb_test_add (test_native_ft_create_unsanitizable_font);
  hb_test_add (test_native_ft_create_from_blob_table_outlives_face);
  hb_test_add (test_static_ft_library_multithreaded);

  return hb_test_run ();
}
