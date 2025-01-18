#ifndef HB_AAT_VAR_HVGL_TABLE_HH
#define HB_AAT_VAR_HVGL_TABLE_HH

#include "hb-ot-var-common.hh"

/*
 * `hvgl` table
 */

#ifndef HB_NO_VAR_HVF

#define HB_AAT_TAG_hvgl HB_TAG('h','v','g','l')


namespace AAT {

using namespace OT;

namespace hvgl {

struct coordinates_t
{
  public:

  unsigned get_size (unsigned axis_count, unsigned segment_count) const
  { return (coords[0].static_size * 4) * segment_count; }

  hb_array_t<const HBFLOAT64LE>
  get_coords(unsigned segment_count) const
  { return hb_array (coords, segment_count * 4); }

  bool sanitize (hb_sanitize_context_t *c,
		 unsigned segment_count) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_range (coords, coords[0].static_size * 4, segment_count));
  }

  protected:
  HBFLOAT64LE coords[HB_VAR_ARRAY]; // length: SegmentCount * 4

  public:
  DEFINE_SIZE_MIN (0);
};

struct deltas_t
{
  public:

  unsigned get_size (unsigned axis_count, unsigned segment_count) const
  { return (matrix[0].static_size * 2 * 4) * axis_count * segment_count; }

  hb_array_t<const HBFLOAT64LE>
  get_column(unsigned column_index, unsigned axis_count, unsigned segment_count) const
  {
    const unsigned rows = 4 * segment_count;
    const unsigned start = column_index * rows;
    return hb_array (matrix + start, rows);
  }

  hb_array_t<const HBFLOAT64LE>
  get_matrix(unsigned axis_count, unsigned segment_count) const
  { return hb_array (matrix, (2 * 4) * axis_count * segment_count); }

  bool sanitize (hb_sanitize_context_t *c,
		 unsigned axis_count,
		 unsigned segment_count) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_range (matrix, matrix[0].static_size * 2 * 4, axis_count, segment_count));
  }

  protected:
  HBFLOAT64LE matrix[HB_VAR_ARRAY]; // column-major: AxisCount * 2 columns, DeltaSegmentCount * 4 rows

  public:
  DEFINE_SIZE_MIN (0);
};

struct PartShape
{
  public:

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return_trace (false);
    hb_barrier ();

    if (unlikely (!segmentCountPerPath.sanitize (c, pathCount))) return_trace (false);
    hb_barrier ();

    const auto &blendTypes = StructAfter<decltype (blendTypesX)> (segmentCountPerPath, pathCount);
    if (unlikely (!blendTypes.sanitize (c, segmentCount))) return_trace (false);
    hb_barrier ();

    const auto &padding = StructAfter<decltype (paddingX)> (blendTypes, segmentCount);
    unsigned offset = (const char *) &padding - (const char *) this;

    const auto &coordinates = StructAfter<decltype (coordinatesX)> (padding, offset);
    if (unlikely (!coordinates.sanitize (c, segmentCount))) return_trace (false);
    hb_barrier ();

    const auto &deltas = StructAfter<decltype (deltasX)> (coordinates, axisCount, segmentCount);
    if (unlikely (!deltas.sanitize (c, axisCount, segmentCount))) return_trace (false);
    hb_barrier ();

    return_trace (true);
  }

  protected:
  HBUINT16LE flags;
  HBUINT16LE axisCount;
  HBUINT16LE pathCount;
  HBUINT16LE segmentCount;
  UnsizedArrayOf<HBUINT16LE> segmentCountPerPath; // length: PathCount; Sizes of paths
  UnsizedArrayOf<HBUINT8> blendTypesX; // length: SegmentCount; Blend types for all segments
  Align<8> paddingX; // Pad to float64le alignment
  coordinates_t coordinatesX; // Master coordinate vector
  deltas_t deltasX; // Delta coordinate matrix
  public:
  DEFINE_SIZE_MIN (8);
};

struct SubPart
{
  public:

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  protected:
  HBUINT32LE partIndex; // Index of part that this subpart renders
  HBUINT16LE treeTransformIndex; // Row index of data in transform vector/matrix
  HBUINT16LE treeAxisIndex; // Row index of data in axis vector/matrix

  public:
  DEFINE_SIZE_STATIC (8);
};

using SubParts = UnsizedArrayOf<SubPart>; // length: subPartCount. Immediate subparts

struct ExtremumColumnStarts
{
  public:

  bool sanitize (hb_sanitize_context_t *c,
		 unsigned axisCount,
		 unsigned sparseMasterAxisValueCount,
		 unsigned sparseExtremumAxisValueCount) const
  {
    TRACE_SANITIZE (this);

    if (unlikely (!extremumColumnStart.sanitize (c, axisCount))) return_trace (false);
    hb_barrier ();

    const auto &masterRowIndex = StructAfter<decltype (masterRowIndexX)> (extremumColumnStart, axisCount);
    if (unlikely (!masterRowIndex.sanitize (c, sparseMasterAxisValueCount))) return_trace (false);
    hb_barrier ();

    const auto &extremumRowIndex = StructAfter<decltype (extremumRowIndexX)> (masterRowIndex, sparseMasterAxisValueCount);
    if (unlikely (!extremumRowIndex.sanitize (c, sparseExtremumAxisValueCount))) return_trace (false);
    hb_barrier ();

    return_trace (true);
  }

  protected:
  UnsizedArrayOf<HBUINT16LE> extremumColumnStart; // length: axisCount; Extremum column starts
  UnsizedArrayOf<HBUINT16LE> masterRowIndexX; // length: sparseMasterAxisValueCount; Master row indices
  UnsizedArrayOf<HBUINT16LE> extremumRowIndexX; // length: sparseExtremumAxisValueCount; Extremum row indices
  Align<4> paddingX; // Pad to uint32le alignment

  public:
  DEFINE_SIZE_MIN (0);
};

using MasterAxisValueDeltas = UnsizedArrayOf<HBFLOAT32LE>; // length: sparseMasterAxisValueCount. Master axis value deltas

using ExtremumAxisValueDeltas = UnsizedArrayOf<HBFLOAT32LE>; // length: sparseExtremumAxisValueCount. Extremum axis value deltas

} // namespace hvgl
} // namespace AAT

#endif // HB_NO_VAR_HVF

#endif  /* HB_AAT_VAR_HVGL_TABLE_HH */
