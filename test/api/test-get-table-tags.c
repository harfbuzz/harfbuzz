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
#include <string.h>
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

  /* A count that overstates the array, so much that adding the offset
   * to it wraps, is clamped to what is left like any other. */
  hb_tag_t rest[15];
  count = (unsigned) -1;
  g_assert_cmpuint (15u, ==, hb_face_get_table_tags (face, 2, &count, rest));
  g_assert_cmpuint (13u, ==, count);
  g_assert_cmpuint (rest[0], ==, HB_TAG ('c', 'v', 't', ' '));
  g_assert_cmpuint (rest[12], ==, HB_TAG ('p', 'r', 'e', 'p'));
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

static void
test_get_table_tags_builder_limit (void)
{
  /* A font file's table directory cannot hold more than 65535 tables. */
  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create ("test", 4, HB_MEMORY_MODE_READONLY, NULL, NULL);

  for (unsigned i = 1; i <= 0xFFFFu; i++)
    g_assert_true (hb_face_builder_add_table (face, i, blob));
  g_assert_false (hb_face_builder_add_table (face, 0x10000u, blob));
  /* Replacing a table is still fine. */
  g_assert_true (hb_face_builder_add_table (face, 1, blob));
  g_assert_cmpuint (0xFFFFu, ==, hb_face_get_table_tags (face, 0, NULL, NULL));

  hb_blob_t *file = hb_face_reference_blob (face);
  g_assert_cmpuint (12 + 0xFFFFu * (16 + 4), ==, hb_blob_get_length (file));

  hb_blob_destroy (file);
  hb_blob_destroy (blob);
  hb_face_destroy (face);
}

static void
test_get_table_tags_builder_unsorted (void)
{
  /* Tags added out of order, one of them twice; must come back
   * sorted, each once, with any page size. */
  static const hb_tag_t added[] = {
    HB_TAG ('m','a','x','p'),
    HB_TAG ('G','S','U','B'),
    HB_TAG ('h','e','a','d'),
    HB_TAG ('z','z','z','z'),
    HB_TAG ('h','e','a','d'),
    HB_TAG ('c','m','a','p'),
    HB_TAG ('G','D','E','F'),
  };
  static const hb_tag_t expected[] = {
    HB_TAG ('G','D','E','F'),
    HB_TAG ('G','S','U','B'),
    HB_TAG ('c','m','a','p'),
    HB_TAG ('h','e','a','d'),
    HB_TAG ('m','a','x','p'),
    HB_TAG ('z','z','z','z'),
  };
  const unsigned num_expected = sizeof (expected) / sizeof (expected[0]);

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create ("test", 4, HB_MEMORY_MODE_READONLY, NULL, NULL);
  for (unsigned i = 0; i < sizeof (added) / sizeof (added[0]); i++)
    g_assert_true (hb_face_builder_add_table (face, added[i], blob));
  hb_blob_destroy (blob);

  g_assert_cmpuint (num_expected, ==, hb_face_get_table_tags (face, 0, NULL, NULL));

  hb_tag_t tags[4];
  unsigned offset = 0, count = sizeof (tags) / sizeof (tags[0]);
  while (((void) hb_face_get_table_tags (face, offset, &count, tags), count))
  {
    g_assert_cmpuint (offset + count, <=, num_expected);
    for (unsigned i = 0; i < count; i++)
      g_assert_cmpuint (tags[i], ==, expected[offset + i]);
    offset += count;
    count = sizeof (tags) / sizeof (tags[0]);
  }
  g_assert_cmpuint (offset, ==, num_expected);

  hb_face_destroy (face);
}

static void
put_u16 (char *p, unsigned v)
{
  p[0] = (char) (v >> 8);
  p[1] = (char) v;
}

static void
put_u32 (char *p, unsigned v)
{
  put_u16 (p, v >> 16);
  put_u16 (p + 2, v & 0xFFFFu);
}

