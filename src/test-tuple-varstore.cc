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
const unsigned char cvar_data[] = "\x0\x1\x0\x0\x0\x2\x0\x14\x0\x51\xa0\x0\xc0\x0\x0\x54\xa0\x0\x40\x0\x2a\x29\x17\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\xd\xff\x0\xfd\x1\x0\xff\x0\xfd\x1\x0\xdb\xdb\xe6\xe6\x82\x0\xfd\x84\x6\xfd\x0\x2\xe3\xe3\xec\xec\x82\x4\x1\xe3\xe3\xec\xec\x82\x0\x1\x2a\x29\x17\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\xd\x1\x0\x5\xfd\x0\x1\x0\x5\xfd\x0\x61\x61\x44\x44\x82\x0\x5\x81\x9\x1\xff\x1\x7\xff\xfb\x49\x49\x35\x35\x82\x4\xff\x49\x49\x35\x35\x82\x0\xff";

static void
test_decompile_cvar ()
{
  const OT::cvar* cvar_table = reinterpret_cast<const OT::cvar*> (cvar_data);
  unsigned point_count = 65;
  unsigned axis_count = 1;

  hb_tag_t axis_tag = HB_TAG ('w', 'g', 'h', 't');
  hb_map_t axis_idx_tag_map;
  axis_idx_tag_map.set (0, axis_tag);

  OT::TupleVariationData<>::tuple_variations_t tuple_variations;
  hb_vector_t<unsigned> shared_indices;
  OT::TupleVariationData<>::tuple_iterator_t iterator;

  const OT::TupleVariationData<>* tuple_var_data = reinterpret_cast<const OT::TupleVariationData<>*> (cvar_data + 4);

  unsigned len = sizeof (cvar_data);
  hb_bytes_t var_data_bytes{(const char* ) cvar_data + 4, len - 4};
  bool result = OT::TupleVariationData<>::get_tuple_iterator (var_data_bytes, axis_count, cvar_table,
                                                            shared_indices, &iterator);
  hb_always_assert (result);

  result = tuple_var_data->decompile_tuple_variations (point_count, false, iterator, &axis_idx_tag_map,
                                                       shared_indices, hb_array<const OT::F2DOT14> (),
                                                       tuple_variations);

  hb_always_assert (result);
  hb_always_assert (tuple_variations.tuple_vars.length == 2);
  for (unsigned i = 0; i < 2; i++)
  {
    hb_always_assert (tuple_variations.tuple_vars[i].axis_tuples.get_population () == 1);
    hb_always_assert (!tuple_variations.tuple_vars[i].deltas_y);
    hb_always_assert (tuple_variations.tuple_vars[i].indices.length == 65);
    hb_always_assert (tuple_variations.tuple_vars[i].indices.length == tuple_variations.tuple_vars[i].deltas_x.length);
  }
  hb_always_assert (tuple_variations.tuple_vars[0].axis_tuples.get (axis_tag) == Triple (-1.0, -1.0, 0.0));
  hb_always_assert (tuple_variations.tuple_vars[1].axis_tuples.get (axis_tag) == Triple (0.0, 1.0, 1.0));

  hb_vector_t<float> deltas_1 {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, -3.0, 1.0, 0.0, -1.0, 0.0, -3.0, 1.0, 0.0, -37.0, -37.0, -26.0, -26.0, 0.0, 0.0, 0.0, -3.0, 0.0, 0.0, 0.0, 0.0, 0.0, -3.0, 0.0, 2.0, -29.0, -29.0, -20.0, -20.0, 0.0, 0.0, 0.0, 1.0, -29.0, -29.0, -20.0, -20.0, 0.0, 0.0, 0.0, 1.0};
  for (unsigned i = 0; i < 65; i++)
  {
    if (i < 23)
      hb_always_assert (tuple_variations.tuple_vars[0].indices[i] == 0);
    else
    {
      hb_always_assert (tuple_variations.tuple_vars[0].indices[i] == 1);
      hb_always_assert (tuple_variations.tuple_vars[0].deltas_x[i] == deltas_1[i]);
    }
  }

  hb_vector_t<float> deltas_2 {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, -3.0, 0.0, 1.0, 0.0, 5.0, -3.0, 0.0, 97.0, 97.0, 68.0, 68.0, 0.0, 0.0, 0.0, 5.0, 0.0, 0.0, 1.0, -1.0, 1.0, 7.0, -1.0, -5.0, 73.0, 73.0, 53.0, 53.0, 0.0, 0.0, 0.0, -1.0, 73.0, 73.0, 53.0, 53.0, 0.0, 0.0, 0.0, -1.0};
  for (unsigned i = 0 ; i < 65; i++)
  {
    if (i < 23)
      hb_always_assert (tuple_variations.tuple_vars[1].indices[i] == 0);
    else
    {
      hb_always_assert (tuple_variations.tuple_vars[1].indices[i] == 1);
      hb_always_assert (tuple_variations.tuple_vars[1].deltas_x[i] == deltas_2[i]);
    }
  }

  /* partial instancing wght=300:800 */
  hb_hashmap_t<hb_tag_t, Triple> normalized_axes_location;
  normalized_axes_location.set (axis_tag, Triple (-0.512817, 0.0, 0.700012));

  hb_hashmap_t<hb_tag_t, TripleDistances> axes_triple_distances;
  axes_triple_distances.set (axis_tag, TripleDistances (1.0, 1.0));

  OT::optimize_scratch_t scratch;
  tuple_variations.instantiate (normalized_axes_location, axes_triple_distances, scratch);

  hb_always_assert (tuple_variations.tuple_vars[0].indices.length == 65);
  hb_always_assert (tuple_variations.tuple_vars[1].indices.length == 65);
  hb_always_assert (!tuple_variations.tuple_vars[0].deltas_y);
  hb_always_assert (!tuple_variations.tuple_vars[1].deltas_y);
  hb_always_assert (tuple_variations.tuple_vars[0].axis_tuples.get (axis_tag) == Triple (-1.0, -1.0, 0.0));
  hb_always_assert (tuple_variations.tuple_vars[1].axis_tuples.get (axis_tag) == Triple (0.0, 1.0, 1.0));

  hb_vector_t<float> rounded_deltas_1 {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1, 0.0, -2, 1, 0.0, -1, 0.0, -2, 1, 0.0, -19, -19, -13, -13, 0.0, 0.0, 0.0, -2, 0.0, 0.0, 0.0, 0.0, 0.0, -2, 0.0, 1, -15, -15, -10.0, -10.0, 0.0, 0.0, 0.0, 1, -15, -15, -10.0, -10.0, 0.0, 0.0, 0.0, 1};

  hb_vector_t<float> rounded_deltas_2 {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1, 0.0, 4, -2, 0.0, 1, 0.0, 4, -2, 0.0, 68, 68, 48, 48, 0.0, 0.0, 0.0, 4, 0.0, 0.0, 1, -1, 1, 5, -1, -4, 51, 51, 37, 37, 0.0, 0.0, 0.0, -1, 51, 51, 37, 37, 0.0, 0.0, 0.0, -1};

  for (unsigned i = 0; i < 65; i++)
  {
    if (i < 23)
    {
      hb_always_assert (tuple_variations.tuple_vars[0].indices[i] == 0);
      hb_always_assert (tuple_variations.tuple_vars[1].indices[i] == 0);
    }
    else
    {
      hb_always_assert (tuple_variations.tuple_vars[0].indices[i] == 1);
      hb_always_assert (tuple_variations.tuple_vars[1].indices[i] == 1);
      hb_always_assert (roundf (tuple_variations.tuple_vars[0].deltas_x[i]) == rounded_deltas_1[i]);
      hb_always_assert (roundf (tuple_variations.tuple_vars[1].deltas_x[i]) == rounded_deltas_2[i]);
    }
  }

  hb_map_t axes_index_map;
  axes_index_map.set (0, 0);
  bool res = tuple_variations.compile_bytes (axes_index_map, axis_idx_tag_map, false);
  hb_always_assert (res);
  hb_always_assert (tuple_variations.tuple_vars[0].compiled_tuple_header.length == 6);
  const unsigned char tuple_var_header_1[] = "\x0\x51\xa0\x0\xc0\x0";
  for (unsigned i = 0; i < 6; i++)
    hb_always_assert(tuple_variations.tuple_vars[0].compiled_tuple_header.arrayZ[i] == tuple_var_header_1[i]);

  hb_always_assert (tuple_variations.tuple_vars[1].compiled_tuple_header.length == 6);
  const unsigned char tuple_var_header_2[] = "\x0\x54\xa0\x0\x40\x0";
  for (unsigned i = 0; i < 6; i++)
    hb_always_assert(tuple_variations.tuple_vars[1].compiled_tuple_header.arrayZ[i] == tuple_var_header_2[i]);

  hb_always_assert (tuple_variations.tuple_vars[0].compiled_deltas.length == 37);
  hb_always_assert (tuple_variations.tuple_vars[1].compiled_deltas.length == 40);
  const unsigned char compiled_deltas_1[] = "\x0d\xff\x00\xfe\x01\x00\xff\x00\xfe\x01\x00\xed\xed\xf3\xf3\x82\x00\xfe\x84\x06\xfe\x00\x01\xf1\xf1\xf6\xf6\x82\x04\x01\xf1\xf1\xf6\xf6\x82\x00\x01";
  for (unsigned i = 0; i < 37; i++)
    hb_always_assert (tuple_variations.tuple_vars[0].compiled_deltas.arrayZ[i] == compiled_deltas_1[i]);

  const unsigned char compiled_deltas_2[] = "\x0d\x01\x00\x04\xfe\x00\x01\x00\x04\xfe\x00\x44\x44\x30\x30\x82\x00\x04\x81\x09\x01\xff\x01\x05\xff\xfc\x33\x33\x25\x25\x82\x04\xff\x33\x33\x25\x25\x82\x00\xff";
  for (unsigned i = 0; i < 40; i++)
    hb_always_assert (tuple_variations.tuple_vars[1].compiled_deltas.arrayZ[i] == compiled_deltas_2[i]);
}

