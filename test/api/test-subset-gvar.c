/*
 * Copyright © 2019 Adobe Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for gvar subsetting */

static void
test_subset_gvar_noop (void)
{
  hb_face_t *face_abc = hb_test_open_font_file("fonts/SourceSansVariable-Roman.abc.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('g','v','a','r'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
}

static void
test_subset_gvar (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.ac.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','v','a','r'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_gvar_retaingids (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.abc.ttf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.ac.retaingids.ttf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','v','a','r'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

#ifndef HB_NO_BEYOND_64K
static hb_face_t *
create_GVAR_face (const char *GVAR_data, unsigned GVAR_length)
{
  hb_face_t *source = hb_test_open_font_file ("fonts/SourceSansVariable-Roman.abc.ttf");
  hb_face_t *face = hb_face_builder_create ();

  unsigned count = hb_face_get_table_tags (source, 0, NULL, NULL);
  hb_tag_t *tags = g_new (hb_tag_t, count);
  unsigned total = count;
  g_assert_cmpuint (hb_face_get_table_tags (source, 0, &count, tags), ==, total);
  for (unsigned i = 0; i < count; i++)
  {
    hb_blob_t *blob = hb_face_reference_table (source, tags[i]);
    g_assert_true (hb_face_builder_add_table (face, tags[i], blob));
    hb_blob_destroy (blob);
  }
  g_free (tags);
  hb_face_destroy (source);

  hb_blob_t *blob = hb_blob_create_or_fail (GVAR_data, GVAR_length,
					     HB_MEMORY_MODE_READONLY,
					     NULL, NULL);
  g_assert_true (hb_face_builder_add_table (face, HB_TAG ('G','V','A','R'), blob));
  hb_blob_destroy (blob);

  return face;
}

static void
test_subset_GVAR_header (void)
{
  static const char GVAR_data[] = {
    0, 1, 0, 0,                 /* version 1.0 */
    0, 2,                       /* axisCount */
    0, 0,                       /* sharedTupleCount */
    0, 0, 0, 31,                /* sharedTuplesOffset */
    0, 0, 4,                    /* glyphCount */
    0, 0,                       /* flags */
    0, 0, 0, 31,                /* glyphVariationDataArrayOffset */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* glyphVariationDataOffsets */
  };
  hb_face_t *face = create_GVAR_face (GVAR_data, sizeof (GVAR_data));

  hb_set_t *codepoints = hb_set_create ();
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');
  hb_face_t *subset = hb_subset_test_create_subset (
      face, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('G','V','A','R'));
  unsigned length;
  const char *data = hb_blob_get_data (blob, &length);
  g_assert_cmpuint (length, ==, sizeof (GVAR_data));
  g_assert_cmpmem (data, length, GVAR_data, sizeof (GVAR_data));
  hb_blob_destroy (blob);

  hb_face_destroy (subset);
  hb_face_destroy (face);
}

static void
test_subset_GVAR_all_axes_pinned (void)
{
  static const char GVAR_data[] = "\0";
  hb_face_t *face = create_GVAR_face (GVAR_data, sizeof (GVAR_data));

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  g_assert_true (hb_subset_input_pin_all_axes_to_default (input, face));
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);

  hb_blob_t *blob = hb_face_reference_table (subset, HB_TAG ('G','V','A','R'));
  g_assert_cmpuint (hb_blob_get_length (blob), ==, 0);
  hb_blob_destroy (blob);

  hb_face_destroy (subset);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}
#endif

static void
test_subset_gvar_shared_glyph_data (void)
{
  /* Every even glyph is the same bytes of glyf, which hold 65536 points: a
   * simple glyph of repeated flags, or a composite glyph.  Partial instancing
   * keeps the points of all the retained glyphs in the plan; the plan must
   * fail instead of holding ~150 MB for this 400 KB font. */
  enum { NUM_GLYPHS = 200, NUM_POINTS = 65536 };
  static char head[54] = {
    0, 1, 0, 0,                 /* version 1.0 */
    0, 0, 0, 0,                 /* fontRevision */
    0, 0, 0, 0,                 /* checksumAdjustment */
    0x5F, 0x0F, 0x3C, (char) 0xF5, /* magicNumber */
    0, 0,                       /* flags */
    0x03, (char) 0xE8,          /* unitsPerEm */
    /* ... indexToLocFormat set below */
  };
  static const char maxp[] = {
    0, 0, 0x50, 0,              /* version 0.5 */
    0, (char) NUM_GLYPHS,       /* numGlyphs */
  };
  static const char fvar[] = {
    0, 1, 0, 0,                 /* version 1.0 */
    0, 16,                      /* axesArrayOffset */
    0, 2,                       /* reserved */
    0, 2,                       /* axisCount */
    0, 20,                      /* axisSize */
    0, 0,                       /* instanceCount */
    0, 12,                      /* instanceSize */
    'w', 'g', 'h', 't', 0, 100, 0, 0, 1, (char) 144, 0, 0, 3, (char) 132, 0, 0, 0, 0, 1, 0,
    'w', 'd', 't', 'h', 0, 50, 0, 0, 0, 100, 0, 0, 0, (char) 200, 0, 0, 0, 0, 1, 1,
  };
  static char loca[4 * (NUM_GLYPHS + 1)];
  static char glyf[10 + 6 * NUM_POINTS];

  head[51] = 1; /* indexToLocFormat: long */

  for (unsigned composite = 0; composite < 2; composite++)
  {
    unsigned length = 0;
    memset (glyf, 0, sizeof (glyf));
    if (composite)
    {
      glyf[0] = glyf[1] = (char) 0xFF; /* numberOfContours = -1 */
      for (length = 10; length < sizeof (glyf); length += 6)
      {
	glyf[length + 1] = 0x20; /* flags: MORE_COMPONENTS */
	glyf[length + 3] = 1;    /* glyphIndex */
      }
      glyf[length - 5] = 0; /* last component */
    }
    else
    {
      glyf[1] = 1; /* numberOfContours */
      glyf[10] = glyf[11] = (char) 0xFF; /* endPtsOfContours[0] */
      for (length = 14; length < 14 + 2 * NUM_POINTS / 256; length += 2)
      {
	glyf[length] = 0x39; /* ON_CURVE | REPEAT | X_SAME | Y_SAME */
	glyf[length + 1] = (char) 0xFF; /* repeat count */
      }
    }
    /* loca alternates 0, length: even glyphs share the data, odd are empty. */
    for (unsigned i = 1; i <= NUM_GLYPHS; i += 2)
    {
      loca[4 * i + 1] = (char) (length >> 16);
      loca[4 * i + 2] = (char) (length >> 8);
      loca[4 * i + 3] = (char) length;
    }

    hb_face_t *face = hb_face_builder_create ();
    HB_FACE_ADD_TABLE (face, "head", head);
    HB_FACE_ADD_TABLE (face, "maxp", maxp);
    HB_FACE_ADD_TABLE (face, "fvar", fvar);
    HB_FACE_ADD_TABLE (face, "loca", loca);
    HB_FACE_ADD_TABLE (face, "glyf", glyf);

    for (unsigned all = 0; all < 2; all++)
    {
      hb_subset_input_t *input = hb_subset_input_create_or_fail ();
      hb_set_add_range (hb_subset_input_glyph_set (input), 0, all ? NUM_GLYPHS - 1 : 4);
      g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','g','h','t'), 500));

      hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
      if (all)
	g_assert_null (plan);
      else
	g_assert_nonnull (plan);

      hb_subset_plan_destroy (plan);
      hb_subset_input_destroy (input);
    }

    hb_face_destroy (face);
  }
}

static void
test_subset_gvar_invalid_glyf_format (void)
{
  /* head.glyphDataFormat > 1 disables the glyf accelerator, so
   * reference_glyf_table () hands back no blob.  Partial instancing (only
   * one of the two axes pinned) still enters
   * get_instance_glyphs_contour_points (), which must not take the length of
   * that null blob. */
  static char head[54] = {
    0, 1, 0, 0,                 /* version 1.0 */
    0, 0, 0, 0,                 /* fontRevision */
    0, 0, 0, 0,                 /* checksumAdjustment */
    0x5F, 0x0F, 0x3C, (char) 0xF5, /* magicNumber */
    0, 0,                       /* flags */
    0x03, (char) 0xE8,          /* unitsPerEm */
    /* ... indexToLocFormat and glyphDataFormat set below */
  };
  static const char maxp[] = {
    0, 0, 0x50, 0,              /* version 0.5 */
    0, 2,                       /* numGlyphs */
  };
  static const char fvar[] = {
    0, 1, 0, 0,                 /* version 1.0 */
    0, 16,                      /* axesArrayOffset */
    0, 2,                       /* reserved */
    0, 2,                       /* axisCount */
    0, 20,                      /* axisSize */
    0, 0,                       /* instanceCount */
    0, 12,                      /* instanceSize */
    'w', 'g', 'h', 't', 0, 100, 0, 0, 1, (char) 144, 0, 0, 3, (char) 132, 0, 0, 0, 0, 1, 0,
    'w', 'd', 't', 'h', 0, 50, 0, 0, 0, 100, 0, 0, 0, (char) 200, 0, 0, 0, 0, 1, 1,
  };
  static const char loca[] = { 0, 0, 0, 0, 0, 0 }; /* short loca, 2 empty glyphs */
  static const char glyf[] = { 0 };

  head[51] = 0; /* indexToLocFormat: short */
  head[53] = 2; /* glyphDataFormat: invalid (> 1) */

  hb_face_t *face = hb_face_builder_create ();
  HB_FACE_ADD_TABLE (face, "head", head);
  HB_FACE_ADD_TABLE (face, "maxp", maxp);
  HB_FACE_ADD_TABLE (face, "fvar", fvar);
  HB_FACE_ADD_TABLE (face, "loca", loca);
  HB_FACE_ADD_TABLE (face, "glyf", glyf);

  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_glyph_set (input), 0, 1);
  g_assert_true (hb_subset_input_pin_axis_location (input, face, HB_TAG ('w','g','h','t'), 500));

  /* The point is that this does not crash. */
  hb_subset_plan_t *plan = hb_subset_plan_create_or_fail (face, input);
  hb_subset_plan_destroy (plan);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_gvar_noop);
  hb_test_add (test_subset_gvar);
  hb_test_add (test_subset_gvar_retaingids);
  hb_test_add (test_subset_gvar_shared_glyph_data);
  hb_test_add (test_subset_gvar_invalid_glyf_format);
#ifndef HB_NO_BEYOND_64K
  hb_test_add (test_subset_GVAR_header);
  hb_test_add (test_subset_GVAR_all_axes_pinned);
#endif

  return hb_test_run ();
}
