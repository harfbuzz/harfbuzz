/*
 * Copyright © 2023  Google, Inc.
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
 * Google Author(s): Qunxin Liu
 */

#include "hb-subset-instancer-solver.hh"

static inline bool approx (Triple a, Triple b)
{
  return fabsf (a.minimum - b.minimum) < 0.000001f &&
         fabsf (a.middle - b.middle) < 0.000001f &&
         fabsf (a.maximum - b.maximum) < 0.000001f;
}

static inline bool approx (float a, float b)
{ return fabsf (a - b) < 0.000001f; }

/* tests ported from
 * https://github.com/fonttools/fonttools/blob/main/Tests/varLib/instancer/solver_test.py */
int
main (int argc, char **argv)
{
  TripleDistances default_axis_distances{1.f, 1.f};
  /* Case 1 */
  {
    /* pin axis*/
    Triple tent (0.f, 1.f, 1.f);
    Triple axis_range (0.f, 0.f, 0.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 0);
  }

  {
    /* pin axis*/
    Triple tent (0.f, 1.f, 1.f);
    Triple axis_range (0.5f, 0.5f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 0.5f);
    assert (out[0].second == Triple ());
  }

  {
    /* tent falls outside the new axis range */
    Triple tent (0.3f, 0.5f, 0.8f);
    Triple axis_range (0.1f, 0.2f, 0.3f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 0);
  }

  /* Case 2 */
  {
    Triple tent (0.f, 1.f, 1.f);
    Triple axis_range (-1.f, 0.f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 0.5f);
    assert (out[0].second == Triple (0.f, 1.f, 1.f));
  }

  /* Case 2 */
  {
    Triple tent (0.f, 1.f, 1.f);
    Triple axis_range (-1.f, 0.f, 0.75f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 0.75f);
    assert (out[0].second == Triple (0.f, 1.f, 1.f));
  }

  /* Without gain: */
  /* Case 3 */
  {
    Triple tent (0.f, 0.2f, 1.f);
    Triple axis_range (-1.f, 0.f, 0.8f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (0.f, 0.25f, 1.25f));
  }

  /* Case 3 boundary */
  {
    Triple tent (0.f, 0.4f, 1.f);
    Triple axis_range (-1.f, 0.f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (0.f, 0.8f, 32767/(float) (1 << 14)));
  }

  /* Case 4 */
  {
    Triple tent (0.f, 0.25f, 1.f);
    Triple axis_range (-1.f, 0.f, 0.4f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 2);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (0.f, 0.625f, 1.f));
    assert (approx (out[1].first, 0.8f));
    assert (out[1].second == Triple (0.625f, 1.f, 1.f));
  }

  /* Case 4 */
  {
    Triple tent (0.25f, 0.3f, 1.05f);
    Triple axis_range (0.f, 0.2f, 0.4f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 2);
    assert (out[0].first == 1.f);
    assert (approx (out[0].second, Triple (0.25f, 0.5f, 1.f)));
    assert (approx (out[1].first, 2.6f/3));
    assert (approx (out[1].second, Triple (0.5f, 1.f, 1.f)));
  }

  /* Case 4 boundary */
  {
    Triple tent (0.25f, 0.5f, 1.f);
    Triple axis_range (0.f, 0.25f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (0.f, 1.f, 1.f));
  }

  /* With gain */
  /* Case 3a/1neg */
  {
    Triple tent (0.f, 0.5f, 1.f);
    Triple axis_range (0.f, 0.5f, 1.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 3);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (out[1].first == -1.f);
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
    assert (out[2].first == -1.f);
    assert (out[2].second == Triple (-1.f, -1.f, 0.f));
  }

  {
    Triple tent (0.f, 0.5f, 1.f);
    Triple axis_range (0.f, 0.5f, 0.75f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 3);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (out[1].first == -0.5f);
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
    assert (out[2].first == -1.f);
    assert (out[2].second == Triple (-1.f, -1.f, 0.f));
  }

  {
    Triple tent (0.f, 0.5f, 1.f);
    Triple axis_range (0.f, 0.25f, 0.8f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 4);
    assert (out[0].first == 0.5f);
    assert (out[0].second == Triple ());
    assert (out[1].first == 0.5f);
    assert (approx (out[1].second, Triple (0.f, 0.454545f, 0.909091f)));
    assert (approx (out[2].first, -0.1f));
    assert (approx (out[2].second, Triple (0.909091f, 1.f, 1.f)));
    assert (out[3].first == -0.5f);
    assert (out[3].second == Triple (-1.f, -1.f, 0.f));
  }

  /* Case 3a/1neg */
  {
    Triple tent (0.f, 0.5f, 2.f);
    Triple axis_range (0.2f, 0.5f, 0.8f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 3);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (approx (out[1].first, -0.2f));
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
    assert (approx (out[2].first, -0.6f));
    assert (out[2].second == Triple (-1.f, -1.f, 0.f));
  }

  /* Case 3a/1neg */
  {
    Triple tent (0.f, 0.5f, 2.f);
    Triple axis_range (0.2f, 0.5f, 1.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 3);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (approx (out[1].first, -1.f/3));
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
    assert (approx (out[2].first, -0.6f));
    assert (out[2].second == Triple (-1.f, -1.f, 0.f));
  }

  /* Case 3 */
  {
    Triple tent (0.f, 0.5f, 1.f);
    Triple axis_range (0.25f, 0.25f, 0.75f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 2);
    assert (out[0].first == 0.5f);
    assert (out[0].second == Triple ());
    assert (out[1].first == 0.5f);
    assert (out[1].second == Triple (0.f, 0.5f, 1.0f));
  }

  /* Case 1neg */
  {
    Triple tent (0.f, 0.5f, 1.f);
    Triple axis_range (0.f, 0.25f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 3);
    assert (out[0].first == 0.5f);
    assert (out[0].second == Triple ());
    assert (out[1].first == 0.5f);
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
    assert (out[2].first == -0.5f);
    assert (out[2].second == Triple (-1.f, -1.f, 0.f));
  }

  /* Case 2neg */
  {
    Triple tent (0.05f, 0.55f, 1.f);
    Triple axis_range (0.f, 0.25f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 4);
    assert (approx (out[0].first, 0.4f));
    assert (out[0].second == Triple ());
    assert (approx (out[1].first, 0.5f));
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
    assert (approx (out[2].first, -0.4f));
    assert (out[2].second == Triple (-1.f, -0.8f, 0.f));
    assert (approx (out[3].first, -0.4f));
    assert (out[3].second == Triple (-1.f, -1.f, -0.8f));
  }

  /* Case 2neg, other side */
  {
    Triple tent (-1.f, -0.55f, -0.05f);
    Triple axis_range (-0.5f, -0.25f, 0.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 4);
    assert (approx (out[0].first, 0.4f));
    assert (out[0].second == Triple ());
    assert (approx (out[1].first, 0.5f));
    assert (out[1].second == Triple (-1.f, -1.f, 0.f));
    assert (approx (out[2].first, -0.4f));
    assert (out[2].second == Triple (0.f, 0.8f, 1.f));
    assert (approx (out[3].first, -0.4f));
    assert (out[3].second == Triple (0.8f, 1.f, 1.f));
  }

  /* Misc corner cases */
  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.5f, 0.5f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
  }

  {
    Triple tent (0.3f, 0.5f, 0.7f);
    Triple axis_range (0.1f, 0.5f, 0.9f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 5);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (out[1].first == -1.f);
    assert (out[1].second == Triple (0.f, 0.5f, 1.f));
    assert (out[2].first == -1.f);
    assert (out[2].second == Triple (0.5f, 1.f, 1.f));
    assert (out[3].first == -1.f);
    assert (approx (out[3].second, Triple (-1.f, -0.5f, 0.f)));
    assert (out[4].first == -1.f);
    assert (approx (out[4].second, Triple (-1.f, -1.f, -0.5f)));
  }

  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.25f, 0.25f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (1.f, 1.f, 1.f));
  }

  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.25f, 0.35f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (1.f, 1.f, 1.f));
  }

  {
    Triple tent (0.5f, 0.5f, 0.55f);
    Triple axis_range (0.25f, 0.35f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (1.f, 1.f, 1.f));
  }

  {
    Triple tent (0.5f, 0.5f, 1.f);
    Triple axis_range (0.5f, 0.5f, 1.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 2);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (out[1].first == -1.f);
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
  }

  {
    Triple tent (0.25f, 0.5f, 1.f);
    Triple axis_range (0.5f, 0.5f, 1.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 2);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (out[1].first == -1.f);
    assert (out[1].second == Triple (0.f, 1.f, 1.f));
  }

  {
    Triple tent (0.f, 0.2f, 1.f);
    Triple axis_range (0.f, 0.f, 0.5f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (0.f, 0.4f, 32767/(float) (1 << 14)));
  }


  {
    Triple tent (0.f, 0.5f, 1.f);
    Triple axis_range (-1.f, 0.25f, 1.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 5);
    assert (out[0].first == 0.5f);
    assert (out[0].second == Triple ());
    assert (out[1].first == 0.5f);
    assert (out[1].second == Triple (0.f, 1.f/3, 2.f/3));
    assert (out[2].first == -0.5f);
    assert (out[2].second == Triple (2.f/3, 1.f, 1.f));
    assert (out[3].first == -0.5f);
    assert (out[3].second == Triple (-1.f, -0.2f, 0.f));
    assert (out[4].first == -0.5f);
    assert (out[4].second == Triple (-1.f, -1.f, -0.2f));
  }

  {
    Triple tent (0.5f, 0.5f, 0.5f);
    Triple axis_range (0.f, 0.5f, 1.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 5);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple ());
    assert (out[1].first == -1.f);
    assert (out[1].second == Triple (0.f, 2/(float) (1 << 14), 1.f));
    assert (out[2].first == -1.f);
    assert (out[2].second == Triple (2/(float) (1 << 14), 1.f, 1.f));
    assert (out[3].first == -1.f);
    assert (out[3].second == Triple (-1.f, -2/(float) (1 << 14), 0.f));
    assert (out[4].first == -1.f);
    assert (out[4].second == Triple (-1.f, -1.f, -2/(float) (1 << 14)));
  }

  {
    Triple tent (0.f, 1.f, 1.f);
    Triple axis_range (-1.f, -0.5f, 1.f);
    result_t out = rebase_tent (tent, axis_range, default_axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (1.f/3, 1.f, 1.f));
  }

  {
    Triple tent (0.f, 1.f, 1.f);
    Triple axis_range (-1.f, -0.5f, 1.f);
    TripleDistances axis_distances{2.f, 1.f};
    result_t out = rebase_tent (tent, axis_range, axis_distances);
    assert (out.length == 1);
    assert (out[0].first == 1.f);
    assert (out[0].second == Triple (0.5f, 1.f, 1.f));
  }
}

