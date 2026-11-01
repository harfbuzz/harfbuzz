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

#if !defined(HB_NO_SHAPE) && !defined(HB_NO_OT_SHAPE) && !defined(HB_NO_AAT_SHAPE)

typedef struct {
  hb_face_t *source;
  hb_blob_t *kerx;
} kerx_budget_data_t;

static hb_blob_t *
kerx_budget_reference_table (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data)
{
  kerx_budget_data_t *data = user_data;
  return tag == HB_TAG ('k','e','r','x') ? hb_blob_reference (data->kerx) :
					hb_face_reference_table (data->source, tag);
}

static void
test_aat_kerx_action_budget (void)
{
  hb_face_t *source = hb_test_open_font_file (
      "../shape/data/in-house/fonts/6bf9a24264162575b51da4af1eafaac975f0af2d.ttf");
  hb_blob_t *original = hb_face_reference_table (source, HB_TAG ('k','e','r','x'));
  const char *bytes = hb_blob_get_data (original, NULL);
  guint32 values_offset;
  memcpy (&values_offset, bytes + 36, sizeof (values_offset));
  values_offset = 20 + GUINT32_FROM_BE (values_offset);

  for (unsigned terminated = 0; terminated < 2; terminated++)
  {
    GByteArray *kerx = g_byte_array_new ();
    g_byte_array_append (kerx, (const guint8 *) bytes, values_offset);
    /* Reusing one glyph's history must still respect its 65536-op budget. */
    guint16 value = GUINT16_TO_BE (1);
    for (unsigned i = 0; i < 65537; i++)
      g_byte_array_append (kerx, (const guint8 *) &value, sizeof (value));
    if (terminated)
    {
      value = GUINT16_TO_BE (0xFFFF);
      g_byte_array_append (kerx, (const guint8 *) &value, sizeof (value));
    }
    guint32 length = GUINT32_TO_BE (kerx->len - 8);
    memcpy (kerx->data + 8, &length, sizeof (length));
    kerx_budget_data_t data = {
      source,
      hb_blob_create ((const char *) kerx->data, kerx->len, HB_MEMORY_MODE_DUPLICATE, NULL, NULL)
    };
    hb_face_t *face = hb_face_create_for_tables (kerx_budget_reference_table, &data, NULL);
    hb_font_t *font = hb_font_create (face);
    hb_buffer_t *buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (buffer, "a", -1, 0, -1);
    hb_buffer_guess_segment_properties (buffer);
    const char *shapers[] = {"ot", NULL};
    hb_shape_full (font, buffer, NULL, 0, shapers);
    g_assert_false (hb_buffer_allocation_successful (buffer));

    hb_buffer_destroy (buffer);
    hb_font_destroy (font);
    hb_face_destroy (face);
    hb_blob_destroy (data.kerx);
    g_byte_array_unref (kerx);
  }
  hb_blob_destroy (original);
  hb_face_destroy (source);
}

#endif

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
#if !defined(HB_NO_SHAPE) && !defined(HB_NO_OT_SHAPE) && !defined(HB_NO_AAT_SHAPE)
  hb_test_add (test_aat_kerx_action_budget);
#endif
#endif

  face = hb_test_open_font_file ("fonts/aat-feat.ttf");
  sbix = hb_test_open_font_file ("fonts/chromacheck-sbix.ttf");
  status = hb_test_run ();
  hb_face_destroy (sbix);
  hb_face_destroy (face);
  return status;
}
