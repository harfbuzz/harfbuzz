/*
 * Copyright © 2020  Google, Inc.
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
 * Google Author(s): Calder Kitagawa
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for CBDT/CBLC subsetting */

static void
test_subset_cbdt_noop (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x38);
  hb_set_add (codepoints, 0x39);
  hb_set_add (codepoints, 0xAE);
  hb_set_add (codepoints, 0x2049);
  hb_set_add (codepoints, 0x20E3);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face, face_subset, HB_TAG ('C','B','L','C'));
  hb_subset_test_check (face, face_subset, HB_TAG ('C','B','D','T'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

static void
test_subset_cbdt_keep_one (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.default.39.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x39);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

static void
test_subset_cbdt_keep_one_last_subtable (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.default.2049.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x2049);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

static void
test_subset_cbdt_keep_multiple_subtables (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.multiple_size_tables.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.multiple_size_tables.default.38,AE,2049.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x38);
  hb_set_add (codepoints, 0xAE);
  hb_set_add (codepoints, 0x2049);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

static void
test_subset_cbdt_index_format_3 (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.index_format3.ttf");
  hb_face_t *face_expected = hb_test_open_font_file ("fonts/NotoColorEmoji.subset.index_format3.default.38,AE,2049.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_subset;
  hb_set_add (codepoints, 0x38);
  hb_set_add (codepoints, 0xAE);
  hb_set_add (codepoints, 0x2049);
  face_subset = hb_subset_test_create_subset (face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  hb_subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  hb_face_destroy (face_subset);
  hb_face_destroy (face_expected);
  hb_face_destroy (face);
}

static void
test_subset_cbdt_overlapping_images (void)
{
  /* One format 3 subtable whose offsetArray alternates 0, CBDT_LENGTH, so
   * every even glyph claims the whole CBDT table as its image and every odd
   * glyph has none.  Each block is in bounds, but copying them all would
   * blow the output up to NUM_GLYPHS / 2 times the source table. */
  enum { NUM_GLYPHS = 1024, CBDT_LENGTH = 1024 };
  static const char MAXP_data[6] =
    "\x00\x00\x50\x00" /* version */
    "\x04\x00" /* numGlyphs */
    ;
  static const char CBDT_data[CBDT_LENGTH] =
    "\x00\x03\x00\x00" /* version */
    ;
  static char CBLC_data[8 + 48 + 8 + 8 + 2 * (NUM_GLYPHS + 1)] =
    "\x00\x03\x00\x00" /* version */
    "\x00\x00\x00\x01" /* numSizes */
    /* BitmapSize */
    "\x00\x00\x00\x38" /* indexSubtableArrayOffset */
    "\x00\x00\x00\x00" /* indexTablesSize */
    "\x00\x00\x00\x01" /* numberOfIndexSubtables */
    "\x00\x00\x00\x00" /* colorRef */
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" /* hori */
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" /* vert */
    "\x00\x00" /* startGlyphIndex */
    "\x03\xFF" /* endGlyphIndex */
    "\x20\x20\x20\x01" /* ppemX, ppemY, bitDepth, flags */
    /* IndexSubtableRecord */
    "\x00\x00" /* firstGlyphIndex */
    "\x03\xFF" /* lastGlyphIndex */
    "\x00\x00\x00\x08" /* offsetToSubtable */
    /* IndexSubtableFormat3 */
    "\x00\x03" /* indexFormat */
    "\x00\x11" /* imageFormat */
    "\x00\x00\x00\x00" /* imageDataOffset */
    ;
  char *offsets = CBLC_data + 8 + 48 + 8 + 8;
  for (unsigned i = 1; i <= NUM_GLYPHS; i += 2)
  {
    offsets[2 * i] = CBDT_LENGTH >> 8;
    offsets[2 * i + 1] = CBDT_LENGTH & 0xFF;
  }

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "maxp", MAXP_data);
  HB_FACE_ADD_TABLE (face, "CBLC", CBLC_data);
  HB_FACE_ADD_TABLE (face, "CBDT", CBDT_data);

  hb_set_t *glyphs = hb_set_create ();
  hb_set_add_range (glyphs, 0, NUM_GLYPHS - 1);
  hb_face_t *face_subset = hb_subset_test_create_subset (
      face, hb_subset_test_create_input_from_glyphs (glyphs));
  hb_set_destroy (glyphs);

  /* The subsetter copies at most HB_SUBSET_CBDT_MAX_COPY_FACTOR times the
   * source table; here that drops the strike instead of emitting 512 KiB. */
  hb_blob_t *cbdt = hb_face_reference_table (face_subset, HB_TAG ('C','B','D','T'));
  g_assert_cmpuint (hb_blob_get_length (cbdt), <=, 64 * CBDT_LENGTH);
  hb_blob_destroy (cbdt);

  hb_face_destroy (face_subset);
  hb_face_destroy (face);
}

// TODO: add support/tests for index formats 2,4,5 (image formats are treated as
// opaque blobs when subsetting so don't need to be tested separately).
// TODO: add a test that keeps no codepoints.

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_cbdt_noop);
  hb_test_add (test_subset_cbdt_keep_one);
  hb_test_add (test_subset_cbdt_keep_one_last_subtable);
  // The following use manually crafted expectation files as they are not
  // binary compatible with FontTools.
  hb_test_add (test_subset_cbdt_keep_multiple_subtables);
  // Can use FontTools after https://github.com/fonttools/fonttools/issues/1817
  // is resolved.
  hb_test_add (test_subset_cbdt_index_format_3);
  hb_test_add (test_subset_cbdt_overlapping_images);

  return hb_test_run();
}
