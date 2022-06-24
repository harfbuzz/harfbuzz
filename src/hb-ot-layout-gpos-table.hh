/*
 * Copyright © 2007,2008,2009,2010  Red Hat, Inc.
 * Copyright © 2010,2012,2013  Google, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OT_LAYOUT_GPOS_TABLE_HH
#define HB_OT_LAYOUT_GPOS_TABLE_HH

#include "OT/Layout/GPOS/GPOS.hh"

namespace OT {

struct MarkArray;
static void Markclass_closure_and_remap_indexes (const Coverage  &mark_coverage,
						 const MarkArray &mark_array,
						 const hb_set_t  &glyphset,
						 hb_map_t*        klass_mapping /* INOUT */);

/* buffer **position** var allocations */
#define attach_chain() var.i16[0] /* glyph to which this attaches to, relative to current glyphs; negative for going back, positive for forward. */
#define attach_type() var.u8[2] /* attachment type */
/* Note! if attach_chain() is zero, the value of attach_type() is irrelevant. */

enum attach_type_t {
  ATTACH_TYPE_NONE	= 0X00,

  /* Each attachment should be either a mark or a cursive; can't be both. */
  ATTACH_TYPE_MARK	= 0X01,
  ATTACH_TYPE_CURSIVE	= 0X02,
};


/* Shared Tables: ValueRecord, Anchor Table, and MarkArray */

struct AnchorFormat1
{
  void get_anchor (hb_ot_apply_context_t *c, hb_codepoint_t glyph_id HB_UNUSED,
		   float *x, float *y) const
  {
    hb_font_t *font = c->font;
    *x = font->em_fscale_x (xCoordinate);
    *y = font->em_fscale_y (yCoordinate);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  AnchorFormat1* copy (hb_serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);
    AnchorFormat1* out = c->embed<AnchorFormat1> (this);
    if (!out) return_trace (out);
    out->format = 1;
    return_trace (out);
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 1 */
  FWORD		xCoordinate;		/* Horizontal value--in design units */
  FWORD		yCoordinate;		/* Vertical value--in design units */
  public:
  DEFINE_SIZE_STATIC (6);
};

struct AnchorFormat2
{
  void get_anchor (hb_ot_apply_context_t *c, hb_codepoint_t glyph_id,
		   float *x, float *y) const
  {
    hb_font_t *font = c->font;

#ifdef HB_NO_HINTING
    *x = font->em_fscale_x (xCoordinate);
    *y = font->em_fscale_y (yCoordinate);
    return;
#endif

    unsigned int x_ppem = font->x_ppem;
    unsigned int y_ppem = font->y_ppem;
    hb_position_t cx = 0, cy = 0;
    bool ret;

    ret = (x_ppem || y_ppem) &&
	  font->get_glyph_contour_point_for_origin (glyph_id, anchorPoint, HB_DIRECTION_LTR, &cx, &cy);
    *x = ret && x_ppem ? cx : font->em_fscale_x (xCoordinate);
    *y = ret && y_ppem ? cy : font->em_fscale_y (yCoordinate);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  AnchorFormat2* copy (hb_serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);
    return_trace (c->embed<AnchorFormat2> (this));
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 2 */
  FWORD		xCoordinate;		/* Horizontal value--in design units */
  FWORD		yCoordinate;		/* Vertical value--in design units */
  HBUINT16	anchorPoint;		/* Index to glyph contour point */
  public:
  DEFINE_SIZE_STATIC (8);
};

struct AnchorFormat3
{
  void get_anchor (hb_ot_apply_context_t *c, hb_codepoint_t glyph_id HB_UNUSED,
		   float *x, float *y) const
  {
    hb_font_t *font = c->font;
    *x = font->em_fscale_x (xCoordinate);
    *y = font->em_fscale_y (yCoordinate);

    if (font->x_ppem || font->num_coords)
      *x += (this+xDeviceTable).get_x_delta (font, c->var_store, c->var_store_cache);
    if (font->y_ppem || font->num_coords)
      *y += (this+yDeviceTable).get_y_delta (font, c->var_store, c->var_store_cache);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && xDeviceTable.sanitize (c, this) && yDeviceTable.sanitize (c, this));
  }

