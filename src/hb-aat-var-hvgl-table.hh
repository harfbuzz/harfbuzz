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

struct TranslationDelta
{
  public:

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  HBFLOAT32LE x; // Translation delta X
  HBFLOAT32LE y; // Translation delta Y

  public:
  DEFINE_SIZE_STATIC (8);
};

struct MatrixIndex
{
  public:

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  HBUINT16LE row; // Row index
  HBUINT16LE column; // Column index

  public:
  DEFINE_SIZE_STATIC (4);
};

struct AllTranslations
{
  public:

  bool sanitize (hb_sanitize_context_t *c,
		 unsigned sparseMasterTranslationCount,
		 unsigned sparseExtremumTranslationCount) const
  {
    TRACE_SANITIZE (this);

    if (unlikely (!masterTranslationDelta.sanitize (c, sparseMasterTranslationCount))) return_trace (false);
    hb_barrier ();

    const auto &extremumTranslationDelta = StructAfter<decltype (extremumTranslationDeltaX)> (masterTranslationDelta, sparseMasterTranslationCount);
    if (unlikely (!extremumTranslationDelta.sanitize (c, sparseExtremumTranslationCount))) return_trace (false);
    hb_barrier ();

    const auto &extremumTranslationIndex = StructAfter<decltype (extremumTranslationIndexX)> (extremumTranslationDelta, sparseExtremumTranslationCount);
    if (unlikely (!extremumTranslationIndex.sanitize (c, sparseExtremumTranslationCount))) return_trace (false);
    hb_barrier ();

    const auto &masterTranslationIndex = StructAfter<decltype (masterTranslationIndexX)> (extremumTranslationIndex, sparseExtremumTranslationCount);
    if (unlikely (!masterTranslationIndex.sanitize (c, sparseMasterTranslationCount))) return_trace (false);
    hb_barrier ();

    return_trace (true);
  }

  protected:
  UnsizedArrayOf<TranslationDelta> masterTranslationDelta; // length: sparseMasterTranslationCount; Master translation deltas
  UnsizedArrayOf<TranslationDelta> extremumTranslationDeltaX; // length: sparseExtremumTranslationCount; Extremum translation deltas
  UnsizedArrayOf<MatrixIndex> extremumTranslationIndexX; // length: sparseExtremumTranslationCount; Extremum translation indices
  UnsizedArrayOf<HBUINT16LE> masterTranslationIndexX; // length: sparseMasterTranslationCount; Master translation indices
  Align<4> padding; // Pad to float32le alignment

  public:
  DEFINE_SIZE_MIN (0);
};

struct AllRotations
{
  public:

  bool sanitize (hb_sanitize_context_t *c,
		 unsigned sparseMasterRotationCount,
		 unsigned sparseExtremumRotationCount) const
  {
    TRACE_SANITIZE (this);

    if (unlikely (!masterRotationDelta.sanitize (c, sparseMasterRotationCount))) return_trace (false);
    hb_barrier ();

    const auto &extremumRotationDelta = StructAfter<decltype (extremumRotationDeltaX)> (masterRotationDelta, sparseMasterRotationCount);
    if (unlikely (!extremumRotationDelta.sanitize (c, sparseExtremumRotationCount))) return_trace (false);
    hb_barrier ();

    const auto &extremumRotationIndex = StructAfter<decltype (extremumRotationIndexX)> (extremumRotationDelta, sparseExtremumRotationCount);
    if (unlikely (!extremumRotationIndex.sanitize (c, sparseExtremumRotationCount))) return_trace (false);
    hb_barrier ();

    const auto &masterRotationIndex = StructAfter<decltype (masterRotationIndexX)> (extremumRotationIndex, sparseExtremumRotationCount);
    if (unlikely (!masterRotationIndex.sanitize (c, sparseMasterRotationCount))) return_trace (false);
    hb_barrier ();

    return_trace (true);
  }

  protected:
  UnsizedArrayOf<HBFLOAT32LE> masterRotationDelta; // length: sparseMasterRotationCount; Master rotation deltas
  UnsizedArrayOf<HBFLOAT32LE> extremumRotationDeltaX; // length: sparseExtremumRotationCount; Extremum rotation deltas
  UnsizedArrayOf<MatrixIndex> extremumRotationIndexX; // length: sparseExtremumRotationCount; Extremum rotation indices
  UnsizedArrayOf<HBUINT16LE> masterRotationIndexX; // length: sparseMasterRotationCount; Master rotation indices
  Align<4> padding; // Pad to float32le alignment

  public:
  DEFINE_SIZE_MIN (0);
};

} // namespace hvgl
} // namespace AAT

#endif // HB_NO_VAR_HVF

#endif  /* HB_AAT_VAR_HVGL_TABLE_HH */
