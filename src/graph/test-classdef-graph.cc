/*
 * Copyright © 2022  Google, Inc.
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
 * Google Author(s): Garret Rieger
 */

#include "gsubgpos-context.hh"
#include "classdef-graph.hh"

typedef hb_codepoint_pair_t gid_and_class_t;
typedef hb_vector_t<gid_and_class_t> gid_and_class_list_t;


static bool incremental_size_is (const gid_and_class_list_t& list, unsigned klass,
                                 unsigned cov_expected, unsigned class_def_expected)
{
  graph::class_def_size_estimator_t estimator (list.iter ());

  unsigned result = estimator.add_class_def_size (klass);
  if (result != class_def_expected)
  {
    printf ("FAIL: class def expected size %u but was %u\n", class_def_expected, result);
    return false;
  }

  result = estimator.coverage_size ();
  if (result != cov_expected)
  {
    printf ("FAIL: coverage expected size %u but was %u\n", cov_expected, result);
    return false;
  }

  return true;
}

static void test_class_and_coverage_size_estimates ()
{
  // TODO(garretrieger): test against the actual serialized sizes of class def tables
  gid_and_class_list_t empty = {
  };
  assert (incremental_size_is (empty, 0, 4, 4));
  assert (incremental_size_is (empty, 1, 4, 4));

  gid_and_class_list_t class_zero = {
    {5, 0},
  };
  assert (incremental_size_is (class_zero, 0, 6, 4));

  gid_and_class_list_t consecutive = {
    {4, 0},
    {5, 0},
    {6, 1},
    {7, 1},
    {8, 2},
    {9, 2},
    {10, 2},
    {11, 2},
  };
  assert (incremental_size_is (consecutive, 0, 8, 4));
  assert (incremental_size_is (consecutive, 1, 8, 8));
  assert (incremental_size_is (consecutive, 2, 10, 10));

  gid_and_class_list_t non_consecutive = {
    {4, 0},
    {6, 0},

    {8, 1},
    {10, 1},

    {9, 2},
    {10, 2},
    {11, 2},
    {13, 2},
  };
  assert (incremental_size_is (non_consecutive, 0, 8, 4));
  assert (incremental_size_is (non_consecutive, 1, 8, 4 + 2*6));
  assert (incremental_size_is (non_consecutive, 2, 12, 4 + 2*6));

  gid_and_class_list_t multiple_ranges = {
    {4, 0},
    {5, 0},

    {6, 1},
    {7, 1},

    {9, 1},

    {11, 1},
    {12, 1},
    {13, 1},
  };
  assert (incremental_size_is (multiple_ranges, 0, 8, 4));
  assert (incremental_size_is (multiple_ranges, 1, 4 + 2 * 6, 4 + 3 * 6));
}

static void test_running_class_and_coverage_size_estimates () {
  // #### With consecutive gids: switches formats ###
  gid_and_class_list_t consecutive_map = {
    // range 1-4 (f1: 8 bytes), (f2: 6 bytes)
    {1, 1},
    {2, 1},
    {3, 1},
    {4, 1},

    // (f1: 2 bytes), (f2: 6 bytes)
    {5, 2},

    // (f1: 14 bytes), (f2: 6 bytes)
    {6, 3},
    {7, 3},
    {8, 3},
    {9, 3},
    {10, 3},
    {11, 3},
    {12, 3},
  };

  graph::class_def_size_estimator_t estimator1(consecutive_map.iter());
  assert(estimator1.add_class_def_size(1) == 4 + 6);  // format 2, 1 range
  assert(estimator1.coverage_size() == 4 + 6);        // format 2, 1 range
  assert(estimator1.add_class_def_size(2) == 4 + 10); // format 1, 5 glyphs
  assert(estimator1.coverage_size() == 4 + 6);        // format 2, 1 range
  assert(estimator1.add_class_def_size(3) == 4 + 18); // format 2, 3 ranges
  assert(estimator1.coverage_size() == 4 + 6);        // format 2, 1 range

  estimator1.reset();
  assert(estimator1.add_class_def_size(2) == 4 + 2);  // format 1, 1 glyph
  assert(estimator1.coverage_size() == 4 + 2);        // format 1, 1 glyph
  assert(estimator1.add_class_def_size(3) == 4 + 12); // format 2, 2 ranges
  assert(estimator1.coverage_size() == 4 + 6);        // format 1, 1 range

  // #### With non-consecutive gids: always uses format 2 ###
  gid_and_class_list_t non_consecutive_map = {
    // range 1-4 (f1: 8 bytes), (f2: 6 bytes)
    {1, 1},
    {2, 1},
    {3, 1},
    {4, 1},

    // (f1: 2 bytes), (f2: 12 bytes)
    {6, 2},
    {8, 2},

    // (f1: 14 bytes), (f2: 6 bytes)
    {9, 3},
    {10, 3},
    {11, 3},
    {12, 3},
    {13, 3},
    {14, 3},
    {15, 3},
  };

  graph::class_def_size_estimator_t estimator2(non_consecutive_map.iter());
  assert(estimator2.add_class_def_size(1) == 4 + 6);  // format 2, 1 range
  assert(estimator2.coverage_size() == 4 + 6);        // format 2, 1 range
  assert(estimator2.add_class_def_size(2) == 4 + 18); // format 2, 3 ranges
  assert(estimator2.coverage_size() == 4 + 2 * 6);    // format 1, 6 glyphs
  assert(estimator2.add_class_def_size(3) == 4 + 24); // format 2, 4 ranges
  assert(estimator2.coverage_size() == 4 + 3 * 6);    // format 2, 3 ranges

  estimator2.reset();
  assert(estimator2.add_class_def_size(2) == 4 + 12); // format 1, 1 range
  assert(estimator2.coverage_size() == 4 + 4);        // format 1, 2 glyphs
  assert(estimator2.add_class_def_size(3) == 4 + 18); // format 2, 2 ranges
  assert(estimator2.coverage_size() == 4 + 2 * 6);    // format 2, 2 ranges
}

static void test_running_class_size_estimates_with_locally_consecutive_glyphs () {
  gid_and_class_list_t consecutive_map = {
    {1, 1},
    {6, 2},
    {7, 3},
  };

  graph::class_def_size_estimator_t estimator(consecutive_map.iter());
  assert(estimator.add_class_def_size(1) == 4 + 2);  // format 1, 1 glyph
  assert(estimator.add_class_def_size(2) == 4 + 12); // format 2, 2 ranges
  assert(estimator.add_class_def_size(3) == 4 + 18); // format 2, 3 ranges

  estimator.reset();
  assert(estimator.add_class_def_size(2) == 4 + 2); // format 1, 1 glyphs
  assert(estimator.add_class_def_size(3) == 4 + 4); // format 1, 2 glyphs
}

int
main (int argc, char **argv)
{
  test_class_and_coverage_size_estimates ();
  test_running_class_and_coverage_size_estimates ();
  test_running_class_size_estimates_with_locally_consecutive_glyphs ();
}