  AnchorFormat3* copy (hb_serialize_context_t *c,
		       const hb_map_t *layout_variation_idx_map) const
  {
    TRACE_SERIALIZE (this);
    if (!layout_variation_idx_map) return_trace (nullptr);

    auto *out = c->embed<AnchorFormat3> (this);
    if (unlikely (!out)) return_trace (nullptr);

    out->xDeviceTable.serialize_copy (c, xDeviceTable, this, 0, hb_serialize_context_t::Head, layout_variation_idx_map);
    out->yDeviceTable.serialize_copy (c, yDeviceTable, this, 0, hb_serialize_context_t::Head, layout_variation_idx_map);
    return_trace (out);
  }

  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    (this+xDeviceTable).collect_variation_indices (c->layout_variation_indices);
    (this+yDeviceTable).collect_variation_indices (c->layout_variation_indices);
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 3 */
  FWORD		xCoordinate;		/* Horizontal value--in design units */
  FWORD		yCoordinate;		/* Vertical value--in design units */
  Offset16To<Device>
		xDeviceTable;		/* Offset to Device table for X
					 * coordinate-- from beginning of
					 * Anchor table (may be NULL) */
  Offset16To<Device>
		yDeviceTable;		/* Offset to Device table for Y
					 * coordinate-- from beginning of
					 * Anchor table (may be NULL) */
  public:
  DEFINE_SIZE_STATIC (10);
};

struct Anchor
{
  void get_anchor (hb_ot_apply_context_t *c, hb_codepoint_t glyph_id,
		   float *x, float *y) const
  {
    *x = *y = 0;
    switch (u.format) {
    case 1: u.format1.get_anchor (c, glyph_id, x, y); return;
    case 2: u.format2.get_anchor (c, glyph_id, x, y); return;
    case 3: u.format3.get_anchor (c, glyph_id, x, y); return;
    default:					      return;
    }
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (!u.format.sanitize (c)) return_trace (false);
    switch (u.format) {
    case 1: return_trace (u.format1.sanitize (c));
    case 2: return_trace (u.format2.sanitize (c));
    case 3: return_trace (u.format3.sanitize (c));
    default:return_trace (true);
    }
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    switch (u.format) {
    case 1: return_trace (bool (reinterpret_cast<Anchor *> (u.format1.copy (c->serializer))));
    case 2:
      if (c->plan->flags & HB_SUBSET_FLAGS_NO_HINTING)
      {
        // AnchorFormat 2 just containins extra hinting information, so
        // if hints are being dropped convert to format 1.
        return_trace (bool (reinterpret_cast<Anchor *> (u.format1.copy (c->serializer))));
      }
      return_trace (bool (reinterpret_cast<Anchor *> (u.format2.copy (c->serializer))));
    case 3: return_trace (bool (reinterpret_cast<Anchor *> (u.format3.copy (c->serializer,
                                                                            c->plan->layout_variation_idx_map))));
    default:return_trace (false);
    }
  }

  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    switch (u.format) {
    case 1: case 2:
      return;
    case 3:
      u.format3.collect_variation_indices (c);
      return;
    default: return;
    }
  }

  protected:
  union {
  HBUINT16		format;		/* Format identifier */
  AnchorFormat1		format1;
  AnchorFormat2		format2;
  AnchorFormat3		format3;
  } u;
  public:
  DEFINE_SIZE_UNION (2, format);
};


struct AnchorMatrix
{
  const Anchor& get_anchor (unsigned int row, unsigned int col,
			    unsigned int cols, bool *found) const
  {
    *found = false;
    if (unlikely (row >= rows || col >= cols)) return Null (Anchor);
    *found = !matrixZ[row * cols + col].is_null ();
    return this+matrixZ[row * cols + col];
  }

  template <typename Iterator,
	    hb_requires (hb_is_iterator (Iterator))>
  void collect_variation_indices (hb_collect_variation_indices_context_t *c,
				  Iterator index_iter) const
  {
    for (unsigned i : index_iter)
      (this+matrixZ[i]).collect_variation_indices (c);
  }

  template <typename Iterator,
      hb_requires (hb_is_iterator (Iterator))>
  bool subset (hb_subset_context_t *c,
               unsigned             num_rows,
               Iterator             index_iter) const
  {
    TRACE_SUBSET (this);

    auto *out = c->serializer->start_embed (this);

    if (!index_iter) return_trace (false);
    if (unlikely (!c->serializer->extend_min (out)))  return_trace (false);

    out->rows = num_rows;
    for (const unsigned i : index_iter)
    {
      auto *offset = c->serializer->embed (matrixZ[i]);
      if (!offset) return_trace (false);
      offset->serialize_subset (c, matrixZ[i], this);
    }

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c, unsigned int cols) const
  {
    TRACE_SANITIZE (this);
    if (!c->check_struct (this)) return_trace (false);
    if (unlikely (hb_unsigned_mul_overflows (rows, cols))) return_trace (false);
    unsigned int count = rows * cols;
    if (!c->check_array (matrixZ.arrayZ, count)) return_trace (false);
    for (unsigned int i = 0; i < count; i++)
      if (!matrixZ[i].sanitize (c, this)) return_trace (false);
    return_trace (true);
  }

  HBUINT16	rows;			/* Number of rows */
  UnsizedArrayOf<Offset16To<Anchor>>
		matrixZ;		/* Matrix of offsets to Anchor tables--
					 * from beginning of AnchorMatrix table */
  public:
  DEFINE_SIZE_ARRAY (2, matrixZ);
};


struct MarkRecord
{
  friend struct MarkArray;

  unsigned get_class () const { return (unsigned) klass; }
  bool sanitize (hb_sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && markAnchor.sanitize (c, base));
  }

  MarkRecord *subset (hb_subset_context_t    *c,
                      const void             *src_base,
                      const hb_map_t         *klass_mapping) const
  {
    TRACE_SUBSET (this);
    auto *out = c->serializer->embed (this);
    if (unlikely (!out)) return_trace (nullptr);

    out->klass = klass_mapping->get (klass);
    out->markAnchor.serialize_subset (c, markAnchor, src_base);
    return_trace (out);
  }

  void collect_variation_indices (hb_collect_variation_indices_context_t *c,
				  const void *src_base) const
  {
    (src_base+markAnchor).collect_variation_indices (c);
  }

  protected:
  HBUINT16	klass;			/* Class defined for this mark */
  Offset16To<Anchor>
		markAnchor;		/* Offset to Anchor table--from
					 * beginning of MarkArray table */
  public:
  DEFINE_SIZE_STATIC (4);
};

