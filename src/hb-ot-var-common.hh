/*
 * Copyright © 2021  Google, Inc.
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

#ifndef HB_OT_VAR_COMMON_HH
#define HB_OT_VAR_COMMON_HH

#include "hb-ot-layout-common.hh"


namespace OT {

template <typename MapCountT>
struct DeltaSetIndexMapFormat01
{
  friend struct DeltaSetIndexMap;

  private:
  DeltaSetIndexMapFormat01* copy (hb_serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);
    auto *out = c->start_embed (this);

    unsigned total_size = min_size + mapCount * get_width ();
    HBUINT8 *p = c->allocate_size<HBUINT8> (total_size);
    if (unlikely (!p)) return_trace (nullptr);

    hb_memcpy (p, this, HBUINT8::static_size * total_size);
    return_trace (out);
  }

  template <typename T>
  bool serialize (hb_serialize_context_t *c, const T &plan)
  {
    unsigned int width = plan.get_width ();
    unsigned int inner_bit_count = plan.get_inner_bit_count ();
    const hb_array_t<const uint32_t> output_map = plan.get_output_map ();

    TRACE_SERIALIZE (this);
    if (unlikely (output_map.length && ((((inner_bit_count-1)&~0xF)!=0) || (((width-1)&~0x3)!=0))))
      return_trace (false);
    if (unlikely (!c->extend_min (this))) return_trace (false);

    entryFormat = ((width-1)<<4)|(inner_bit_count-1);
    mapCount = output_map.length;
    HBUINT8 *p = c->allocate_size<HBUINT8> (width * output_map.length);
    if (unlikely (!p)) return_trace (false);
    for (unsigned int i = 0; i < output_map.length; i++)
    {
      unsigned int v = output_map[i];
      unsigned int outer = v >> 16;
      unsigned int inner = v & 0xFFFF;
      unsigned int u = (outer << inner_bit_count) | inner;
      for (unsigned int w = width; w > 0;)
      {
        p[--w] = u;
        u >>= 8;
      }
      p += width;
    }
    return_trace (true);
  }

  uint32_t map (unsigned int v) const /* Returns 16.16 outer.inner. */
  {
    /* If count is zero, pass value unchanged.  This takes
     * care of direct mapping for advance map. */
    if (!mapCount)
      return v;

    if (v >= mapCount)
      v = mapCount - 1;

    unsigned int u = 0;
    { /* Fetch it. */
      unsigned int w = get_width ();
      const HBUINT8 *p = mapDataZ.arrayZ + w * v;
      for (; w; w--)
        u = (u << 8) + *p++;
    }

    { /* Repack it. */
      unsigned int n = get_inner_bit_count ();
      unsigned int outer = u >> n;
      unsigned int inner = u & ((1 << n) - 1);
      u = (outer<<16) | inner;
    }

    return u;
  }

  unsigned get_map_count () const       { return mapCount; }
  unsigned get_width () const           { return ((entryFormat >> 4) & 3) + 1; }
  unsigned get_inner_bit_count () const { return (entryFormat & 0xF) + 1; }


  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  c->check_range (mapDataZ.arrayZ,
                                  mapCount,
                                  get_width ()));
  }

  protected:
  HBUINT8       format;         /* Format identifier--format = 0 */
  HBUINT8       entryFormat;    /* A packed field that describes the compressed
                                 * representation of delta-set indices. */
  MapCountT     mapCount;       /* The number of mapping entries. */
  UnsizedArrayOf<HBUINT8>
                mapDataZ;       /* The delta-set index mapping data. */

  public:
  DEFINE_SIZE_ARRAY (2+MapCountT::static_size, mapDataZ);
};

struct DeltaSetIndexMap
{
  template <typename T>
  bool serialize (hb_serialize_context_t *c, const T &plan)
  {
    TRACE_SERIALIZE (this);
    unsigned length = plan.get_output_map ().length;
    u.format = length <= 0xFFFF ? 0 : 1;
    switch (u.format) {
    case 0: return_trace (u.format0.serialize (c, plan));
    case 1: return_trace (u.format1.serialize (c, plan));
    default:return_trace (false);
    }
  }

