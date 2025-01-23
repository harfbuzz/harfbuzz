#include "hb-aat-var-hvgl-table.hh"

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
			const hb_transform_t &transform,
		        hb_set_t *visited,
		        signed *edges_left,
		        signed depth_left) const
{

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
PartComposite::get_path_at (const struct hvgl &hvgl,
			    hb_draw_session_t &draw_session,
			    hb_array_t<float> coords,
			    const hb_transform_t &transform,
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

#if 0
  const auto &allTranslations = StructAtOffset<AllTranslations> (this, allTranslationsOff4 * 4);
  const auto &allRotations = StructAtOffset<AllRotations> (this, allRotationsOff4 * 4);
#endif

  for (const auto &subPart : subParts.as_array (subPartCount))
  {
    hvgl.get_part_path_at (subPart.partIndex,
			   draw_session, coords_tail.sub_array (subPart.treeAxisIndex),
			   transform,
			   visited, edges_left, depth_left);
  }
}


} // namespace hvgl_impl
} // namespace AAT

#endif