struct MarkArray : Array16Of<MarkRecord>	/* Array of MarkRecords--in Coverage order */
{
  bool apply (hb_ot_apply_context_t *c,
	      unsigned int mark_index, unsigned int glyph_index,
	      const AnchorMatrix &anchors, unsigned int class_count,
	      unsigned int glyph_pos) const
  {
    TRACE_APPLY (this);
    hb_buffer_t *buffer = c->buffer;
    const MarkRecord &record = Array16Of<MarkRecord>::operator[](mark_index);
    unsigned int mark_class = record.klass;

    const Anchor& mark_anchor = this + record.markAnchor;
    bool found;
    const Anchor& glyph_anchor = anchors.get_anchor (glyph_index, mark_class, class_count, &found);
    /* If this subtable doesn't have an anchor for this base and this class,
     * return false such that the subsequent subtables have a chance at it. */
    if (unlikely (!found)) return_trace (false);

    float mark_x, mark_y, base_x, base_y;

    buffer->unsafe_to_break (glyph_pos, buffer->idx + 1);
    mark_anchor.get_anchor (c, buffer->cur().codepoint, &mark_x, &mark_y);
    glyph_anchor.get_anchor (c, buffer->info[glyph_pos].codepoint, &base_x, &base_y);

    hb_glyph_position_t &o = buffer->cur_pos();
    o.x_offset = roundf (base_x - mark_x);
    o.y_offset = roundf (base_y - mark_y);
    o.attach_type() = ATTACH_TYPE_MARK;
    o.attach_chain() = (int) glyph_pos - (int) buffer->idx;
    buffer->scratch_flags |= HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT;

    buffer->idx++;
    return_trace (true);
  }

  template <typename Iterator,
      hb_requires (hb_is_iterator (Iterator))>
  bool subset (hb_subset_context_t *c,
               Iterator		    coverage,
               const hb_map_t      *klass_mapping) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();

    auto* out = c->serializer->start_embed (this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);

    auto mark_iter =
    + hb_zip (coverage, this->iter ())
    | hb_filter (glyphset, hb_first)
    | hb_map (hb_second)
    ;

    unsigned new_length = 0;
    for (const auto& mark_record : mark_iter) {
      if (unlikely (!mark_record.subset (c, this, klass_mapping)))
        return_trace (false);
      new_length++;
    }

    if (unlikely (!c->serializer->check_assign (out->len, new_length,
                                                HB_SERIALIZE_ERROR_ARRAY_OVERFLOW)))
      return_trace (false);

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (Array16Of<MarkRecord>::sanitize (c, this));
  }
};


/* Lookups */

typedef AnchorMatrix BaseArray;		/* base-major--
					 * in order of BaseCoverage Index--,
					 * mark-minor--
					 * ordered by class--zero-based. */

static void Markclass_closure_and_remap_indexes (const Coverage  &mark_coverage,
						 const MarkArray &mark_array,
						 const hb_set_t  &glyphset,
						 hb_map_t*        klass_mapping /* INOUT */)
{
  hb_set_t orig_classes;

  + hb_zip (mark_coverage, mark_array)
  | hb_filter (glyphset, hb_first)
  | hb_map (hb_second)
  | hb_map (&MarkRecord::get_class)
  | hb_sink (orig_classes)
  ;

  unsigned idx = 0;
  for (auto klass : orig_classes.iter ())
  {
    if (klass_mapping->has (klass)) continue;
    klass_mapping->set (klass, idx);
    idx++;
  }
}

struct MarkBasePosFormat1
{
  bool intersects (const hb_set_t *glyphs) const
  {
    return (this+markCoverage).intersects (glyphs) &&
	   (this+baseCoverage).intersects (glyphs);
  }

