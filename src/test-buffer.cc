/*
 * Copyright © 2026  Behdad Esfahbod
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
 */

#include "hb.hh"
#include "hb-buffer.hh"

static void
test_cluster_flag_ranges ()
{
  struct test_t {
    unsigned clusters[4];
    unsigned minimum;
    bool monotone;
  };
  const test_t tests[] = {
    {{0, 0, 1, 1}, 0, true},
    {{1, 1, 0, 0}, 0, true},
    {{2, 0, 1, 3}, 0, false},
    {{0, 2, 0, 1}, 0, false},
    {{2, 0, 1, 0}, 0, false},
    {{3, 3, 4, 4}, 3, true},
  };
  hb_buffer_t *buffer = hb_buffer_create ();
  hb_always_assert (hb_buffer_allocation_successful (buffer));
  const hb_mask_t feature_mask = 1u << 10;
  const hb_mask_t flags = HB_GLYPH_FLAG_UNSAFE_TO_BREAK | HB_GLYPH_FLAG_UNSAFE_TO_CONCAT;

  for (unsigned level = 0; level <= HB_BUFFER_CLUSTER_LEVEL_GRAPHEMES; level++)
  {
    buffer->cluster_level = (hb_buffer_cluster_level_t) level;
    for (const auto &test : tests)
    {
      if (HB_BUFFER_CLUSTER_LEVEL_IS_MONOTONE (level) && !test.monotone)
        continue;

      hb_glyph_info_t infos[4] = {};
      for (unsigned i = 0; i < ARRAY_LENGTH (infos); i++)
      {
        infos[i].cluster = test.clusters[i];
        infos[i].mask = feature_mask;
      }
      hb_always_assert (buffer->_infos_find_min_cluster (infos, 0, 4) == test.minimum);
      hb_always_assert (buffer->_infos_find_min_cluster (infos, 0, 4, 0) == 0);
      hb_always_assert (buffer->_infos_find_min_cluster (infos, 4, 4, 99) == 99);

      buffer->_infos_set_glyph_flags (infos, 0, 4, test.minimum, flags);
      buffer->_infos_set_glyph_flags (infos, 4, 4, test.minimum, flags);
      for (unsigned i = 0; i < ARRAY_LENGTH (infos); i++)
      {
        hb_mask_t expected = feature_mask | (infos[i].cluster == test.minimum ? 0 : flags);
        hb_always_assert (infos[i].mask == expected);
      }
    }
  }
  hb_buffer_destroy (buffer);
}

static void
test_output_flag_ranges ()
{
  hb_buffer_t *buffer = hb_buffer_create ();
  for (unsigned i = 0; i < 24; i++)
    hb_buffer_add (buffer, 'a', i);
  hb_buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_GLYPHS);
  hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (buffer, nullptr);
  hb_always_assert (positions);

  /* Model expansion of the consumed input: 12 input glyphs became 18
   * output glyphs, with another 12 input glyphs still unconsumed. */
  buffer->out_info = reinterpret_cast<hb_glyph_info_t *> (positions);
  buffer->out_len = 18;
  buffer->idx = 12;
  buffer->have_output = true;
  buffer->have_positions = false;

  const hb_mask_t feature_mask = 1u << 10;
  const hb_mask_t flags = HB_GLYPH_FLAG_UNSAFE_TO_BREAK | HB_GLYPH_FLAG_UNSAFE_TO_CONCAT;
  const unsigned ranges[][2] = {{15, 18}, {9, 12}, {17, 18}, {18, 18}};
  for (unsigned level = 0; level <= HB_BUFFER_CLUSTER_LEVEL_GRAPHEMES; level++)
    for (const auto &range : ranges)
    {
      buffer->cluster_level = (hb_buffer_cluster_level_t) level;
      buffer->max_ops = 1000;
      for (unsigned i = 0; i < buffer->len; i++)
        buffer->info[i].mask = feature_mask;
      for (unsigned i = 0; i < buffer->out_len; i++)
      {
        buffer->out_info[i] = {};
        buffer->out_info[i].cluster = i / 3 * 2 + (i % 3 == 2);
        buffer->out_info[i].mask = feature_mask;
      }

      buffer->unsafe_to_break_in_outbuffer (range[0], range[1]);
      for (unsigned i = 0; i < buffer->len; i++)
        hb_always_assert (buffer->info[i].mask == feature_mask);
      for (unsigned i = 0; i < buffer->out_len; i++)
      {
        bool unsafe = range[1] - range[0] == 3 && i == range[1] - 1;
        hb_always_assert (buffer->out_info[i].mask == (feature_mask | (unsafe ? flags : 0)));
        hb_always_assert (buffer->out_info[i].cluster == i / 3 * 2 + (i % 3 == 2));
      }
      hb_always_assert (buffer->successful);
    }

  buffer->max_ops = 0;
  buffer->unsafe_to_break_in_outbuffer (15, 18);
  hb_always_assert (!buffer->successful);
  hb_buffer_destroy (buffer);
}

int
main (int argc HB_UNUSED, char **argv HB_UNUSED)
{
  test_cluster_flag_ranges ();
  test_output_flag_ranges ();
  return 0;
}
