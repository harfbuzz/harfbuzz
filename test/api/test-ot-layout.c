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

#ifndef HB_NO_BEYOND_64K
static void
test_ot_layout_gdef_1_4_offset2 (void)
{
  const char gdef[] = {
    0x00, 0x01, 0x00, 0x04, /* version 1.4 */
    0x00, 0x26,             /* glyphClassDefOffset */
    0x00, 0x00,             /* attachListOffset */
    0x00, 0x00,             /* ligCaretListOffset */
    0x00, 0x00,             /* markAttachClassDefOffset */
    0x00, 0x00,             /* markGlyphSetsDefOffset */
    0x00, 0x00, 0x00, 0x00, /* itemVarStoreOffset */
    0x00, 0x00, 0x00, 0x2E, /* glyphClassDefOffset2 */
    0x00, 0x00, 0x00, 0x00, /* attachListOffset2 */
    0x00, 0x00, 0x00, 0x00, /* ligCaretListOffset2 */
    0x00, 0x00, 0x00, 0x00, /* markAttachClassDefOffset2 */
    0x00, 0x00, 0x00, 0x00, /* markGlyphSetsDefOffset2 */
    0x00, 0x01, 0x00, 0x05, /* legacy ClassDef format 1, glyph 5 */
    0x00, 0x01, 0x00, 0x01, /* one glyph in class 1 */
    0x00, 0x01, 0x00, 0x05, /* ClassDef2 format 1, glyph 5 */
    0x00, 0x01, 0x00, 0x03, /* one glyph in class 3 */
  };
  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (gdef, sizeof (gdef),
				    HB_MEMORY_MODE_READONLY, NULL, NULL);

  g_assert_true (hb_face_builder_add_table (face, HB_OT_TAG_GDEF, blob));
  hb_blob_destroy (blob);

  g_assert_true (hb_ot_layout_has_glyph_classes (face));
  g_assert_cmpuint (hb_ot_layout_get_glyph_class (face, 5), ==,
		    HB_OT_LAYOUT_GLYPH_CLASS_MARK);

  hb_face_destroy (face);
}

