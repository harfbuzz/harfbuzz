/*
 * Copyright © 2018  Ebrahim Byagowi
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
#include <hb-aat.h>

/* Unit tests for hb-aat.h */

static hb_face_t *face;
static hb_face_t *sbix;

#ifndef HB_NO_AAT

static void
test_aat_get_feature_types (void)
{
  hb_aat_layout_feature_type_t features[3];
  unsigned int count = 3;
  g_assert_cmpuint (11, ==, hb_aat_layout_get_feature_types (face, 0, &count, features));

  g_assert_cmpuint (1, ==, features[0]);
  g_assert_cmpuint (3, ==, features[1]);
  g_assert_cmpuint (6, ==, features[2]);

  g_assert_cmpuint (258, ==, hb_aat_layout_feature_type_get_name_id (face, features[0]));
  g_assert_cmpuint (261, ==, hb_aat_layout_feature_type_get_name_id (face, features[1]));
  g_assert_cmpuint (265, ==, hb_aat_layout_feature_type_get_name_id (face, features[2]));
}

static void
test_aat_get_feature_selectors (void)
{
  unsigned int default_index;
  hb_aat_layout_feature_selector_info_t settings[3];
  unsigned int count = 3;

  g_assert_cmpuint (4, ==, hb_aat_layout_feature_type_get_selector_infos (face,
									  HB_AAT_LAYOUT_FEATURE_TYPE_DESIGN_COMPLEXITY_TYPE,
									  0, &count, settings,
									  &default_index));
  g_assert_cmpuint (3, ==, count);
  g_assert_cmpuint (0, ==, default_index);

  g_assert_cmpuint (0, ==, settings[0].enable);
  g_assert_cmpuint (294, ==, settings[0].name_id);

  g_assert_cmpuint (1, ==, settings[1].enable);
  g_assert_cmpuint (295, ==, settings[1].name_id);

  g_assert_cmpuint (2, ==, settings[2].enable);
  g_assert_cmpuint (296, ==, settings[2].name_id);

  count = 3;
  g_assert_cmpuint (4, ==, hb_aat_layout_feature_type_get_selector_infos (face,
									  HB_AAT_LAYOUT_FEATURE_TYPE_DESIGN_COMPLEXITY_TYPE,
									  3, &count, settings,
									  &default_index));
  g_assert_cmpuint (1, ==, count);
  g_assert_cmpuint (0, ==, default_index);

  g_assert_cmpuint (3, ==, settings[0].enable);
  g_assert_cmpuint (297, ==, settings[0].name_id);

  count = 1;
  g_assert_cmpuint (1, ==, hb_aat_layout_feature_type_get_selector_infos (face,
									  HB_AAT_LAYOUT_FEATURE_TYPE_TYPOGRAPHIC_EXTRAS,
									  0, &count, settings,
									  &default_index));
  g_assert_cmpuint (1, ==, count);
  g_assert_cmpuint (HB_AAT_LAYOUT_NO_SELECTOR_INDEX, ==, default_index);

  g_assert_cmpuint (8, ==, settings[0].enable);
  g_assert_cmpuint (308, ==, settings[0].name_id);

  count = 100;
  g_assert_cmpuint (0, ==, hb_aat_layout_feature_type_get_selector_infos (face, HB_AAT_LAYOUT_FEATURE_TYPE_INVALID,
									  0, &count, settings,
									  NULL));
  g_assert_cmpuint (0, ==, count);
}

static void
test_aat_has (void)
{
  hb_face_t *morx = hb_test_open_font_file ("fonts/aat-morx.ttf");
  hb_face_t *trak;
  g_assert_true (hb_aat_layout_has_substitution (morx));
  hb_face_destroy (morx);

  trak = hb_test_open_font_file ("fonts/aat-trak.ttf");
  g_assert_true (hb_aat_layout_has_tracking (trak));
  hb_face_destroy (trak);
}


static hb_bool_t
count_started_subtables (hb_buffer_t *buffer HB_UNUSED,
			 hb_font_t   *font HB_UNUSED,
			 const char  *message,
			 void        *user_data)
{
  if (!strncmp (message, "start subtable ", 15) ||
      !strncmp (message, "start chainsubtable ", 20))
    (*(unsigned *) user_data)++;
  return true;
}

/* Shapes "aa" with a two-glyph font that has the table, which is
 * freed; returns the number of its subtables that were applied.
 * A table with more than HB_AAT_MAX_SUBTABLES subtables is rejected
 * by sanitize, so none of them is. */