  void closure_lookups (hb_closure_lookups_context_t *c) const {}

  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    + hb_zip (this+markCoverage, this+markArray)
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    | hb_apply ([&] (const MarkRecord& record) { record.collect_variation_indices (c, &(this+markArray)); })
    ;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+markCoverage, this+markArray, *c->glyph_set, &klass_mapping);

    unsigned basecount = (this+baseArray).rows;
    auto base_iter =
    + hb_zip (this+baseCoverage, hb_range (basecount))
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    ;

    hb_sorted_vector_t<unsigned> base_indexes;
    for (const unsigned row : base_iter)
    {
      + hb_range ((unsigned) classCount)
      | hb_filter (klass_mapping)
      | hb_map ([&] (const unsigned col) { return row * (unsigned) classCount + col; })
      | hb_sink (base_indexes)
      ;
    }
    (this+baseArray).collect_variation_indices (c, base_indexes.iter ());
  }

  void collect_glyphs (hb_collect_glyphs_context_t *c) const
  {
    if (unlikely (!(this+markCoverage).collect_coverage (c->input))) return;
    if (unlikely (!(this+baseCoverage).collect_coverage (c->input))) return;
  }

  const Coverage &get_coverage () const { return this+markCoverage; }

  bool apply (hb_ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    hb_buffer_t *buffer = c->buffer;
    unsigned int mark_index = (this+markCoverage).get_coverage  (buffer->cur().codepoint);
    if (likely (mark_index == NOT_COVERED)) return_trace (false);

    /* Now we search backwards for a non-mark glyph */
    hb_ot_apply_context_t::skipping_iterator_t &skippy_iter = c->iter_input;
    skippy_iter.reset (buffer->idx, 1);
    skippy_iter.set_lookup_props (LookupFlag::IgnoreMarks);
    do {
      unsigned unsafe_from;
      if (!skippy_iter.prev (&unsafe_from))
      {
	buffer->unsafe_to_concat_from_outbuffer (unsafe_from, buffer->idx + 1);
	return_trace (false);
      }

      /* We only want to attach to the first of a MultipleSubst sequence.
       * https://github.com/harfbuzz/harfbuzz/issues/740
       * Reject others...
       * ...but stop if we find a mark in the MultipleSubst sequence:
       * https://github.com/harfbuzz/harfbuzz/issues/1020 */
      if (!_hb_glyph_info_multiplied (&buffer->info[skippy_iter.idx]) ||
	  0 == _hb_glyph_info_get_lig_comp (&buffer->info[skippy_iter.idx]) ||
	  (skippy_iter.idx == 0 ||
	   _hb_glyph_info_is_mark (&buffer->info[skippy_iter.idx - 1]) ||
	   _hb_glyph_info_get_lig_id (&buffer->info[skippy_iter.idx]) !=
	   _hb_glyph_info_get_lig_id (&buffer->info[skippy_iter.idx - 1]) ||
	   _hb_glyph_info_get_lig_comp (&buffer->info[skippy_iter.idx]) !=
	   _hb_glyph_info_get_lig_comp (&buffer->info[skippy_iter.idx - 1]) + 1
	   ))
	break;
      skippy_iter.reject ();
    } while (true);

    /* Checking that matched glyph is actually a base glyph by GDEF is too strong; disabled */
    //if (!_hb_glyph_info_is_base_glyph (&buffer->info[skippy_iter.idx])) { return_trace (false); }

    unsigned int base_index = (this+baseCoverage).get_coverage  (buffer->info[skippy_iter.idx].codepoint);
    if (base_index == NOT_COVERED)
    {
      buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
      return_trace (false);
    }

    return_trace ((this+markArray).apply (c, mark_index, base_index, this+baseArray, classCount, skippy_iter.idx));
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();
    const hb_map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);
    out->format = format;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+markCoverage, this+markArray, glyphset, &klass_mapping);

    if (!klass_mapping.get_population ()) return_trace (false);
    out->classCount = klass_mapping.get_population ();

    auto mark_iter =
    + hb_zip (this+markCoverage, this+markArray)
    | hb_filter (glyphset, hb_first)
    ;

    hb_sorted_vector_t<hb_codepoint_t> new_coverage;
    + mark_iter
    | hb_map (hb_first)
    | hb_map (glyph_map)
    | hb_sink (new_coverage)
    ;

    if (!out->markCoverage.serialize_serialize (c->serializer, new_coverage.iter ()))
      return_trace (false);

    out->markArray.serialize_subset (c, markArray, this,
                                     (this+markCoverage).iter (),
                                     &klass_mapping);

    unsigned basecount = (this+baseArray).rows;
    auto base_iter =
    + hb_zip (this+baseCoverage, hb_range (basecount))
    | hb_filter (glyphset, hb_first)
    ;

    new_coverage.reset ();
    + base_iter
    | hb_map (hb_first)
    | hb_map (glyph_map)
    | hb_sink (new_coverage)
    ;

    if (!out->baseCoverage.serialize_serialize (c->serializer, new_coverage.iter ()))
      return_trace (false);

    hb_sorted_vector_t<unsigned> base_indexes;
    for (const unsigned row : + base_iter
			      | hb_map (hb_second))
    {
      + hb_range ((unsigned) classCount)
      | hb_filter (klass_mapping)
      | hb_map ([&] (const unsigned col) { return row * (unsigned) classCount + col; })
      | hb_sink (base_indexes)
      ;
    }

    out->baseArray.serialize_subset (c, baseArray, this,
                                     base_iter.len (),
                                     base_indexes.iter ());

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  markCoverage.sanitize (c, this) &&
		  baseCoverage.sanitize (c, this) &&
		  markArray.sanitize (c, this) &&
		  baseArray.sanitize (c, this, (unsigned int) classCount));
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 1 */
  Offset16To<Coverage>
		markCoverage;		/* Offset to MarkCoverage table--from
					 * beginning of MarkBasePos subtable */
  Offset16To<Coverage>
		baseCoverage;		/* Offset to BaseCoverage table--from
					 * beginning of MarkBasePos subtable */
  HBUINT16	classCount;		/* Number of classes defined for marks */
  Offset16To<MarkArray>
		markArray;		/* Offset to MarkArray table--from
					 * beginning of MarkBasePos subtable */
  Offset16To<BaseArray>
		baseArray;		/* Offset to BaseArray table--from
					 * beginning of MarkBasePos subtable */
  public:
  DEFINE_SIZE_STATIC (12);
};

struct MarkBasePos
{
  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  {
    TRACE_DISPATCH (this, u.format);
    if (unlikely (!c->may_dispatch (this, &u.format))) return_trace (c->no_dispatch_return_value ());
    switch (u.format) {
    case 1: return_trace (c->dispatch (u.format1, std::forward<Ts> (ds)...));
    default:return_trace (c->default_return_value ());
    }
  }

  protected:
  union {
  HBUINT16		format;		/* Format identifier */
  MarkBasePosFormat1	format1;
  } u;
};


typedef AnchorMatrix LigatureAttach;	/* component-major--
					 * in order of writing direction--,
					 * mark-minor--
					 * ordered by class--zero-based. */