static void
test_ot_layout_lookups_substitute_closure_context_format4 (void)
{
  /* Lookup 0 is a SequenceContextFormat4 (24-bit glyph ids) with the
   * single rule "1 2", applying lookup 1 at position 1.  Lookup 1 is a
   * SingleSubst 2 -> 3.  Closing over {1, 2} must add glyph 3. */
  const char gsub[] = {
    0x00, 0x01, 0x00, 0x00, /* version 1.0 */
    0x00, 0x0A,             /* scriptListOffset */
    0x00, 0x0C,             /* featureListOffset */
    0x00, 0x0E,             /* lookupListOffset */
    0x00, 0x00,             /* ScriptList: no scripts */
    0x00, 0x00,             /* FeatureList: no features */
    0x00, 0x02, 0x00, 0x06, 0x00, 0x2F, /* LookupList: two lookups */
    0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, /* Lookup 0: type 5, one subtable */
    0x00, 0x04,             /* SequenceContextFormat4 */
    0x00, 0x00, 0x00, 0x0C, /* coverageOffset */
    0x00, 0x00, 0x01,       /* ruleSetCount */
    0x00, 0x00, 0x12,       /* ruleSetOffsets[0] */
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, /* Coverage format 1, glyph 1 */
    0x00, 0x01, 0x00, 0x04, /* RuleSet: one rule */
    0x00, 0x02, 0x00, 0x01, /* Rule: inputCount 2, lookupCount 1 */
    0x00, 0x00, 0x02,       /* input[1] = glyph 2 */
    0x00, 0x01, 0x00, 0x01, /* LookupRecord: sequenceIndex 1, lookup 1 */
    0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, /* Lookup 1: type 1, one subtable */
    0x00, 0x01, 0x00, 0x06, 0x00, 0x01, /* SingleSubstFormat1, delta +1 */
    0x00, 0x01, 0x00, 0x01, 0x00, 0x02, /* Coverage format 1, glyph 2 */
  };
  const char maxp[] = {0x00, 0x00, 0x50, 0x00, 0x00, 0x04}; /* version 0.5, 4 glyphs */
  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (gsub, sizeof (gsub),
				    HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_set_t *lookups = hb_set_create ();
  hb_set_t *glyphs = hb_set_create ();

  g_assert_true (hb_face_builder_add_table (face, HB_OT_TAG_GSUB, blob));
  hb_blob_destroy (blob);
  blob = hb_blob_create (maxp, sizeof (maxp),
			 HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_TAG ('m','a','x','p'), blob));
  hb_blob_destroy (blob);

  hb_set_add (lookups, 0);
  hb_set_add (glyphs, 1);
  hb_set_add (glyphs, 2);
  hb_ot_layout_lookups_substitute_closure (face, lookups, glyphs);

  g_assert_true (hb_set_has (glyphs, 3));
  g_assert_cmpuint (hb_set_get_population (glyphs), ==, 3);

  hb_set_destroy (glyphs);
  hb_set_destroy (lookups);
  hb_face_destroy (face);
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

#ifndef HB_NO_VAR
static void
test_ot_layout_collect_lookup_variations (void)
{
  hb_face_t *face = hb_test_open_font_file (
      "../shape/data/in-house/fonts/4e9f0bc6a8f25b5fd3547bbc17423ce8cedb915f.ttf");
  const hb_tag_t features[] = {HB_TAG ('l','i','g','a'), HB_TAG_NONE};
  hb_set_t *lookups = hb_set_create ();

  hb_ot_layout_collect_lookups (face, HB_OT_TAG_GSUB,
				NULL, NULL, features, lookups);

  g_assert_cmpuint (hb_set_get_population (lookups), ==, 4);
  g_assert_true (hb_set_has (lookups, 0));
  g_assert_true (hb_set_has (lookups, 1));
  g_assert_true (hb_set_has (lookups, 2));
  g_assert_true (hb_set_has (lookups, 5));

  const hb_tag_t gpos_features[] = {HB_TAG ('k','e','r','n'), HB_TAG_NONE};
  hb_set_clear (lookups);
  hb_ot_layout_collect_lookups (face, HB_OT_TAG_GPOS,
				NULL, NULL, gpos_features, lookups);

  g_assert_cmpuint (hb_set_get_population (lookups), ==, 2);
  g_assert_true (hb_set_has (lookups, 0));
  g_assert_true (hb_set_has (lookups, 1));

  hb_set_destroy (lookups);
  hb_face_destroy (face);
}
#endif

/* A Context lookup that recurses to a MarkBasePos lookup at input positions
 * 1,0,1,0,... makes the latter drop its cached backward base search every
 * other time and rescan from scratch.  On a run of marks with no base glyph
 * each rescan walks the whole buffer, so it must be charged to the buffer's
 * work budget: shaping must give up rather than take quadratic time. */
static void
test_ot_layout_gpos_mark_base_rescan_budget (void)
{
#define LOOKUP_RECORDS 20
#define MARKS 10000
  static const char gdef[] = {
    0x00, 0x01, 0x00, 0x00, /* version 1.0 */
    0x00, 0x0C,             /* glyphClassDefOffset */
    0x00, 0x00,             /* attachListOffset */
    0x00, 0x00,             /* ligCaretListOffset */
    0x00, 0x00,             /* markAttachClassDefOffset */
    0x00, 0x01, 0x00, 0x02, /* ClassDef format 1, glyph 2 */
    0x00, 0x01, 0x00, 0x03, /* one glyph in class 3 (mark) */
  };
  static const char cmap[] = {
    0x00, 0x00, 0x00, 0x01, /* version 0, one encoding record */
    0x00, 0x03, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0C, /* (3,10) at 12 */
    0x00, 0x0C, 0x00, 0x00, /* format 12 */
    0x00, 0x00, 0x00, 0x1C, /* length */
    0x00, 0x00, 0x00, 0x00, /* language */
    0x00, 0x00, 0x00, 0x01, /* one group: */
    0x00, 0x00, 0x00, 0x6D, 0x00, 0x00, 0x00, 0x6D, 0x00, 0x00, 0x00, 0x02, /* U+006D -> glyph 2 */
  };
  static const char maxp[] = {
    0x00, 0x00, 0x50, 0x00, /* version 0.5 */
    0x00, 0x03,             /* numGlyphs */
  };
  /* GPOS up to the LookupRecords of the Context lookup. */
  static const char gpos_head[] = {
    0x00, 0x01, 0x00, 0x00, /* version 1.0 */
    0x00, 0x0A,             /* scriptListOffset */
    0x00, 0x1E,             /* featureListOffset */
    0x00, 0x2C,             /* lookupListOffset */
    /* ScriptList: DFLT with a default LangSys using feature 0 */
    0x00, 0x01, 'D', 'F', 'L', 'T', 0x00, 0x08,
    0x00, 0x04, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x00,
    /* FeatureList: 'mark' using lookup 0 */
    0x00, 0x01, 'm', 'a', 'r', 'k', 0x00, 0x08,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    /* LookupList: two lookups */
    0x00, 0x02, 0x00, 0x06, 0x00, 0x6E,
    /* Lookup 0: ContextPos, one subtable */
    0x00, 0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08,
    /* ContextPos format 3: input [glyph 2] [glyph 2] */
    0x00, 0x03, 0x00, 0x02, 0x00, LOOKUP_RECORDS, 0x00, 0x5A, 0x00, 0x5A,
    /* LookupRecords go here */
  };
  static const char gpos_tail[] = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x02, /* Coverage: glyph 2 */
    /* Lookup 1: MarkBasePos, one subtable */
    0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08,
    /* MarkBasePos format 1: mark [glyph 2], base [glyph 1], one class */
    0x00, 0x01, 0x00, 0x0C, 0x00, 0x12, 0x00, 0x01, 0x00, 0x18, 0x00, 0x24,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x02, /* markCoverage */
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, /* baseCoverage */
    0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, /* MarkArray */
    0x00, 0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, /* BaseArray */
  };
  char gpos[sizeof (gpos_head) + 4 * LOOKUP_RECORDS + sizeof (gpos_tail)];
  char *records = gpos + sizeof (gpos_head);
  unsigned int out_of_order, i;

  memcpy (gpos, gpos_head, sizeof (gpos_head));
  memcpy (records + 4 * LOOKUP_RECORDS, gpos_tail, sizeof (gpos_tail));

  for (out_of_order = 0; out_of_order < 2; out_of_order++)
  {
    hb_face_t *face = hb_face_builder_create ();
    hb_font_t *font;
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_blob_t *blob;

    /* All records recurse to lookup 1, either at input positions
     * 0,...,0,1,...,1 (forward only) or at 1,0,1,0,... */
    for (i = 0; i < LOOKUP_RECORDS; i++)
    {
      records[4 * i + 0] = 0;
      records[4 * i + 1] = out_of_order ? i & 1 : i >= LOOKUP_RECORDS / 2;
      records[4 * i + 2] = 0;
      records[4 * i + 3] = 1;
    }

    blob = hb_blob_create (gdef, sizeof (gdef), HB_MEMORY_MODE_READONLY, NULL, NULL);
    g_assert_true (hb_face_builder_add_table (face, HB_OT_TAG_GDEF, blob));
    hb_blob_destroy (blob);
    blob = hb_blob_create (gpos, sizeof (gpos), HB_MEMORY_MODE_READONLY, NULL, NULL);
    g_assert_true (hb_face_builder_add_table (face, HB_OT_TAG_GPOS, blob));
    hb_blob_destroy (blob);
    blob = hb_blob_create (cmap, sizeof (cmap), HB_MEMORY_MODE_READONLY, NULL, NULL);
    g_assert_true (hb_face_builder_add_table (face, HB_TAG ('c','m','a','p'), blob));
    hb_blob_destroy (blob);
    blob = hb_blob_create (maxp, sizeof (maxp), HB_MEMORY_MODE_READONLY, NULL, NULL);
    g_assert_true (hb_face_builder_add_table (face, HB_TAG ('m','a','x','p'), blob));
    hb_blob_destroy (blob);

    font = hb_font_create (face);

    for (i = 0; i < MARKS; i++)
      hb_buffer_add (buffer, 0x006Du, i);
    hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
    hb_buffer_guess_segment_properties (buffer);

    hb_shape (font, buffer, NULL, 0);

    if (out_of_order)
      g_assert_true (!hb_buffer_allocation_successful (buffer));
    else
    {
      g_assert_true (hb_buffer_allocation_successful (buffer));
      g_assert_cmpuint (hb_buffer_get_length (buffer), ==, MARKS);
    }

    hb_buffer_destroy (buffer);
    hb_font_destroy (font);
    hb_face_destroy (face);
  }
#undef MARKS
#undef LOOKUP_RECORDS
}