static unsigned
count_applied_subtables (hb_tag_t tag, char *table, unsigned length)
{
  static const char maxp[] = {0x00, 0x00, 0x50, 0x00, 0x00, 0x02};
  static const char cmap[] = {0, 0, 0, 1,  0, 3, 0, 10, 0, 0, 0, 12,
			      0, 12, 0, 0,  0, 0, 0, 28,  0, 0, 0, 0,  0, 0, 0, 1,
			      0, 0, 0, 'a',  0, 0, 0, 'a',  0, 0, 0, 1};

  hb_face_t *builder = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (maxp, sizeof (maxp), HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (builder, HB_TAG ('m','a','x','p'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create (cmap, sizeof (cmap), HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (builder, HB_TAG ('c','m','a','p'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create (table, length, HB_MEMORY_MODE_READONLY, table, free);
  hb_face_builder_add_table (builder, tag, blob);
  hb_blob_destroy (blob);
  blob = hb_face_reference_blob (builder);
  hb_face_destroy (builder);
  hb_face_t *limit_face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  hb_font_t *font = hb_font_create (limit_face);
  hb_face_destroy (limit_face);

  unsigned started = 0;
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (buffer, "aa", -1, 0, -1);
  hb_buffer_guess_segment_properties (buffer);
  hb_buffer_set_message_func (buffer, count_started_subtables, &started, NULL);
  hb_shape (font, buffer, NULL, 0);

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);

  return started;
}

/* A kern table of format-3 subtables. */
static char *
make_kern_table (unsigned num_subtables, unsigned *length)
{
  *length = 4 + 14 * num_subtables;
  char *kern = (char *) calloc (*length, 1);
  kern[2] = (char) (num_subtables >> 8);
  kern[3] = (char) (num_subtables & 0xFF);
  for (unsigned i = 0; i < num_subtables; i++)
  {
    char *subtable = kern + 4 + 14 * i;
    subtable[3] = 14; /* length */
    subtable[4] = 3;  /* format */
    subtable[5] = 1;  /* coverage: horizontal */
    subtable[7] = 1;  /* glyphCount */
  }
  return kern;
}

static void
test_aat_kern_subtable_limit (void)
{
  unsigned length;
  char *kern;

  kern = make_kern_table (1024, &length);
  g_assert_cmpuint (1024, ==, count_applied_subtables (HB_TAG ('k','e','r','n'), kern, length));

  kern = make_kern_table (1025, &length);
  g_assert_cmpuint (0, ==, count_applied_subtables (HB_TAG ('k','e','r','n'), kern, length));
}

/* A morx table of chains of five noncontextual subtables each. */
static char *
make_morx_table (unsigned num_chains, unsigned *length)
{
  *length = 8 + (16 + 5 * 20) * num_chains;
  char *morx = (char *) calloc (*length, 1);
  morx[1] = 2; /* version */
  morx[6] = (char) (num_chains >> 8);
  morx[7] = (char) (num_chains & 0xFF);
  for (unsigned i = 0; i < num_chains; i++)
  {
    char *chain = morx + 8 + (16 + 5 * 20) * i;
    chain[3] = 1;              /* defaultFlags */
    chain[7] = 16 + 5 * 20;    /* length */
    chain[15] = 5;             /* subtableCount */
    for (unsigned j = 0; j < 5; j++)
    {
      char *subtable = chain + 16 + 20 * j;
      subtable[3] = 20;   /* length */
      subtable[4] = 0x20; /* coverage: all directions */
      subtable[7] = 4;    /* type */
      subtable[11] = 1;   /* subFeatureFlags */
      subtable[13] = 8;   /* lookup format */
      subtable[15] = 1;   /* firstGlyph */
      subtable[17] = 1;   /* glyphCount */
      subtable[19] = 1;   /* value */
    }
  }
  return morx;
}

static void
test_aat_morx_subtable_limit (void)
{
  unsigned length;
  char *morx;

  morx = make_morx_table (204, &length); /* 1020 subtables */
  g_assert_cmpuint (1020, ==, count_applied_subtables (HB_TAG ('m','o','r','x'), morx, length));

  morx = make_morx_table (205, &length); /* 1025 subtables */
  g_assert_cmpuint (0, ==, count_applied_subtables (HB_TAG ('m','o','r','x'), morx, length));
}


/* A font mapping 'a' and 'b' to glyphs 1 and 2.  Its first morx chain turns
 * glyph 1 into glyph 2 where common ligatures are on; they are off by default.
 * That chain is followed by empty_chains chains that have nothing in them. */
static hb_face_t *
create_morx_face (unsigned int empty_chains)
{
  static const unsigned char cmap[] = {
    0x00,0x00, 0x00,0x01, 0x00,0x03, 0x00,0x0A, 0x00,0x00,0x00,0x0C,
    0x00,0x0C, 0x00,0x00, 0x00,0x00,0x00,0x1C, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x01,
    0x00,0x00,0x00,0x61, 0x00,0x00,0x00,0x62, 0x00,0x00,0x00,0x01,
  };
  static const unsigned char maxp[] = {0x00,0x00,0x50,0x00, 0x00,0x03};
  static const unsigned char feat[] = {
    0x00,0x01,0x00,0x00, 0x00,0x01, 0x00,0x00, 0x00,0x00,0x00,0x00,
    0x00,0x01, 0x00,0x02, 0x00,0x00,0x00,0x18, 0x00,0x00, 0x01,0x00,
    0x00,0x02, 0x01,0x00, 0x00,0x03, 0x01,0x00,
  };
  static const unsigned char chain[] = {
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x3C, 0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0x01,
    0x00,0x01, 0x00,0x02, 0x00,0x00,0x00,0x01, 0xFF,0xFF,0xFF,0xFF,
    0x00,0x01, 0x00,0x03, 0x00,0x00,0x00,0x00, 0xFF,0xFF,0xFF,0xFE,
    0x00,0x00,0x00,0x14, 0x20,0x00,0x00,0x04, 0x00,0x00,0x00,0x01,
    0x00,0x08, 0x00,0x01, 0x00,0x01, 0x00,0x02,
  };
  unsigned int chain_count = 1 + empty_chains;
  unsigned int morx_length = 8 + sizeof (chain) + 16 * empty_chains;
  unsigned char *morx = (unsigned char *) calloc (1, morx_length);
  hb_face_t *builder = hb_face_builder_create ();
  hb_blob_t *blob;

  g_assert_nonnull (morx);
  morx[1] = 2;
  morx[4] = chain_count >> 24;
  morx[5] = chain_count >> 16;
  morx[6] = chain_count >> 8;
  morx[7] = chain_count;
  memcpy (morx + 8, chain, sizeof (chain));
  for (unsigned int i = 0; i < empty_chains; i++)
    morx[8 + sizeof (chain) + 16 * i + 7] = 16;

  blob = hb_blob_create ((const char *) cmap, sizeof (cmap), HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (builder, HB_TAG ('c','m','a','p'), blob));
  hb_blob_destroy (blob);
  blob = hb_blob_create ((const char *) maxp, sizeof (maxp), HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (builder, HB_TAG ('m','a','x','p'), blob));
  hb_blob_destroy (blob);
  blob = hb_blob_create ((const char *) feat, sizeof (feat), HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (builder, HB_TAG ('f','e','a','t'), blob));
  hb_blob_destroy (blob);
  blob = hb_blob_create ((const char *) morx, morx_length, HB_MEMORY_MODE_READONLY, morx, free);
  g_assert_true (hb_face_builder_add_table (builder, HB_TAG ('m','o','r','x'), blob));
  hb_blob_destroy (blob);

  return builder;
}

/* Shapes a run of 'a' that has ligatures on in every character whose index
 * is not a multiple of period, each character having its own feature range. */
static hb_buffer_t *
shape_ranged_features (hb_face_t *morx_face, unsigned int length, unsigned int period)
{
  hb_font_t *font = hb_font_create (morx_face);
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_feature_t *features = (hb_feature_t *) calloc (length, sizeof (hb_feature_t));

  g_assert_nonnull (features);
  for (unsigned int i = 0; i < length; i++)
  {
    hb_buffer_add (buffer, 'a', i);
    features[i].tag = HB_TAG ('l','i','g','a');
    features[i].value = i % period != 0;
    features[i].start = i;
    features[i].end = i + 1;
  }
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, features, length);
  g_assert_cmpuint (length, ==, hb_buffer_get_length (buffer));

  free (features);
  hb_font_destroy (font);
  return buffer;
}

static void
test_aat_ranged_features (void)
{
  hb_face_t *morx_face = create_morx_face (1);
  hb_buffer_t *buffer = shape_ranged_features (morx_face, 100, 3);
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, NULL);

  for (unsigned int i = 0; i < 100; i++)
    g_assert_cmpuint (i % 3 ? 2 : 1, ==, info[i].codepoint);

  hb_buffer_destroy (buffer);
  hb_face_destroy (morx_face);
}

static void
test_aat_ranged_features_many_chains (void)
{
  /* The number of chains times the number of ranges is what must not
   * be paid for, in time or memory.  The first ranges still work. */
  hb_face_t *morx_face = create_morx_face (20000);
  hb_buffer_t *buffer = shape_ranged_features (morx_face, 5000, 2);
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (buffer, NULL);

  for (unsigned int i = 0; i < 100; i++)
    g_assert_cmpuint (i % 2 ? 2 : 1, ==, info[i].codepoint);

  hb_buffer_destroy (buffer);
  hb_face_destroy (morx_face);
}

#endif

int
main (int argc, char **argv)
{
  unsigned int status;
  hb_test_init (&argc, &argv);

#ifndef HB_NO_AAT
  hb_test_add (test_aat_get_feature_types);
  hb_test_add (test_aat_get_feature_selectors);
  hb_test_add (test_aat_has);
  hb_test_add (test_aat_kern_subtable_limit);
  hb_test_add (test_aat_morx_subtable_limit);
  hb_test_add (test_aat_ranged_features);
  hb_test_add (test_aat_ranged_features_many_chains);
#endif

  face = hb_test_open_font_file ("fonts/aat-feat.ttf");
  sbix = hb_test_open_font_file ("fonts/chromacheck-sbix.ttf");
  status = hb_test_run ();
  hb_face_destroy (sbix);
  hb_face_destroy (face);
  return status;
}