/* Array of LigatureAttach tables ordered by LigatureCoverage Index */
struct LigatureArray : List16OfOffset16To<LigatureAttach>
{
  template <typename Iterator,
	    hb_requires (hb_is_iterator (Iterator))>
  bool subset (hb_subset_context_t *c,
               Iterator		    coverage,
	       unsigned		    class_count,
	       const hb_map_t	   *klass_mapping) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();

    auto *out = c->serializer->start_embed (this);
    if (unlikely (!c->serializer->extend_min (out)))  return_trace (false);

    for (const auto _ : + hb_zip (coverage, *this)
		  | hb_filter (glyphset, hb_first))
    {
      auto *matrix = out->serialize_append (c->serializer);
      if (unlikely (!matrix)) return_trace (false);

      const LigatureAttach& src = (this + _.second);
      auto indexes =
          + hb_range (src.rows * class_count)
          | hb_filter ([=] (unsigned index) { return klass_mapping->has (index % class_count); })
          ;
      matrix->serialize_subset (c,
				_.second,
				this,
                                src.rows,
                                indexes);
    }
    return_trace (this->len);
  }
};

struct MarkLigPosFormat1
{
  bool intersects (const hb_set_t *glyphs) const
  {
    return (this+markCoverage).intersects (glyphs) &&
	   (this+ligatureCoverage).intersects (glyphs);
  }

  void closure_lookups (hb_closure_lookups_context_t *c) const {}

  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    + hb_zip (this+markCoverage, this+markArray)
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    | hb_apply ([&] (const MarkRecord& record) { record.collect_variation_indices (c, &(this+markArray)); })
    ;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+markCoverage, this+markArray, *c->glyph_set, &klass_mapping);

    unsigned ligcount = (this+ligatureArray).len;
    auto lig_iter =
    + hb_zip (this+ligatureCoverage, hb_range (ligcount))
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    ;

    const LigatureArray& lig_array = this+ligatureArray;
    for (const unsigned i : lig_iter)
    {
      hb_sorted_vector_t<unsigned> lig_indexes;
      unsigned row_count = lig_array[i].rows;
      for (unsigned row : + hb_range (row_count))
      {
	+ hb_range ((unsigned) classCount)
	| hb_filter (klass_mapping)
	| hb_map ([&] (const unsigned col) { return row * (unsigned) classCount + col; })
	| hb_sink (lig_indexes)
	;
      }

      lig_array[i].collect_variation_indices (c, lig_indexes.iter ());
    }
  }

  void collect_glyphs (hb_collect_glyphs_context_t *c) const
  {
    if (unlikely (!(this+markCoverage).collect_coverage (c->input))) return;
    if (unlikely (!(this+ligatureCoverage).collect_coverage (c->input))) return;
  }

  const Coverage &get_coverage () const { return this+markCoverage; }

  bool apply (hb_ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    hb_buffer_t *buffer = c->buffer;
    unsigned int mark_index = (this+markCoverage).get_coverage  (buffer->cur().codepoint);
    if (likely (mark_index == NOT_COVERED)) return_trace (false);

    /* Now we search backwards for a non-mark glyph */
    hb_ot_apply_context_t::skipping_iterator_t &skippy_iter = c->iter_input;
    skippy_iter.reset (buffer->idx, 1);
    skippy_iter.set_lookup_props (LookupFlag::IgnoreMarks);
    unsigned unsafe_from;
    if (!skippy_iter.prev (&unsafe_from))
    {
      buffer->unsafe_to_concat_from_outbuffer (unsafe_from, buffer->idx + 1);
      return_trace (false);
    }

    /* Checking that matched glyph is actually a ligature by GDEF is too strong; disabled */
    //if (!_hb_glyph_info_is_ligature (&buffer->info[skippy_iter.idx])) { return_trace (false); }

    unsigned int j = skippy_iter.idx;
    unsigned int lig_index = (this+ligatureCoverage).get_coverage  (buffer->info[j].codepoint);
    if (lig_index == NOT_COVERED)
    {
      buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
      return_trace (false);
    }

    const LigatureArray& lig_array = this+ligatureArray;
    const LigatureAttach& lig_attach = lig_array[lig_index];

    /* Find component to attach to */
    unsigned int comp_count = lig_attach.rows;
    if (unlikely (!comp_count))
    {
      buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
      return_trace (false);
    }

    /* We must now check whether the ligature ID of the current mark glyph
     * is identical to the ligature ID of the found ligature.  If yes, we
     * can directly use the component index.  If not, we attach the mark
     * glyph to the last component of the ligature. */
    unsigned int comp_index;
    unsigned int lig_id = _hb_glyph_info_get_lig_id (&buffer->info[j]);
    unsigned int mark_id = _hb_glyph_info_get_lig_id (&buffer->cur());
    unsigned int mark_comp = _hb_glyph_info_get_lig_comp (&buffer->cur());
    if (lig_id && lig_id == mark_id && mark_comp > 0)
      comp_index = hb_min (comp_count, _hb_glyph_info_get_lig_comp (&buffer->cur())) - 1;
    else
      comp_index = comp_count - 1;

    return_trace ((this+markArray).apply (c, mark_index, comp_index, lig_attach, classCount, j));
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();
    const hb_map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);
    out->format = format;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+markCoverage, this+markArray, glyphset, &klass_mapping);

    if (!klass_mapping.get_population ()) return_trace (false);
    out->classCount = klass_mapping.get_population ();

    auto mark_iter =
    + hb_zip (this+markCoverage, this+markArray)
    | hb_filter (glyphset, hb_first)
    ;

    auto new_mark_coverage =
    + mark_iter
    | hb_map_retains_sorting (hb_first)
    | hb_map_retains_sorting (glyph_map)
    ;

    if (!out->markCoverage.serialize_serialize (c->serializer, new_mark_coverage))
      return_trace (false);

    out->markArray.serialize_subset (c, markArray, this,
                                     (this+markCoverage).iter (),
                                     &klass_mapping);

    auto new_ligature_coverage =
    + hb_iter (this + ligatureCoverage)
    | hb_filter (glyphset)
    | hb_map_retains_sorting (glyph_map)
    ;

    if (!out->ligatureCoverage.serialize_serialize (c->serializer, new_ligature_coverage))
      return_trace (false);

    out->ligatureArray.serialize_subset (c, ligatureArray, this,
                                         hb_iter (this+ligatureCoverage), classCount, &klass_mapping);

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  markCoverage.sanitize (c, this) &&
		  ligatureCoverage.sanitize (c, this) &&
		  markArray.sanitize (c, this) &&
		  ligatureArray.sanitize (c, this, (unsigned int) classCount));
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 1 */
  Offset16To<Coverage>
		markCoverage;		/* Offset to Mark Coverage table--from
					 * beginning of MarkLigPos subtable */
  Offset16To<Coverage>
		ligatureCoverage;	/* Offset to Ligature Coverage
					 * table--from beginning of MarkLigPos
					 * subtable */
  HBUINT16	classCount;		/* Number of defined mark classes */
  Offset16To<MarkArray>
		markArray;		/* Offset to MarkArray table--from
					 * beginning of MarkLigPos subtable */
  Offset16To<LigatureArray>
		ligatureArray;		/* Offset to LigatureArray table--from
					 * beginning of MarkLigPos subtable */
  public:
  DEFINE_SIZE_STATIC (12);
};


