/*
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#include "hb-test.h"

/* Unit tests for hb-shape.h */

/*
 * This test provides a framework to test aspects of hb_shape() that are
 * font-independent.  Please add tests for any feature that fits that
 * description.
 */

/* TODO Make this test data-driven and add some real test data */
/* TODO Test positions too. And test non-native direction.  Test commit 2e18c6dbdfb */


static const char test_data[] = "test\0data";

static hb_position_t
glyph_h_advance_func (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
		      hb_codepoint_t glyph,
		      void *user_data HB_UNUSED)
{
  switch (glyph) {
  case 1: return 10;
  case 2: return 6;
  case 3: return 5;
  }
  return 0;
}

static hb_bool_t
glyph_func (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
	    hb_codepoint_t unicode,
	    hb_codepoint_t *glyph,
	    void *user_data HB_UNUSED)
{
  switch (unicode) {
  case 'T': *glyph = 1; return TRUE;
  case 'e': *glyph = 2; return TRUE;
  case 's': *glyph = 3; return TRUE;
  }
  return FALSE;
}

static const char TesT[] = "TesT";

static void
test_font (hb_font_t *font)
{
  hb_buffer_t *buffer;
  unsigned int len;
  hb_glyph_info_t *glyphs;
  hb_glyph_position_t *positions;

  buffer =  hb_buffer_create ();
  hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
  hb_buffer_add_utf8 (buffer, TesT, 4, 0, 4);

  hb_shape (font, buffer, NULL, 0);

  len = hb_buffer_get_length (buffer);
  glyphs = hb_buffer_get_glyph_infos (buffer, NULL);
  positions = hb_buffer_get_glyph_positions (buffer, NULL);

  {
    const hb_codepoint_t output_glyphs[] = {1, 2, 3, 1};
    const hb_position_t output_x_advances[] = {10, 6, 5, 10};
    const hb_position_t output_x_offsets[] = {0, 0, 0, 0};
    unsigned int i;
    g_assert_cmpint (len, ==, 4);
    for (i = 0; i < len; i++) {
      g_assert_cmphex (glyphs[i].codepoint, ==, output_glyphs[i]);
      g_assert_cmphex (glyphs[i].cluster,   ==, i);
    }
    for (i = 0; i < len; i++) {
      g_assert_cmpint (output_x_advances[i], ==, positions[i].x_advance);
      g_assert_cmpint (output_x_offsets [i], ==, positions[i].x_offset);
      g_assert_cmpint (0, ==, positions[i].y_advance);
      g_assert_cmpint (0, ==, positions[i].y_offset);
    }
  }

  hb_buffer_destroy (buffer);
}

static void
test_shape (void)
{
  hb_blob_t *blob;
  hb_face_t *face;
  hb_font_funcs_t *ffuncs;
  hb_font_t *font, *sub_font;

  blob = hb_blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = hb_face_create (blob, 0);
  hb_blob_destroy (blob);
  font = hb_font_create (face);
  hb_face_destroy (face);
  hb_font_set_scale (font, 10, 10);

  ffuncs = hb_font_funcs_create ();
  hb_font_funcs_set_glyph_h_advance_func (ffuncs, glyph_h_advance_func, NULL, NULL);
  hb_font_funcs_set_nominal_glyph_func (ffuncs, glyph_func, malloc (10), free);
  hb_font_set_funcs (font, ffuncs, NULL, NULL);
  hb_font_funcs_destroy (ffuncs);

  test_font (font);

  sub_font = hb_font_create_sub_font (font);
  test_font (sub_font);

  hb_font_destroy (sub_font);
  hb_font_destroy (font);
}

static void
test_shape_clusters (void)
{
  hb_face_t *face;
  hb_font_t *font;
  hb_buffer_t *buffer;
  unsigned int len;
  hb_glyph_info_t *glyphs;

  face = hb_face_create (NULL, 0);
  font = hb_font_create (face);
  hb_face_destroy (face);

  buffer =  hb_buffer_create ();
  hb_buffer_set_direction (buffer, HB_DIRECTION_LTR);
  {
    /* https://crbug.com/497578 */
    hb_codepoint_t test[] = {0xFFF1, 0xF0B6};
    hb_buffer_add_utf32 (buffer, test, 2, 0, 2);
  }

  hb_shape (font, buffer, NULL, 0);

  len = hb_buffer_get_length (buffer);
  glyphs = hb_buffer_get_glyph_infos (buffer, NULL);

  {
    const hb_codepoint_t output_glyphs[] = {0};
    const hb_position_t output_clusters[] = {0};
    unsigned int i;
    g_assert_cmpint (len, ==, 1);
    for (i = 0; i < len; i++) {
      g_assert_cmphex (glyphs[i].codepoint, ==, output_glyphs[i]);
      g_assert_cmphex (glyphs[i].cluster,   ==, output_clusters[i]);
    }
  }

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
}