  uint32_t map (unsigned v) const
  {
    switch (u.format) {
    case 0: return (u.format0.map (v));
    case 1: return (u.format1.map (v));
    default:return v;
    }
  }

  unsigned get_map_count () const
  {
    switch (u.format) {
    case 0: return u.format0.get_map_count ();
    case 1: return u.format1.get_map_count ();
    default:return 0;
    }
  }

  unsigned get_width () const
  {
    switch (u.format) {
    case 0: return u.format0.get_width ();
    case 1: return u.format1.get_width ();
    default:return 0;
    }
  }

  unsigned get_inner_bit_count () const
  {
    switch (u.format) {
    case 0: return u.format0.get_inner_bit_count ();
    case 1: return u.format1.get_inner_bit_count ();
    default:return 0;
    }
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return_trace (false);
    switch (u.format) {
    case 0: return_trace (u.format0.sanitize (c));
    case 1: return_trace (u.format1.sanitize (c));
    default:return_trace (true);
    }
  }

  DeltaSetIndexMap* copy (hb_serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);
    switch (u.format) {
    case 0: return_trace (reinterpret_cast<DeltaSetIndexMap *> (u.format0.copy (c)));
    case 1: return_trace (reinterpret_cast<DeltaSetIndexMap *> (u.format1.copy (c)));
    default:return_trace (nullptr);
    }
  }

  protected:
  union {
  HBUINT8                            format;         /* Format identifier */
  DeltaSetIndexMapFormat01<HBUINT16> format0;
  DeltaSetIndexMapFormat01<HBUINT32> format1;
  } u;
  public:
  DEFINE_SIZE_UNION (1, format);
};


struct VarStoreInstancer
{
  VarStoreInstancer (const VariationStore *varStore,
		     const DeltaSetIndexMap *varIdxMap,
		     hb_array_t<int> coords) :
    varStore (varStore), varIdxMap (varIdxMap), coords (coords) {}

  operator bool () const { return varStore && bool (coords); }

  /* according to the spec, if colr table has varStore but does not have
   * varIdxMap, then an implicit identity mapping is used */
  float operator() (uint32_t varIdx, unsigned short offset = 0) const
  { return varStore->get_delta (varIdxMap ? varIdxMap->map (VarIdx::add (varIdx, offset)) : varIdx + offset, coords); }

  const VariationStore *varStore;
  const DeltaSetIndexMap *varIdxMap;
  hb_array_t<int> coords;
};

/* https://docs.microsoft.com/en-us/typography/opentype/spec/otvarcommonformats#tuplevariationheader */
struct TupleVariationHeader
{
  unsigned get_size (unsigned axis_count) const
  { return min_size + get_all_tuples (axis_count).get_size (); }

  unsigned get_data_size () const { return varDataSize; }

  const TupleVariationHeader &get_next (unsigned axis_count) const
  { return StructAtOffset<TupleVariationHeader> (this, get_size (axis_count)); }

  bool unpack_axis_tuples (unsigned axis_count,
                           const hb_array_t<const F2DOT14> shared_tuples,
                           const hb_map_t *axes_old_index_tag_map,
                           hb_hashmap_t<hb_tag_t, Triple>& axis_tuples /* OUT */) const
  {
    const F2DOT14 *peak_tuple = nullptr;
    if (has_peak ())
      peak_tuple = get_peak_tuple (axis_count).arrayZ;
    else
    {
      unsigned int index = get_index ();
      if (unlikely ((index + 1) * axis_count > shared_tuples.length))
        return false;
      peak_tuple = shared_tuples.sub_array (axis_count * index, axis_count).arrayZ;
    }

    const F2DOT14 *start_tuple = nullptr;
    const F2DOT14 *end_tuple = nullptr;
    bool has_interm = has_intermediate ();

    if (has_interm)
    {
      start_tuple = get_start_tuple (axis_count).arrayZ;
      end_tuple = get_end_tuple (axis_count).arrayZ;
    }

    for (unsigned i = 0; i < axis_count; i++)
    {
      float peak = peak_tuple[i].to_float ();
      if (peak == 0.f) continue;

      hb_tag_t *axis_tag;
      if (!axes_old_index_tag_map->has (i, &axis_tag))
        return false;

      float start, end;
      if (has_interm)
      {
        start = start_tuple[i].to_float ();
        end = end_tuple[i].to_float ();
      }
      else
      {
        start = hb_min (peak, 0.f);
        end = hb_max (peak, 0.f);
      }
      axis_tuples.set (*axis_tag, Triple (start, peak, end));
    }

    return true;
  }

