#include "hb-aat-var-hvgl-table.hh"

#include <complex>

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

using segment_t = hb_array_t<double>;

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

  auto v = hb_vector_t<double> {coordinates.get_coords (segmentCount)};

  coords = coords.sub_array (0, axisCount);
  // Apply deltas
  if (coords)
  {
    const auto &deltas = StructAfter<decltype (deltasX)> (coordinates, axisCount, segmentCount);
    for (auto _ : hb_enumerate (coords))
    {
      unsigned axis_index = _.first;
      float coord = _.second;
      bool pos = coord > 0;
      if (!coord) continue;
      double scalar = (double) fabs(coord);
      unsigned column_idx = axis_index * 2 + pos;

      const auto delta = deltas.get_column (column_idx, axisCount, segmentCount);
      unsigned count = hb_min (v.length, delta.length);
      for (unsigned i = 0; i < count; i++)
        v.arrayZ[i] += scalar * delta.arrayZ[i];
    }
  }

  // Resolve blend types, one path at a time.
  unsigned start = 0;
  for (unsigned pathSegmentCount : segmentCountPerPath.as_array (pathCount))
  {
    unsigned end = start + pathSegmentCount;

    if (unlikely (end > segmentCount))
      break;

    // Resolve blend type
    for (unsigned i = start; i < end; i++)
    {
      unsigned blendType = blendTypes[i];

      auto segment = v.as_array ().sub_array (i * 4, 4);
      unsigned prev_i = i == start ? end - 1 : i - 1;
      auto prev_segment = v.as_array ().sub_array (prev_i * 4, 4);
      unsigned next_i = i == end - 1 ? start : i + 1;
      auto next_segment = v.as_array ().sub_array (next_i * 4, 4);

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
      const auto first_segment = v.as_array ().sub_array (start * 4, 4);
      float x0 = (float) first_segment[SEGMENT_POINT_ON_CURVE_X];
      float y0 = (float) first_segment[SEGMENT_POINT_ON_CURVE_Y];
      transform.transform_point (x0, y0);
      draw_session.move_to (x0, y0);
      for (unsigned i = start; i < end; i++)
      {
	const auto segment = v.as_array ().sub_array (i * 4, 4);
	unsigned next_i = i == end - 1 ? start : i + 1;
	auto next_segment = v.as_array ().sub_array (next_i * 4, 4);

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

  auto master_translation_deltas = masterTranslationDelta.as_array (sparseMasterTranslationCount);
  auto master_translation_indices = masterTranslationIndex.as_array (sparseMasterTranslationCount);
  auto extremum_translation_deltas = extremumTranslationDelta.as_array (sparseExtremumTranslationCount);
  auto extremum_translation_indices = extremumTranslationIndex.as_array (sparseExtremumTranslationCount);

  auto master_rotation_deltas = masterRotationDelta.as_array (sparseMasterRotationCount);
  auto master_rotation_indices = masterRotationIndex.as_array (sparseMasterRotationCount);
  auto extremum_rotation_deltas = extremumRotationDelta.as_array (sparseExtremumRotationCount);
  auto extremum_rotation_indices = extremumRotationIndex.as_array (sparseExtremumRotationCount);

  for (unsigned row = 0; row < transforms.length; row++)
  {
    hb_transform_t transform;

    auto master_translation_delta = Null(TranslationDelta);
    if (row == master_translation_indices[0])
    {
      master_translation_delta = master_translation_deltas[0];
      master_translation_deltas++;
      master_translation_indices++;
    }
    auto master_rotation_delta = Null(HBFLOAT32LE);
    if (row == master_rotation_indices[0])
    {
      master_rotation_delta = master_rotation_deltas[0];
      master_rotation_deltas++;
      master_rotation_indices++;
    }
    transform.translate (master_translation_delta.x, master_translation_delta.y);
    transform.rotate (master_rotation_delta);

    unsigned translation_index_end = 0;
    while (translation_index_end < extremum_translation_indices.length &&
	   row == extremum_translation_indices[translation_index_end].row)
      translation_index_end++;

    unsigned rotation_index_end = 0;
    while (rotation_index_end < extremum_rotation_indices.length &&
	   row == extremum_rotation_indices[rotation_index_end].row)
      rotation_index_end++;

    unsigned translation_index = 0;
    unsigned rotation_index = 0;
    while (translation_index < translation_index_end ||
	   rotation_index < rotation_index_end)
    {
      auto extremum_translation_delta = Null(TranslationDelta);
      auto extremum_rotation_delta = Null(HBFLOAT32LE);

      unsigned column;
      if (translation_index < translation_index_end &&
	  (rotation_index == rotation_index_end ||
	   extremum_translation_indices[translation_index].column < extremum_rotation_indices[rotation_index].column))
        column = extremum_translation_indices[translation_index].column;
      else
	column = extremum_rotation_indices[rotation_index].column;

      if (translation_index < translation_index_end &&
	  extremum_translation_indices[translation_index].column == column)
      {
        extremum_translation_delta = extremum_translation_deltas[translation_index];
	translation_index++;
      }
      if (rotation_index < rotation_index_end &&
	  extremum_rotation_indices[rotation_index].column == column)
      {
	extremum_rotation_delta = extremum_rotation_deltas[rotation_index];
	rotation_index++;
      }

      bool pos = column & 1;
      unsigned axis_idx = column / 2;
      float coord = coords[axis_idx];
      if (!coord || (pos != (coord > 0)))
        continue;
      float scalar = fabsf (coord);
      if (!scalar)
	continue;

      hb_transform_t scaled_extremum_transform;

      std::complex<float> t {extremum_translation_delta.x, extremum_translation_delta.y};
      std::complex<float> _1_minus_e_iangle = 1.f - std::exp (std::complex<float> (0, 1) * (float) extremum_rotation_delta);
      if (_1_minus_e_iangle != 0.f)
      {
	std::complex<float> eigen = t / _1_minus_e_iangle;
	float eigen_x = eigen.real ();
	float eigen_y = eigen.imag ();
        // Scale rotation around eigen vector
	scaled_extremum_transform.translate (eigen_x, eigen_y);
	scaled_extremum_transform.rotate (extremum_rotation_delta * scalar);
	scaled_extremum_transform.translate (-eigen_x, -eigen_y);
      }
      else
      {
	// No rotation, just scale the translate
	scaled_extremum_transform.translate (extremum_translation_delta.x * scalar,
					     extremum_translation_delta.y * scalar);
      }

      transform.transform (scaled_extremum_transform);
    }
    extremum_translation_deltas += translation_index_end;;
    extremum_translation_indices += translation_index_end;
    extremum_rotation_deltas += rotation_index_end;
    extremum_rotation_indices += rotation_index_end;

    transforms[row].transform (transform);
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
    total_transform.transform (transforms_tail[subPart.treeTransformIndex]);
    transforms_tail[subPart.treeTransformIndex] = total_transform;
    hvgl.get_part_path_at (subPart.partIndex,
			   draw_session, coords_tail.sub_array (subPart.treeAxisIndex),
			   transforms_tail.sub_array (subPart.treeTransformIndex),
			   visited, edges_left, depth_left);
  }
}


} // namespace hvgl_impl
} // namespace AAT

#endif
