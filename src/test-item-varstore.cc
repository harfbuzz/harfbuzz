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
#include "hb-ot-var-common.hh"
#include "hb-ot-var-hvar-table.hh"
// HVAR table data from SourceSerif4Variable-Roman_subset.otf
const char hvar_data[] = "\x0\x1\x0\x0\x0\x0\x0\x14\x0\x0\x0\xc4\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1\x0\x0\x0\x10\x0\x2\x0\x0\x0\x74\x0\x0\x0\x7a\x0\x2\x0\x8\xc0\x0\xc0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x40\x0\x40\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\xc0\x0\xc0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x40\x0\x40\x0\xc0\x0\xc0\x0\x0\x0\xc0\x0\xc0\x0\x0\x0\xc0\x0\xc0\x0\x0\x0\x0\x0\x40\x0\x40\x0\x0\x0\x40\x0\x40\x0\xc0\x0\xc0\x0\x0\x0\x0\x0\x40\x0\x40\x0\x0\x0\x40\x0\x40\x0\x0\x1\x0\x0\x0\x0\x0\x4\x0\x0\x0\x8\x0\x0\x0\x1\x0\x2\x0\x3\x0\x4\x0\x5\x0\x6\x0\x7\xf9\xf\x2f\xbf\xfb\xfb\x35\xf9\x4\x4\xf3\xb4\xf2\xfb\x2e\xf3\x4\x4\xe\xad\xfa\x1\x1a\x1\x15\x22\x59\xd6\xe3\xf6\x6\xf5\x0\x1\x0\x5\x0\x4\x7\x5\x6";

static void
test_var_region_list_serialize ()
{
  hb_vector_t<hb_tag_t> axes;
  axes.push (HB_TAG ('w', 'g', 'h', 't'));
  hb_hashmap_t<hb_tag_t, Triple> region;
  hb_always_assert (region.set (axes[0], Triple (0., 0.5, 1.)));
  hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple> *> regions;
  regions.push (&region);

  char buf[32], copy_buf[32];
  hb_serialize_context_t c (buf, sizeof (buf));
  auto *out = c.start_serialize<OT::VarRegionList> ();
  hb_always_assert (out->serialize (&c, axes, regions));
  hb_always_assert (!c.in_error ());
  hb_always_assert (out->axisCount == 1 && out->regionCount == 1);
  const int coords[] = {8192};
  hb_always_assert (out->evaluate (0, coords, ARRAY_LENGTH (coords)) == 1.f);

  hb_inc_bimap_t region_map;
  region_map.add (0);
  hb_always_assert (region_map.get_population () == 1);
  hb_serialize_context_t copy_c (copy_buf, sizeof (copy_buf));
  auto *copy = copy_c.start_serialize<OT::VarRegionList> ();
  hb_always_assert (copy->serialize (&copy_c, out, region_map));
  hb_always_assert (!copy_c.in_error ());
  hb_always_assert (!hb_memcmp (out, copy, out->get_size ()));
}

