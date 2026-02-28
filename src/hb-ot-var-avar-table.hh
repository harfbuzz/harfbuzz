/*
 * Copyright © 2017  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_VAR_AVAR_TABLE_HH
#define HB_OT_VAR_AVAR_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-var-common.hh"


/*
 * avar -- Axis Variations
 * https://docs.microsoft.com/en-us/typography/opentype/spec/avar
 */

#define HB_OT_TAG_avar HB_TAG('a','v','a','r')


namespace OT {


/* "Spec": https://github.com/be-fonts/boring-expansion-spec/issues/14 */
struct avarV2Tail
{
  friend struct avar;

  bool sanitize (hb_sanitize_context_t *c,
		 const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (varIdxMap.sanitize (c, base) &&
		  varStore.sanitize (c, base));
  }

  protected:
  Offset32To<DeltaSetIndexMap>	varIdxMap;	/* Offset from the beginning of 'avar' table. */
  Offset32To<ItemVariationStore>	varStore;	/* Offset from the beginning of 'avar' table. */

  public:
  DEFINE_SIZE_STATIC (8);
};


struct AxisValueMap
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  void set_mapping (float from_coord, float to_coord)
  {
    coords[0].set_float (from_coord);
    coords[1].set_float (to_coord);
  }

  bool is_outside_axis_range (const Triple& axis_range) const
  {
    double from_coord = (double) coords[0].to_float ();
    return !axis_range.contains (from_coord);
  }

  bool must_include () const
  {
    float from_coord = coords[0].to_float ();
    float to_coord = coords[1].to_float ();
    return (from_coord == -1.f && to_coord == -1.f) ||
           (from_coord == 0.f && to_coord == 0.f) ||
           (from_coord == 1.f && to_coord == 1.f);
  }

  void instantiate (const Triple& axis_range,
                    const Triple& unmapped_range,
                    const TripleDistances& triple_distances)
  {
    float from_coord = coords[0].to_float ();
    float to_coord = coords[1].to_float ();

    from_coord = renormalizeValue ((double) from_coord, unmapped_range, triple_distances);
    to_coord = renormalizeValue ((double) to_coord, axis_range, triple_distances);

    coords[0].set_float (from_coord);
    coords[1].set_float (to_coord);
  }

  HB_INTERNAL static int cmp (const void *pa, const void *pb)
  {
    const AxisValueMap *a = (const AxisValueMap *) pa;
    const AxisValueMap *b = (const AxisValueMap *) pb;

    int a_from = a->coords[0].to_int ();
    int b_from = b->coords[0].to_int ();
    if (a_from != b_from)
      return a_from - b_from;

    /* this should never be reached. according to the spec, all of the axis
     * value map records for a given axis must have different fromCoord values
     * */
    int a_to = a->coords[1].to_int ();
    int b_to = b->coords[1].to_int ();
    return a_to - b_to;
  }

  bool serialize (hb_serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);
    return_trace (c->embed (this));
  }

  public:
  F2DOT14	coords[2];
//   F2DOT14	fromCoord;	/* A normalized coordinate value obtained using
//				 * default normalization. */
//   F2DOT14	toCoord;	/* The modified, normalized coordinate value. */

  public:
  DEFINE_SIZE_STATIC (4);
};