  float calculate_scalar (hb_array_t<int> coords, unsigned int coord_count,
                          const hb_array_t<const F2DOT14> shared_tuples,
			  const hb_vector_t<int> *shared_tuple_active_idx = nullptr) const
  {
    const F2DOT14 *peak_tuple;

    unsigned start_idx = 0;
    unsigned end_idx = coord_count;

    if (has_peak ())
      peak_tuple = get_peak_tuple (coord_count).arrayZ;
    else
    {
      unsigned int index = get_index ();
      if (unlikely ((index + 1) * coord_count > shared_tuples.length))
        return 0.f;
      peak_tuple = shared_tuples.sub_array (coord_count * index, coord_count).arrayZ;

      if (shared_tuple_active_idx)
      {
	if (unlikely (index >= shared_tuple_active_idx->length))
	  return 0.f;
	int v = (*shared_tuple_active_idx).arrayZ[index];
	if (v != -1)
	{
	  start_idx = v;
	  end_idx = start_idx + 1;
	}
      }
    }

    const F2DOT14 *start_tuple = nullptr;
    const F2DOT14 *end_tuple = nullptr;
    bool has_interm = has_intermediate ();
    if (has_interm)
    {
      start_tuple = get_start_tuple (coord_count).arrayZ;
      end_tuple = get_end_tuple (coord_count).arrayZ;
    }

    float scalar = 1.f;
    for (unsigned int i = start_idx; i < end_idx; i++)
    {
      int peak = peak_tuple[i].to_int ();
      if (!peak) continue;

      int v = coords[i];
      if (v == peak) continue;

      if (has_interm)
      {
        int start = start_tuple[i].to_int ();
        int end = end_tuple[i].to_int ();
        if (unlikely (start > peak || peak > end ||
                      (start < 0 && end > 0 && peak))) continue;
        if (v < start || v > end) return 0.f;
        if (v < peak)
        { if (peak != start) scalar *= (float) (v - start) / (peak - start); }
        else
        { if (peak != end) scalar *= (float) (end - v) / (end - peak); }
      }
      else if (!v || v < hb_min (0, peak) || v > hb_max (0, peak)) return 0.f;
      else
        scalar *= (float) v / peak;
    }
    return scalar;
  }

  bool           has_peak () const { return tupleIndex & TuppleIndex::EmbeddedPeakTuple; }
  bool   has_intermediate () const { return tupleIndex & TuppleIndex::IntermediateRegion; }
  bool has_private_points () const { return tupleIndex & TuppleIndex::PrivatePointNumbers; }
  unsigned      get_index () const { return tupleIndex & TuppleIndex::TupleIndexMask; }

  protected:
  struct TuppleIndex : HBUINT16
  {
    enum Flags {
      EmbeddedPeakTuple   = 0x8000u,
      IntermediateRegion  = 0x4000u,
      PrivatePointNumbers = 0x2000u,
      TupleIndexMask      = 0x0FFFu
    };

    DEFINE_SIZE_STATIC (2);
  };

  hb_array_t<const F2DOT14> get_all_tuples (unsigned axis_count) const
  { return StructAfter<UnsizedArrayOf<F2DOT14>> (tupleIndex).as_array ((has_peak () + has_intermediate () * 2) * axis_count); }
  hb_array_t<const F2DOT14> get_peak_tuple (unsigned axis_count) const
  { return get_all_tuples (axis_count).sub_array (0, axis_count); }
  hb_array_t<const F2DOT14> get_start_tuple (unsigned axis_count) const
  { return get_all_tuples (axis_count).sub_array (has_peak () * axis_count, axis_count); }
  hb_array_t<const F2DOT14> get_end_tuple (unsigned axis_count) const
  { return get_all_tuples (axis_count).sub_array (has_peak () * axis_count + axis_count, axis_count); }

