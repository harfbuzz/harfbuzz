#include "hb-aat-var-hvgl-table.hh"

#include <complex>

#ifdef __APPLE__
// For endianness
#include <CoreFoundation/CoreFoundation.h>
#endif

#if !defined(HB_NO_APPLE_SIMD) && !(defined(__APPLE__) && \
  (!defined(MAC_OS_X_VERSION_MIN_REQUIRED) || MAC_OS_X_VERSION_MIN_REQUIRED >= 101300) \
)
#define HB_NO_APPLE_SIMD
#endif

#ifndef HB_NO_APPLE_SIMD
#include <simd/simd.h> // Apple SIMD https://developer.apple.com/documentation/accelerate/simd
#endif

#ifndef HB_NO_VAR_HVF

namespace AAT {

namespace hvgl_impl {


enum
segment_point_t
{
  SEGMENT_POINT_ON_CURVE_X = 0,
  SEGMENT_POINT_ON_CURVE_Y = 1,
  SEGMENT_POINT_OFF_CURVE_X = 2,
  SEGMENT_POINT_OFF_CURVE_Y = 3,
};

enum
blend_type_t
{
  BLEND_TYPE_CURVE = 0,
  BLEND_TYPE_CORNER = 1,
  BLEND_TYPE_TANGET = 2,
  BLEND_TYPE_TANGET_PAIR_FIRST = 3,
  BLEND_TYPE_TANGET_PAIR_SECOND = 4,
};

using segment_t = double*;

static void
project_on_curve_to_tangent (const segment_t offcurve1,
			     segment_t oncurve,
			     const segment_t offcurve2)
{
  double x = oncurve[SEGMENT_POINT_ON_CURVE_X];
  double y = oncurve[SEGMENT_POINT_ON_CURVE_Y];

  double x1 = offcurve1[SEGMENT_POINT_OFF_CURVE_X];
  double y1 = offcurve1[SEGMENT_POINT_OFF_CURVE_Y];
  double x2 = offcurve2[SEGMENT_POINT_OFF_CURVE_X];
  double y2 = offcurve2[SEGMENT_POINT_OFF_CURVE_Y];

  double dx = x2 - x1;
  double dy = y2 - y1;

  double t = (dx * (x - x1) + dy * (y - y1)) / (dx * dx + dy * dy);

  x = x1 + dx * t;
  y = y1 + dy * t;

  oncurve[SEGMENT_POINT_ON_CURVE_X] = x;
  oncurve[SEGMENT_POINT_ON_CURVE_Y] = y;
}

void
PartShape::get_path_at (const struct hvgl &hvgl,
		        hb_draw_session_t &draw_session,
		        hb_array_t<const float> coords,
			hb_array_t<hb_transform_t> transforms,
		        hb_set_t *visited,
		        signed *edges_left,
		        signed depth_left) const
{
  hb_transform_t transform = transforms[0];

  const auto &blendTypes = StructAfter<decltype (blendTypesX)> (segmentCountPerPath, pathCount);

  const auto &padding = StructAfter<decltype (paddingX)> (blendTypes, segmentCount);
  const auto &coordinates = StructAfter<decltype (coordinatesX)> (padding, this);

 // auto v = hb_vector_t<double> {coordinates.get_coords (segmentCount)};
  auto a = coordinates.get_coords (segmentCount);
  auto v = hb_vector_t<double> ();

#ifdef __BYTE_ORDER
  constexpr bool be = __BYTE_ORDER == __BIG_ENDIAN;
#elif defined(__APPLE__)
  bool be = CFByteOrderGetCurrent () == CFByteOrderBigEndian;
#else
  constexpr bool be = true; // Set to true to avoid fast-path if we can't determine endianness
#endif

  if (!be)
  {
    // Endianness matches; Faster to memcpy().
    v.resize (a.length, false, false);
    memcpy (v.arrayZ, a.arrayZ, v.length * sizeof (a[0]));
  }
  else
    v = a;

  coords = coords.sub_array (0, axisCount);
  // Apply deltas
  if (coords)
  {
    const auto &deltas = StructAfter<decltype (deltasX)> (coordinates, axisCount, segmentCount);
    unsigned axis_count = coords.length;
    unsigned axis_index = 0;

#ifndef HB_NO_APPLE_SIMD
    // APPLE SIMD

    bool src_aligned = (uintptr_t) (deltas.get_matrix (axisCount, segmentCount).arrayZ) % 8 == 0;
    // dest is always aligned.
    bool be = CFByteOrderGetCurrent () == CFByteOrderBigEndian;
    if (!be && src_aligned)
    {
      unsigned rows_count = v.length;
      double c[4];
      unsigned column_idx[4];
      while (axis_index < axis_count)
      {
	unsigned j;
	for (j = 0; j < 4; j++)
	{
	  while (axis_index < axis_count && !coords.arrayZ[axis_index])
	    axis_index++;
	  if (axis_index >= axis_count)
	  {
	    if (!j)
	      break;
	    for (; j < 4; j++)
	    {
	      c[j] = 0.;
	      column_idx[j] = 0;
	    }
	    break;
	  }
	  c[j] = (double) coords.arrayZ[axis_index];
	  column_idx[j] = axis_index * 2 + (c[j] > 0.);
	  axis_index++;
	}
	if (!j)
	  break;

	simd_double4 scalar4 = simd_abs (* (const simd_packed_double4 *) c);
	const auto delta0 = deltas.get_column (column_idx[0], axisCount, segmentCount).arrayZ;
	const auto delta1 = deltas.get_column (column_idx[1], axisCount, segmentCount).arrayZ;
	const auto delta2 = deltas.get_column (column_idx[2], axisCount, segmentCount).arrayZ;
	const auto delta3 = deltas.get_column (column_idx[3], axisCount, segmentCount).arrayZ;

	// Note: Count is always a multiple of 4, unless allocation failure
	for (unsigned i = 0; i + 4 <= rows_count; i += 4)
	{
	  const auto &src0 = * (const simd_packed_double4 *) (void *) (delta0 + i);
	  const auto &src1 = * (const simd_packed_double4 *) (void *) (delta1 + i);
	  const auto &src2 = * (const simd_packed_double4 *) (void *) (delta2 + i);
	  const auto &src3 = * (const simd_packed_double4 *) (void *) (delta3 + i);
	  const auto matrix = simd_matrix (src0, src1, src2, src3);

	  * (simd_packed_double4 *) (void *) (v.arrayZ + i) += simd_mul (matrix, scalar4);
	}
      }
    }
#endif

    for (;axis_index < axis_count; axis_index++)
    {
      float coord = coords.arrayZ[axis_index];
      if (!coord) continue;
      double scalar = (double) fabsf(coord);
      bool pos = coord > 0;
      unsigned column_idx = axis_index * 2 + pos;

      const auto delta = deltas.get_column (column_idx, axisCount, segmentCount);
      unsigned count = hb_min (v.length, delta.length);
      auto *dest = v.arrayZ;
      const auto *src = delta.arrayZ;
      unsigned i = 0;
      // This loop is really hot
      for (; i + 4 <= count; i += 4)
      {
	dest[i] += src[i] * scalar;
	dest[i + 1] += src[i + 1] * scalar;
	dest[i + 2] += src[i + 2] * scalar;
	dest[i + 3] += src[i + 3] * scalar;
      }
      for (; i < count; i++)
        dest[i] += src[i] * scalar;
    }
  }

  // Resolve blend types, one path at a time.
  unsigned start = 0;
  for (unsigned pathSegmentCount : segmentCountPerPath.as_array (pathCount))
  {
    unsigned end = start + pathSegmentCount;

    if (unlikely (end > segmentCount || end * 4 > v.length))
      break;

    // Resolve blend type
    for (unsigned i = start; i < end; i++)
    {
      unsigned blendType = blendTypes[i];

      segment_t segment = &v.arrayZ[i * 4];
      unsigned prev_i = i == start ? end - 1 : i - 1;
      segment_t prev_segment = &v.arrayZ[prev_i * 4];
      unsigned next_i = i == end - 1 ? start : i + 1;
      segment_t next_segment = &v.arrayZ[next_i * 4];

      switch (blendType)
      {
      default:
	break;

      case BLEND_TYPE_CURVE:
	{
	  double t = segment[SEGMENT_POINT_ON_CURVE_X];
	  t = hb_clamp (t, 0.0, 1.0);

	  /* Interpolate between the off-curve points */
	  double x = prev_segment[SEGMENT_POINT_OFF_CURVE_X] + (segment[SEGMENT_POINT_OFF_CURVE_X] - prev_segment[SEGMENT_POINT_OFF_CURVE_X]) * t;
	  double y = prev_segment[SEGMENT_POINT_OFF_CURVE_Y] + (segment[SEGMENT_POINT_OFF_CURVE_Y] - prev_segment[SEGMENT_POINT_OFF_CURVE_Y]) * t;

	  segment[SEGMENT_POINT_ON_CURVE_X] = x;
	  segment[SEGMENT_POINT_ON_CURVE_Y] = y;
	}
	break;

      case BLEND_TYPE_CORNER:
	break;

      case BLEND_TYPE_TANGET:
	{
	  /* Project onto the line between the off-curve point
	   * of the previous segment and the off-curve point of
	   * this segment */
	  project_on_curve_to_tangent (prev_segment, segment, segment);
	}
	break;

      case BLEND_TYPE_TANGET_PAIR_FIRST:
	  project_on_curve_to_tangent (prev_segment, segment, next_segment);
	  project_on_curve_to_tangent (prev_segment, next_segment, next_segment);
	break;
      }
    }

    // Draw
    if (likely (start != end))
    {
      const segment_t first_segment = &v.arrayZ[start * 4];
      float x0 = (float) first_segment[SEGMENT_POINT_ON_CURVE_X];
      float y0 = (float) first_segment[SEGMENT_POINT_ON_CURVE_Y];
      transform.transform_point (x0, y0);
      draw_session.move_to (x0, y0);
      for (unsigned i = start; i < end; i++)
      {
	const segment_t segment = &v.arrayZ[i * 4];
	unsigned next_i = i == end - 1 ? start : i + 1;
	const segment_t next_segment = &v.arrayZ[next_i * 4];

	float x1 = (float) segment[SEGMENT_POINT_OFF_CURVE_X];
	float y1 = (float) segment[SEGMENT_POINT_OFF_CURVE_Y];
	transform.transform_point (x1, y1);
	float x2 = (float) next_segment[SEGMENT_POINT_ON_CURVE_X];
	float y2 = (float) next_segment[SEGMENT_POINT_ON_CURVE_Y];
	transform.transform_point (x2, y2);
	draw_session.quadratic_to (x1, y1, x2, y2);
      }
      draw_session.close_path ();
    }

    start = end;
  }
}

void ExtremumColumnStarts::apply_to_coords (hb_array_t<float> out_coords,
					    hb_array_t<const float> coords,
					    unsigned axis_count,
					    hb_array_t<const HBFLOAT32LE> master_axis_value_deltas,
					    hb_array_t<const HBFLOAT32LE> extremum_axis_value_deltas) const
{
  const auto &masterRowIndex = StructAfter<decltype (masterRowIndexX)> (extremumColumnStart, 2 * axis_count + 1);
  hb_array_t<const HBUINT16LE> master_row_index = masterRowIndex.as_array (master_axis_value_deltas.length);
  for (auto pair : hb_zip (master_row_index, master_axis_value_deltas))
    out_coords[pair.first] += pair.second;

  const auto &extremumRowIndex = StructAfter<decltype (extremumRowIndexX)> (masterRowIndex, master_axis_value_deltas.length);
  hb_array_t<const HBUINT16LE> extremum_row_index = extremumRowIndex.as_array (extremum_axis_value_deltas.length);

  axis_count = hb_min (axis_count, coords.length);
  for (unsigned axis_idx = 0; axis_idx < axis_count; axis_idx++)
  {
    float coord = coords.arrayZ[axis_idx];
    if (!coord)
      continue;
    bool pos = coord > 0;
    float scalar = fabsf (coord);
    unsigned column_idx = axis_idx * 2 + pos;

    unsigned sparse_row_start = extremumColumnStart.arrayZ[column_idx];
    unsigned sparse_row_end = extremumColumnStart.arrayZ[column_idx + 1];
    sparse_row_end = hb_min (sparse_row_end, extremum_row_index.length);
    for (unsigned row_idx = sparse_row_start; row_idx < sparse_row_end; row_idx++)
    {
      unsigned row = extremum_row_index.arrayZ[row_idx];
      float delta = extremum_axis_value_deltas.arrayZ[row_idx];
      out_coords[row] += delta * scalar;
    }
  }
}

void
PartComposite::apply_transforms (hb_array_t<hb_transform_t> transforms,
				 hb_array_t<const float> coords) const
{
  const auto &allTranslations = StructAtOffset<AllTranslations> (this, allTranslationsOff4 * 4);
  const auto &allRotations = StructAtOffset<AllRotations> (this, allRotationsOff4 * 4);

  const auto &masterTranslationDelta = allTranslations.masterTranslationDelta;
  const auto &extremumTranslationDelta = StructAfter<decltype (allTranslations.extremumTranslationDeltaX)> (masterTranslationDelta, sparseMasterTranslationCount);
  const auto &extremumTranslationIndex = StructAfter<decltype (allTranslations.extremumTranslationIndexX)> (extremumTranslationDelta, sparseExtremumTranslationCount);
  const auto &masterTranslationIndex = StructAfter<decltype (allTranslations.masterTranslationIndexX)> (extremumTranslationIndex, sparseExtremumTranslationCount);

  const auto &masterRotationDelta = allRotations.masterRotationDelta;
  const auto &extremumRotationDelta = StructAfter<decltype (allRotations.extremumRotationDeltaX)> (masterRotationDelta, sparseMasterRotationCount);
  const auto &extremumRotationIndex = StructAfter<decltype (allRotations.extremumRotationIndexX)> (extremumRotationDelta, sparseExtremumRotationCount);
  const auto &masterRotationIndex = StructAfter<decltype (allRotations.masterRotationIndexX)> (extremumRotationIndex, sparseExtremumRotationCount);

  {
    const auto master_translation_indices = masterTranslationIndex.arrayZ;
    const auto master_translation_deltas = masterTranslationDelta.arrayZ;
    unsigned count = sparseMasterTranslationCount;
    for (unsigned i = 0; i < count; i++)
      transforms[master_translation_indices[i]].translate (master_translation_deltas[i].x,
							   master_translation_deltas[i].y);
  }
  {
    const auto master_rotation_indices = masterRotationIndex.arrayZ;
    const auto master_rotation_deltas = masterRotationDelta.arrayZ;
    unsigned count = sparseMasterRotationCount;
    for (unsigned i = 0; i < count; i++)
      transforms[master_rotation_indices[i]].rotate (master_rotation_deltas[i]);
  }

  auto extremum_translation_indices = extremumTranslationIndex.arrayZ;
  auto extremum_translation_deltas = extremumTranslationDelta.arrayZ;
  unsigned extremum_translation_count = sparseExtremumTranslationCount;
  auto extremum_rotation_indices = extremumRotationIndex.arrayZ;
  auto extremum_rotation_deltas = extremumRotationDelta.arrayZ;
  unsigned extremum_rotation_count = sparseExtremumRotationCount;
  while (true)
  {
    unsigned row = transforms.length;
    if (extremum_translation_count)
      row = hb_min (row, extremum_translation_indices->row);
    if (extremum_rotation_count)
      row = hb_min (row, extremum_rotation_indices->row);
    if (row == transforms.length)
      break;

    hb_transform_t &transform = transforms[row];

    auto extremum_translation_delta = Null(TranslationDelta);
    auto extremum_rotation_delta = Null(HBFLOAT32LE);

    unsigned column = 2 * axisCount;
    if (extremum_translation_count &&
	extremum_translation_indices->row == row)
      column = hb_min (column, extremum_translation_indices->column);
    if (extremum_rotation_count &&
	extremum_rotation_indices->row == row)
      column = hb_min (column, extremum_rotation_indices->column);
    if (column == 2 * axisCount)
      break;

    if (extremum_translation_count &&
	extremum_translation_indices->row == row &&
	extremum_translation_indices->column == column)
    {
      extremum_translation_delta = *extremum_translation_deltas;
      extremum_translation_count--;
      extremum_translation_indices++;
      extremum_translation_deltas++;
    }
    if (extremum_rotation_count &&
	extremum_rotation_indices->row == row &&
	extremum_rotation_indices->column == column)
    {
      extremum_rotation_delta = *extremum_rotation_deltas;
      extremum_rotation_count--;
      extremum_rotation_indices++;
      extremum_rotation_deltas++;
    }

    unsigned axis_idx = column / 2;
    float coord = coords[axis_idx];
    if (!coord)
      continue;
    bool pos = column & 1;
    if (pos != (coord > 0))
      continue;
    float scalar = fabsf (coord);

    if (extremum_rotation_delta)
    {
      std::complex<float> t {extremum_translation_delta.x, extremum_translation_delta.y};
      std::complex<float> _1_minus_e_iangle = 1.f - std::exp (std::complex<float> (0, 1) * (float) extremum_rotation_delta);
      std::complex<float> eigen = t / _1_minus_e_iangle;
      float eigen_x = eigen.real ();
      float eigen_y = eigen.imag ();
      // Scale rotation around eigen vector
      transform.translate (eigen_x, eigen_y);
      transform.rotate (extremum_rotation_delta * scalar);
      transform.translate (-eigen_x, -eigen_y);
    }
    else
    {
      // No rotation, just scale the translate
      transform.translate (extremum_translation_delta.x * scalar,
			   extremum_translation_delta.y * scalar);
    }
  }
}

void
PartComposite::get_path_at (const struct hvgl &hvgl,
			    hb_draw_session_t &draw_session,
			    hb_array_t<float> coords,
			    hb_array_t<hb_transform_t> transforms,
			    hb_set_t *visited,
			    signed *edges_left,
			    signed depth_left) const
{
  const auto &subParts = StructAtOffset<SubParts> (this, subPartsOff4 * 4);
  const auto &extremumColumnStarts = StructAtOffset<ExtremumColumnStarts> (this, extremumColumnStartsOff4 * 4);
  const auto &masterAxisValueDeltas = StructAtOffset<MasterAxisValueDeltas> (this, masterAxisValueDeltasOff4 * 4);
  hb_array_t<const HBFLOAT32LE> master_axis_value_deltas = masterAxisValueDeltas.as_array (sparseMasterAxisValueCount);
  const auto &extremumAxisValueDeltas = StructAtOffset<ExtremumAxisValueDeltas> (this, extremumAxisValueDeltasOff4 * 4);
  hb_array_t<const HBFLOAT32LE> extremum_axis_value_deltas = extremumAxisValueDeltas.as_array (sparseExtremumAxisValueCount);

  coords = coords.sub_array (0, totalNumAxes);
  auto coords_head = coords.sub_array (0, axisCount);
  auto coords_tail = coords.sub_array (axisCount);

  extremumColumnStarts.apply_to_coords (coords_tail,
					coords_head,
					axisCount,
					master_axis_value_deltas,
					extremum_axis_value_deltas);

  transforms = transforms.sub_array (0, totalNumParts);
  auto transforms_head = transforms[0];
  auto transforms_tail = transforms.sub_array (1);

  apply_transforms (transforms_tail, coords);

  for (const auto &subPart : subParts.as_array (subPartCount))
  {
    hb_transform_t total_transform (transforms_head);
    hb_transform_t &transform = transforms_tail[subPart.treeTransformIndex];
    total_transform.transform (transform);
    transform = total_transform;
    hvgl.get_part_path_at (subPart.partIndex,
			   draw_session, coords_tail.sub_array (subPart.treeAxisIndex),
			   transforms_tail.sub_array (subPart.treeTransformIndex),
			   visited, edges_left, depth_left);
  }
}


} // namespace hvgl_impl
} // namespace AAT

#endif