struct MarkLigPos
{
  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  {
    TRACE_DISPATCH (this, u.format);
    if (unlikely (!c->may_dispatch (this, &u.format))) return_trace (c->no_dispatch_return_value ());
    switch (u.format) {
    case 1: return_trace (c->dispatch (u.format1, std::forward<Ts> (ds)...));
    default:return_trace (c->default_return_value ());
    }
  }

  protected:
  union {
  HBUINT16		format;		/* Format identifier */
  MarkLigPosFormat1	format1;
  } u;
};


typedef AnchorMatrix Mark2Array;	/* mark2-major--
					 * in order of Mark2Coverage Index--,
					 * mark1-minor--
					 * ordered by class--zero-based. */

struct MarkMarkPosFormat1
{
  bool intersects (const hb_set_t *glyphs) const
  {
    return (this+mark1Coverage).intersects (glyphs) &&
	   (this+mark2Coverage).intersects (glyphs);
  }

  void closure_lookups (hb_closure_lookups_context_t *c) const {}

  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    + hb_zip (this+mark1Coverage, this+mark1Array)
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    | hb_apply ([&] (const MarkRecord& record) { record.collect_variation_indices (c, &(this+mark1Array)); })
    ;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+mark1Coverage, this+mark1Array, *c->glyph_set, &klass_mapping);

    unsigned mark2_count = (this+mark2Array).rows;
    auto mark2_iter =
    + hb_zip (this+mark2Coverage, hb_range (mark2_count))
    | hb_filter (c->glyph_set, hb_first)
    | hb_map (hb_second)
    ;

    hb_sorted_vector_t<unsigned> mark2_indexes;
    for (const unsigned row : mark2_iter)
    {
      + hb_range ((unsigned) classCount)
      | hb_filter (klass_mapping)
      | hb_map ([&] (const unsigned col) { return row * (unsigned) classCount + col; })
      | hb_sink (mark2_indexes)
      ;
    }
    (this+mark2Array).collect_variation_indices (c, mark2_indexes.iter ());
  }

  void collect_glyphs (hb_collect_glyphs_context_t *c) const
  {
    if (unlikely (!(this+mark1Coverage).collect_coverage (c->input))) return;
    if (unlikely (!(this+mark2Coverage).collect_coverage (c->input))) return;
  }

  const Coverage &get_coverage () const { return this+mark1Coverage; }

  bool apply (hb_ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    hb_buffer_t *buffer = c->buffer;
    unsigned int mark1_index = (this+mark1Coverage).get_coverage  (buffer->cur().codepoint);
    if (likely (mark1_index == NOT_COVERED)) return_trace (false);

    /* now we search backwards for a suitable mark glyph until a non-mark glyph */
    hb_ot_apply_context_t::skipping_iterator_t &skippy_iter = c->iter_input;
    skippy_iter.reset (buffer->idx, 1);
    skippy_iter.set_lookup_props (c->lookup_props & ~LookupFlag::IgnoreFlags);
    unsigned unsafe_from;
    if (!skippy_iter.prev (&unsafe_from))
    {
      buffer->unsafe_to_concat_from_outbuffer (unsafe_from, buffer->idx + 1);
      return_trace (false);
    }

    if (!_hb_glyph_info_is_mark (&buffer->info[skippy_iter.idx]))
    {
      buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
      return_trace (false);
    }

    unsigned int j = skippy_iter.idx;

    unsigned int id1 = _hb_glyph_info_get_lig_id (&buffer->cur());
    unsigned int id2 = _hb_glyph_info_get_lig_id (&buffer->info[j]);
    unsigned int comp1 = _hb_glyph_info_get_lig_comp (&buffer->cur());
    unsigned int comp2 = _hb_glyph_info_get_lig_comp (&buffer->info[j]);

    if (likely (id1 == id2))
    {
      if (id1 == 0) /* Marks belonging to the same base. */
	goto good;
      else if (comp1 == comp2) /* Marks belonging to the same ligature component. */
	goto good;
    }
    else
    {
      /* If ligature ids don't match, it may be the case that one of the marks
       * itself is a ligature.  In which case match. */
      if ((id1 > 0 && !comp1) || (id2 > 0 && !comp2))
	goto good;
    }

    /* Didn't match. */
    buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
    return_trace (false);

    good:
    unsigned int mark2_index = (this+mark2Coverage).get_coverage  (buffer->info[j].codepoint);
    if (mark2_index == NOT_COVERED)
    {
      buffer->unsafe_to_concat_from_outbuffer (skippy_iter.idx, buffer->idx + 1);
      return_trace (false);
    }

    return_trace ((this+mark1Array).apply (c, mark1_index, mark2_index, this+mark2Array, classCount, j));
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();
    const hb_map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);
    out->format = format;

    hb_map_t klass_mapping;
    Markclass_closure_and_remap_indexes (this+mark1Coverage, this+mark1Array, glyphset, &klass_mapping);

    if (!klass_mapping.get_population ()) return_trace (false);
    out->classCount = klass_mapping.get_population ();

    auto mark1_iter =
    + hb_zip (this+mark1Coverage, this+mark1Array)
    | hb_filter (glyphset, hb_first)
    ;

    hb_sorted_vector_t<hb_codepoint_t> new_coverage;
    + mark1_iter
    | hb_map (hb_first)
    | hb_map (glyph_map)
    | hb_sink (new_coverage)
    ;

    if (!out->mark1Coverage.serialize_serialize (c->serializer, new_coverage.iter ()))
      return_trace (false);

    out->mark1Array.serialize_subset (c, mark1Array, this,
                                      (this+mark1Coverage).iter (),
                                      &klass_mapping);

    unsigned mark2count = (this+mark2Array).rows;
    auto mark2_iter =
    + hb_zip (this+mark2Coverage, hb_range (mark2count))
    | hb_filter (glyphset, hb_first)
    ;

    new_coverage.reset ();
    + mark2_iter
    | hb_map (hb_first)
    | hb_map (glyph_map)
    | hb_sink (new_coverage)
    ;

    if (!out->mark2Coverage.serialize_serialize (c->serializer, new_coverage.iter ()))
      return_trace (false);

    hb_sorted_vector_t<unsigned> mark2_indexes;
    for (const unsigned row : + mark2_iter
			      | hb_map (hb_second))
    {
      + hb_range ((unsigned) classCount)
      | hb_filter (klass_mapping)
      | hb_map ([&] (const unsigned col) { return row * (unsigned) classCount + col; })
      | hb_sink (mark2_indexes)
      ;
    }

    out->mark2Array.serialize_subset (c, mark2Array, this, mark2_iter.len (), mark2_indexes.iter ());

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  mark1Coverage.sanitize (c, this) &&
		  mark2Coverage.sanitize (c, this) &&
		  mark1Array.sanitize (c, this) &&
		  mark2Array.sanitize (c, this, (unsigned int) classCount));
  }

  protected:
  HBUINT16	format;			/* Format identifier--format = 1 */
  Offset16To<Coverage>
		mark1Coverage;		/* Offset to Combining Mark1 Coverage
					 * table--from beginning of MarkMarkPos
					 * subtable */
  Offset16To<Coverage>
		mark2Coverage;		/* Offset to Combining Mark2 Coverage
					 * table--from beginning of MarkMarkPos
					 * subtable */
  HBUINT16	classCount;		/* Number of defined mark classes */
  Offset16To<MarkArray>
		mark1Array;		/* Offset to Mark1Array table--from
					 * beginning of MarkMarkPos subtable */
  Offset16To<Mark2Array>
		mark2Array;		/* Offset to Mark2Array table--from
					 * beginning of MarkMarkPos subtable */
  public:
  DEFINE_SIZE_STATIC (12);
};