  HBUINT16      varDataSize;    /* The size in bytes of the serialized
                                 * data for this tuple variation table. */
  TuppleIndex   tupleIndex;     /* A packed field. The high 4 bits are flags (see below).
                                   The low 12 bits are an index into a shared tuple
                                   records array. */
  /* UnsizedArrayOf<F2DOT14> peakTuple - optional */
                                /* Peak tuple record for this tuple variation table — optional,
                                 * determined by flags in the tupleIndex value.
                                 *
                                 * Note that this must always be included in the 'cvar' table. */
  /* UnsizedArrayOf<F2DOT14> intermediateStartTuple - optional */
                                /* Intermediate start tuple record for this tuple variation table — optional,
                                   determined by flags in the tupleIndex value. */
  /* UnsizedArrayOf<F2DOT14> intermediateEndTuple - optional */
                                /* Intermediate end tuple record for this tuple variation table — optional,
                                 * determined by flags in the tupleIndex value. */
  public:
  DEFINE_SIZE_MIN (4);
};

struct tuple_delta_t
{
  public:
  hb_hashmap_t<hb_tag_t, Triple> axis_tuples;

  /* indices_length = point_count, indice[i] = 1 means point i is referenced */
  hb_vector_t<bool> indices;
  
  hb_vector_t<float> deltas_x;
  /* empty for cvar tuples */
  hb_vector_t<float> deltas_y;

  tuple_delta_t () = default;
  tuple_delta_t (const tuple_delta_t& o) = default;

  tuple_delta_t (tuple_delta_t&& o) : tuple_delta_t ()
  {
    axis_tuples = std::move (o.axis_tuples);
    indices = std::move (o.indices);
    deltas_x = std::move (o.deltas_x);
    deltas_y = std::move (o.deltas_y);
  }

  tuple_delta_t& operator = (tuple_delta_t&& o)
  {
    hb_swap (*this, o);
    return *this;
  }

  void remove_axis (hb_tag_t axis_tag)
  { axis_tuples.del (axis_tag); }

  bool set_tent (hb_tag_t axis_tag, Triple tent)
  { return axis_tuples.set (axis_tag, tent); }

  tuple_delta_t& operator += (const tuple_delta_t& o)
  {
    unsigned num = indices.length;
    for (unsigned i = 0; i < num; i++)
    {
      if (indices.arrayZ[i])
      {
        if (o.indices.arrayZ[i])
        {
          deltas_x[i] += o.deltas_x[i];
          if (deltas_y && o.deltas_y)
            deltas_y[i] += o.deltas_y[i];
        }
      }
      else
      {
        if (!o.indices.arrayZ[i]) continue;
        deltas_x[i] = o.deltas_x[i];
        if (deltas_y && o.deltas_y)
          deltas_y[i] = o.deltas_y[i];
      }
    }
    return *this;
  }

  tuple_delta_t& operator *= (float scalar)
  {
    if (scalar == 1.0f)
      return *this;

    unsigned num = indices.length;
    for (unsigned i = 0; i < num; i++)
    {
      if (!indices.arrayZ[i]) continue;

      deltas_x[i] *= scalar;
      if (deltas_y)
        deltas_y[i] *= scalar;
    }
    return *this;
  }

  hb_vector_t<tuple_delta_t> change_tuple_var_axis_limit (hb_tag_t axis_tag, Triple axis_limit) const
  {
    hb_vector_t<tuple_delta_t> out;
    Triple *tent;
    if (!axis_tuples.has (axis_tag, &tent))
    {
      out.push (*this);
      return out;
    }

    if ((tent->minimum < 0.f && tent->maximum > 0.f) ||
        !(tent->minimum <= tent->middle && tent->middle <= tent->maximum))
      return out;

    if (tent->middle == 0.f)
    {
      out.push (*this);
      return out;
    }

    result_t solutions = rebase_tent (*tent, axis_limit);
    for (auto t : solutions)
    {
      tuple_delta_t new_var = *this;
      if (t.second == Triple ())
        new_var.remove_axis (axis_tag);
      else
        new_var.set_tent (axis_tag, t.second);

      new_var *= t.first;
      out.push (std::move (new_var));
    }

    return out;
  }
};

