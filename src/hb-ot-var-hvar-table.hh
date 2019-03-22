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

#ifndef HB_OT_VAR_HVAR_TABLE_HH
#define HB_OT_VAR_HVAR_TABLE_HH

#include "hb-ot-layout-common.hh"


namespace OT {


struct DeltaSetIndexMap
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  c->check_range (mapDataZ.arrayZ,
				  mapCount,
				  get_width ()));
  }

  template <typename T>
  bool serialize (hb_serialize_context_t *c, const T &plan)
  {
    unsigned int width = plan.get_width ();
    unsigned int inner_bit_count = plan.get_inner_bit_count ();
    const hb_array_t<const unsigned int> output_map = plan.get_output_map ();

    TRACE_SERIALIZE (this);
    if (unlikely (output_map.length && ((((inner_bit_count-1)&~0xF)!=0) || (((width-1)&~0x3)!=0))))
      return_trace (false);
    if (unlikely (!c->extend_min (*this))) return_trace (false);

    format.set (((width-1)<<4)|(inner_bit_count-1));
    mapCount.set (output_map.length);
    HBUINT8 *p = c->allocate_size<HBUINT8> (width * output_map.length);
    if (unlikely (!p)) return_trace (false);
    for (unsigned int i = 0; i < output_map.length; i++)
    {
      unsigned int v = output_map[i];
      unsigned int outer = v >> 16;
      unsigned int inner = v & 0xFFFF;
      unsigned int u = (outer << inner_bit_count)|inner;
      for (unsigned int w = width; w > 0;)
      {
	p[--w].set (u);
	u >>= 8;
      }
      p += width;
    }
    return_trace (true);
  }

  unsigned int map (unsigned int v) const /* Returns 16.16 outer.inner. */
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

  unsigned int get_map_count () const	    { return mapCount; }
  unsigned int get_width () const           { return ((format >> 4) & 3) + 1; }
  unsigned int get_inner_bit_count () const { return (format & 0xF) + 1; }

  protected:
  HBUINT16	format;		/* A packed field that describes the compressed
				 * representation of delta-set indices. */
  HBUINT16	mapCount;	/* The number of mapping entries. */
  UnsizedArrayOf<HBUINT8>
 		mapDataZ;	/* The delta-set index mapping data. */

  public:
  DEFINE_SIZE_ARRAY (4, mapDataZ);
};

struct index_map_subset_plan_t
{
  enum index_map_index_t {
    ADV_INDEX,
    LSB_INDEX,	/* dual as TSB */
    RSB_INDEX,	/* dual as BSB */
    VORG_INDEX
  };

  void init (const DeltaSetIndexMap &index_map,
	     unsigned int 	    im_index,
	     hb_bimap_t		    &outer_remap,
	     hb_vector_t<hb_bimap_t> &inner_remaps,
	     const hb_subset_plan_t *plan)
  {
    map_count = 0;
    outer_bit_count = 0;
    inner_bit_count = 0;
    max_inners.init ();
    output_map.init ();

    if (&index_map == &Null(DeltaSetIndexMap))
    {
      /* Advance width index map is required. If its offset is missing,
       * treat it as an indentity map. */
      if (im_index == ADV_INDEX)
      {
	outer_remap.add (0);
	hb_codepoint_t old_gid;
	for (hb_codepoint_t gid = 0; gid < plan->num_output_glyphs (); gid++)
	  if (plan->old_gid_for_new_gid (gid, &old_gid))
	    inner_remaps[0].add (old_gid);
      }
      return;
    }

    unsigned int	last_val = (unsigned int)-1;
    hb_codepoint_t	last_gid = (hb_codepoint_t)-1;
    hb_codepoint_t	gid = (hb_codepoint_t)index_map.get_map_count ();

    outer_bit_count = (index_map.get_width () * 8) - index_map.get_inner_bit_count ();
    max_inners.resize (inner_remaps.length);
    for (unsigned i = 0; i < inner_remaps.length; i++) max_inners[i] = 0;

    /* Search backwards for a map value different from the last map value */
    for (; gid > 0; gid--)
    {
      hb_codepoint_t	old_gid;
      if (!plan->old_gid_for_new_gid (gid - 1, &old_gid))
      	continue;

      unsigned int	v = index_map.map (old_gid);
      if (last_gid == (hb_codepoint_t)-1)
      {
	last_val = v;
	last_gid = gid;
	continue;
      }
      if (v != last_val) break;
  
      last_gid = gid;
    }

    map_count = last_gid;
    for (gid = 0; gid < map_count; gid++)
    {
      hb_codepoint_t	old_gid;
      if (!plan->old_gid_for_new_gid (gid, &old_gid))
      	continue;
      unsigned int v = index_map.map (old_gid);
      unsigned int outer = v >> 16;
      unsigned int inner = v & 0xFFFF;
      outer_remap.add (outer);
      if (inner > max_inners[outer]) max_inners[outer] = inner;
      inner_remaps[outer].add (inner);
    }
  }