struct SegmentMaps : Array16Of<AxisValueMap>
{
  float map_float (float value, unsigned int from_offset = 0, unsigned int to_offset = 1) const
  {
#define fromCoord coords[from_offset].to_float ()
#define toCoord coords[to_offset].to_float ()

    const auto *map = arrayZ;

    /* The following special-cases are not part of OpenType, which requires
     * that at least -1, 0, and +1 must be mapped. But we include these as
     * part of a better error recovery scheme. */
    if (len < 2)
    {
      if (!len)
	return value;
      else /* len == 1*/
	return value - map[0].fromCoord + map[0].toCoord;
    }

    // At least two mappings now.

    /* CoreText is wild...
     * PingFangUI avar needs all this special-casing...
     * So we implement an extended version of the spec here,
     * which is more robust and more likely to be compatible with
     * the wild. */

    unsigned start = 0;
    unsigned end = len;
    if (map[start].fromCoord == -1 && map[start].toCoord == -1 && map[start+1].fromCoord == -1)
      start++;
    if (map[end-1].fromCoord == +1 && map[end-1].toCoord == +1 && map[end-2].fromCoord == +1)
      end--;

    /* Look for exact match first, and do lots of special-casing. */
    unsigned i;
    for (i = start; i < end; i++)
      if (value == map[i].fromCoord)
	break;
    if (i < end)
    {
      // There's at least one exact match. See if there are more.
      unsigned j = i;
      for (; j + 1 < end; j++)
	if (value != map[j + 1].fromCoord)
	  break;

      // [i,j] inclusive are all exact matches:

      // If there's only one, return it. This is the only spec-compliant case.
      if (i == j)
	return map[i].toCoord;
      // If there's exactly three, return the middle one.
      if (i + 2 == j)
	return map[i + 1].toCoord;

      // Ignore the middle ones. Return the one mapping closer to 0.
      if (value < 0) return map[j].toCoord;
      if (value > 0) return map[i].toCoord;

      // Mapping 0? CoreText seems confused. It seems to prefer 0 here...
      // So we'll just return the smallest one. lol
      return fabsf (map[i].toCoord) < fabsf (map[j].toCoord) ? map[i].toCoord : map[j].toCoord;

      // Mapping 0? Return one not mapping to 0.
      if (map[i].toCoord == 0)
	return map[j].toCoord;
      else
	return map[i].toCoord;
    }

    /* There's at least two and we're not an exact match. Prepare to lerp. */

    // Find the segment we're in.
    for (i = start; i < end; i++)
      if (value < map[i].fromCoord)
	break;

    if (i == 0)
    {
      // Value before all segments; Shift.
      return value - map[0].fromCoord + map[0].toCoord;
    }
    if (i == end)
    {
      // Value after all segments; Shift.
      return value - map[end - 1].fromCoord + map[end - 1].toCoord;
    }

    // Actually interpolate.
    auto &before = map[i-1];
    auto &after = map[i];
    float denom = after.fromCoord - before.fromCoord; // Can't be zero by now.
    return before.toCoord + ((after.toCoord - before.toCoord) * (value - before.fromCoord)) / denom;

#undef toCoord
#undef fromCoord
  }

  float unmap_float (float value) const { return map_float (value, 1, 0); }


  // TODO Kill this.
  Triple unmap_axis_range (const Triple& axis_range) const
  {
    float unmapped_min = unmap_float (axis_range.minimum);
    float unmapped_middle = unmap_float (axis_range.middle);
    float unmapped_max = unmap_float (axis_range.maximum);

    return Triple{(double) unmapped_min, (double) unmapped_middle, (double) unmapped_max};
  }

  bool subset (hb_subset_context_t *c, hb_tag_t axis_tag) const
  {
    TRACE_SUBSET (this);

    /* This function cannot work on avar2 table (and currently doesn't).
     * We should instead keep the design coords in the shape plan and use
     * those. unmap_axis_range needs to be killed. */

    /* avar mapped normalized axis range*/
    Triple *axis_range;
    if (!c->plan->axes_location.has (axis_tag, &axis_range))
      return c->serializer->embed (*this);

    TripleDistances *axis_triple_distances;
    if (!c->plan->axes_triple_distances.has (axis_tag, &axis_triple_distances))
      return_trace (false);

    auto *out = c->serializer->start_embed (this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);

    Triple unmapped_range = unmap_axis_range (*axis_range);

    /* create a vector of retained mappings and sort */
    hb_vector_t<AxisValueMap> value_mappings;
    for (const auto& _ : as_array ())
    {
      if (_.is_outside_axis_range (unmapped_range))
        continue;
      AxisValueMap mapping;
      mapping = _;
      mapping.instantiate (*axis_range, unmapped_range, *axis_triple_distances);
      /* (-1, -1), (0, 0), (1, 1) mappings will be added later, so avoid
       * duplicates here */
      if (mapping.must_include ())
        continue;
      value_mappings.push (mapping);
    }

    AxisValueMap m;
    m.set_mapping (-1.f, -1.f);
    value_mappings.push (m);

    m.set_mapping (0.f, 0.f);
    value_mappings.push (m);

    m.set_mapping (1.f, 1.f);
    value_mappings.push (m);

    value_mappings.qsort ();

    for (const auto& _ : value_mappings)
    {
      if (!_.serialize (c->serializer))
        return_trace (false);
    }
    return_trace (c->serializer->check_assign (out->len, value_mappings.length, HB_SERIALIZE_ERROR_INT_OVERFLOW));
  }

