/*
 * Copyright © 2018 Adobe Inc.
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

/* Unit tests for CFF2 subsetting */

static void
test_subset_cff2_noop (void)
{
  hb_face_t *face_abc = hb_test_open_font_file("fonts/AdobeVFPrototype.abc.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'b');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_abc, face_abc_subset, HB_TAG ('C','F','F','2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
}

static void
test_subset_cff2 (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  face_abc_subset = hb_subset_test_create_subset (face_abc, hb_subset_test_create_input (codepoints));
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F','2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_strip_hints (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_desubr (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.nosubrs.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_desubr_strip_hints (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.nosubrs.nohints.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_DESUBROUTINIZE | HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

static void
test_subset_cff2_retaingids (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_ac = hb_test_open_font_file ("fonts/AdobeVFPrototype.ac.retaingids.otf");

  hb_set_t *codepoints = hb_set_create ();
  hb_subset_input_t *input;
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 'a');
  hb_set_add (codepoints, 'c');
  input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_ac);
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_cff2_iftb_requirements (void)
{
  hb_face_t *face_abc = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  hb_face_t *face_long_off = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.long_off.otf");

  hb_set_t *codepoints = hb_set_create();
  hb_face_t *face_abc_subset;
  hb_set_add (codepoints, 97);
  hb_set_add (codepoints, 98);
  hb_set_add (codepoints, 99);

  hb_subset_input_t *input = hb_subset_test_create_input (codepoints);
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_IFTB_REQUIREMENTS);
  face_abc_subset = hb_subset_test_create_subset (face_abc, input);
  hb_set_destroy (codepoints);

  hb_subset_test_check (face_long_off, face_abc_subset, HB_TAG ('C','F','F', '2'));

  hb_face_destroy (face_abc_subset);
  hb_face_destroy (face_abc);
  hb_face_destroy (face_long_off);

}
#endif

/* Caller-dependent subroutine numbers cannot be remapped at a shared offset. */
static void
test_subset_cff2_caller_supplied_subr (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2_caller_subr.otf");
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'B');
  for (unsigned i = 0; i < 2; i++)
  {
    hb_subset_input_set_flags (input, i ? HB_SUBSET_FLAGS_NO_HINTING : HB_SUBSET_FLAGS_DEFAULT);
    hb_face_t *subset = hb_subset_or_fail (face, input);
    g_assert_null (subset);
  }
  /* Explicit flattening does not remap caller-supplied operands. */
  hb_subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  hb_font_t *font = hb_font_create (subset);
  hb_glyph_extents_t extents;
  g_assert_true (hb_font_get_glyph_extents (font, 1, &extents));
  g_assert_cmpint (extents.width, ==, 10);
  g_assert_true (hb_font_get_glyph_extents (font, 2, &extents));
  g_assert_cmpint (extents.width, ==, 20);
  hb_font_destroy (font);
  hb_face_destroy (subset);
  hb_subset_input_destroy (input);
  hb_face_destroy (face);
}

static void
test_subset_cff2_long_private_blends (void)
{
  hb_face_t *face = hb_test_open_font_file ("fonts/cff2_long_private_blends.otf");
  hb_blob_t *source = hb_face_reference_table (face, HB_TAG ('C','F','F','2'));
  unsigned source_length;
  const char *source_data = hb_blob_get_data (source, &source_length);
  /* This fixture's Private DICT occupies [395, 974), including its final
   * BlueValues operator. Its live stack never exceeds 513 operands. */
  const unsigned private_offset = 395, private_length = 579;
  g_assert_cmpuint (private_offset + private_length, <=, source_length);
  hb_subset_input_t *input = hb_subset_input_create_or_fail ();
  hb_set_add_range (hb_subset_input_unicode_set (input), 'A', 'B');
  hb_face_t *subset = hb_subset_or_fail (face, input);
  g_assert_nonnull (subset);
  hb_blob_t *result = hb_face_reference_table (subset, HB_TAG ('C','F','F','2'));
  unsigned length;
  const char *data = hb_blob_get_data (result, &length);
  gboolean found = FALSE;
  for (unsigned i = 0; i + private_length <= length; i++)
    if (!memcmp (source_data + private_offset, data + i, private_length))
    {
      found = TRUE;
      break;
    }
  g_assert_true (found);
  hb_blob_destroy (result);
  hb_face_destroy (subset);
  hb_subset_input_destroy (input);
  hb_blob_destroy (source);
  hb_face_destroy (face);
}

static gboolean
blob_contains_bytes (hb_blob_t *blob, const unsigned char *needle, unsigned needle_len)
{
  unsigned len = 0;
  const unsigned char *data = (const unsigned char *) hb_blob_get_data (blob, &len);
  if (len < needle_len)
    return FALSE;
  for (unsigned i = 0; i + needle_len <= len; i++)
    if (!memcmp (data + i, needle, needle_len))
      return TRUE;
  return FALSE;
}

static void
test_subset_cff2_pinned_private_blend_overflow (void)
{
  /* StdVW blend delta of 1e12; full pinning and CFF2->CFF1 downgrade flatten
   * Private DICT blends directly without a partial instancer and must clamp
   * out-of-range rounded values to INT_MAX instead of invoking float-to-int UB
   * (which produces x86 integer indefinite INT_MIN 0x80000000 in release builds). */
  /* CFF BCD encoding of +2.1474836E9 (clamped INT_MAX) + OpCode_StdVW (0x0b),
   * vs negative -2.1474836E9 (0x1e 0xe2 ...) from unpatched float-to-int UB. */
  static const unsigned char clamped_int_max[8] = {0x1e, 0x21, 0x47, 0x48, 0x36, 0xb2, 0xff, 0x0b};
  static const unsigned char indef_int_min[6]   = {0x1e, 0xe2, 0x14, 0x74, 0x83, 0x6b};
  hb_face_t *face = hb_test_open_font_file ("fonts/AdobeVFPrototype.abc.huge-blend.otf");
  const hb_subset_flags_t modes[] = {
    HB_SUBSET_FLAGS_DEFAULT,
    HB_SUBSET_FLAGS_DOWNGRADE_CFF2,
  };

  for (unsigned m = 0; m < G_N_ELEMENTS (modes); m++)
  {
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add (hb_subset_input_unicode_set (input), 0x61);
    hb_subset_input_set_flags (input, modes[m]);
    g_assert_true (hb_subset_input_pin_axis_location (input, face,
                                                      HB_TAG ('w', 'g', 'h', 't'), 600.f));
    g_assert_true (hb_subset_input_pin_axis_location (input, face,
                                                      HB_TAG ('C', 'N', 'T', 'R'), 100.f));

    hb_face_t *subset = hb_subset_or_fail (face, input);
    hb_subset_input_destroy (input);
    g_assert_nonnull (subset);

    hb_blob_t *cff_blob = hb_face_reference_table (subset,
                                                   modes[m] == HB_SUBSET_FLAGS_DOWNGRADE_CFF2
                                                   ? HB_TAG ('C', 'F', 'F', ' ')
                                                   : HB_TAG ('C', 'F', 'F', '2'));
    g_assert_cmpuint (hb_blob_get_length (cff_blob), >, 0);
    g_assert_true (blob_contains_bytes (cff_blob, clamped_int_max, sizeof (clamped_int_max)));
    g_assert_false (blob_contains_bytes (cff_blob, indef_int_min, sizeof (indef_int_min)));
    hb_blob_destroy (cff_blob);

    hb_font_t *font = hb_font_create (subset);
    hb_codepoint_t glyph = 0;
    hb_glyph_extents_t extents;
    g_assert_true (hb_font_get_nominal_glyph (font, 0x61, &glyph));
    g_assert_true (hb_font_get_glyph_extents (font, glyph, &extents));
    g_assert_cmpint (extents.width, >, 0);
    hb_font_destroy (font);

    /* Re-subsetting the produced static face must also succeed cleanly. */
    hb_subset_input_t *reinput = hb_subset_input_create_or_fail ();
    hb_set_add (hb_subset_input_unicode_set (reinput), 0x61);
    hb_face_t *resubset = hb_subset_or_fail (subset, reinput);
    hb_subset_input_destroy (reinput);
    g_assert_nonnull (resubset);
    hb_face_destroy (resubset);
    hb_face_destroy (subset);
  }

  hb_face_destroy (face);
}

static hb_face_t *
create_cff2_face_with_top_dict_expansion (unsigned matrix_count, unsigned short_cs_ops)
{
  /* Build a valid 1-glyph (.notdef) CFF2 + maxp face whose input Top DICT fits
   * within HBUINT16 (topDictSize = 8 * matrix_count + 2 * short_cs_ops + 13 <= 65535),
   * while each 2-byte short CharStrings op (0x8b 0x11, overridden by the final real
   * CharStrings offset) expands to a 6-byte link4 op during CFF2 Top DICT subset
   * serialization (+4 bytes per short_cs_ops). */
  const unsigned top_dict_size = 8 * matrix_count + 2 * short_cs_ops + 13;
  g_assert_cmpuint (top_dict_size, <=, 0xFFFFu);
  const unsigned cs_off = 5 + top_dict_size + 4;
  const unsigned fd_off = cs_off + 7;
  const unsigned total_len = fd_off + 14;

  unsigned char *buf = (unsigned char *) g_malloc0 (total_len);
  unsigned p = 0;
  buf[p++] = 0x02;
  buf[p++] = 0x00;
  buf[p++] = 0x05;
  buf[p++] = (unsigned char) (top_dict_size >> 8);
  buf[p++] = (unsigned char) (top_dict_size & 0xFF);

  static const unsigned char font_matrix_op[8] = {
    0x8c, 0x8b, 0x8b, 0x8c, 0x8b, 0x8b, 0x0c, 0x07
  };
  for (unsigned i = 0; i < matrix_count; i++)
  {
    memcpy (buf + p, font_matrix_op, sizeof (font_matrix_op));
    p += sizeof (font_matrix_op);
  }
  for (unsigned i = 0; i < short_cs_ops; i++)
  {
    buf[p++] = 0x8b;
    buf[p++] = 0x11;
  }

  /* Final authoritative CharStrings offset (5-byte int + op 17 = 6 bytes) */
  buf[p++] = 0x1d;
  buf[p++] = (unsigned char) (cs_off >> 24);
  buf[p++] = (unsigned char) (cs_off >> 16);
  buf[p++] = (unsigned char) (cs_off >> 8);
  buf[p++] = (unsigned char) (cs_off & 0xFF);
  buf[p++] = 0x11;

  /* FDArray offset (5-byte int + op 12 36 = 7 bytes) */
  buf[p++] = 0x1d;
  buf[p++] = (unsigned char) (fd_off >> 24);
  buf[p++] = (unsigned char) (fd_off >> 16);
  buf[p++] = (unsigned char) (fd_off >> 8);
  buf[p++] = (unsigned char) (fd_off & 0xFF);
  buf[p++] = 0x0c;
  buf[p++] = 0x24;
  g_assert_cmpuint (p, ==, 5 + top_dict_size);

  /* GlobalSubrs INDEX: count = 0 (4 bytes) */
  p += 4;
  g_assert_cmpuint (p, ==, cs_off);

  /* CharStrings INDEX: 1 empty .notdef charstring (7 bytes) */
  static const unsigned char cs_index[7] = {0, 0, 0, 1, 1, 1, 1};
  memcpy (buf + p, cs_index, sizeof (cs_index));
  p += sizeof (cs_index);
  g_assert_cmpuint (p, ==, fd_off);

  /* FDArray INDEX: 1 Font DICT of 7 bytes with 0-byte Private at total_len */
  buf[p++] = 0x00; buf[p++] = 0x00; buf[p++] = 0x00; buf[p++] = 0x01;
  buf[p++] = 0x01;
  buf[p++] = 0x01; buf[p++] = 0x08;
  buf[p++] = 0x8b; /* Private size = 0 */
  buf[p++] = 0x1d;
  buf[p++] = (unsigned char) (total_len >> 24);
  buf[p++] = (unsigned char) (total_len >> 16);
  buf[p++] = (unsigned char) (total_len >> 8);
  buf[p++] = (unsigned char) (total_len & 0xFF);
  buf[p++] = 0x12; /* OpCode_Private */
  g_assert_cmpuint (p, ==, total_len);

  static const unsigned char maxp_data[6] = {0x00, 0x00, 0x50, 0x00, 0x00, 0x01};
  hb_face_t *builder = hb_face_builder_create ();
  hb_blob_t *cff2_blob = hb_blob_create ((const char *) buf, total_len,
                                         HB_MEMORY_MODE_READONLY, buf, g_free);
  hb_blob_t *maxp_blob = hb_blob_create ((const char *) maxp_data, sizeof (maxp_data),
                                         HB_MEMORY_MODE_READONLY, NULL, NULL);
  g_assert_true (hb_face_builder_add_table (builder, HB_TAG ('C', 'F', 'F', '2'), cff2_blob));
  g_assert_true (hb_face_builder_add_table (builder, HB_TAG ('m', 'a', 'x', 'p'), maxp_blob));
  hb_blob_destroy (cff2_blob);
  hb_blob_destroy (maxp_blob);
  return builder;
}

static void
test_subset_cff2_top_dict_size_overflow (void)
{
  /* Positive control: input topDictSize = 32021, output topDictSize = 32037 <= 0xFFFF. */
  {
    hb_face_t *face = create_cff2_face_with_top_dict_expansion (4000, 4);
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add (hb_subset_input_glyph_set (input), 0);
    hb_face_t *subset = hb_subset_or_fail (face, input);
    hb_subset_input_destroy (input);
    g_assert_nonnull (subset);

    hb_blob_t *out_cff2 = hb_face_reference_table (subset, HB_TAG ('C', 'F', 'F', '2'));
    unsigned out_len = 0;
    const unsigned char *out_data = (const unsigned char *) hb_blob_get_data (out_cff2, &out_len);
    g_assert_cmpuint (out_len, >, 32042u);
    unsigned out_top_dict_size = ((unsigned) out_data[3] << 8) | out_data[4];
    g_assert_cmpuint (out_top_dict_size, ==, 32037u);
    hb_blob_destroy (out_cff2);
    hb_face_destroy (subset);
    hb_face_destroy (face);
  }

  /* Negative overflow case: input topDictSize = 65525 <= 0xFFFF expands during
   * CFF2 Top DICT serialization to 65541 > 0xFFFF, which must fail cleanly
   * instead of narrowing cff2->topDictSize modulo 65536 (to 5). */
  {
    hb_face_t *face = create_cff2_face_with_top_dict_expansion (8188, 4);
    hb_subset_input_t *input = hb_subset_input_create_or_fail ();
    hb_set_add (hb_subset_input_glyph_set (input), 0);
    hb_face_t *subset = hb_subset_or_fail (face, input);
    hb_subset_input_destroy (input);
    g_assert_null (subset);
    hb_face_destroy (face);
  }
}

int
main (int argc, char **argv)
{
  hb_test_init (&argc, &argv);

  hb_test_add (test_subset_cff2_caller_supplied_subr);
  hb_test_add (test_subset_cff2_long_private_blends);
  hb_test_add (test_subset_cff2_pinned_private_blend_overflow);
  hb_test_add (test_subset_cff2_top_dict_size_overflow);

  hb_test_add (test_subset_cff2_noop);
  hb_test_add (test_subset_cff2);
  hb_test_add (test_subset_cff2_strip_hints);
  hb_test_add (test_subset_cff2_desubr);
  hb_test_add (test_subset_cff2_desubr_strip_hints);
  hb_test_add (test_subset_cff2_retaingids);

#ifdef HB_EXPERIMENTAL_API
  hb_test_add (test_subset_cff2_iftb_requirements);
#endif

  return hb_test_run ();
}