  void fini ()
  {
    max_inners.fini ();
    output_map.fini ();
  }

  void remap (const hb_subset_plan_t *plan,
	      const DeltaSetIndexMap *input_map,
	      const hb_bimap_t &outer_remap,
	      const hb_vector_t<hb_bimap_t> &inner_remaps)
  {
    /* Leave output_map empty for an identity map */
    /* TODO: if retain_gids, convert identity to a customized map, or not subset varstore? */
    if (input_map == &Null(DeltaSetIndexMap))
      return;

    for (unsigned int i = 0; i < max_inners.length; i++)
    {
      if (inner_remaps[i].get_count () == 0) continue;
      unsigned int bit_count = (max_inners[i]==0)? 1: hb_bit_storage (inner_remaps[i][max_inners[i]]);
      if (bit_count > inner_bit_count) inner_bit_count = bit_count;
    }

    output_map.resize (map_count);
    for (hb_codepoint_t gid = 0; gid < output_map.length; gid++)
    {
      hb_codepoint_t	old_gid = 0;
      (void)plan->old_gid_for_new_gid (gid, &old_gid);
      unsigned int v = input_map->map (old_gid);
      unsigned int outer = v >> 16;
      output_map[gid] = (outer_remap[outer] << 16) | (inner_remaps[outer][v & 0xFFFF]);
    }
  }

  unsigned int get_inner_bit_count () const { return inner_bit_count; }
  unsigned int get_width () const { return ((outer_bit_count + inner_bit_count + 7) / 8); }
  unsigned int get_map_count () const { return map_count; }

  unsigned int get_size () const
  { return (map_count? (DeltaSetIndexMap::min_size + get_width () * map_count): 0); }

  bool is_identity () const { return get_output_map ().length == 0; }
  hb_array_t<const unsigned int> get_output_map () const { return output_map.as_array (); }

  protected:
  unsigned int	map_count;
  hb_vector_t<unsigned int>
  		max_inners;
  unsigned int	outer_bit_count;
  unsigned int	inner_bit_count;
  hb_vector_t<unsigned int>
		output_map;
};

struct hvarvvar_subset_plan_t
{
  hvarvvar_subset_plan_t() : inner_remaps (), index_map_plans () {}
  ~hvarvvar_subset_plan_t() { fini (); }

  void init (const hb_array_t<const DeltaSetIndexMap *> &index_maps,
	     const VariationStore &_var_store,
	     const hb_subset_plan_t *plan)
  {
    index_map_plans.resize (index_maps.length);

    var_store = &_var_store;
    inner_remaps.resize (var_store->get_sub_table_count ());

    for (unsigned int i = 0; i < inner_remaps.length; i++)
      inner_remaps[i].init ();

    for (unsigned int i = 0; i < index_maps.length; i++)
      index_map_plans[i].init (*index_maps[i], i, outer_remap, inner_remaps, plan);

    outer_remap.reorder ();
    for (unsigned int i = 0; i < inner_remaps.length; i++)
      if (inner_remaps[i].get_count () > 0) inner_remaps[i].reorder ();

    for (unsigned int i = 0; i < index_maps.length; i++)
      index_map_plans[i].remap (plan, index_maps[i], outer_remap, inner_remaps);
  }

  void fini ()
  {
    inner_remaps.fini_deep ();
    index_map_plans.fini_deep ();
  }

  hb_bimap_t			outer_remap;
  hb_vector_t<hb_bimap_t>	inner_remaps;
  hb_vector_t<index_map_subset_plan_t>
  				index_map_plans;
  const VariationStore		*var_store;
};

/*
 * HVAR -- Horizontal Metrics Variations
 * https://docs.microsoft.com/en-us/typography/opentype/spec/hvar
 * VVAR -- Vertical Metrics Variations
 * https://docs.microsoft.com/en-us/typography/opentype/spec/vvar
 */
#define HB_OT_TAG_HVAR HB_TAG('H','V','A','R')
#define HB_OT_TAG_VVAR HB_TAG('V','V','A','R')

