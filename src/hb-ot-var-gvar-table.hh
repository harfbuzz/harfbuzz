/*
 * Copyright © 2019  Adobe Inc.
 * Copyright © 2019  Ebrahim Byagowi
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
 * Adobe Author(s): Michiharu Ariza
 */

#ifndef HB_OT_VAR_GVAR_TABLE_HH
#define HB_OT_VAR_GVAR_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-var-common.hh"

/*
 * gvar -- Glyph Variation Table
 * https://docs.microsoft.com/en-us/typography/opentype/spec/gvar
 */
#define HB_OT_TAG_gvar HB_TAG('g','v','a','r')

namespace OT {

struct contour_point_t
{
  void init (float x_ = 0.f, float y_ = 0.f, bool is_end_point_ = false)
  { flag = 0; x = x_; y = y_; is_end_point = is_end_point_; }

  void translate (const contour_point_t &p) { x += p.x; y += p.y; }

  float x = 0.f;
  float y = 0.f;
  uint8_t flag = 0;
  bool is_end_point = false;
};

struct contour_point_vector_t : hb_vector_t<contour_point_t>
{
  void extend (const hb_array_t<contour_point_t> &a)
  {
    unsigned int old_len = length;
    if (unlikely (!resize (old_len + a.length, false)))
      return;
    auto arrayZ = this->arrayZ + old_len;
    unsigned count = a.length;
    hb_memcpy (arrayZ, a.arrayZ, count * sizeof (arrayZ[0]));
  }

  void transform (const float (&matrix)[4])
  {
    if (matrix[0] == 1.f && matrix[1] == 0.f &&
	matrix[2] == 0.f && matrix[3] == 1.f)
      return;
    auto arrayZ = this->arrayZ;
    unsigned count = length;
    for (unsigned i = 0; i < count; i++)
    {
      contour_point_t &p = arrayZ[i];
      float x_ = p.x * matrix[0] + p.y * matrix[2];
	   p.y = p.x * matrix[1] + p.y * matrix[3];
      p.x = x_;
    }
  }

  void translate (const contour_point_t& delta)
  {
    if (delta.x == 0.f && delta.y == 0.f)
      return;
    auto arrayZ = this->arrayZ;
    unsigned count = length;
    for (unsigned i = 0; i < count; i++)
      arrayZ[i].translate (delta);
  }
};

struct GlyphVariationData : TupleVariationData
{};

struct gvar
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_gvar;

