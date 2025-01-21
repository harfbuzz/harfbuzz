#ifndef HB_AAT_VAR_HVGL_TABLE_HH
#define HB_AAT_VAR_HVGL_TABLE_HH

#include "hb-draw.hh"
#include "hb-ot-var-common.hh"

/*
 * `hvgl` table
 */

#ifndef HB_NO_VAR_HVF

#define HB_AAT_TAG_hvgl HB_TAG('h','v','g','l')


namespace AAT {

using namespace OT;

struct hvgl;

namespace hvgl_impl {

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

  HB_INTERNAL void
  get_path_at (const struct hvgl &hvgl,
	       hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_set_t *visited,
	       signed *edges_left,
	       signed depth_left) const;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return_trace (false);
    hb_barrier ();

    if (unlikely (!segmentCountPerPath.sanitize (c, pathCount))) return_trace (false);

    const auto &blendTypes = StructAfter<decltype (blendTypesX)> (segmentCountPerPath, pathCount);
    if (unlikely (!blendTypes.sanitize (c, segmentCount))) return_trace (false);

    const auto &padding = StructAfter<decltype (paddingX)> (blendTypes, segmentCount);
    unsigned offset = (const char *) &padding - (const char *) this;

    const auto &coordinates = StructAfter<decltype (coordinatesX)> (padding, offset);
    if (unlikely (!coordinates.sanitize (c, segmentCount))) return_trace (false);

    const auto &deltas = StructAfter<decltype (deltasX)> (coordinates, axisCount, segmentCount);
    if (unlikely (!deltas.sanitize (c, axisCount, segmentCount))) return_trace (false);

    return_trace (true);
  }

  protected:
  HBUINT16LE flags; // 0x0001 for shape part
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

  public:
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

    const auto &masterRowIndex = StructAfter<decltype (masterRowIndexX)> (extremumColumnStart, axisCount);
    if (unlikely (!masterRowIndex.sanitize (c, sparseMasterAxisValueCount))) return_trace (false);

    const auto &extremumRowIndex = StructAfter<decltype (extremumRowIndexX)> (masterRowIndex, sparseMasterAxisValueCount);
    if (unlikely (!extremumRowIndex.sanitize (c, sparseExtremumAxisValueCount))) return_trace (false);

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

    const auto &extremumTranslationDelta = StructAfter<decltype (extremumTranslationDeltaX)> (masterTranslationDelta, sparseMasterTranslationCount);
    if (unlikely (!extremumTranslationDelta.sanitize (c, sparseExtremumTranslationCount))) return_trace (false);

    const auto &extremumTranslationIndex = StructAfter<decltype (extremumTranslationIndexX)> (extremumTranslationDelta, sparseExtremumTranslationCount);
    if (unlikely (!extremumTranslationIndex.sanitize (c, sparseExtremumTranslationCount))) return_trace (false);

    const auto &masterTranslationIndex = StructAfter<decltype (masterTranslationIndexX)> (extremumTranslationIndex, sparseExtremumTranslationCount);
    if (unlikely (!masterTranslationIndex.sanitize (c, sparseMasterTranslationCount))) return_trace (false);

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

    const auto &extremumRotationDelta = StructAfter<decltype (extremumRotationDeltaX)> (masterRotationDelta, sparseMasterRotationCount);
    if (unlikely (!extremumRotationDelta.sanitize (c, sparseExtremumRotationCount))) return_trace (false);

    const auto &extremumRotationIndex = StructAfter<decltype (extremumRotationIndexX)> (extremumRotationDelta, sparseExtremumRotationCount);
    if (unlikely (!extremumRotationIndex.sanitize (c, sparseExtremumRotationCount))) return_trace (false);

    const auto &masterRotationIndex = StructAfter<decltype (masterRotationIndexX)> (extremumRotationIndex, sparseExtremumRotationCount);
    if (unlikely (!masterRotationIndex.sanitize (c, sparseMasterRotationCount))) return_trace (false);

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

using Offset16LEMul4NN = Offset<HBUINT16LE, false>;

struct PartComposite
{
  public:

  HB_INTERNAL void
  get_path_at (const struct hvgl &hvgl,
	       hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_set_t *visited,
	       signed *edges_left,
	       signed depth_left) const;

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    if (unlikely (!c->check_struct (this))) return_trace (false);

    const auto &subParts = StructAtOffset<SubParts> (this, subPartsOff4 * 4);
    if (unlikely (!subParts.sanitize (c, subPartCount))) return_trace (false);