static void
test_get_table_tags_dfont (void)
{
  /* A DFont whose resource type list is as long as its 16-bit count
   * allows, with 'sfnt' as the last type, holding one font with as many
   * tables as a table directory holds.  Finding the face walks the type
   * list; that must happen once, not again for every table looked up,
   * or the loop below takes billions of steps. */
  enum {
    num_types = 0x10000,
    num_tables = 0xFFFF,
    data_offset = 0x100, /* Also what identifies the file as a DFont. */
    sfnt_offset = data_offset + 4,
    maxp_offset = 12 + 16 * num_tables, /* From the start of the sfnt. */
    sfnt_length = maxp_offset + 6,
    map_offset = sfnt_offset + sfnt_length,
    type_list_offset = map_offset + 28,
    length = type_list_offset + 2 + 8 * num_types
  };
  static const char maxp[6] = {0, 0, 0x50, 0, 0, 1}; /* Version 0.5, one glyph. */

  char *data = (char *) calloc (1, length);
  g_assert_nonnull (data);

  /* Resource fork header. */
  put_u32 (data + 0, data_offset);
  put_u32 (data + 4, map_offset);
  put_u32 (data + 8, 4 + sfnt_length);
  put_u32 (data + 12, length - map_offset);

  /* The resource: its length, then a font of empty tables and a 'maxp'. */
  put_u32 (data + data_offset, sfnt_length);
  char *sfnt = data + sfnt_offset;
  put_u32 (sfnt + 0, 0x00010000u);
  put_u16 (sfnt + 4, num_tables);
  for (unsigned i = 0; i < num_tables - 1; i++)
    put_u32 (sfnt + 12 + 16 * i, HB_TAG ('a',0,0,0) + i);
  char *maxp_record = sfnt + 12 + 16 * (num_tables - 1);
  put_u32 (maxp_record + 0, HB_TAG ('m','a','x','p'));
  put_u32 (maxp_record + 8, maxp_offset);
  put_u32 (maxp_record + 12, sizeof (maxp));
  memcpy (sfnt + maxp_offset, maxp, sizeof (maxp));

  /* Resource map and its type list.  All types but the last are left
   * zeroed: not 'sfnt', no resources.  The one resource record of 'sfnt'
   * lies on the first of them, and so is zeroed too: the resource at the
   * start of the data. */
  put_u16 (data + map_offset + 24, type_list_offset - map_offset);
  char *types = data + type_list_offset;
  put_u16 (types, num_types - 1);
  char *sfnt_type = types + 2 + 8 * (num_types - 1);
  put_u32 (sfnt_type + 0, HB_TAG ('s','f','n','t'));
  put_u16 (sfnt_type + 4, 0); /* Resource count minus 1. */
  put_u16 (sfnt_type + 6, 2); /* Resource records, from the type list. */

  hb_blob_t *blob = hb_blob_create (data, length, HB_MEMORY_MODE_WRITABLE, data, free);
  g_assert_cmpuint (1u, ==, hb_face_count (blob));
  hb_face_t *missing = hb_face_create (blob, 1);
  g_assert_cmpuint (hb_face_get_table_tags (missing, 0, NULL, NULL), ==, 0);
  hb_face_destroy (missing);

  hb_face_t *face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);

  g_assert_cmpuint (num_tables, ==, hb_face_get_table_tags (face, 0, NULL, NULL));

  hb_tag_t tags[32];
  unsigned offset = 0, count = sizeof (tags) / sizeof (tags[0]);
  while (((void) hb_face_get_table_tags (face, offset, &count, tags), count))
  {
    for (unsigned i = 0; i < count; i++)
    {
      hb_blob_t *table = hb_face_reference_table (face, tags[i]);
      if (offset + i < num_tables - 1)
      {
	g_assert_cmpuint (tags[i], ==, HB_TAG ('a',0,0,0) + offset + i);
	g_assert_cmpuint (0u, ==, hb_blob_get_length (table));
      }
      else
      {
	g_assert_cmpuint (tags[i], ==, HB_TAG ('m','a','x','p'));
	g_assert_cmpuint (sizeof (maxp), ==, hb_blob_get_length (table));
	g_assert_true (hb_blob_get_data (table, NULL) == sfnt + maxp_offset);
      }
      hb_blob_destroy (table);
    }
    offset += count;
    count = sizeof (tags) / sizeof (tags[0]);
  }
  g_assert_cmpuint (offset, ==, num_tables);
  g_assert_cmpuint (1u, ==, hb_face_get_glyph_count (face));

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

static unsigned long
_ft_stream_read (FT_Stream       stream,
		 unsigned long   offset,
		 unsigned char  *buffer,
		 unsigned long   count)
{
  hb_blob_t *blob = (hb_blob_t *) stream->descriptor.pointer;
  unsigned int length;
  const char *data = hb_blob_get_data (blob, &length);
  if (offset > length)
    return 0;
  if (count == 0) /* Seek. */
    return 0;
  if (offset + count > length)
    count = length - offset;
  memcpy (buffer, data + offset, count);
  return count;
}

static void
_ft_stream_close (FT_Stream stream HB_UNUSED) {}

static void
test_get_table_tags_ft_stream (void)
{
  /* Exercise the streaming-face path, which (unlike an in-memory face) uses
   * hb-ft's own _hb_ft_get_table_tags callback. */
  hb_face_t *source = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_blob_t *blob = hb_face_reference_blob (source);
  hb_face_destroy (source);

  FT_Library ft_library;
  g_assert_cmpint (0, ==, FT_Init_FreeType (&ft_library));

  FT_StreamRec stream = {0};
  stream.size = hb_blob_get_length (blob);
  stream.descriptor.pointer = blob;
  stream.read = _ft_stream_read;
  stream.close = _ft_stream_close;

  FT_Open_Args args = {0};
  args.flags = FT_OPEN_STREAM;
  args.stream = &stream;

  FT_Face ft_face;
  g_assert_cmpint (0, ==, FT_Open_Face (ft_library, &args, 0, &ft_face));

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
  hb_face_t *null_face = hb_coretext_face_create (NULL);
  g_assert_true (null_face == hb_face_get_empty ());
  hb_face_destroy (null_face);

  hb_font_t *null_font = hb_coretext_font_create (NULL);
  g_assert_true (null_font == hb_font_get_empty ());
  hb_font_destroy (null_font);

  hb_face_t *source = hb_test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  hb_blob_t *blob = hb_face_reference_blob (source);
  hb_face_destroy (source);

  CGDataProviderRef provider = CGDataProviderCreateWithData (blob,
							     hb_blob_get_data (blob, NULL),
							     hb_blob_get_length (blob),
							     NULL);
  g_assert_true (provider);

  CGFontRef cg_font = CGFontCreateWithDataProvider (provider);
  g_assert_true (cg_font);
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
  hb_test_add (test_get_table_tags_builder_limit);
  hb_test_add (test_get_table_tags_builder_unsorted);
  hb_test_add (test_get_table_tags_dfont);
#ifdef HAVE_FREETYPE
  hb_test_add (test_get_table_tags_ft);
  hb_test_add (test_get_table_tags_ft_stream);
#endif
#ifdef HAVE_CORETEXT
  hb_test_add (test_get_table_tags_ct);
#endif

  return hb_test_run();
}
