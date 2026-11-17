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

int
main (int argc HB_UNUSED, char **argv HB_UNUSED)
{
  test_cluster_flag_ranges ();
  return 0;
}