static char *
write_be16 (char *p, unsigned v)
{
  *p++ = (char) (v >> 8);
  *p++ = (char) v;
  return p;
}

static void
test_ot_layout_lookups_substitute_closure_large_coverage_one (unsigned lookup_type)
{
  /* Lookup 0 is a ContextSubst covering NUM_GLYPHS glyphs; every rule recurses
   * into lookup 1, a MultipleSubst or AlternateSubst covering the same glyphs
   * and mapping each to OUT1 and OUT2.  Closure visits lookup 1 once per glyph
   * with a single active glyph, so the cost of a visit must not scale with the
   * size of lookup 1's coverage. */
  enum { NUM_GLYPHS = 16000, OUT1 = NUM_GLYPHS + 1, OUT2 = NUM_GLYPHS + 2 };
  const unsigned context_len = 6 + 2 * NUM_GLYPHS + 4 + 8 + 10;
  const unsigned subst_len = 6 + 2 * NUM_GLYPHS + 6 + 10;
  const unsigned gsub_len = 10 + 6 + 16 + context_len + subst_len;
  char *gsub = g_malloc0 (gsub_len);
  char *p = gsub;
  unsigned i;

  p = write_be16 (p, 1); p = write_be16 (p, 0);		/* version 1.0 */
  p = write_be16 (p, 0); p = write_be16 (p, 0);		/* no ScriptList, no FeatureList */
  p = write_be16 (p, 10);				/* LookupList */

  p = write_be16 (p, 2);				/* lookupCount */
  p = write_be16 (p, 6); p = write_be16 (p, 14);	/* Lookup offsets */
  p = write_be16 (p, 5); p = write_be16 (p, 0);		/* ContextSubst, no flags */
  p = write_be16 (p, 1); p = write_be16 (p, 16);	/* one subtable */
  p = write_be16 (p, lookup_type); p = write_be16 (p, 0);
  p = write_be16 (p, 1); p = write_be16 (p, 8 + context_len);

  /* ContextSubstFormat1: all rule sets share one rule applying lookup 1 at 0. */
  p = write_be16 (p, 1); p = write_be16 (p, context_len - 10);
  p = write_be16 (p, NUM_GLYPHS);
  for (i = 0; i < NUM_GLYPHS; i++)
    p = write_be16 (p, 6 + 2 * NUM_GLYPHS);
  p = write_be16 (p, 1); p = write_be16 (p, 4);		/* RuleSet */
  p = write_be16 (p, 1); p = write_be16 (p, 1);		/* Rule: glyphCount, lookupCount */
  p = write_be16 (p, 0); p = write_be16 (p, 1);		/* SequenceLookupRecord */
  p = write_be16 (p, 2); p = write_be16 (p, 1);		/* CoverageFormat2, one range */
  p = write_be16 (p, 1); p = write_be16 (p, NUM_GLYPHS); p = write_be16 (p, 0);

  /* MultipleSubstFormat1 / AlternateSubstFormat1: all glyphs share one
   * Sequence / AlternateSet. */
  p = write_be16 (p, 1); p = write_be16 (p, subst_len - 10);
  p = write_be16 (p, NUM_GLYPHS);
  for (i = 0; i < NUM_GLYPHS; i++)
    p = write_be16 (p, 6 + 2 * NUM_GLYPHS);
  p = write_be16 (p, 2); p = write_be16 (p, OUT1); p = write_be16 (p, OUT2);
  p = write_be16 (p, 2); p = write_be16 (p, 1);		/* CoverageFormat2, one range */
  p = write_be16 (p, 1); p = write_be16 (p, NUM_GLYPHS); p = write_be16 (p, 0);

  g_assert_cmpuint (p - gsub, ==, gsub_len);

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (gsub, gsub_len, HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_OT_TAG_GSUB, blob));
  hb_blob_destroy (blob);
  hb_face_set_glyph_count (face, OUT2 + 1);

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add_range (glyphs, 1, NUM_GLYPHS);
  hb_ot_layout_lookups_substitute_closure (face, NULL, glyphs);

  g_assert_cmpuint (hb_set_get_population (glyphs), ==, NUM_GLYPHS + 2);
  g_assert_true (hb_set_has (glyphs, OUT1));
  g_assert_true (hb_set_has (glyphs, OUT2));

  hb_set_destroy (glyphs);
  hb_face_destroy (face);
  g_free (gsub);
}

