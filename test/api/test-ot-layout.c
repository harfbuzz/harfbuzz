/*
 * Copyright © 2021  Khaled Hosny
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
 */

#include "hb-test.h"

#include <hb.h>
#include <hb-ot.h>

#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MPROTECT) && defined(HAVE_MMAP)
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/mman.h>
#endif

#define STATIC_ARRAY_SIZE 255

#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MPROTECT) && defined(HAVE_MMAP)
static void
test_ot_layout_gdef_unsupported_version (void)
{
  const char gdef[] = {0x87, 0x00, 0x00, 0x06, 0x30};
  long pagesize = sysconf (_SC_PAGESIZE);
  char *mapping;
  char *table;
  hb_blob_t *blob;
  hb_face_t *face;

  g_assert_cmpint (pagesize, >, 0);
  mapping = mmap (NULL, 2 * pagesize, PROT_NONE,
		  MAP_PRIVATE | MAP_ANON, -1, 0);
  g_assert_true (mapping != MAP_FAILED);
  g_assert_cmpint (mprotect (mapping, pagesize, PROT_READ | PROT_WRITE), ==, 0);

  table = mapping + pagesize - sizeof (gdef);
  memcpy (table, gdef, sizeof (gdef));
  g_assert_cmpint (mprotect (mapping, pagesize, PROT_READ), ==, 0);

  face = hb_face_builder_create ();
  blob = hb_blob_create (table, sizeof (gdef), HB_MEMORY_MODE_READONLY,
			 NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_OT_TAG_GDEF, blob));
  hb_blob_destroy (blob);

  g_assert_false (hb_ot_layout_has_glyph_classes (face));

  hb_face_destroy (face);
  g_assert_cmpint (munmap (mapping, 2 * pagesize), ==, 0);
}
#endif

static void
test_ot_layout_table_get_script_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  unsigned int total = 0;
  unsigned int count = STATIC_ARRAY_SIZE;
  unsigned int offset = 0;
  hb_tag_t tags[STATIC_ARRAY_SIZE];
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_table_get_script_tags (face, HB_OT_TAG_GSUB, offset, &count, tags);
    g_assert_cmpuint (3, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (3, ==, count);
      g_assert_cmpuint (HB_TAG ('a','r','a','b'), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('d','f','l','t'), ==, tags[1]);
      g_assert_cmpuint (HB_TAG ('l','a','t','n'), ==, tags[2]);
    }
  }
  count = STATIC_ARRAY_SIZE;
  offset = 0;
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_table_get_script_tags (face, HB_OT_TAG_GPOS, offset, &count, tags);
    g_assert_cmpuint (1, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (1, ==, count);
      g_assert_cmpuint (HB_TAG ('a','r','a','b'), ==, tags[0]);
    }
  }

  hb_face_destroy (face);
}

static void
test_ot_layout_table_find_script (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");
  unsigned int index;

  g_assert_true (hb_ot_layout_table_find_script (face, HB_OT_TAG_GSUB, HB_TAG ('a','r','a','b'), &index));
  g_assert_cmpuint (0, ==, index);
  g_assert_true (hb_ot_layout_table_find_script (face, HB_OT_TAG_GSUB, HB_TAG ('d','f','l','t'), &index));
  g_assert_cmpuint (1, ==, index);
  g_assert_true (hb_ot_layout_table_find_script (face, HB_OT_TAG_GSUB, HB_TAG ('l','a','t','n'), &index));
  g_assert_cmpuint (2, ==, index);

  g_assert_true (hb_ot_layout_table_find_script (face, HB_OT_TAG_GPOS, HB_TAG ('a','r','a','b'), &index));
  g_assert_cmpuint (0, ==, index);
  g_assert_true (!hb_ot_layout_table_find_script (face, HB_OT_TAG_GPOS, HB_TAG ('d','f','l','t'), &index));
  g_assert_cmpuint (0xFFFF, ==, index);
  g_assert_true (!hb_ot_layout_table_find_script (face, HB_OT_TAG_GPOS, HB_TAG ('l','a','t','n'), &index));
  g_assert_cmpuint (0xFFFF, ==, index);

  hb_face_destroy (face);
}

