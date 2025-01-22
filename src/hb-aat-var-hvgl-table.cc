#include "hb-aat-var-hvgl-table.hh"

#ifndef HB_NO_VAR_HVF

#include "hb-draw.hh"
#include "hb-geometry.hh"
#include "hb-ot-layout-common.hh"

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

using segment_t = hb_array_t<HBFLOAT64LE>;

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
PartShape::get_path_at (hb_font_t *font,
			const struct hvgl &hvgl,
		        hb_draw_session_t &draw_session,
		        hb_array_t<const int> coords,
		        hb_set_t *visited,
		        signed *edges_left,
		        signed depth_left) const
{
  const auto &blendTypes = StructAfter<decltype (blendTypesX)> (segmentCountPerPath, pathCount);

  const auto &padding = StructAfter<decltype (paddingX)> (blendTypes, segmentCount);
  const auto &coordinates = StructAfter<decltype (coordinatesX)> (padding, this);

  auto v = hb_vector_t<HBFLOAT64LE> {coordinates.get_coords (segmentCount)};

  // Apply deltas
  if (coords)
  {
    const auto &deltas = StructAfter<decltype (deltasX)> (coordinates, axisCount, segmentCount);
    for (unsigned axisIndex = 0; axisIndex < hb_min (coords.length, axisCount); axisIndex++)
    {
      signed coord = coords[axisIndex];
      if (!coord) continue;
      bool pos = coord >= 0;
      double scalar = fabs (coord / (double) (1 << 14));

      const auto delta = deltas.get_column (axisIndex * 2 + pos, axisCount, segmentCount);
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
      draw_session.move_to (font->em_fscalef_x (first_segment[SEGMENT_POINT_ON_CURVE_X]),
			    font->em_fscalef_y (double (first_segment[SEGMENT_POINT_ON_CURVE_Y])));
      for (unsigned i = start; i < end; i++)
      {
	const auto segment = v.as_array ().sub_array (i * 4, 4);
	unsigned next_i = i == end - 1 ? start : i + 1;
	auto next_segment = v.as_array ().sub_array (next_i * 4, 4);

	draw_session.quadratic_to (font->em_fscalef_x (segment[SEGMENT_POINT_OFF_CURVE_X]),
				   font->em_fscalef_y ((double) segment[SEGMENT_POINT_OFF_CURVE_Y]),
				   font->em_fscalef_x (next_segment[SEGMENT_POINT_ON_CURVE_X]),
				   font->em_fscalef_y ((double) next_segment[SEGMENT_POINT_ON_CURVE_Y]));
      }
      draw_session.close_path ();
    }

    start = end;
  }
}


void
PartComposite::get_path_at (hb_font_t *font,
			    const struct hvgl &hvgl,
			    hb_draw_session_t &draw_session,
			    hb_array_t<const int> coords,
			    hb_set_t *visited,
			    signed *edges_left,
			    signed depth_left) const
{
  const auto &subParts = StructAtOffset<SubParts> (this, subPartsOff4 * 4);

  for (const auto &subPart : subParts.as_array (subPartCount))
  {
    hvgl.get_part_path_at (font,
			   subPart.partIndex,
			   draw_session, coords, visited, edges_left, depth_left);
  }
}


} // namespace hvgl_impl
} // namespace AAT

#endif