static void
test_ot_layout_lookups_substitute_closure_large_coverage (void)
{
  test_ot_layout_lookups_substitute_closure_large_coverage_one (2); /* MultipleSubst */
  test_ot_layout_lookups_substitute_closure_large_coverage_one (3); /* AlternateSubst */
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);
#if defined(HAVE_SYS_MMAN_H) && defined(HAVE_MPROTECT) && defined(HAVE_MMAP)
  hb_test_add (test_ot_layout_gdef_unsupported_version);
#endif
#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_ot_layout_gdef_1_4_offset2);
  hb_test_add (test_ot_layout_lookups_substitute_closure_context_format4);
#endif
  hb_test_add (test_ot_layout_table_get_script_tags);
  hb_test_add (test_ot_layout_table_find_script);
  hb_test_add (test_ot_layout_script_get_language_tags);
  hb_test_add (test_ot_layout_table_get_feature_tags);
  hb_test_add (test_ot_layout_language_get_feature_tags);
#ifndef HB_NO_VAR
  hb_test_add (test_ot_layout_collect_lookup_variations);
#endif
  hb_test_add (test_ot_layout_gpos_mark_base_rescan_budget);
  hb_test_add (test_ot_layout_lookups_substitute_closure_large_coverage);
  return hb_test_run ();
}