static void
test_shape_overlong_mark_cluster (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoSans-Bold.ttf");
  hb_font_t *font = hb_font_create (face);
  hb_buffer_t *buffer = hb_buffer_create ();

  hb_font_set_scale (font, 12 * 65536, 12 * 65536);
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  hb_buffer_add (buffer, 'e', 0);
  for (unsigned i = 0; i < 35000; i++)
    hb_buffer_add (buffer, 0x0301, i + 1);
  hb_buffer_add (buffer, 'X', 35001);
  hb_buffer_guess_segment_properties (buffer);

  hb_shape (font, buffer, NULL, 0);

  unsigned len;
  hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (buffer, &len);
  g_assert_cmpuint (len, ==, 35001);
  g_assert_cmpint (positions[len - 2].y_offset, ==, INT32_MAX);

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static hb_bool_t
soft_hyphen_glyph_func (hb_font_t *font HB_UNUSED, void *font_data HB_UNUSED,
			hb_codepoint_t unicode,
			hb_codepoint_t *glyph,
			void *user_data HB_UNUSED)
{
  if (unicode != 0x00ADu)
    return FALSE;
  *glyph = 1;
  return TRUE;
}

static void
test_shape_overlong_context_scan (void)
{
  /* A 'calt' lookup of type 8 (reverse chaining single substitution) that
   * lists the same subtable 1000 times.  The subtable covers glyph 1 and has
   * one backtrack Coverage, for glyph 2, which never occurs in the buffer.
   * Glyph 1 is a default-ignorable, so the backtrack scan of every subtable
   * at every position walks back over all the glyphs before it. */
  static const char gsub_head[] = {
    0x00, 0x01, 0x00, 0x00,		/* version 1.0 */
    0x00, 0x0A, 0x00, 0x1E, 0x00, 0x2C,	/* ScriptList, FeatureList, LookupList */
    /* ScriptList: DFLT, default LangSys with feature 0 */
    0x00, 0x01, 'D', 'F', 'L', 'T', 0x00, 0x08,
    0x00, 0x04, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x00,
    /* FeatureList: calt with lookup 0 */
    0x00, 0x01, 'c', 'a', 'l', 't', 0x00, 0x08,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    /* LookupList: one lookup of type 8 with 1000 subtables at offset 2006 */
    0x00, 0x01, 0x00, 0x04,
    0x00, 0x08, 0x00, 0x00, 0x03, 0xE8,
  };
  static const char subtable[] = {
    0x00, 0x01, 0x00, 0x0E,		/* format 1, Coverage */
    0x00, 0x01, 0x00, 0x14,		/* one backtrack Coverage */
    0x00, 0x00,				/* no lookahead */
    0x00, 0x01, 0x00, 0x01,		/* substitute glyph 1 */
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01,	/* Coverage: glyph 1 */
    0x00, 0x01, 0x00, 0x01, 0x00, 0x02,	/* Coverage: glyph 2 */
  };
  char gsub[sizeof (gsub_head) + 2 * 1000 + sizeof (subtable)];

  memcpy (gsub, gsub_head, sizeof (gsub_head));
  for (unsigned i = 0; i < 1000; i++)
  {
    gsub[sizeof (gsub_head) + 2 * i] = 0x07;
    gsub[sizeof (gsub_head) + 2 * i + 1] = 0xD6;
  }
  memcpy (gsub + sizeof (gsub_head) + 2 * 1000, subtable, sizeof (subtable));

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create (gsub, sizeof (gsub), HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_TAG ('G','S','U','B'), blob));
  hb_blob_destroy (blob);

  hb_font_t *font = hb_font_create (face);
  hb_font_funcs_t *ffuncs = hb_font_funcs_create ();
  hb_font_funcs_set_nominal_glyph_func (ffuncs, soft_hyphen_glyph_func, NULL, NULL);
  hb_font_set_funcs (font, ffuncs, NULL, NULL);
  hb_font_funcs_destroy (ffuncs);

  hb_buffer_t *buffer = hb_buffer_create ();
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  for (unsigned i = 0; i < 10000; i++)
    hb_buffer_add (buffer, 0x00ADu, i);
  hb_buffer_guess_segment_properties (buffer);

  hb_shape (font, buffer, NULL, 0);

  /* The scans are charged to the buffer's work budget, which runs out. */
  g_assert_false (hb_buffer_allocation_successful (buffer));

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
}