static void
test_shared_tuple_scalar_cache ()
{
  const unsigned char plain_data[] = {0, 0, 0, 0};
  const unsigned char intermediate_data[] = {0, 0, 0x40, 0, 0, 0, 0x40, 0};
  const unsigned char peak_data[] = {0x20, 0};
  const auto &plain = *reinterpret_cast<const OT::TupleVariationHeader *> (plain_data);
  const auto &intermediate = *reinterpret_cast<const OT::TupleVariationHeader *> (intermediate_data);
  auto peaks = hb_array (reinterpret_cast<const OT::F2DOT14 *> (peak_data), 1);

  /* The two tuples share a peak at 0.5, but the intermediate region ends
   * at 1.0. A cached default-region scalar must not suppress its deltas. */
  for (int coord : {0, 4096, 8192, 12288, 16384})
    for (bool intermediate_first : {false, true})
    {
      auto coords = hb_array (&coord, 1);
      float plain_scalar = plain.calculate_scalar (coords, 1, peaks);
      float intermediate_scalar = intermediate.calculate_scalar (coords, 1, peaks);
      if (coord == 12288)
      {
        hb_always_assert (plain_scalar == 0.f);
        hb_always_assert (intermediate_scalar == 0.5f);
      }

      OT::hb_scalar_cache_t scratch;
      auto *cache = OT::hb_scalar_cache_t::create (1, &scratch);
      if (intermediate_first)
      {
        hb_always_assert (intermediate.calculate_scalar (coords, 1, peaks, cache) == intermediate_scalar);
        hb_always_assert (plain.calculate_scalar (coords, 1, peaks, cache) == plain_scalar);
      }
      else
      {
        hb_always_assert (plain.calculate_scalar (coords, 1, peaks, cache) == plain_scalar);
        hb_always_assert (intermediate.calculate_scalar (coords, 1, peaks, cache) == intermediate_scalar);
      }
    }
}