  public:
  DEFINE_SIZE_ARRAY (2, *this);
};

struct avar
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_avar;

  bool has_data () const { return version.to_int (); }

  const SegmentMaps* get_segment_maps () const
  { return &firstAxisSegmentMaps; }

  unsigned get_axis_count () const
  { return axisCount; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!(version.sanitize (c) &&
	  hb_barrier () &&
	  (version.major == 1
#ifndef HB_NO_AVAR2
	   || version.major == 2
#endif
	   ) &&
	  c->check_struct (this)))
      return_trace (false);

    const SegmentMaps *map = &firstAxisSegmentMaps;
    unsigned int count = axisCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (unlikely (!map->sanitize (c)))
	return_trace (false);
      map = &StructAfter<SegmentMaps> (*map);
    }

#ifndef HB_NO_AVAR2
    if (version.major < 2)
      return_trace (true);
    hb_barrier ();

    const auto &v2 = * (const avarV2Tail *) map;
    if (unlikely (!v2.sanitize (c, this)))
      return_trace (false);
#endif

    return_trace (true);
  }

  void map_coords_16_16 (int *coords, unsigned int coords_length) const
  {
    unsigned int count = hb_min (coords_length, axisCount);

    const SegmentMaps *map = &firstAxisSegmentMaps;
    for (unsigned int i = 0; i < count; i++)
    {
      coords[i] = roundf (map->map_float (coords[i] / 65536.f) * 65536.f);
      map = &StructAfter<SegmentMaps> (*map);
    }

#ifndef HB_NO_AVAR2
    if (version.major < 2)
      return;
    hb_barrier ();

    for (; count < axisCount; count++)
      map = &StructAfter<SegmentMaps> (*map);

    const auto &v2 = * (const avarV2Tail *) map;

    const auto &varidx_map = this+v2.varIdxMap;
    const auto &var_store = this+v2.varStore;
    auto *var_store_cache = var_store.create_cache ();

    hb_vector_t<int> coords_2_14;
    coords_2_14.resize (coords_length);
    for (unsigned i = 0; i < coords_length; i++)
      coords_2_14[i] = roundf (coords[i] / 4.f); // 16.16 -> 2.14

    hb_vector_t<int> out;
    out.alloc (coords_length);
    for (unsigned i = 0; i < coords_length; i++)
    {
      int v = coords[i];
      uint32_t varidx = varidx_map.map (i);
      float delta = var_store.get_delta (varidx, coords_2_14.arrayZ, coords_2_14.length, var_store_cache);
      v += roundf (delta * 4); // 2.14 -> 16.16
      v = hb_clamp (v, -(1<<16), +(1<<16));
      out.push (v);
    }
    for (unsigned i = 0; i < coords_length; i++)
      coords[i] = out[i];

    OT::ItemVariationStore::destroy_cache (var_store_cache);
#endif
  }

  bool has_v2_data () const { return version.major > 1; }

  /* Apply avar v1 segment maps only, deliberately omitting avar v2 processing.
   * Used during avar2 partial instancing to compute intermediate-space coords. */
  bool map_coords_v1_only (float *coords, unsigned int coords_length) const
  {
    unsigned int count = hb_min (coords_length, axisCount);
    const SegmentMaps *map = &firstAxisSegmentMaps;
    for (unsigned int i = 0; i < count; i++)
    {
      int v = roundf (map->map_float (coords[i]) * 16384.f);
      coords[i] = v / 16384.f;
      map = &StructAfter<SegmentMaps> (*map);
    }
    return true;
  }

  // axis normalization is done in 2.14 here
  // TODO: deprecate this API once fonttools is updated to use 16.16 normalization
  bool map_coords_2_14 (float *coords, unsigned int coords_length) const
  {
    hb_vector_t<int> coords_2_14;
    if (!coords_2_14.resize (coords_length)) return false;
    unsigned int count = hb_min (coords_length, axisCount);

    const SegmentMaps *map = &firstAxisSegmentMaps;
    for (unsigned int i = 0; i < count; i++)
    {
      int v = roundf (map->map_float (coords[i]) * 16384.f);
      coords_2_14[i] = v;
      coords[i] = v / 16384.f;
      map = &StructAfter<SegmentMaps> (*map);
    }

#ifndef HB_NO_AVAR2
    if (version.major < 2)
      return true;
    hb_barrier ();

    for (; count < axisCount; count++)
      map = &StructAfter<SegmentMaps> (*map);

    const auto &v2 = * (const avarV2Tail *) map;

    const auto &varidx_map = this+v2.varIdxMap;
    const auto &var_store = this+v2.varStore;
    auto *var_store_cache = var_store.create_cache ();

    for (unsigned i = 0; i < coords_length; i++)
    {
      int v = coords_2_14[i];
      uint32_t varidx = varidx_map.map (i);
      float delta = var_store.get_delta (varidx, coords_2_14.arrayZ, coords_2_14.length, var_store_cache);
      v += roundf (delta);
      v = hb_clamp (v, -(1<<16), +(1<<16));
      coords[i] = v / 16384.f;
    }

    OT::ItemVariationStore::destroy_cache (var_store_cache);
    return true;
#else
    return version.major < 2;
#endif
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    unsigned retained_axis_count = c->plan->axes_index_map.get_population ();
    if (!retained_axis_count) //all axes are pinned/dropped
      return_trace (false);

    avar *out = c->serializer->allocate_min<avar> ();
    if (unlikely (!out)) return_trace (false);

    out->version.major = c->plan->has_avar2 ? 2 : 1;
    out->version.minor = 0;
    if (!c->serializer->check_assign (out->axisCount, retained_axis_count, HB_SERIALIZE_ERROR_INT_OVERFLOW))
      return_trace (false);

    const hb_map_t& axes_index_map = c->plan->axes_index_map;
    const SegmentMaps *map = &firstAxisSegmentMaps;
    unsigned count = axisCount;
    for (unsigned int i = 0; i < count; i++)
    {
      if (axes_index_map.has (i))
      {
        hb_tag_t *axis_tag;
        if (!c->plan->axes_old_index_tag_map.has (i, &axis_tag))
          return_trace (false);

        if (c->plan->has_avar2 &&
            c->plan->user_axes_location.has (*axis_tag) &&
            c->plan->user_axes_location.get (*axis_tag).is_point ())
        {
          /* Pinned axis in avar2 mode: serialize identity segment map
           * {-1->-1, 0->0, 1->1}. The axis is kept in fvar as hidden,
           * so avar needs a segment map entry for it. */
          auto *identity_map = c->serializer->start_embed<SegmentMaps> ();
          if (unlikely (!c->serializer->extend_min (identity_map)))
            return_trace (false);
          AxisValueMap m;
          m.set_mapping (-1.f, -1.f);
          if (!m.serialize (c->serializer)) return_trace (false);
          m.set_mapping (0.f, 0.f);
          if (!m.serialize (c->serializer)) return_trace (false);
          m.set_mapping (1.f, 1.f);
          if (!m.serialize (c->serializer)) return_trace (false);
          if (!c->serializer->check_assign (identity_map->len, 3u,
                                            HB_SERIALIZE_ERROR_INT_OVERFLOW))
            return_trace (false);
        }
        else
        {
          /* Restricted or free axis: use standard SegmentMaps::subset() */
          if (!map->subset (c, *axis_tag))
            return_trace (false);
        }
      }
      map = &StructAfter<SegmentMaps> (*map);
    }

    if (c->plan->has_avar2)
      return_trace (_subset_avar2 (c, out));

    return_trace (true);
  }

  private:
  bool _subset_avar2 (hb_subset_context_t *c, avar *out) const
  {
#ifdef HB_NO_AVAR2
    return false;
#endif

    /* 1. Locate original avar2 data */
    const SegmentMaps *map = &firstAxisSegmentMaps;
    for (unsigned i = 0; i < axisCount; i++)
      map = &StructAfter<SegmentMaps> (*map);

    const auto &v2 = * (const avarV2Tail *) map;
    const auto &varidx_map = this+v2.varIdxMap;
    const auto &var_store = this+v2.varStore;

    /* 2. Compute default deltas by evaluating VarStore at old defaults */
    hb_vector_t<int> default_coords;
    if (!default_coords.resize (axisCount)) return false;
    for (unsigned i = 0; i < axisCount; i++)
    {
      hb_tag_t *axis_tag;
      if (c->plan->axes_old_index_tag_map.has (i, &axis_tag) &&
          c->plan->old_intermediates.has (*axis_tag))
      {
        float d_i = (float) c->plan->old_intermediates.get (*axis_tag).middle;
        default_coords[i] = roundf (d_i * 16384.f);
      }
      else
        default_coords[i] = 0;
    }

    auto *store_cache = var_store.create_cache ();
    hb_vector_t<float> default_deltas;
    if (!default_deltas.resize (axisCount)) {
      ItemVariationStore::destroy_cache (store_cache);
      return false;
    }
    for (unsigned i = 0; i < axisCount; i++)
    {
      uint32_t varidx = varidx_map.map (i);
      if (varidx == HB_OT_LAYOUT_NO_VARIATIONS_INDEX)
        default_deltas[i] = 0.f;
      else
        default_deltas[i] = var_store.get_delta (varidx, default_coords.arrayZ,
                                                  default_coords.length, store_cache);
    }
    ItemVariationStore::destroy_cache (store_cache);

    /* 3. Rebase IVS regions */
    item_variations_t item_vars;
    if (!item_vars.create_from_item_varstore (var_store, c->plan->axes_old_index_tag_map))
      return false;
    if (!item_vars.instantiate_tuple_vars_no_region_build (c->plan->axes_location,
                                                           c->plan->axes_triple_distances))
      return false;

    /* 4. Detect self-contained pinned axes.
     * A pinned axis is self-contained if after IVS rebasing, no remaining
     * TupleVariation has a non-zero delta at that axis's inner position. */
    /* (For now, skip self-contained detection -- all pinned axes kept as hidden) */

    /* 5. Build per-axis varIdx mapping (may create new VarDatas) */
    hb_vector_t<uint32_t> new_varidx_mapping;
    if (!new_varidx_mapping.resize (axisCount)) return false;
    for (unsigned i = 0; i < axisCount; i++)
      new_varidx_mapping[i] = varidx_map.map (i);

    /* 6. Add offset compensation tuples */
    for (unsigned i = 0; i < axisCount; i++)
    {
      hb_tag_t *axis_tag_ptr;
      if (!c->plan->axes_old_index_tag_map.has (i, &axis_tag_ptr))
        continue;
      hb_tag_t axis_tag = *axis_tag_ptr;

      if (c->plan->user_axes_location.has (axis_tag))
      {
        /* This axis is being restricted or pinned */
        Triple *old_int;
        if (!c->plan->old_intermediates.has (axis_tag, &old_int))
          continue;

        float a_i = (float) old_int->minimum;
        float d_i = (float) old_int->middle;
        float b_i = (float) old_int->maximum;

        int a_int = roundf (a_i * 16384.f);
        int d_int = roundf (d_i * 16384.f);
        int b_int = roundf (b_i * 16384.f);

        bool is_pinned = c->plan->user_axes_location.get (axis_tag).is_point ();

        uint32_t varidx = new_varidx_mapping[i];
        unsigned outer, inner, item_count;

        if (varidx == HB_OT_LAYOUT_NO_VARIATIONS_INDEX)
        {
          /* No existing avar2 mapping. Create new VarData. */
          outer = item_vars.add_vardata (1);
          inner = 0;
          item_count = 1;
          new_varidx_mapping[i] = (outer << 16) | inner;
          default_deltas[i] = 0.f; /* no prior default delta */
        }
        else
        {
          outer = varidx >> 16;
          inner = varidx & 0xFFFF;
          item_count = item_vars.get_item_count (outer);
        }

        /* Empty-region bias: d_int + round(defaultDelta) */
        int bias = d_int + (int) roundf (default_deltas[i]);
        if (bias != 0)
        {
          hb_hashmap_t<hb_tag_t, Triple> empty_region;
          item_vars.add_tuple (outer, std::move (empty_region),
                               inner, bias, item_count);
        }

        if (!is_pinned)
        {
          /* Tent (-1, -1, 0): delta = a_int + (1<<14) - d_int */
          int neg_delta = a_int + (1 << 14) - d_int;
          if (neg_delta != 0)
          {
            hb_hashmap_t<hb_tag_t, Triple> neg_region;
            neg_region.set (axis_tag, Triple (-1., -1., 0.));
            item_vars.add_tuple (outer, std::move (neg_region),
                                 inner, neg_delta, item_count);
          }

          /* Tent (0, +1, +1): delta = b_int - (1<<14) - d_int */
          int pos_delta = b_int - (1 << 14) - d_int;
          if (pos_delta != 0)
          {
            hb_hashmap_t<hb_tag_t, Triple> pos_region;
            pos_region.set (axis_tag, Triple (0., 1., 1.));
            item_vars.add_tuple (outer, std::move (pos_region),
                                 inner, pos_delta, item_count);
          }
        }
      }
      else
      {
        /* Free or private axis — not being restricted.
         * If it has a non-zero default delta, add it back as a bias. */
        uint32_t varidx = new_varidx_mapping[i];
        if (varidx == HB_OT_LAYOUT_NO_VARIATIONS_INDEX)
          continue;

        unsigned outer = varidx >> 16;
        unsigned inner = varidx & 0xFFFF;
        int dd = (int) roundf (default_deltas[i]);
        if (dd != 0)
        {
          unsigned item_count = item_vars.get_item_count (outer);
          hb_hashmap_t<hb_tag_t, Triple> empty_region;
          item_vars.add_tuple (outer, std::move (empty_region),
                               inner, dd, item_count);
        }
      }
    }

    /* 7. Finalize: build region list + convert to varstore */
    if (!item_vars.build_region_list ()) return false;
    if (!item_vars.as_item_varstore (true /* optimize */,
                                     false /* use_no_variation_idx */))
      return false;

    /* 8. Apply varidx_map optimization remapping */
    const auto &opt_varidx_map = item_vars.get_varidx_map ();
    for (unsigned i = 0; i < axisCount; i++)
    {
      uint32_t varidx = new_varidx_mapping[i];
      if (varidx == HB_OT_LAYOUT_NO_VARIATIONS_INDEX)
        continue;
      uint32_t *new_idx;
      if (opt_varidx_map.has (varidx, &new_idx))
        new_varidx_mapping[i] = *new_idx;
    }

    /* 9. Serialize avarV2Tail */
    /* The avarV2Tail has two Offset32 fields (varIdxMap and varStore)
     * whose offsets are relative to the beginning of the avar table.
     * We serialize each sub-object via push/pop_pack, write the tail
     * struct inline, and link offsets to the avar table start. */

    /* Serialize DeltaSetIndexMap (push a new object) */
    hb_serialize_context_t::objidx_t packed_map;
    {
      /* Compute width and inner_bit_count for the mapping */
      unsigned max_outer = 0, max_inner = 0;
      for (unsigned i = 0; i < axisCount; i++)
      {
        uint32_t varidx = new_varidx_mapping[i];
        if (varidx == HB_OT_LAYOUT_NO_VARIATIONS_INDEX)
          continue;
        unsigned o = varidx >> 16;
        unsigned in = varidx & 0xFFFF;
        if (o > max_outer) max_outer = o;
        if (in > max_inner) max_inner = in;
      }

      unsigned inner_bit_count = hb_max (1u, hb_bit_storage (max_inner));
      unsigned outer_bit_count = hb_max (1u, hb_bit_storage (max_outer));
      unsigned width = (inner_bit_count + outer_bit_count + 7) / 8;
      width = hb_max (width, 1u);
      width = hb_min (width, 4u);

      /* Recalculate inner_bit_count based on width */
      if (inner_bit_count + outer_bit_count > width * 8)
        inner_bit_count = width * 8 - outer_bit_count;

      /* Serialize DeltaSetIndexMap format 0 (HBUINT16 mapCount) */
      c->serializer->push ();
      auto *fmt = c->serializer->allocate_size<HBUINT8> (1);
      if (unlikely (!fmt)) { c->serializer->pop_discard (); return false; }
      *fmt = 0; /* format 0 */

      auto *entry_format = c->serializer->allocate_size<HBUINT8> (1);
      if (unlikely (!entry_format)) { c->serializer->pop_discard (); return false; }
      *entry_format = ((width - 1) << 4) | (inner_bit_count - 1);

      auto *map_count_field = c->serializer->allocate_size<HBUINT16> (HBUINT16::static_size);
      if (unlikely (!map_count_field)) { c->serializer->pop_discard (); return false; }
      *map_count_field = axisCount;

      HBUINT8 *p = c->serializer->allocate_size<HBUINT8> (width * axisCount);
      if (unlikely (!p)) { c->serializer->pop_discard (); return false; }

      for (unsigned i = 0; i < axisCount; i++)
      {
        uint32_t varidx = new_varidx_mapping[i];
        unsigned o = varidx >> 16;
        unsigned in = varidx & 0xFFFF;
        unsigned u = (o << inner_bit_count) | in;
        for (unsigned w = width; w > 0;)
        {
          p[--w] = u;
          u >>= 8;
        }
        p += width;
      }

      packed_map = c->serializer->pop_pack ();
    }

    /* Serialize ItemVariationStore (push a new object) */
    hb_serialize_context_t::objidx_t packed_store;
    {
      c->serializer->push ();
      auto *ivs = c->serializer->start_embed<ItemVariationStore> ();
      if (!ivs->serialize (c->serializer,
                           item_vars.has_long_word (),
                           c->plan->axis_tags,
                           item_vars.get_region_list (),
                           item_vars.get_vardata_encodings ()))
      {
        c->serializer->pop_discard ();
        return false;
      }
      packed_store = c->serializer->pop_pack ();
    }

    /* Write the avarV2Tail struct inline in the avar table */
    auto *tail = c->serializer->allocate_size<avarV2Tail> (avarV2Tail::static_size);
    if (unlikely (!tail)) return false;

    /* Link offsets relative to the avar table start */
    unsigned bias = c->serializer->to_bias (out);
    c->serializer->add_link (tail->varIdxMap, packed_map,
                              hb_serialize_context_t::Head, bias);
    c->serializer->add_link (tail->varStore, packed_store,
                              hb_serialize_context_t::Head, bias);

    return true;
  }

  public:

  protected:
  FixedVersion<>version;	/* Version of the avar table
				 * initially set to 0x00010000u */
  HBUINT16	reserved;	/* This field is permanently reserved. Set to 0. */
  HBUINT16	axisCount;	/* The number of variation axes in the font. This
				 * must be the same number as axisCount in the
				 * 'fvar' table. */
  SegmentMaps	firstAxisSegmentMaps;

  public:
  DEFINE_SIZE_MIN (8);
};

} /* namespace OT */


#endif /* HB_OT_VAR_AVAR_TABLE_HH */