    const auto &extremumColumnStarts = StructAtOffset<ExtremumColumnStarts> (this, extremumColumnStartsOff4 * 4);
    if (unlikely (!extremumColumnStarts.sanitize (c, axisCount, sparseMasterAxisValueCount, sparseExtremumAxisValueCount))) return_trace (false);

    const auto &masterAxisValueDeltas = StructAtOffset<MasterAxisValueDeltas> (this, masterAxisValueDeltasOff4 * 4);
    if (unlikely (!masterAxisValueDeltas.sanitize (c, sparseMasterAxisValueCount))) return_trace (false);

    const auto &extremumAxisValueDeltas = StructAtOffset<ExtremumAxisValueDeltas> (this, extremumAxisValueDeltasOff4 * 4);
    if (unlikely (!extremumAxisValueDeltas.sanitize (c, sparseExtremumAxisValueCount))) return_trace (false);

    const auto &allTranslations = StructAtOffset<AllTranslations> (this, allTranslationsOff4 * 4);
    if (unlikely (!allTranslations.sanitize (c, sparseMasterTranslationCount, sparseExtremumTranslationCount))) return_trace (false);

    const auto &allRotations = StructAtOffset<AllRotations> (this, allRotationsOff4 * 4);
    if (unlikely (!allRotations.sanitize (c, sparseMasterRotationCount, sparseExtremumRotationCount))) return_trace (false);

    return_trace (true);
  }

  protected:
  HBUINT16LE flags; // 0x0001 for composite part
  HBUINT16LE axisCount; // Number of axes
  HBUINT16LE subPartCount; // Number of direct subparts
  HBUINT16LE totalNumParts; // Number of nodes including root
  HBUINT16LE totalNumAxes; // Sum of axis count for all nodes including root
  HBUINT16LE maxNumExtremes; // Maximum number of extremes (2*AxisCount) in all nodes
  HBUINT16LE sparseMasterAxisValueCount; // Count of non-zero axis value deltas for master
  HBUINT16LE sparseExtremumAxisValueCount; // Count of non-zero axis value deltas for extrema
  HBUINT16LE sparseMasterTranslationCount; // Count of non-zero translations for master
  HBUINT16LE sparseMasterRotationCount; // Count of non-zero rotations for master
  HBUINT16LE sparseExtremumTranslationCount; // Count of non-zero translations for extrema
  HBUINT16LE sparseExtremumRotationCount; // Count of non-zero rotations for extrema
  Offset16LEMul4NN/*To<SubParts>*/ subPartsOff4; // Offset to subpart array/4
  Offset16LEMul4NN/*To<ExtremumColumnStarts>*/ extremumColumnStartsOff4; // Offset to extremum column starts/4
  Offset16LEMul4NN/*To<MasterAxisValueDeltas>*/ masterAxisValueDeltasOff4; // Offset to master axis value deltas/4
  Offset16LEMul4NN/*To<ExtremumAxisValueDeltas>*/ extremumAxisValueDeltasOff4; // Offset to extremum axis value deltas/4
  Offset16LEMul4NN/*To<AllTranslations>*/ allTranslationsOff4; // Offset to all translations/4
  Offset16LEMul4NN/*To<AllRotations>*/ allRotationsOff4; // Offset to all rotations/4

  public:
  DEFINE_SIZE_STATIC (36);
};

struct Part
{
  public:

  void
  get_path_at (const struct hvgl &hvgl,
	       hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_set_t *visited,
	       signed *edges_left,
	       signed depth_left) const
  {
    switch (u.flags & 1) {
    case 0: hb_barrier(); u.shape.get_path_at (hvgl, draw_session, coords, visited, edges_left, depth_left); break;
    case 1: hb_barrier(); u.composite.get_path_at (hvgl, draw_session, coords, visited, edges_left, depth_left); break;
    }
  }

  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  {
    if (unlikely (!c->may_dispatch (this, &u.flags))) return c->no_dispatch_return_value ();
    TRACE_DISPATCH (this, u.flags);
    switch (u.flags & 1) {
    case 0: hb_barrier (); return_trace (c->dispatch (u.shape, std::forward<Ts> (ds)...));
    case 1: hb_barrier (); return_trace (c->dispatch (u.composite, std::forward<Ts> (ds)...));
    default:return_trace (c->default_return_value ());
    }
  }

  protected:
  union {
  HBUINT16LE	flags;	/* Flag identifier */
  PartShape	shape;
  PartComposite	composite;
  } u;
  public:
  // A null flags will imply PartShape. So, our null size is the size of PartShape::min_size.
  DEFINE_SIZE_MIN (PartShape::min_size);
};

struct Index
{
  public:

  hb_bytes_t get (unsigned index, unsigned count) const
  {
    if (unlikely (index >= count)) return hb_bytes_t ();
    hb_barrier ();
    unsigned offset0 = offsets[index];
    unsigned offset1 = offsets[index + 1];
    if (unlikely (offset1 < offset0 || offset1 > offsets[count]))
      return hb_bytes_t ();
    return hb_bytes_t ((const char *) this + offset0, offset1 - offset0);
  }

  unsigned int get_size (unsigned count) const { return offsets[count]; }

  bool sanitize (hb_sanitize_context_t *c, unsigned count) const
  {
    TRACE_SANITIZE (this);
    return_trace (likely (count < count + 1u &&
			  offsets.sanitize (c, count + 1) &&
			  hb_barrier () &&
			  offsets[count] >= offsets.get_size (count + 1) &&
			  c->check_range (this, offsets[count])));
  }

  protected:
  UnsizedArrayOf<HBUINT32LE>	offsets;	/* The array of (count + 1) offsets. */
  public:
  DEFINE_SIZE_MIN (4);
};

template <typename Type>
struct IndexOf : Index
{
  public:

  const Type &get (unsigned index, unsigned count) const
  {
    hb_bytes_t data = Index::get (index, count);
    hb_sanitize_context_t c (data.begin (), data.end ());
    const Type &item = *reinterpret_cast<const Type *> (data.begin ());
    c.start_processing ();
    bool sane = c.dispatch (item);
    c.end_processing ();
    if (unlikely (!sane)) return Null(Type);
    return item;
  }

  bool sanitize (hb_sanitize_context_t *c, unsigned count) const
  { return Index::sanitize (c, count); }
};

using PartsIndex = IndexOf<Part>;

} // namespace hvgl_impl

struct hvgl
{
  friend struct hvgl_impl::PartComposite;

  static constexpr hb_tag_t tableTag = HB_TAG ('h', 'v', 'g', 'l');

  protected:

  bool
  get_part_path_at (hb_codepoint_t part_id,
		    hb_draw_session_t &draw_session,
		    hb_array_t<const int> coords,
		    hb_set_t *visited,
		    signed *edges_left,
		    signed depth_left) const
  {
    if (depth_left <= 0)
      return true;

    if (*edges_left <= 0)
      return true;
    (*edges_left)--;

    if (visited->has (part_id) || visited->in_error ())
      return true;
    visited->add (part_id);

    const auto &parts = StructAtOffset<hvgl_impl::PartsIndex> (this, partsOff);
    const auto &part = parts.get (part_id, partCount);

    part.get_path_at (*this, draw_session, coords, visited, edges_left, depth_left);

    visited->del (part_id);

    return true;
  }

  public:

  bool
  get_path_at (HB_UNUSED hb_font_t *font,
	       hb_codepoint_t gid,
	       hb_draw_session_t &draw_session,
	       hb_array_t<const int> coords,
	       hb_set_t *visited = nullptr,
	       signed *edges_left = nullptr,
	       signed depth_left = HB_MAX_NESTING_LEVEL) const
  {
    if (unlikely (gid >= numGlyphs)) return false;

    hb_set_t stack_set;
    if (visited == nullptr)
      visited = &stack_set;
    signed stack_edges = HB_MAX_GRAPH_EDGE_COUNT;
    if (edges_left == nullptr)
      edges_left = &stack_edges;

    return get_part_path_at (gid, draw_session, coords, visited, edges_left, depth_left);
  }

  bool
  get_path (hb_font_t *font, hb_codepoint_t gid, hb_draw_session_t &draw_session) const
  {
    return get_path_at (font, gid, draw_session, hb_array (font->coords, font->num_coords));
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return_trace (false);
    hb_barrier ();

    const auto &parts = StructAtOffset<hvgl_impl::PartsIndex> (this, partsOff);
    if (unlikely (!parts.sanitize (c, (unsigned) partCount))) return_trace (false);

    return_trace (true);
  }

  protected:
  HBUINT16LE	versionMajor;	/* Major version of the hvgl table, currently 3 */
  HBUINT16LE	versionMinor;	/* Minor version of the hvgl table, currently 1 */
  HBUINT32LE	flags;		/* Flags; currently all zero */
  HBUINT32LE	partCount;	/* Number of all shapes and composites */
  Offset<HBUINT32LE>  partsOff;	/* To: Index. length: partCount. Parts */
  HBUINT32LE	numGlyphs;	/* Number of externally visible parts */
  HBUINT32LE	reserved;	/* Reserved; currently zero */
  public:
  DEFINE_SIZE_STATIC (24);
};

} // namespace AAT

#endif // HB_NO_VAR_HVF

#endif  /* HB_AAT_VAR_HVGL_TABLE_HH */