static void
test_var_region_list_serialize_bounds ()
{
  const unsigned counts[][2] = {
    {0x10000u, 1},          /* axisCount does not fit uint16. */
    {1, 0x8000u},          /* regionCount's high bit is reserved. */
    {0x40000u, 0x4000u},   /* axisCount * regionCount wraps uint32. */
  };
  hb_hashmap_t<hb_tag_t, Triple> region;
  for (const auto &count : counts)
  {
    hb_vector_t<hb_tag_t> axes;
    hb_always_assert (axes.resize (count[0]));
    for (auto &tag : axes)
      tag = HB_TAG ('w', 'g', 'h', 't');
    hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple> *> regions;
    hb_always_assert (regions.resize (count[1]));
    for (auto &r : regions)
      r = &region;

    char buf[16];
    hb_serialize_context_t c (buf, sizeof (buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    hb_always_assert (!out->serialize (&c, axes, regions));
    hb_always_assert (c.errors == HB_SERIALIZE_ERROR_INT_OVERFLOW);
  }

  /* Only the header is needed: reject overflowing sizes before reading axes. */
  const unsigned char source[] = {0xFF, 0xFF, 0x7F, 0xFF};
  const auto *src = reinterpret_cast<const OT::VarRegionList *> (source);
  const unsigned mapped_counts[] = {
    0x8000u, /* Reserved high bit. */
    0x10000u, /* Narrowing would wrap to zero. */
    0x7FFFu, /* Valid counts, but 0xFFFF * 0x7FFF * 6 overflows uint32. */
  };
  for (unsigned count : mapped_counts)
  {
    hb_inc_bimap_t region_map;
    for (unsigned i = 0; i < count; i++)
      region_map.add (i);
    hb_always_assert (region_map.get_population () == count);

    char buf[16];
    hb_serialize_context_t c (buf, sizeof (buf));
    auto *out = c.start_serialize<OT::VarRegionList> ();
    hb_always_assert (!out->serialize (&c, src, region_map));
    hb_always_assert (c.errors == HB_SERIALIZE_ERROR_INT_OVERFLOW);
  }
}

static void
test_scalar_cache_precision ()
{
  const float scalars[] = {
    0.f,
    1.f,
    1.f / 3.f,
    /* Three variation axes can produce a scalar below the old cache's
     * fixed-point precision. */
    1.f / (16384.f * 16384.f * 16384.f),
  };
  const unsigned counts[] = {4, 129};
  OT::hb_scalar_cache_t scratch_cache;
  for (unsigned count : counts)
  {
    auto *cache = OT::hb_scalar_cache_t::create (count, &scratch_cache);
    hb_always_assert (cache->length == count);
    for (unsigned i = 0; i < count; i++)
    {
      float value;
      float scalar = scalars[i % ARRAY_LENGTH (scalars)];
      hb_always_assert (!cache->get (i, &value));
      cache->set (i, scalar);
      hb_always_assert (cache->get (i, &value));
      hb_always_assert (value == scalar);
    }
    cache->clear ();
    for (unsigned i = 0; i < count; i++)
    {
      float value;
      hb_always_assert (!cache->get (i, &value));
    }
    OT::hb_scalar_cache_t::destroy (cache, &scratch_cache);
  }
}

static void
test_item_variations ()
{
  const OT::HVAR* hvar_table = reinterpret_cast<const OT::HVAR*> (hvar_data);

  hb_tag_t axis_tag = HB_TAG ('w', 'g', 'h', 't');
  hb_map_t axis_idx_tag_map;
  axis_idx_tag_map.set (0, axis_tag);

  axis_tag = HB_TAG ('o', 'p', 's', 'z');
  axis_idx_tag_map.set (1, axis_tag);

  OT::item_variations_t item_vars;
  const OT::ItemVariationStore& src_var_store = hvar_table+(hvar_table->varStore);
  bool result = item_vars.create_from_item_varstore (src_var_store, axis_idx_tag_map);
      
  hb_always_assert (result);

  /* partial instancing wght=300:800 */
  hb_hashmap_t<hb_tag_t, Triple> normalized_axes_location;
  normalized_axes_location.set (axis_tag, Triple (-0.512817, 0.0, 0.7000120));

  hb_hashmap_t<hb_tag_t, TripleDistances> axes_triple_distances;
  axes_triple_distances.set (axis_tag, TripleDistances (200.0, 500.0));

  result = item_vars.instantiate_tuple_vars (normalized_axes_location, axes_triple_distances);
  hb_always_assert (result);
  result = item_vars.as_item_varstore (false);
  hb_always_assert (result);
  hb_always_assert (item_vars.get_region_list().length == 8);
}

static void
test_implicit_advance_mapping ()
{
  char data[sizeof (hvar_data)];
  hb_memcpy (data, hvar_data, sizeof (data));
  auto *hvar = reinterpret_cast<OT::HVAR *> (data);
  const auto &store = hvar + hvar->varStore;
  const int locations[] = {-16384, 0, 16384};
  bool exercised_outer_one = false;

  for (int weight : locations)
    for (int size : locations)
    {
      int coords[] = {weight, size};
      unsigned mapped = (hvar + hvar->advMap).map (0x10000u);
      hb_always_assert (hvar->get_advance_delta_unscaled (0x10000u, coords, 2) ==
                        store.get_delta (mapped, coords, 2));
    }

  hvar->advMap = 0;
  for (int weight : locations)
    for (int size : locations)
    {
      int coords[] = {weight, size};
      exercised_outer_one |= store.get_delta (0x10000u, coords, 2) != 0;
      hb_always_assert (hvar->get_advance_delta_unscaled (0, coords, 2) ==
                        store.get_delta (0, coords, 2));
      hb_always_assert (hvar->get_advance_delta_unscaled (0xFFFFu, coords, 2) == 0);
      hb_always_assert (hvar->get_advance_delta_unscaled (0x10000u, coords, 2) == 0);
      hb_always_assert (hvar->get_advance_delta_unscaled (0x10001u, coords, 2) == 0);
    }
  hb_always_assert (exercised_outer_one);
}

static void
test_item_variations_overflow ()
{
  hb_vector_t<hb_vector_t<int>> row_storage;
  hb_hashmap_t<unsigned, const hb_vector_t<int>*> front_mapping;
  OT::item_variations_t item_vars;
  unsigned old_varidx = 0;
  const unsigned total_count = 0xFFFFu * 2u;

  const unsigned counts[] = {
    128u,                 /* 0..127: 1-byte deltas */
    32768u - 128u,        /* 128..32767: 2-byte deltas */
    0xFFFFu * 2u - 32768u /* 32768..131069: 4-byte deltas */
  };
  const int starts[] = {
    0,
    128,
    32768
  };

  hb_always_assert (row_storage.alloc (total_count));

  for (unsigned bucket = 0; bucket < 3; bucket++)
  {
    hb_vector_t<const hb_vector_t<int>*> rows;
    hb_always_assert (rows.alloc (counts[bucket]));

    for (unsigned i = 0; i < counts[bucket]; i++)
    {
      hb_vector_t<int> row;
      hb_always_assert (row.resize (1));
      row[0] = starts[bucket] + i;
      row_storage.push (std::move (row));
    }

    hb_always_assert (!row_storage.in_error ());

    unsigned base = row_storage.length - counts[bucket];
    for (unsigned i = 0; i < counts[bucket]; i++)
    {
      const hb_vector_t<int> *row = &row_storage.arrayZ[base + i];
      rows.push (row);
      front_mapping.set (old_varidx++, row);
    }

    hb_always_assert (!rows.in_error ());
    hb_always_assert (item_vars.add_vardata_encoding_for_testing (std::move (rows), 1));
  }

  hb_always_assert (!front_mapping.in_error ());
  hb_always_assert (item_vars.compile_varidx_map_for_testing (front_mapping));

  const auto& encodings = item_vars.get_vardata_encodings ();
  hb_always_assert (encodings.length == 4);
  hb_always_assert (encodings[0].items.length == 128u);
  hb_always_assert (encodings[1].items.length == 32768u - 128u);
  hb_always_assert (encodings[2].items.length == 0xFFFFu);
  hb_always_assert (encodings[3].items.length == 32767u);

  const hb_map_t& varidx_map = item_vars.get_varidx_map ();
  hb_always_assert (varidx_map.get (0) == 0);
  hb_always_assert (varidx_map.get (127) == 127);
  hb_always_assert (varidx_map.get (128) == 0x00010000u);
  hb_always_assert (varidx_map.get (32767) == 0x00017F7Fu);
  hb_always_assert (varidx_map.get (32768) == 0x00020000u);
  hb_always_assert (varidx_map.get (98302) == 0x0002FFFEu);
  hb_always_assert (varidx_map.get (98303) == 0x00030000u);
  hb_always_assert (varidx_map.get (131069) == 0x00037FFEu);
}

static void
test_signed_delta_widths ()
{
  const int deltas[] = {32767, 32768, 65535, 65536,
                       -32768, -32769, -65536, -65537};
  hb_tag_t axis = HB_TAG ('T', 'E', 'S', 'T');
  hb_vector_t<hb_tag_t> axes;
  axes.push (axis);
  hb_map_t axis_map;
  axis_map.set (0, axis);
  hb_hashmap_t<hb_tag_t, Triple> region;
  region.set (axis, Triple (0, 1, 1));
  hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple> *> regions;
  regions.push (&region);

  for (int delta : deltas)
  {
    hb_vector_t<int> row;
    row.push (delta);
    hb_vector_t<const hb_vector_t<int> *> rows;
    rows.push (&row);
    hb_vector_t<OT::delta_row_encoding_t> encodings;
    encodings.push (OT::delta_row_encoding_t (std::move (rows), 1));

    char buf[128];
    hb_serialize_context_t s (buf, sizeof (buf));
    auto *out = s.start_serialize<OT::ItemVariationStore> ();
    hb_always_assert (out->serialize (&s, true, axes, regions, encodings));
    s.end_serialize ();
    hb_bytes_t bytes = s.copy_bytes ();
    const auto &store = *reinterpret_cast<const OT::ItemVariationStore *> (bytes.arrayZ);
    const auto &data = store.get_sub_table (0);
    hb_always_assert (data.get_item_delta (0, 0) == delta);

    hb_inc_bimap_t inner_map, region_map;
    inner_map.add (0);
    region_map.add (0);
    char subset_buf[32];
    hb_serialize_context_t subset_s (subset_buf, sizeof (subset_buf));
    auto *subset = subset_s.start_serialize<OT::VarData> ();
    hb_always_assert (subset->serialize (&subset_s, &data, inner_map, region_map));
    subset_s.end_serialize ();
    hb_bytes_t subset_bytes = subset_s.copy_bytes ();
    const auto &subset_data = *reinterpret_cast<const OT::VarData *> (subset_bytes.arrayZ);
    hb_always_assert (subset_data.get_item_delta (0, 0) == delta);

    OT::item_variations_t item_vars;
    hb_always_assert (item_vars.create_from_item_varstore (store, axis_map));
    hb_hashmap_t<hb_tag_t, Triple> locations;
    hb_hashmap_t<hb_tag_t, TripleDistances> distances;
    hb_always_assert (item_vars.instantiate_tuple_vars (locations, distances));
    hb_always_assert (item_vars.as_item_varstore ());
    hb_always_assert (item_vars.has_long_word () ==
                      (delta < -32768 || delta > 32767));
    subset_bytes.fini ();
    bytes.fini ();
  }
}

static void
test_accumulated_delta_widths ()
{
  const int deltas[] = {20000, -20000, 1073741824};
  for (int delta : deltas)
  {
    OT::item_variations_t item_vars;
    unsigned outer = item_vars.add_vardata (1);
    item_vars.add_tuple (outer, hb_hashmap_t<hb_tag_t, Triple> (), 0, delta, 1);
    item_vars.add_tuple (outer, hb_hashmap_t<hb_tag_t, Triple> (), 0, delta, 1);
    hb_always_assert (item_vars.build_region_list ());
    if (delta == 1073741824)
      hb_always_assert (!item_vars.as_item_varstore ());
    else
    {
      hb_always_assert (item_vars.as_item_varstore ());
      hb_always_assert (item_vars.has_long_word ());
      hb_always_assert ((*item_vars.get_vardata_encodings ()[0].items[0])[0] == 2 * delta);
    }
  }
}

static void
test_condition_delta_precision ()
{
  hb_vector_t<hb_tag_t> axes;
  axes.push (HB_TAG ('T','E','S','T'));
  hb_hashmap_t<hb_tag_t, Triple> neutral, region;
  region.set (axes[0], Triple (0, 1, 1));
  hb_vector_t<const hb_hashmap_t<hb_tag_t, Triple> *> regions;
  regions.push (&neutral);
  regions.push (&region);
  hb_vector_t<int> row;
  row.push (16777217);
  row.push (-16777216);
  hb_vector_t<const hb_vector_t<int> *> rows;
  rows.push (&row);
  hb_vector_t<OT::delta_row_encoding_t> encodings;
  encodings.push (OT::delta_row_encoding_t (std::move (rows), 2));
  char buf[128];
  hb_serialize_context_t s (buf, sizeof (buf));
  auto *out = s.start_serialize<OT::ItemVariationStore> ();
  hb_always_assert (out->serialize (&s, true, axes, regions, encodings));
  s.end_serialize ();
  hb_bytes_t bytes = s.copy_bytes ();
  const auto &store = *reinterpret_cast<const OT::ItemVariationStore *> (bytes.arrayZ);
  int coords[] = {16384};
  OT::ItemVarStoreInstancer instancer (&store, nullptr, hb_array (coords));
  // Positioning keeps float arithmetic, but conditions must retain the unit
  // left after cancellation of two deltas exceeding float's integer range.
  hb_always_assert (instancer[0] == 0.f);
  hb_always_assert (instancer.get_condition_delta (0) == 1.);
  const char condition_data[] = "\x00\x02\x00\x00\x00\x00\x00\x00";
  const auto &condition = *reinterpret_cast<const OT::Condition *> (condition_data);
  hb_always_assert (condition.evaluate (coords, 1, &instancer));
  bytes.fini ();
}

static void
test_failed_encoding ()
{
  hb_vector_t<int> row;
  row.push (1);
  hb_vector_t<const hb_vector_t<int> *> rows;
  rows.push (&row);
  OT::item_variations_t item_vars;
  hb_always_assert (item_vars.add_vardata_encoding_for_testing (std::move (rows), 1));
  auto &encoding = const_cast<OT::delta_row_encoding_t &> (item_vars.get_vardata_encodings ()[0]);
  /* Simulate a failed allocation of the encoding's column metadata. */
  encoding.chars.clear ();
  encoding.chars.set_error ();
  hb_always_assert (encoding.in_error ());
  hb_hashmap_t<unsigned, const hb_vector_t<int> *> front_mapping;
  front_mapping.set (0, &row);
  hb_always_assert (!item_vars.compile_varidx_map_for_testing (front_mapping));
}

static void
test_item_varstore_instancer_varidx_overflow ()
{
  /* Minimal sanitized ItemVariationStore: 1 axis, 1 region [0, 1, 1],
   * 1 VarData item [0][0] with 8-bit delta = +42. */
  alignas (4) static const uint8_t store_bytes[] = {
    0, 1,             /* format = 1 */
    0, 0, 0, 12,      /* regionListOffset = 12 */
    0, 1,             /* dataCount = 1 */
    0, 0, 0, 22,      /* dataSets[0] = 22 */
    /* VarRegionList @ 12 (10 bytes): axisCount=1, regionCount=1, [0, 1, 1] */
    0, 1, 0, 1,
    0x00, 0x00, 0x40, 0x00, 0x40, 0x00,
    /* VarData @ 22 (9 bytes): itemCount=1, wordDeltaCount=0, regionIdxCount=1, reg[0]=0, delta=42 */
    0, 1, 0, 0, 0, 1, 0, 0, 42
  };
  hb_blob_t *blob = hb_blob_create ((const char *) store_bytes, sizeof (store_bytes),
                                    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_sanitize_context_t sc (blob);
  const auto &store = *reinterpret_cast<const OT::ItemVariationStore *> (store_bytes);
  hb_always_assert (store.sanitize (&sc));
  hb_blob_destroy (blob);

  int coords[] = {16384};
  OT::ItemVarStoreInstancer instancer (&store, nullptr, hb_array (coords));
#ifdef HB_NO_VAR
  hb_always_assert (instancer (0, 0) == 0.f);
#else
  hb_always_assert (instancer (0, 0) == 42.f);
#endif
  hb_always_assert (instancer (OT::VarIdx::NO_VARIATION, 0) == 0.f);
  hb_always_assert (instancer (OT::VarIdx::NO_VARIATION, 1) == 0.f);
  /* 0xFFFFFFFE + 1 == NO_VARIATION and 0xFFFFFFFE + 2 wraps 32-bit to 0. */
  hb_always_assert (instancer (0xFFFFFFFEu, 1) == 0.f);
  hb_always_assert (instancer (0xFFFFFFFEu, 2) == 0.f);
}


int
main (int argc, char **argv)
{
  test_var_region_list_serialize ();
  test_var_region_list_serialize_bounds ();
  test_item_varstore_instancer_varidx_overflow ();
  test_scalar_cache_precision ();
  test_item_variations ();
  test_implicit_advance_mapping ();
  test_item_variations_overflow ();
  test_signed_delta_widths ();
  test_failed_encoding ();
  test_accumulated_delta_widths ();
  test_condition_delta_precision ();
}
