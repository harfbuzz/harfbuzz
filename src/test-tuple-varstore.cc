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
 */
#include "hb-ot-var-cvar-table.hh"

// cvar table data from Multi-ABC.ttf
const char cvar_data[] = "\x0\x1\x0\x0\x0\x2\x0\x14\x0\x51\xa0\x0\xc0\x0\x0\x54\xa0\x0\x40\x0\x2a\x29\x17\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\xd\xff\x0\xfd\x1\x0\xff\x0\xfd\x1\x0\xdb\xdb\xe6\xe6\x82\x0\xfd\x84\x6\xfd\x0\x2\xe3\xe3\xec\xec\x82\x4\x1\xe3\xe3\xec\xec\x82\x0\x1\x2a\x29\x17\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\xd\x1\x0\x5\xfd\x0\x1\x0\x5\xfd\x0\x61\x61\x44\x44\x82\x0\x5\x81\x9\x1\xff\x1\x7\xff\xfb\x49\x49\x35\x35\x82\x4\xff\x49\x49\x35\x35\x82\x0\xff";

static void
test_decompile_cvar ()
{
  const OT::cvar* cvar_table = reinterpret_cast<const OT::cvar*> (cvar_data);
  unsigned point_count = 65;
  unsigned axis_count = 1;
  OT::TupleVariationData::tuple_variations_t tuple_variations;
  bool result = cvar_table->decompile_tuple_variations (axis_count, point_count, false, tuple_variations);
  assert (result);
  assert (tuple_variations.tuple_vars.length == 2);
  for (unsigned i = 0; i < 2; i++)
  {
    assert (tuple_variations.tuple_vars[i].axis_tuples.get_population () == 1);
    assert (!tuple_variations.tuple_vars[i].deltas_y);
    assert (tuple_variations.tuple_vars[i].indices.length == 65);
    assert (tuple_variations.tuple_vars[i].indices.length == tuple_variations.tuple_vars[i].deltas_x.length);
  }
  assert (tuple_variations.tuple_vars[0].axis_tuples.get (0) == Triple (-1.f, -1.f, 0.f));
  assert (tuple_variations.tuple_vars[1].axis_tuples.get (0) == Triple (0.f, 1.f, 1.f));
  
  hb_vector_t<float> deltas_1 {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, -3.f, 1.f, 0.f, -1.f, 0.f, -3.f, 1.f, 0.f, -37.f, -37.f, -26.f, -26.f, 0.f, 0.f, 0.f, -3.f, 0.f, 0.f, 0.f, 0.f, 0.f, -3.f, 0.f, 2.f, -29.f, -29.f, -20.f, -20.f, 0.f, 0.f, 0.f, 1.f, -29.f, -29.f, -20.f, -20.f, 0.f, 0.f, 0.f, 1.f};
  for (unsigned i = 0; i < 65; i++)
  {
    if (i < 23)
      assert (tuple_variations.tuple_vars[0].indices[i] == 0);
    else
    {
      assert (tuple_variations.tuple_vars[0].indices[i] == 1);
      assert (tuple_variations.tuple_vars[0].deltas_x[i] == deltas_1[i]);
    }
  }

  hb_vector_t<float> deltas_2 {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 5.f, -3.f, 0.f, 1.f, 0.f, 5.f, -3.f, 0.f, 97.f, 97.f, 68.f, 68.f, 0.f, 0.f, 0.f, 5.f, 0.f, 0.f, 1.f, -1.f, 1.f, 7.f, -1.f, -5.f, 73.f, 73.f, 53.f, 53.f, 0.f, 0.f, 0.f, -1.f, 73.f, 73.f, 53.f, 53.f, 0.f, 0.f, 0.f, -1.f};
  for (unsigned i = 0 ; i < 65; i++)
  {
    if (i < 23)
      assert (tuple_variations.tuple_vars[1].indices[i] == 0);
    else
    {
      assert (tuple_variations.tuple_vars[1].indices[i] == 1);
      assert (tuple_variations.tuple_vars[1].deltas_x[i] == deltas_2[i]);
    }
  }
}

int
main (int argc, char **argv)
{
  test_decompile_cvar ();
}