struct MarkMarkPos
{
  template <typename context_t, typename ...Ts>
  typename context_t::return_t dispatch (context_t *c, Ts&&... ds) const
  {
    TRACE_DISPATCH (this, u.format);
    if (unlikely (!c->may_dispatch (this, &u.format))) return_trace (c->no_dispatch_return_value ());
    switch (u.format) {
    case 1: return_trace (c->dispatch (u.format1, std::forward<Ts> (ds)...));
    default:return_trace (c->default_return_value ());
    }
  }

  protected:
  union {
  HBUINT16		format;		/* Format identifier */
  MarkMarkPosFormat1	format1;
  } u;
};


struct ContextPos : Context {};

struct ChainContextPos : ChainContext {};

struct ExtensionPos : Extension<ExtensionPos>
{
  typedef struct PosLookupSubTable SubTable;
};




static void
reverse_cursive_minor_offset (hb_glyph_position_t *pos, unsigned int i, hb_direction_t direction, unsigned int new_parent)
{
  int chain = pos[i].attach_chain(), type = pos[i].attach_type();
  if (likely (!chain || 0 == (type & ATTACH_TYPE_CURSIVE)))
    return;

  pos[i].attach_chain() = 0;

  unsigned int j = (int) i + chain;

  /* Stop if we see new parent in the chain. */
  if (j == new_parent)
    return;

  reverse_cursive_minor_offset (pos, j, direction, new_parent);

  if (HB_DIRECTION_IS_HORIZONTAL (direction))
    pos[j].y_offset = -pos[i].y_offset;
  else
    pos[j].x_offset = -pos[i].x_offset;

  pos[j].attach_chain() = -chain;
  pos[j].attach_type() = type;
}
static void
propagate_attachment_offsets (hb_glyph_position_t *pos,
			      unsigned int len,
			      unsigned int i,
			      hb_direction_t direction,
			      unsigned nesting_level = HB_MAX_NESTING_LEVEL)
{
  /* Adjusts offsets of attached glyphs (both cursive and mark) to accumulate
   * offset of glyph they are attached to. */
  int chain = pos[i].attach_chain(), type = pos[i].attach_type();
  if (likely (!chain))
    return;

  pos[i].attach_chain() = 0;

  unsigned int j = (int) i + chain;

  if (unlikely (j >= len))
    return;

  if (unlikely (!nesting_level))
    return;

  propagate_attachment_offsets (pos, len, j, direction, nesting_level - 1);

  assert (!!(type & ATTACH_TYPE_MARK) ^ !!(type & ATTACH_TYPE_CURSIVE));

  if (type & ATTACH_TYPE_CURSIVE)
  {
    if (HB_DIRECTION_IS_HORIZONTAL (direction))
      pos[i].y_offset += pos[j].y_offset;
    else
      pos[i].x_offset += pos[j].x_offset;
  }
  else /*if (type & ATTACH_TYPE_MARK)*/
  {
    pos[i].x_offset += pos[j].x_offset;
    pos[i].y_offset += pos[j].y_offset;

    assert (j < i);
    if (HB_DIRECTION_IS_FORWARD (direction))
      for (unsigned int k = j; k < i; k++) {
	pos[i].x_offset -= pos[k].x_advance;
	pos[i].y_offset -= pos[k].y_advance;
      }
    else
      for (unsigned int k = j + 1; k < i + 1; k++) {
	pos[i].x_offset += pos[k].x_advance;
	pos[i].y_offset += pos[k].y_advance;
      }
  }
}