static bool
decompile_cvar_with_budget (OT::TupleVariationData<>::tuple_variations_t &tuple_variations,
                            int64_t *budget)
{
  const OT::cvar* cvar_table = reinterpret_cast<const OT::cvar*> (cvar_data);
  hb_map_t axis_idx_tag_map;
  axis_idx_tag_map.set (0, HB_TAG ('w', 'g', 'h', 't'));

  hb_blob_t *blob = hb_blob_create ((const char *) cvar_data, sizeof (cvar_data),
                                    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  bool result = cvar_table->decompile_tuple_variations (1, 65, blob, false, &axis_idx_tag_map,
                                                        tuple_variations, budget);
  hb_blob_destroy (blob);
  return result;
}

static void
test_tuple_points_budget ()
{
  /* Each tuple is expanded to 65 points; the decode charges them. */
  {
    OT::TupleVariationData<>::tuple_variations_t tuple_variations;
    int64_t budget = 2 * 65 - 1;
    hb_always_assert (!decompile_cvar_with_budget (tuple_variations, &budget));
  }

  OT::TupleVariationData<>::tuple_variations_t tuple_variations;
  int64_t budget = 2 * 65;
  hb_always_assert (decompile_cvar_with_budget (tuple_variations, &budget));
  hb_always_assert (tuple_variations.tuple_vars.length == 2);
  hb_always_assert (budget == 0);
}

int
main (int argc, char **argv)
{
  test_shared_tuple_scalar_cache ();
  test_decompile_cvar ();
  test_tuple_points_budget ();
}
