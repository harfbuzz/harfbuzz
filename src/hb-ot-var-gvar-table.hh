/*
 * Copyright © 2019 Adobe Inc.
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

/*
 * gvar -- Glyph Variation Table
 * https://docs.microsoft.com/en-us/typography/opentype/spec/gvar
 */
#define HB_OT_TAG_gvar HB_TAG('g','v','a','r')

namespace OT {

struct Tuple : UnsizedArrayOf<F2DOT14> {};

struct TuppleIndex : HBUINT16
{
  bool has_peak () const { return ((*this) & EmbeddedPeakTuple) != 0; }
  bool has_intermediate () const { return ((*this) & IntermediateRegion) != 0; }
  bool has_private_points () const { return ((*this) & PrivatePointNumbers) != 0; }
  unsigned int get_index () const { return (*this) & TupleIndexMask; }

  protected:
  enum Flags {
    EmbeddedPeakTuple	= 0x8000u,
    IntermediateRegion	= 0x4000u,
    PrivatePointNumbers	= 0x2000u,
    TupleIndexMask	= 0x0FFFu
  };

  public:
  DEFINE_SIZE_STATIC (2);
};

struct TupleVarHeader
{
  unsigned int get_size (unsigned int axis_count) const
  {
    return min_size +
    	   (tupleIndex.has_peak ()? get_peak_tuple ().get_size (axis_count): 0) +
    	   (tupleIndex.has_intermediate ()? (get_start_tuple (axis_count).get_size (axis_count) +
					     get_end_tuple (axis_count).get_size (axis_count)): 0);
  }

  const Tuple &get_peak_tuple () const
  { return StructAfter<Tuple> (tupleIndex); }
  const Tuple &get_start_tuple (unsigned int axis_count) const
  { return StructAfter<Tuple> (get_peak_tuple ()[tupleIndex.has_peak ()? axis_count: 0]); }
  const Tuple &get_end_tuple (unsigned int axis_count) const
  { return StructAfter<Tuple> (get_peak_tuple ()[tupleIndex.has_peak ()? (axis_count * 2): 0]); }

  HBUINT16		varDataSize;
  TuppleIndex		tupleIndex;
  /* UnsizedArrayOf<F2DOT14> peakTuple - optional */
  /* UnsizedArrayOf<F2DOT14> intermediateStartTuple - optional */
  /* UnsizedArrayOf<F2DOT14> intermediateEndTuple - optional */

  DEFINE_SIZE_MIN (4);
};

struct TupleVarCount : HBUINT16
{
  bool has_shared_point_numbers () const { return ((*this) & SharedPointNumbers) != 0; }
  unsigned int get_count () const { return (*this) & CountMask; }

  protected:
  enum Flags {
    SharedPointNumbers	= 0x8000u,
    CountMask		= 0x0FFFu
  };

  public:
  DEFINE_SIZE_STATIC (2);
};

struct GlyphVarData
{
  bool check_size (unsigned int axis_count, unsigned int len) const
  { return (get_header_size (axis_count) <= len) && (data <= len); }

  unsigned int get_header_size (unsigned int axis_count) const
  {
    unsigned int size = min_size;
    for (unsigned int i = 0; i < tupleVarCount.get_count (); i++)
      size += get_tuple_var_header (axis_count, i).get_size (axis_count);	// FIX THIS O(n^2)
    
    return size;
  }

  const TupleVarHeader &get_tuple_var_header (unsigned int axis_count, unsigned int i) const
  {
    const TupleVarHeader *header = &StructAfter<TupleVarHeader>(data);
    while (i-- > 0)
      header = &StructAtOffset<TupleVarHeader> (header, header->get_size (axis_count));
    return *header;
  }

  TupleVarCount		tupleVarCount;
  OffsetTo<HBUINT8>	data;
  /* TupleVarHeader tupleVarHeaders[] */
  
  DEFINE_SIZE_MIN (4);
};

struct gvar
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_gvar;

  bool sanitize_shallow (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && (version.major == 1) &&
    		 (glyphCount == c->get_num_glyphs ()) &&
    		 c->check_array (&(this+sharedTuples), axisCount * sharedTupleCount) &&
    		 (is_long_offset ()?
		    c->check_array (get_long_offset_array (), glyphCount+1):
		    c->check_array (get_short_offset_array (), glyphCount+1)) &&
		 c->check_array (((const HBUINT8*)&(this+dataZ)) + get_offset (0),
				 get_offset (glyphCount) - get_offset (0)));
  }

  /* GlyphVarData not sanitized here; must be checked while accessing each glyph varation data */
  bool sanitize (hb_sanitize_context_t *c) const
  { return sanitize_shallow (c); }

  bool subset (hb_subset_context_t *c) const
  { return true; } // TOOD

  protected:
  const GlyphVarData *get_glyph_var_data (unsigned int gid, unsigned int *length/*OUT*/) const
  {
    unsigned int start_offset = get_offset (gid);
    unsigned int end_offset = get_offset (gid+1);

    if ((start_offset == end_offset) ||
	unlikely ((start_offset > get_offset (glyphCount)) ||
		  (start_offset + GlyphVarData::min_size > end_offset)))
    {
      *length = 0;
      return &Null(GlyphVarData);
    }
    const GlyphVarData *var_data = &(((unsigned char *)this+start_offset)+dataZ);
    if (unlikely (!var_data->check_size (axisCount, end_offset - start_offset)))
    {
      *length = 0;
      return &Null (GlyphVarData);
    }
    *length = end_offset - start_offset;
    return var_data;
  }

  bool is_long_offset () const { return (flags & 1)!=0; }

  unsigned int get_offset (unsigned int gid) const
  {
    if (is_long_offset ())
      return get_long_offset_array ()[gid];
    else
      return get_short_offset_array ()[gid] * 2;
  }

  const HBUINT32 *get_long_offset_array () const { return (const HBUINT32 *)&offsetZ; }
  const HBUINT16 *get_short_offset_array () const { return (const HBUINT16 *)&offsetZ; }

  protected:
  FixedVersion<>		version;		/* Version of gvar table. Set to 0x00010000u. */
  HBUINT16			axisCount;
  HBUINT16			sharedTupleCount;
  LOffsetTo<F2DOT14>		sharedTuples;		/* Actually LOffsetTo<UnsizedArrayOf<Tupple>> */
  HBUINT16			glyphCount;
  HBUINT16			flags;
  LOffsetTo<GlyphVarData>	dataZ;			/* Array of GlyphVarData */
  UnsizedArrayOf<HBUINT8>	offsetZ;		/* Array of 16-bit or 32-bit (glyphCount+1) offsets */

  public:
  DEFINE_SIZE_MIN (20);
};

} /* namespace OT */

#endif /* HB_OT_VAR_GVAR_TABLE_HH */