  bool sanitize_shallow (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && (version.major == 1) &&
		  sharedTuples.sanitize (c, this, axisCount * sharedTupleCount) &&
		  (is_long_offset () ?
		     c->check_array (get_long_offset_array (), glyphCount+1) :
		     c->check_array (get_short_offset_array (), glyphCount+1)));
  }

  /* GlyphVariationData not sanitized here; must be checked while accessing each glyph variation data */
  bool sanitize (hb_sanitize_context_t *c) const
  { return sanitize_shallow (c); }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);

    gvar *out = c->serializer->allocate_min<gvar> ();
    if (unlikely (!out)) return_trace (false);

    out->version.major = 1;
    out->version.minor = 0;
    out->axisCount = axisCount;
    out->sharedTupleCount = sharedTupleCount;

    unsigned int num_glyphs = c->plan->num_output_glyphs ();
    out->glyphCount = num_glyphs;

    unsigned int subset_data_size = 0;
    for (hb_codepoint_t gid = (c->plan->flags & HB_SUBSET_FLAGS_NOTDEF_OUTLINE) ? 0 : 1;
         gid < num_glyphs;
         gid++)
    {
      hb_codepoint_t old_gid;
      if (!c->plan->old_gid_for_new_gid (gid, &old_gid)) continue;
      subset_data_size += get_glyph_var_data_bytes (c->source_blob, old_gid).length;
    }

    bool long_offset = subset_data_size & ~0xFFFFu;
    out->flags = long_offset ? 1 : 0;

    HBUINT8 *subset_offsets = c->serializer->allocate_size<HBUINT8> ((long_offset ? 4 : 2) * (num_glyphs + 1));
    if (!subset_offsets) return_trace (false);

    /* shared tuples */
    if (!sharedTupleCount || !sharedTuples)
      out->sharedTuples = 0;
    else
    {
      unsigned int shared_tuple_size = F2DOT14::static_size * axisCount * sharedTupleCount;
      F2DOT14 *tuples = c->serializer->allocate_size<F2DOT14> (shared_tuple_size);
      if (!tuples) return_trace (false);
      out->sharedTuples = (char *) tuples - (char *) out;
      hb_memcpy (tuples, this+sharedTuples, shared_tuple_size);
    }

    char *subset_data = c->serializer->allocate_size<char> (subset_data_size);
    if (!subset_data) return_trace (false);
    out->dataZ = subset_data - (char *) out;

    unsigned int glyph_offset = 0;
    for (hb_codepoint_t gid = (c->plan->flags & HB_SUBSET_FLAGS_NOTDEF_OUTLINE) ? 0 : 1;
         gid < num_glyphs;
         gid++)
    {
      hb_codepoint_t old_gid;
      hb_bytes_t var_data_bytes = c->plan->old_gid_for_new_gid (gid, &old_gid)
				? get_glyph_var_data_bytes (c->source_blob, old_gid)
				: hb_bytes_t ();

      if (long_offset)
	((HBUINT32 *) subset_offsets)[gid] = glyph_offset;
      else
	((HBUINT16 *) subset_offsets)[gid] = glyph_offset / 2;

      if (var_data_bytes.length > 0)
	hb_memcpy (subset_data, var_data_bytes.arrayZ, var_data_bytes.length);
      subset_data += var_data_bytes.length;
      glyph_offset += var_data_bytes.length;
    }
    if (long_offset)
      ((HBUINT32 *) subset_offsets)[num_glyphs] = glyph_offset;
    else
      ((HBUINT16 *) subset_offsets)[num_glyphs] = glyph_offset / 2;

    return_trace (true);
  }

  protected:
  const hb_bytes_t get_glyph_var_data_bytes (hb_blob_t *blob, hb_codepoint_t glyph) const
  {
    unsigned start_offset = get_offset (glyph);
    unsigned end_offset = get_offset (glyph+1);
    if (unlikely (end_offset < start_offset)) return hb_bytes_t ();
    unsigned length = end_offset - start_offset;
    hb_bytes_t var_data = blob->as_bytes ().sub_array (((unsigned) dataZ) + start_offset, length);
    return likely (var_data.length >= GlyphVariationData::min_size) ? var_data : hb_bytes_t ();
  }

  bool is_long_offset () const { return flags & 1; }

  unsigned get_offset (unsigned i) const
  {
    if (unlikely (i > glyphCount)) return 0;
    _hb_compiler_memory_r_barrier ();
    return is_long_offset () ? get_long_offset_array ()[i] : get_short_offset_array ()[i] * 2;
  }

  const HBUINT32 * get_long_offset_array () const { return (const HBUINT32 *) &offsetZ; }
  const HBUINT16 *get_short_offset_array () const { return (const HBUINT16 *) &offsetZ; }

  public:
  struct accelerator_t
  {
    accelerator_t (hb_face_t *face)
    { table = hb_sanitize_context_t ().reference_table<gvar> (face); }
    ~accelerator_t () { table.destroy (); }

    private:

    static float infer_delta (const hb_array_t<contour_point_t> points,
			      const hb_array_t<contour_point_t> deltas,
			      unsigned int target, unsigned int prev, unsigned int next,
			      float contour_point_t::*m)
    {
      float target_val = points.arrayZ[target].*m;
      float prev_val = points.arrayZ[prev].*m;
      float next_val = points.arrayZ[next].*m;
      float prev_delta =  deltas.arrayZ[prev].*m;
      float next_delta =  deltas.arrayZ[next].*m;

      if (prev_val == next_val)
	return (prev_delta == next_delta) ? prev_delta : 0.f;
      else if (target_val <= hb_min (prev_val, next_val))
	return (prev_val < next_val) ? prev_delta : next_delta;
      else if (target_val >= hb_max (prev_val, next_val))
	return (prev_val > next_val) ? prev_delta : next_delta;

      /* linear interpolation */
      float r = (target_val - prev_val) / (next_val - prev_val);
      return prev_delta + r * (next_delta - prev_delta);
    }

    static unsigned int next_index (unsigned int i, unsigned int start, unsigned int end)
    { return (i >= end) ? start : (i + 1); }

    public:
    bool apply_deltas_to_points (hb_codepoint_t glyph,
				 hb_array_t<int> coords,
				 const hb_array_t<contour_point_t> points) const
    {
      if (!coords) return true;

      if (unlikely (glyph >= table->glyphCount)) return true;

      hb_bytes_t var_data_bytes = table->get_glyph_var_data_bytes (table.get_blob (), glyph);
      if (!var_data_bytes.as<GlyphVariationData> ()->has_data ()) return true;
      hb_vector_t<unsigned int> shared_indices;
      GlyphVariationData::tuple_iterator_t iterator;
      if (!GlyphVariationData::get_tuple_iterator (var_data_bytes, table->axisCount,
						   var_data_bytes.arrayZ,
						   shared_indices, &iterator))
	return true; /* so isn't applied at all */

      /* Save original points for inferred delta calculation */
      contour_point_vector_t orig_points_vec;
      orig_points_vec.extend (points);
      if (unlikely (orig_points_vec.in_error ())) return false;
      auto orig_points = orig_points_vec.as_array ();

      contour_point_vector_t deltas_vec; /* flag is used to indicate referenced point */
      if (unlikely (!deltas_vec.resize (points.length, false))) return false;
      auto deltas = deltas_vec.as_array ();

      hb_vector_t<unsigned> end_points;
      for (unsigned i = 0; i < points.length; ++i)
	if (points.arrayZ[i].is_end_point)
	  end_points.push (i);

      unsigned num_coords = table->axisCount;
      hb_array_t<const F2DOT14> shared_tuples = (table+table->sharedTuples).as_array (table->sharedTupleCount * table->axisCount);

      hb_vector_t<unsigned int> private_indices;
      hb_vector_t<int> x_deltas;
      hb_vector_t<int> y_deltas;
      do
      {
	float scalar = iterator.current_tuple->calculate_scalar (coords, num_coords, shared_tuples);
	if (scalar == 0.f) continue;
	const HBUINT8 *p = iterator.get_serialized_data ();
	unsigned int length = iterator.current_tuple->get_data_size ();
	if (unlikely (!iterator.var_data_bytes.check_range (p, length)))
	  return false;

	const HBUINT8 *end = p + length;

	bool has_private_points = iterator.current_tuple->has_private_points ();
	if (has_private_points &&
	    !GlyphVariationData::unpack_points (p, private_indices, end))
	  return false;
	const hb_array_t<unsigned int> &indices = has_private_points ? private_indices : shared_indices;

	bool apply_to_all = (indices.length == 0);
	unsigned int num_deltas = apply_to_all ? points.length : indices.length;
	if (unlikely (!x_deltas.resize (num_deltas, false))) return false;
	if (unlikely (!GlyphVariationData::unpack_deltas (p, x_deltas, end))) return false;
	if (unlikely (!y_deltas.resize (num_deltas, false))) return false;
	if (unlikely (!GlyphVariationData::unpack_deltas (p, y_deltas, end))) return false;

	hb_memset (deltas.arrayZ, 0, deltas.get_size ());

	unsigned ref_points = 0;
	if (scalar != 1.0f)
	  for (unsigned int i = 0; i < num_deltas; i++)
	  {
	    unsigned int pt_index = apply_to_all ? i : indices[i];
	    if (unlikely (pt_index >= deltas.length)) continue;
	    auto &delta = deltas.arrayZ[pt_index];
	    ref_points += !delta.flag;
	    delta.flag = 1;	/* this point is referenced, i.e., explicit deltas specified */
	    delta.x += x_deltas.arrayZ[i] * scalar;
	    delta.y += y_deltas.arrayZ[i] * scalar;
	  }
	else
	  for (unsigned int i = 0; i < num_deltas; i++)
	  {
	    unsigned int pt_index = apply_to_all ? i : indices[i];
	    if (unlikely (pt_index >= deltas.length)) continue;
	    auto &delta = deltas.arrayZ[pt_index];
	    ref_points += !delta.flag;
	    delta.flag = 1;	/* this point is referenced, i.e., explicit deltas specified */
	    delta.x += x_deltas.arrayZ[i];
	    delta.y += y_deltas.arrayZ[i];
	  }

	/* infer deltas for unreferenced points */
	if (ref_points && ref_points < orig_points.length)
	{
	  unsigned start_point = 0;
	  for (unsigned c = 0; c < end_points.length; c++)
	  {
	    unsigned end_point = end_points.arrayZ[c];

	    /* Check the number of unreferenced points in a contour. If no unref points or no ref points, nothing to do. */
	    unsigned unref_count = 0;
	    for (unsigned i = start_point; i < end_point + 1; i++)
	      unref_count += deltas.arrayZ[i].flag;
	    unref_count = (end_point - start_point + 1) - unref_count;

	    unsigned j = start_point;
	    if (unref_count == 0 || unref_count > end_point - start_point)
	      goto no_more_gaps;

	    for (;;)
	    {
	      /* Locate the next gap of unreferenced points between two referenced points prev and next.
	       * Note that a gap may wrap around at left (start_point) and/or at right (end_point).
	       */
	      unsigned int prev, next, i;
	      for (;;)
	      {
		i = j;
		j = next_index (i, start_point, end_point);
		if (deltas.arrayZ[i].flag && !deltas.arrayZ[j].flag) break;
	      }
	      prev = j = i;
	      for (;;)
	      {
		i = j;
		j = next_index (i, start_point, end_point);
		if (!deltas.arrayZ[i].flag && deltas.arrayZ[j].flag) break;
	      }
	      next = j;
	      /* Infer deltas for all unref points in the gap between prev and next */
	      i = prev;
	      for (;;)
	      {
		i = next_index (i, start_point, end_point);
		if (i == next) break;
		deltas.arrayZ[i].x = infer_delta (orig_points, deltas, i, prev, next, &contour_point_t::x);
		deltas.arrayZ[i].y = infer_delta (orig_points, deltas, i, prev, next, &contour_point_t::y);
		if (--unref_count == 0) goto no_more_gaps;
	      }
	    }
	  no_more_gaps:
	    start_point = end_point + 1;
	  }
	}

	/* apply specified / inferred deltas to points */
	for (unsigned int i = 0; i < points.length; i++)
	{
	  points.arrayZ[i].x += deltas.arrayZ[i].x;
	  points.arrayZ[i].y += deltas.arrayZ[i].y;
	}
      } while (iterator.move_to_next ());

      return true;
    }

    unsigned int get_axis_count () const { return table->axisCount; }

    private:
    hb_blob_ptr_t<gvar> table;
  };

  protected:
  FixedVersion<>version;	/* Version number of the glyph variations table
				 * Set to 0x00010000u. */
  HBUINT16	axisCount;	/* The number of variation axes for this font. This must be
				 * the same number as axisCount in the 'fvar' table. */
  HBUINT16	sharedTupleCount;
				/* The number of shared tuple records. Shared tuple records
				 * can be referenced within glyph variation data tables for
				 * multiple glyphs, as opposed to other tuple records stored
				 * directly within a glyph variation data table. */
  NNOffset32To<UnsizedArrayOf<F2DOT14>>
		sharedTuples;	/* Offset from the start of this table to the shared tuple records.
				 * Array of tuple records shared across all glyph variation data tables. */
  HBUINT16	glyphCount;	/* The number of glyphs in this font. This must match the number of
				 * glyphs stored elsewhere in the font. */
  HBUINT16	flags;		/* Bit-field that gives the format of the offset array that follows.
				 * If bit 0 is clear, the offsets are uint16; if bit 0 is set, the
				 * offsets are uint32. */
  Offset32To<GlyphVariationData>
		dataZ;		/* Offset from the start of this table to the array of
				 * GlyphVariationData tables. */
  UnsizedArrayOf<HBUINT8>
		offsetZ;	/* Offsets from the start of the GlyphVariationData array
				 * to each GlyphVariationData table. */
  public:
  DEFINE_SIZE_ARRAY (20, offsetZ);
};

struct gvar_accelerator_t : gvar::accelerator_t {
  gvar_accelerator_t (hb_face_t *face) : gvar::accelerator_t (face) {}
};

} /* namespace OT */

#endif /* HB_OT_VAR_GVAR_TABLE_HH */
