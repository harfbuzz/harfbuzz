/*
 * Copyright © 2024  Google LLC.
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
 * Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_VAR_HVAC_TABLE_HH
#define HB_OT_VAR_HVAC_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-var-common.hh"

/*
 * HVAC -- gvar replacement table with floats
 */
#define HB_OT_TAG_HVAC HB_TAG('H','V','A','C')

namespace OT {

struct GlyphVariationDelta
{
  unsigned get_size (unsigned deltasCount) const
  {
    return regionIndex.static_size + deltasCount * sizeof (deltasZ) * 2;
  }

  void apply_deltas_to_points (unsigned deltasCount,
			       hb_array_t<const int> coords,
			       hb_array_t<contour_point_t> points,
			       const SparseVarRegionList &varRegionList) const
  {
    float scalar = varRegionList.evaluate (regionIndex, coords, coords.length);
    if (scalar == 0)
      return;

    const float *x = deltasZ;
    const float *y = deltasZ + deltasCount;
    if (scalar == 1)
      for (unsigned i = 0; i < deltasCount; i++)
      {
        auto &point = points[i];
	point.x += x[i];
	point.y += y[i];
      }
    else
      for (unsigned i = 0; i < deltasCount; i++)
      {
	auto &point = points[i];
	point.x += x[i] * scalar;
	point.y += y[i] * scalar;
      }
  }

  bool sanitize (hb_sanitize_context_t *c, unsigned deltasCount) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  hb_barrier () &&
		  c->check_range (this, get_size (deltasCount)));
  }

  protected:
  HBUINT32 regionIndex; /* Index of the sparse variation region that applies to this delta set. */
  float deltasZ[HB_VAR_ARRAY]; /* Array of variation data; Xs first then Ys. */

  public:
  DEFINE_SIZE_MIN (4);
};

struct GlyphVariationDeltas
{
  void apply_deltas_to_points (hb_array_t<const int> coords,
			       hb_array_t<contour_point_t> points,
			       const SparseVarRegionList &varRegionList) const
  {
    const HBUINT8 *p = deltaSets;
    for (unsigned i = 0; i < deltasCount; i++)
    {
      const GlyphVariationDelta &delta = StructAtOffsetUnaligned<const GlyphVariationDelta> (p, 0);

      delta.apply_deltas_to_points (deltasCount, coords, points, varRegionList);

      p += delta.get_size (deltasCount);
    }
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this)))
      return_trace (false);

    const HBUINT8 *p = deltaSets;
    for (unsigned i = 0; i < deltasCount; i++)
    {
      const GlyphVariationDelta &delta = StructAtOffsetUnaligned<const GlyphVariationDelta> (p, 0);

      if (unlikely (!delta.sanitize (c, deltasCount)))
        return_trace (false);

      p += delta.get_size (deltasCount);
    }

    return_trace (true);
  }

  protected:
  HBUINT16 deltasCount;	/* Number of points in the variation data */
  HBUINT16 deltaSetCount; /* Number of variation delta sets */
  HBUINT8 deltaSets[HB_VAR_ARRAY]; /* Data */

  public:
  DEFINE_SIZE_ARRAY (4, deltaSets);
};

struct HVAC
{
  static constexpr hb_tag_t tableTag = HB_OT_TAG_HVAC;

  bool has_data () const { return version.major != 0; }

  void apply_deltas_to_points (hb_codepoint_t gid,
			       hb_array_t<const int> coords,
			       hb_array_t<contour_point_t> points) const
  {
    unsigned idx = (this+coverage).get_coverage (gid);
    if (idx == NOT_COVERED)
      return;

    const auto &deltaSet = (this+glyphDeltaSets[idx]);
    deltaSet.apply_deltas_to_points (coords, points, (this+varRegionList));
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  hb_barrier () &&
		  (version.major == 1) &&
		  varRegionList.sanitize (c, this) &&
		  coverage.sanitize (c, this) &&
		  glyphDeltaSets.sanitize (c, this));
  }

  protected:
  FixedVersion<>version;	/* Version number of the glyph variations table
				 * Set to 0x00010000u. */
  Offset32To<SparseVarRegionList> varRegionList;
				/* Offset from the start of this table to the array of
				 * sparse variation region lists. */

  Offset32To<Coverage> coverage; /* Offset from the start of this table to the coverage table. */
  Array32Of<Offset32To<GlyphVariationDeltas>>
		       glyphDeltaSets; /* Array of glyph variation data tables. */
  public:
  DEFINE_SIZE_ARRAY (16, glyphDeltaSets);
};

} /* namespace OT */

#endif /* HB_OT_VAR_HVAC_TABLE_HH */
