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

#include <immintrin.h>

static inline void updatePointsAVXscale(const float* x, const float* y, float scalar, unsigned deltasCount, contour_point_t* points)
{
    __m256 scalarVec = _mm256_set1_ps(scalar);

    unsigned i = 0;

    // Process 8 elements at a time with AVX
    for (; i <= deltasCount - 8; i += 8)
    {
        // Load 8 elements from x and y arrays
        __m256 xVec = _mm256_loadu_ps(&x[i]);
        __m256 yVec = _mm256_loadu_ps(&y[i]);

        // Gather point x and y values
        __m256 pointXVec = _mm256_set_ps(points[i+7].x, points[i+6].x, points[i+5].x, points[i+4].x,
                                         points[i+3].x, points[i+2].x, points[i+1].x, points[i].x);
        __m256 pointYVec = _mm256_set_ps(points[i+7].y, points[i+6].y, points[i+5].y, points[i+4].y,
                                         points[i+3].y, points[i+2].y, points[i+1].y, points[i].y);

        // Multiply x and y vectors by the scalar
        xVec = _mm256_mul_ps(xVec, scalarVec);
        yVec = _mm256_mul_ps(yVec, scalarVec);

        // Add the scaled x and y to the point coordinates
        pointXVec = _mm256_add_ps(pointXVec, xVec);
        pointYVec = _mm256_add_ps(pointYVec, yVec);

        // Store the updated coordinates back to the points array
        points[i].x = _mm256_cvtss_f32(pointXVec);
        points[i].y = _mm256_cvtss_f32(pointYVec);

        points[i+1].x = _mm256_cvtss_f32(_mm256_permute_ps(pointXVec, _MM_SHUFFLE(0, 0, 0, 1)));
        points[i+1].y = _mm256_cvtss_f32(_mm256_permute_ps(pointYVec, _MM_SHUFFLE(0, 0, 0, 1)));

        points[i+2].x = _mm256_cvtss_f32(_mm256_permute_ps(pointXVec, _MM_SHUFFLE(0, 0, 0, 2)));
        points[i+2].y = _mm256_cvtss_f32(_mm256_permute_ps(pointYVec, _MM_SHUFFLE(0, 0, 0, 2)));

        points[i+3].x = _mm256_cvtss_f32(_mm256_permute_ps(pointXVec, _MM_SHUFFLE(0, 0, 0, 3)));
        points[i+3].y = _mm256_cvtss_f32(_mm256_permute_ps(pointYVec, _MM_SHUFFLE(0, 0, 0, 3)));

        points[i+4].x = _mm256_cvtss_f32(_mm256_permute_ps(pointXVec, _MM_SHUFFLE(0, 0, 0, 4)));
        points[i+4].y = _mm256_cvtss_f32(_mm256_permute_ps(pointYVec, _MM_SHUFFLE(0, 0, 0, 4)));

        points[i+5].x = _mm256_cvtss_f32(_mm256_permute_ps(pointXVec, _MM_SHUFFLE(0, 0, 0, 5)));
        points[i+5].y = _mm256_cvtss_f32(_mm256_permute_ps(pointYVec, _MM_SHUFFLE(0, 0, 0, 5)));

        points[i+6].x = _mm256_cvtss_f32(_mm256_permute_ps(pointXVec, _MM_SHUFFLE(0, 0, 0, 6)));
        points[i+6].y = _mm256_cvtss_f32(_mm256_permute_ps(pointYVec, _MM_SHUFFLE(0, 0, 0, 6)));

        points[i+7].x = _mm256_cvtss_f32(_mm256_permute_ps(pointXVec, _MM_SHUFFLE(0, 0, 0, 7)));
        points[i+7].y = _mm256_cvtss_f32(_mm256_permute_ps(pointYVec, _MM_SHUFFLE(0, 0, 0, 7)));
    }

    // Process remaining elements
    for (; i < deltasCount; i++)
    {
        points[i].x += x[i] * scalar;
        points[i].y += y[i] * scalar;
    }
}

static inline void updatePointsAVX(const float* x, const float* y, unsigned deltasCount, contour_point_t* points)
{
    unsigned i = 0;

    // Process 8 elements at a time with AVX
    for (; i <= deltasCount - 8; i += 8)
    {
        // Load 8 elements from x and y arrays
        __m256 xVec = _mm256_loadu_ps(&x[i]);
        __m256 yVec = _mm256_loadu_ps(&y[i]);

        // Gather point x and y values into AVX vectors
        __m256 pointXVec = _mm256_set_ps(points[i+7].x, points[i+6].x, points[i+5].x, points[i+4].x,
                                         points[i+3].x, points[i+2].x, points[i+1].x, points[i].x);
        __m256 pointYVec = _mm256_set_ps(points[i+7].y, points[i+6].y, points[i+5].y, points[i+4].y,
                                         points[i+3].y, points[i+2].y, points[i+1].y, points[i].y);

        // Add x and y values to the point coordinates
        pointXVec = _mm256_add_ps(pointXVec, xVec);
        pointYVec = _mm256_add_ps(pointYVec, yVec);

        // Scatter the updated coordinates back to the points array
        points[i].x = ((float*)&pointXVec)[0];
        points[i].y = ((float*)&pointYVec)[0];
        points[i+1].x = ((float*)&pointXVec)[1];
        points[i+1].y = ((float*)&pointYVec)[1];
        points[i+2].x = ((float*)&pointXVec)[2];
        points[i+2].y = ((float*)&pointYVec)[2];
        points[i+3].x = ((float*)&pointXVec)[3];
        points[i+3].y = ((float*)&pointYVec)[3];
        points[i+4].x = ((float*)&pointXVec)[4];
        points[i+4].y = ((float*)&pointYVec)[4];
        points[i+5].x = ((float*)&pointXVec)[5];
        points[i+5].y = ((float*)&pointYVec)[5];
        points[i+6].x = ((float*)&pointXVec)[6];
        points[i+6].y = ((float*)&pointYVec)[6];
        points[i+7].x = ((float*)&pointXVec)[7];
        points[i+7].y = ((float*)&pointYVec)[7];
    }

    // Process remaining elements
    for (; i < deltasCount; i++)
    {
        points[i].x += x[i];
        points[i].y += y[i];
    }
}


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
    if (scalar == 0.f)
      return;

    const float *x = deltasZ;
    const float *y = deltasZ + deltasCount;


    if (scalar == 1.f)
      updatePointsAVX(x, y, deltasCount, points.arrayZ);
    else
      updatePointsAVXscale(x, y, scalar, deltasCount, points.arrayZ);
    return;

    if (scalar == 1.f)
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
      const GlyphVariationDelta &delta = *(const GlyphVariationDelta *) p;

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
      const GlyphVariationDelta &delta = *(const GlyphVariationDelta *) p;

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