struct HVARVVAR
{
  static constexpr hb_tag_t HVARTag = HB_OT_TAG_HVAR;
  static constexpr hb_tag_t VVARTag = HB_OT_TAG_VVAR;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
		  likely (version.major == 1) &&
		  varStore.sanitize (c, this) &&
		  advMap.sanitize (c, this) &&
		  lsbMap.sanitize (c, this) &&
		  rsbMap.sanitize (c, this));
  }

  void listup_index_maps (hb_vector_t<const DeltaSetIndexMap *> &index_maps) const
  {
    index_maps.push (&(this+advMap));
    index_maps.push (&(this+lsbMap));
    index_maps.push (&(this+rsbMap));
  }

  bool serialize_index_maps (hb_serialize_context_t *c,
			     const hb_array_t<index_map_subset_plan_t> &im_plans)
  {
    TRACE_SUBSET (this);
    if (im_plans[index_map_subset_plan_t::ADV_INDEX].is_identity ())
      advMap.set (0);
    else if (unlikely (!advMap.serialize (c, this).serialize (c, im_plans[index_map_subset_plan_t::ADV_INDEX])))
      return_trace (false);
    if (im_plans[index_map_subset_plan_t::LSB_INDEX].is_identity ())
      lsbMap.set (0);
    else if (unlikely (!lsbMap.serialize (c, this).serialize (c, im_plans[index_map_subset_plan_t::LSB_INDEX])))
      return_trace (false);
    if (im_plans[index_map_subset_plan_t::RSB_INDEX].is_identity ())
      rsbMap.set (0);
    else if (unlikely (!rsbMap.serialize (c, this).serialize (c, im_plans[index_map_subset_plan_t::RSB_INDEX])))
      return_trace (false);

    return_trace (true);
  }

  template <typename T>
  bool _subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    hvarvvar_subset_plan_t	hvar_plan;
    hb_vector_t<const DeltaSetIndexMap *>
				index_maps;

    ((T*)this)->listup_index_maps (index_maps);
    hvar_plan.init (index_maps.as_array (), this+varStore, c->plan);

    T *out = c->serializer->allocate_min<T> ();
    if (unlikely (!out)) return_trace (false);

    out->version.major.set (1);
    out->version.minor.set (0);

    if (!unlikely (out->varStore.serialize (c->serializer, out)
		     .serialize (c->serializer, hvar_plan.var_store, hvar_plan.inner_remaps.as_array ())))
      return_trace (false);

    return_trace (out->T::serialize_index_maps (c->serializer,
						hvar_plan.index_map_plans.as_array ()));
  }

  float get_advance_var (hb_codepoint_t glyph,
			 const int *coords, unsigned int coord_count) const
  {
    unsigned int varidx = (this+advMap).map (glyph);
    return (this+varStore).get_delta (varidx, coords, coord_count);
  }

  float get_side_bearing_var (hb_codepoint_t glyph,
			      const int *coords, unsigned int coord_count) const
  {
    if (!has_side_bearing_deltas ()) return 0.f;
    unsigned int varidx = (this+lsbMap).map (glyph);
    return (this+varStore).get_delta (varidx, coords, coord_count);
  }

  bool has_side_bearing_deltas () const { return lsbMap && rsbMap; }

  protected:
  FixedVersion<>version;	/* Version of the metrics variation table
				 * initially set to 0x00010000u */
  LOffsetTo<VariationStore>
		varStore;	/* Offset to item variation store table. */
  LOffsetTo<DeltaSetIndexMap>
		advMap;		/* Offset to advance var-idx mapping. */
  LOffsetTo<DeltaSetIndexMap>
		lsbMap;		/* Offset to lsb/tsb var-idx mapping. */
  LOffsetTo<DeltaSetIndexMap>
		rsbMap;		/* Offset to rsb/bsb var-idx mapping. */

  public:
  DEFINE_SIZE_STATIC (20);
};

struct HVAR : HVARVVAR {
  static constexpr hb_tag_t tableTag = HB_OT_TAG_HVAR;
  bool subset (hb_subset_context_t *c) const { return HVARVVAR::_subset<HVAR> (c); }
};
struct VVAR : HVARVVAR {
  static constexpr hb_tag_t tableTag = HB_OT_TAG_VVAR;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (static_cast<const HVARVVAR *> (this)->sanitize (c) &&
		  vorgMap.sanitize (c, this));
  }

  void listup_index_maps (hb_vector_t<const DeltaSetIndexMap *> &index_maps) const
  {
    HVARVVAR::listup_index_maps (index_maps);
    index_maps.push (&(this+vorgMap));
  }

  bool serialize_index_maps (hb_serialize_context_t *c,
			     const hb_array_t<index_map_subset_plan_t> &im_plans)
  {
    TRACE_SUBSET (this);
    if (unlikely (!HVARVVAR::serialize_index_maps (c, im_plans)))
      return_trace (false);
    if (!im_plans[index_map_subset_plan_t::VORG_INDEX].get_map_count ())
      vorgMap.set (0);
    else if (unlikely (!vorgMap.serialize (c, this).serialize (c, im_plans[index_map_subset_plan_t::VORG_INDEX])))
      return_trace (false);

    return_trace (true);
  }

  bool subset (hb_subset_context_t *c) const { return HVARVVAR::_subset<VVAR> (c); }

  protected:
  LOffsetTo<DeltaSetIndexMap>
		vorgMap;	/* Offset to vertical-origin var-idx mapping. */

  public:
  DEFINE_SIZE_STATIC (24);
};

} /* namespace OT */


#endif /* HB_OT_VAR_HVAR_TABLE_HH */