void
GPOS::position_start (hb_font_t *font HB_UNUSED, hb_buffer_t *buffer)
{
  unsigned int count = buffer->len;
  for (unsigned int i = 0; i < count; i++)
    buffer->pos[i].attach_chain() = buffer->pos[i].attach_type() = 0;
}

void
GPOS::position_finish_advances (hb_font_t *font HB_UNUSED, hb_buffer_t *buffer HB_UNUSED)
{
  //_hb_buffer_assert_gsubgpos_vars (buffer);
}

void
GPOS::position_finish_offsets (hb_font_t *font, hb_buffer_t *buffer)
{
  _hb_buffer_assert_gsubgpos_vars (buffer);

  unsigned int len;
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, &len);
  hb_direction_t direction = buffer->props.direction;

  /* Handle attachments */
  if (buffer->scratch_flags & HB_BUFFER_SCRATCH_FLAG_HAS_GPOS_ATTACHMENT)
    for (unsigned i = 0; i < len; i++)
      propagate_attachment_offsets (pos, len, i, direction);

  if (unlikely (font->slant))
  {
    for (unsigned i = 0; i < len; i++)
      if (unlikely (pos[i].y_offset))
        pos[i].x_offset += _hb_roundf (font->slant_xy * pos[i].y_offset);
  }
}


struct GPOS_accelerator_t : GPOS::accelerator_t {
  GPOS_accelerator_t (hb_face_t *face) : GPOS::accelerator_t (face) {}
};


/* Out-of-class implementation for methods recursing */

#ifndef HB_NO_OT_LAYOUT
template <typename context_t>
/*static*/ typename context_t::return_t PosLookup::dispatch_recurse_func (context_t *c, unsigned int lookup_index)
{
  const PosLookup &l = c->face->table.GPOS.get_relaxed ()->table->get_lookup (lookup_index);
  return l.dispatch (c);
}

template <>
inline hb_closure_lookups_context_t::return_t
PosLookup::dispatch_recurse_func<hb_closure_lookups_context_t> (hb_closure_lookups_context_t *c, unsigned this_index)
{
  const PosLookup &l = c->face->table.GPOS.get_relaxed ()->table->get_lookup (this_index);
  return l.closure_lookups (c, this_index);
}

template <>
inline bool PosLookup::dispatch_recurse_func<hb_ot_apply_context_t> (hb_ot_apply_context_t *c, unsigned int lookup_index)
{
  const PosLookup &l = c->face->table.GPOS.get_relaxed ()->table->get_lookup (lookup_index);
  unsigned int saved_lookup_props = c->lookup_props;
  unsigned int saved_lookup_index = c->lookup_index;
  c->set_lookup_index (lookup_index);
  c->set_lookup_props (l.get_props ());
  bool ret = l.dispatch (c);
  c->set_lookup_index (saved_lookup_index);
  c->set_lookup_props (saved_lookup_props);
  return ret;
}
#endif


} /* namespace OT */


#endif /* HB_OT_LAYOUT_GPOS_TABLE_HH */