static void
test_ot_layout_table_get_feature_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoNastaliqUrdu-Regular.ttf");

  unsigned int total = 0;
  unsigned int count = STATIC_ARRAY_SIZE;
  unsigned int offset = 0;
  hb_tag_t tags[STATIC_ARRAY_SIZE];
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_table_get_feature_tags (face, HB_OT_TAG_GSUB, offset, &count, tags);
    g_assert_cmpuint (14, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (14, ==, count);
      g_assert_cmpuint (HB_TAG ('c','c','m','p'), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('i','s','o','l'), ==, tags[10]);
      g_assert_cmpuint (HB_TAG ('r','l','i','g'), ==, tags[13]);
    }
  }
  count = STATIC_ARRAY_SIZE;
  offset = 0;
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_table_get_feature_tags (face, HB_OT_TAG_GPOS, offset, &count, tags);
    g_assert_cmpuint (3, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (3, ==, count);
      g_assert_cmpuint (HB_TAG ('c','u','r','s'), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('m','a','r','k'), ==, tags[1]);
      g_assert_cmpuint (HB_TAG ('m','k','m','k'), ==, tags[2]);
    }
  }

  hb_face_destroy (face);
}

static void
test_ot_layout_script_get_language_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Estedad-VF.ttf");

  unsigned int total = 0;
  unsigned int count = STATIC_ARRAY_SIZE;
  unsigned int offset = 0;
  hb_tag_t tags[STATIC_ARRAY_SIZE];
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_script_get_language_tags (face, HB_OT_TAG_GSUB, 0, offset, &count, tags);
    g_assert_cmpuint (2, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (2, ==, count);
      g_assert_cmpuint (HB_TAG ('F','A','R',' '), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('K','U','R',' '), ==, tags[1]);
    }
  }
  count = STATIC_ARRAY_SIZE;
  offset = 0;
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_script_get_language_tags (face, HB_OT_TAG_GPOS, 1, offset, &count, tags);
    g_assert_cmpuint (2, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (2, ==, count);
      g_assert_cmpuint (HB_TAG ('F','A','R',' '), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('K','U','R',' '), ==, tags[1]);
    }
  }

  hb_face_destroy (face);
}

static void
test_ot_layout_language_get_feature_tags (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/Estedad-VF.ttf");

  unsigned int total = 0;
  unsigned int count = STATIC_ARRAY_SIZE;
  unsigned int offset = 0;
  hb_tag_t tags[STATIC_ARRAY_SIZE];
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_language_get_feature_tags (face, HB_OT_TAG_GSUB, 0, 0, offset, &count, tags);
    g_assert_cmpuint (6, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (6, ==, count);
      g_assert_cmpuint (HB_TAG ('c','a','l','t'), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('f','i','n','a'), ==, tags[1]);
      g_assert_cmpuint (HB_TAG ('i','n','i','t'), ==, tags[2]);
      g_assert_cmpuint (HB_TAG ('l','i','g','a'), ==, tags[3]);
      g_assert_cmpuint (HB_TAG ('m','e','d','i'), ==, tags[4]);
      g_assert_cmpuint (HB_TAG ('r','l','i','g'), ==, tags[5]);
    }
  }
  count = STATIC_ARRAY_SIZE;
  offset = 0;
  while (count == STATIC_ARRAY_SIZE)
  {
    total = hb_ot_layout_language_get_feature_tags (face, HB_OT_TAG_GPOS, 1, 0, offset, &count, tags);
    g_assert_cmpuint (3, ==, total);
    offset += count;
    if (count)
    {
      g_assert_cmpuint (3, ==, count);
      g_assert_cmpuint (HB_TAG ('k','e','r','n'), ==, tags[0]);
      g_assert_cmpuint (HB_TAG ('m','a','r','k'), ==, tags[1]);
      g_assert_cmpuint (HB_TAG ('m','k','m','k'), ==, tags[2]);
    }
  }

  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MPROTECT) && defined(HAVE_MMAP)
  hb_test_add (test_ot_layout_gdef_unsupported_version);
#endif
  hb_test_add (test_ot_layout_table_get_script_tags);
  hb_test_add (test_ot_layout_table_find_script);
  hb_test_add (test_ot_layout_script_get_language_tags);
  hb_test_add (test_ot_layout_table_get_feature_tags);
  hb_test_add (test_ot_layout_language_get_feature_tags);
  return hb_test_run ();
}