static void
test_shape_rule_set_budget (void)
{
  /* A 'liga' lookup whose 20 subtable offsets share one ContextSubstFormat2
   * subtable, whose RuleSet has 32700 Rule offsets that all point at one
   * Rule that never matches.  Trying all of those at every glyph must
   * exhaust the buffer's work budget instead of running to completion. */
  static const char cmap[] =
    "\0\0\0\1\0\3\0\12\0\0\0\14" /* One format 12 subtable. */
    "\0\14\0\0\0\0\0\34\0\0\0\0\0\0\0\1"
    "\0\0\0\141\0\0\0\141\0\0\0\1"; /* 'a' -> glyph 1 */
  static const char maxp[] = "\0\0\120\0\0\3";
  static const uint16_t header[] = {
    1, 0, 10, 30, 44,
    1, 0x4446, 0x4C54, 8, 4, 0, 0, 0xFFFF, 1, 0, /* ScriptList: DFLT */
    1, 0x6C69, 0x6761, 8, 0, 1, 0, /* FeatureList: liga */
    1, 4, /* LookupList */
  };
  static const uint16_t subtable[] = {
    2, 12, 18, 2, 0, 26,
    1, 1, 1, /* Coverage: glyph 1 */
    1, 1, 1, 1, /* ClassDef: glyph 1 has class 1 */
  };
  static const uint16_t rule[] = {3, 0, 1, 2}; /* Classes 1, 2: never matches. */
  const unsigned num_subtables = 20, num_rules = 32700;
  unsigned length = sizeof (header) + 6 + 2 * num_subtables +
		    sizeof (subtable) + 2 + 2 * num_rules + sizeof (rule);
  char *GSUB = (char *) malloc (length), *p = GSUB;
  g_assert_nonnull (GSUB);

#define PUT(v) (*p++ = (char) ((v) >> 8), *p++ = (char) (v))
  for (unsigned i = 0; i < G_N_ELEMENTS (header); i++)
    PUT (header[i]);
  PUT (5); PUT (0); PUT (num_subtables); /* Lookup type 5 */
  for (unsigned i = 0; i < num_subtables; i++)
    PUT (6 + 2 * num_subtables);
  for (unsigned i = 0; i < G_N_ELEMENTS (subtable); i++)
    PUT (subtable[i]);
  PUT (num_rules);
  for (unsigned i = 0; i < num_rules; i++)
    PUT (2 + 2 * num_rules);
  for (unsigned i = 0; i < G_N_ELEMENTS (rule); i++)
    PUT (rule[i]);
#undef PUT
  g_assert_cmpuint (p - GSUB, ==, length);

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob;
  blob = hb_blob_create (cmap, sizeof (cmap) - 1, HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('c','m','a','p'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create (maxp, sizeof (maxp) - 1, HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('m','a','x','p'), blob);
  hb_blob_destroy (blob);
  blob = hb_blob_create (GSUB, length, HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face_builder_add_table (face, HB_TAG ('G','S','U','B'), blob);
  hb_blob_destroy (blob);

  hb_font_t *font = hb_font_create (face);
  hb_buffer_t *buffer = hb_buffer_create ();
  for (unsigned i = 0; i < 64; i++)
    hb_buffer_add (buffer, 'a', i);
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  hb_buffer_guess_segment_properties (buffer);

  hb_shape (font, buffer, NULL, 0);

  g_assert_false (hb_buffer_allocation_successful (buffer));

  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
  free (GSUB);
}

static void
put16 (char *p, unsigned v)
{
  p[0] = (char) (v >> 8);
  p[1] = (char) v;
}

static hb_face_t *
create_aliased_lookups_face (unsigned lookup_count)
{
  /* A GSUB whose one 'ccmp' feature lists lookup_count lookups, all of them
   * the same 20-byte single substitution of glyph 1 by itself. */
  unsigned feature_len = 2 + 6 + 4 + 2 * lookup_count;
  unsigned lookup_list_len = 2 + 2 * lookup_count + 20;
  unsigned gsub_len = 10 + 20 + feature_len + lookup_list_len;
  char *gsub = calloc (gsub_len, 1);
  char *p = gsub;
  unsigned i;

  /* Header */
  put16 (p, 1);
  put16 (p + 4, 10);
  put16 (p + 6, 10 + 20);
  put16 (p + 8, 10 + 20 + feature_len);
  p += 10;
  /* ScriptList: 'DFLT' with a default LangSys using feature 0 */
  put16 (p, 1);
  memcpy (p + 2, "DFLT", 4);
  put16 (p + 6, 8);
  put16 (p + 8, 4);
  put16 (p + 10, 0);
  put16 (p + 12, 0);
  put16 (p + 14, 0xFFFF);
  put16 (p + 16, 1);
  put16 (p + 18, 0);
  p += 20;
  /* FeatureList: 'ccmp' listing every lookup */
  put16 (p, 1);
  memcpy (p + 2, "ccmp", 4);
  put16 (p + 6, 8);
  put16 (p + 8, 0);
  put16 (p + 10, lookup_count);
  for (i = 0; i < lookup_count; i++)
    put16 (p + 12 + 2 * i, i);
  p += feature_len;
  /* LookupList: every offset points at the one lookup after the offsets */
  put16 (p, lookup_count);
  for (i = 0; i < lookup_count; i++)
    put16 (p + 2 + 2 * i, 2 + 2 * lookup_count);
  p += 2 + 2 * lookup_count;
  put16 (p, 1);      /* Lookup: type 1 */
  put16 (p + 2, 0);  /* flag */
  put16 (p + 4, 1);  /* one subtable */
  put16 (p + 6, 8);
  put16 (p + 8, 1);  /* SingleSubstFormat1 */
  put16 (p + 10, 6); /* coverage */
  put16 (p + 12, 0); /* deltaGlyphID */
  put16 (p + 14, 1); /* CoverageFormat1 */
  put16 (p + 16, 1);
  put16 (p + 18, 1); /* glyph 1 */

  /* cmap: format 12, 'a' -> glyph 1 */
  static const char cmap_data[] = {0, 0, 0, 1,  0, 3, 0, 10,  0, 0, 0, 12,
				   0, 12, 0, 0,  0, 0, 0, 28,  0, 0, 0, 0,  0, 0, 0, 1,
				   0, 0, 0, 'a',  0, 0, 0, 'a',  0, 0, 0, 1};

  hb_face_t *face = hb_face_builder_create ();
  hb_blob_t *blob = hb_blob_create_or_fail (gsub, gsub_len, HB_MEMORY_MODE_WRITABLE, gsub, free);
  hb_face_builder_add_table (face, HB_TAG ('G','S','U','B'), blob);
  hb_blob_destroy (blob);
  HB_FACE_ADD_TABLE (face, "cmap", cmap_data);
  hb_face_set_glyph_count (face, 2);
  return face;
}

static void
test_shape_lookup_budget (void)
{
  /* Lookups and features can alias the same bytes, so a small font can make
   * the shaper walk the whole buffer thousands of times.  Those passes must
   * be charged to the buffer's work budget and cut off when it runs out. */
  hb_face_t *face;
  hb_font_t *font;
  hb_buffer_t *buffer;
  unsigned i;

  face = create_aliased_lookups_face (16);
  font = hb_font_create (face);
  buffer = hb_buffer_create ();
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  for (i = 0; i < 1024; i++)
    hb_buffer_add (buffer, 'a', i);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, NULL, 0);
  g_assert_true (hb_buffer_allocation_successful (buffer));
  g_assert_cmpuint (hb_buffer_get_length (buffer), ==, 1024);
  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);

  face = create_aliased_lookups_face (8192);
  font = hb_font_create (face);
  buffer = hb_buffer_create ();
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
  for (i = 0; i < 1024; i++)
    hb_buffer_add (buffer, 'a', i);
  hb_buffer_guess_segment_properties (buffer);
  hb_shape (font, buffer, NULL, 0);
  g_assert_false (hb_buffer_allocation_successful (buffer));
  hb_buffer_destroy (buffer);
  hb_font_destroy (font);
  hb_face_destroy (face);
}

static void
test_shape_list (void)
{
  const char **shapers = hb_shape_list_shapers ();

  unsigned int i;
  for (i = 0; shapers[i]; i++)
    ;

  g_assert_cmpint (i, >, 1);
  g_assert_true (!strcmp (shapers[i - 1], "fallback"));
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_shape);
  hb_test_add (test_shape_clusters);
  hb_test_add (test_shape_overlong_mark_cluster);
  hb_test_add (test_shape_lookup_budget);
  hb_test_add (test_shape_overlong_context_scan);
  hb_test_add (test_shape_rule_set_budget);
  /* TODO test fallback shaper */
  /* TODO test shaper_full */
  hb_test_add (test_shape_list);

  return hb_test_run();
}