struct TupleVariationData
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    // here check on min_size only, TupleVariationHeader and var data will be
    // checked while accessing through iterator.
    return_trace (c->check_struct (this));
  }

  unsigned get_size (unsigned axis_count) const
  {
    unsigned total_size = min_size;
    unsigned count = tupleVarCount.get_count ();
    const TupleVariationHeader *tuple_var_header = &(get_tuple_var_header());
    for (unsigned i = 0; i < count; i++)
    {
      total_size += tuple_var_header->get_size (axis_count) + tuple_var_header->get_data_size ();
      tuple_var_header = &tuple_var_header->get_next (axis_count);
    }

    return total_size;
  }

  const TupleVariationHeader &get_tuple_var_header (void) const
  { return StructAfter<TupleVariationHeader> (data); }

  struct tuple_iterator_t;
  struct tuple_variations_t
  {
    hb_vector_t<tuple_delta_t> tuple_vars;

    void fini () { tuple_vars.fini (); }
    bool create_from_tuple_var_data (tuple_iterator_t iterator,
                                     unsigned tuple_var_count,
                                     unsigned point_count,
                                     bool is_gvar,
                                     const hb_map_t *axes_old_index_tag_map,
                                     const hb_vector_t<unsigned> &shared_indices,
                                     const hb_array_t<const F2DOT14> shared_tuples)
    {
      do
      {
        const HBUINT8 *p = iterator.get_serialized_data ();
        unsigned int length = iterator.current_tuple->get_data_size ();
        if (unlikely (!iterator.var_data_bytes.check_range (p, length)))
        { fini (); return false; }

        hb_hashmap_t<hb_tag_t, Triple> axis_tuples;
        if (!iterator.current_tuple->unpack_axis_tuples (iterator.get_axis_count (), shared_tuples, axes_old_index_tag_map, axis_tuples)
            || axis_tuples.is_empty ())
        { fini (); return false; }

        hb_vector_t<unsigned> private_indices;
        bool has_private_points = iterator.current_tuple->has_private_points ();
        const HBUINT8 *end = p + length;
        if (has_private_points &&
            !TupleVariationData::unpack_points (p, private_indices, end))
        { fini (); return false; }

        const hb_vector_t<unsigned> &indices = has_private_points ? private_indices : shared_indices;
        unsigned num_deltas = indices.length;

        hb_vector_t<int> deltas_x;

        if (unlikely (!deltas_x.resize (num_deltas, false) ||
                      !TupleVariationData::unpack_deltas (p, deltas_x, end)))
        { fini (); return false; }

        hb_vector_t<int> deltas_y;
        if (is_gvar)
        {
          if (unlikely (!deltas_y.resize (num_deltas, false) ||
                        !TupleVariationData::unpack_deltas (p, deltas_y, end)))
          { fini (); return false; }
        }

        tuple_delta_t var;
        var.axis_tuples = std::move (axis_tuples);
        if (unlikely (!var.indices.resize (point_count) ||
                      !var.deltas_x.resize (point_count, false)))
        { fini (); return false; }

        if (is_gvar && unlikely (!var.deltas_y.resize (point_count, false)))
        { fini (); return false; }

        for (unsigned i = 0; i < num_deltas; i++)
        {
          unsigned idx = indices[i];
          var.indices[idx] = true;
          var.deltas_x[idx] = static_cast<float> (deltas_x[i]);
          if (is_gvar)
            var.deltas_y[idx] = static_cast<float> (deltas_y[i]);
        }
        tuple_vars.push (std::move (var));
      } while (iterator.move_to_next ());
      return true;
    }

    void change_tuple_variations_axis_limits (const hb_hashmap_t<hb_tag_t, Triple> *normalized_axes_location)
    {
      for (auto _ : *normalized_axes_location)
      {
        hb_tag_t axis_tag = _.first;
        Triple axis_limit = _.second;
        hb_vector_t<tuple_delta_t> new_vars;
        for (const tuple_delta_t& var : tuple_vars)
        {
          hb_vector_t<tuple_delta_t> out = var.change_tuple_var_axis_limit (axis_tag, axis_limit);
          if (!out) continue;
          unsigned new_len = new_vars.length + out.length;

          if (unlikely (!new_vars.alloc (new_len, false)))
          { fini (); return;}

          for (unsigned i = 0; i < out.length; i++)
            new_vars.push (std::move (out[i]));
        }
        tuple_vars.fini ();
        tuple_vars = std::move (new_vars);
      }
    }

    /* merge tuple variations with overlapping tents */
    void merge_tuple_variations ()
    {
      hb_vector_t<tuple_delta_t> new_vars;
      hb_hashmap_t<hb_hashmap_t<hb_tag_t, Triple>, unsigned> m;
      unsigned i = 0;
      for (const tuple_delta_t& var : tuple_vars)
      {
        /* if all axes are pinned, drop the tuple variation */
        if (var.axis_tuples.is_empty ()) continue;

        unsigned *idx;
        if (m.has (var.axis_tuples, &idx))
        {
          new_vars[*idx] += var;
        }
        else
        {
          new_vars.push (var);
          m.set (var.axis_tuples, i);
          i++;
        }
      }
      tuple_vars.fini ();
      tuple_vars = std::move (new_vars);
    }
  };

  struct tuple_iterator_t
  {
    unsigned get_axis_count () const { return axis_count; }

    void init (hb_bytes_t var_data_bytes_, unsigned int axis_count_, const void *table_base_)
    {
      var_data_bytes = var_data_bytes_;
      var_data = var_data_bytes_.as<TupleVariationData> ();
      index = 0;
      axis_count = axis_count_;
      current_tuple = &var_data->get_tuple_var_header ();
      data_offset = 0;
      table_base = table_base_;
    }

    bool get_shared_indices (hb_vector_t<unsigned int> &shared_indices /* OUT */)
    {
      if (var_data->has_shared_point_numbers ())
      {
        const HBUINT8 *base = &(table_base+var_data->data);
        const HBUINT8 *p = base;
        if (!unpack_points (p, shared_indices, (const HBUINT8 *) (var_data_bytes.arrayZ + var_data_bytes.length))) return false;
        data_offset = p - base;
      }
      return true;
    }

    bool is_valid () const
    {
      return (index < var_data->tupleVarCount.get_count ()) &&
             var_data_bytes.check_range (current_tuple, TupleVariationHeader::min_size) &&
             var_data_bytes.check_range (current_tuple, hb_max (current_tuple->get_data_size (),
                                                                current_tuple->get_size (axis_count)));
    }

    bool move_to_next ()
    {
      data_offset += current_tuple->get_data_size ();
      current_tuple = &current_tuple->get_next (axis_count);
      index++;
      return is_valid ();
    }

    const HBUINT8 *get_serialized_data () const
    { return &(table_base+var_data->data) + data_offset; }

    private:
    const TupleVariationData *var_data;
    unsigned int index;
    unsigned int axis_count;
    unsigned int data_offset;
    const void *table_base;

    public:
    hb_bytes_t var_data_bytes;
    const TupleVariationHeader *current_tuple;
  };

  static bool get_tuple_iterator (hb_bytes_t var_data_bytes, unsigned axis_count,
                                  const void *table_base,
                                  hb_vector_t<unsigned int> &shared_indices /* OUT */,
                                  tuple_iterator_t *iterator /* OUT */)
  {
    iterator->init (var_data_bytes, axis_count, table_base);
    if (!iterator->get_shared_indices (shared_indices))
      return false;
    return iterator->is_valid ();
  }

  bool has_shared_point_numbers () const { return tupleVarCount.has_shared_point_numbers (); }

  static bool unpack_points (const HBUINT8 *&p /* IN/OUT */,
                             hb_vector_t<unsigned int> &points /* OUT */,
                             const HBUINT8 *end)
  {
    enum packed_point_flag_t
    {
      POINTS_ARE_WORDS     = 0x80,
      POINT_RUN_COUNT_MASK = 0x7F
    };

    if (unlikely (p + 1 > end)) return false;

    unsigned count = *p++;
    if (count & POINTS_ARE_WORDS)
    {
      if (unlikely (p + 1 > end)) return false;
      count = ((count & POINT_RUN_COUNT_MASK) << 8) | *p++;
    }
    if (unlikely (!points.resize (count, false))) return false;

    unsigned n = 0;
    unsigned i = 0;
    while (i < count)
    {
      if (unlikely (p + 1 > end)) return false;
      unsigned control = *p++;
      unsigned run_count = (control & POINT_RUN_COUNT_MASK) + 1;
      unsigned stop = i + run_count;
      if (unlikely (stop > count)) return false;
      if (control & POINTS_ARE_WORDS)
      {
        if (unlikely (p + run_count * HBUINT16::static_size > end)) return false;
        for (; i < stop; i++)
        {
          n += *(const HBUINT16 *)p;
          points.arrayZ[i] = n;
          p += HBUINT16::static_size;
        }
      }
      else
      {
        if (unlikely (p + run_count > end)) return false;
        for (; i < stop; i++)
        {
          n += *p++;
          points.arrayZ[i] = n;
        }
      }
    }
    return true;
  }

  static bool unpack_deltas (const HBUINT8 *&p /* IN/OUT */,
                             hb_vector_t<int> &deltas /* IN/OUT */,
                             const HBUINT8 *end)
  {
    enum packed_delta_flag_t
    {
      DELTAS_ARE_ZERO      = 0x80,
      DELTAS_ARE_WORDS     = 0x40,
      DELTA_RUN_COUNT_MASK = 0x3F
    };

    unsigned i = 0;
    unsigned count = deltas.length;
    while (i < count)
    {
      if (unlikely (p + 1 > end)) return false;
      unsigned control = *p++;
      unsigned run_count = (control & DELTA_RUN_COUNT_MASK) + 1;
      unsigned stop = i + run_count;
      if (unlikely (stop > count)) return false;
      if (control & DELTAS_ARE_ZERO)
      {
        for (; i < stop; i++)
          deltas.arrayZ[i] = 0;
      }
      else if (control & DELTAS_ARE_WORDS)
      {
        if (unlikely (p + run_count * HBUINT16::static_size > end)) return false;
        for (; i < stop; i++)
        {
          deltas.arrayZ[i] = * (const HBINT16 *) p;
          p += HBUINT16::static_size;
        }
      }
      else
      {
        if (unlikely (p + run_count > end)) return false;
        for (; i < stop; i++)
        {
          deltas.arrayZ[i] = * (const HBINT8 *) p++;
        }
      }
    }
    return true;
  }

  bool has_data () const { return tupleVarCount; }

  bool decompile_tuple_variations (unsigned point_count,
                                   bool is_gvar,
                                   tuple_iterator_t iterator,
                                   const hb_map_t *axes_old_index_tag_map,
                                   const hb_vector_t<unsigned> &shared_indices,
                                   const hb_array_t<const F2DOT14> shared_tuples,
                                   tuple_variations_t& tuple_variations /* OUT */) const
  {
    return tuple_variations.create_from_tuple_var_data (iterator, tupleVarCount,
                                                        point_count, is_gvar,
                                                        axes_old_index_tag_map,
                                                        shared_indices,
                                                        shared_tuples);
  }

  protected:
  struct TupleVarCount : HBUINT16
  {
    bool has_shared_point_numbers () const { return ((*this) & SharedPointNumbers); }
    unsigned int get_count () const { return (*this) & CountMask; }

    protected:
    enum Flags
    {
      SharedPointNumbers= 0x8000u,
      CountMask         = 0x0FFFu
    };
    public:
    DEFINE_SIZE_STATIC (2);
  };

  TupleVarCount tupleVarCount;  /* A packed field. The high 4 bits are flags, and the
                                 * low 12 bits are the number of tuple variation tables
                                 * for this glyph. The number of tuple variation tables
                                 * can be any number between 1 and 4095. */
  Offset16To<HBUINT8>
                data;           /* Offset from the start of the base table
                                 * to the serialized data. */
  /* TupleVariationHeader tupleVariationHeaders[] *//* Array of tuple variation headers. */
  public:
  DEFINE_SIZE_MIN (4);
};

} /* namespace OT */


#endif /* HB_OT_VAR_COMMON_HH */
