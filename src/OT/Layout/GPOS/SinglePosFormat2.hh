#ifndef OT_LAYOUT_GPOS_SINGLEPOSFORMAT2_HH
#define OT_LAYOUT_GPOS_SINGLEPOSFORMAT2_HH

#include "Common.hh"

namespace OT {
namespace Layout {
namespace GPOS_impl {

struct SinglePosFormat2
{
  protected:
  HBUINT16      format;                 /* Format identifier--format = 2 */
  Offset16To<Coverage>
                coverage;               /* Offset to Coverage table--from
                                         * beginning of subtable */
  ValueFormat   valueFormat;            /* Defines the types of data in the
                                         * ValueRecord */
  HBUINT16      valueCount;             /* Number of ValueRecords */
  ValueRecord   values;                 /* Array of ValueRecords--positioning
                                         * values applied to glyphs */
  public:
  DEFINE_SIZE_ARRAY (8, values);

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  coverage.sanitize (c, this) &&
                  valueFormat.sanitize_values (c, this, values, valueCount));
  }

  bool intersects (const hb_set_t *glyphs) const
  { return (this+coverage).intersects (glyphs); }

  void closure_lookups (hb_closure_lookups_context_t *c) const {}
  void collect_variation_indices (hb_collect_variation_indices_context_t *c) const
  {
    if (!valueFormat.has_device ()) return;

    auto it =
    + hb_zip (this+coverage, hb_range ((unsigned) valueCount))
    | hb_filter (c->glyph_set, hb_first)
    ;

    if (!it) return;

    unsigned sub_length = valueFormat.get_len ();
    const hb_array_t<const Value> values_array = values.as_array (valueCount * sub_length);

    for (unsigned i : + it
                      | hb_map (hb_second))
      valueFormat.collect_variation_indices (c, this, values_array.sub_array (i * sub_length, sub_length));

  }

  void collect_glyphs (hb_collect_glyphs_context_t *c) const
  { if (unlikely (!(this+coverage).collect_coverage (c->input))) return; }

  const Coverage &get_coverage () const { return this+coverage; }

  ValueFormat get_value_format () const { return valueFormat; }

  bool apply (hb_ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    hb_buffer_t *buffer = c->buffer;
    unsigned int index = (this+coverage).get_coverage  (buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return_trace (false);

    if (likely (index >= valueCount)) return_trace (false);

    valueFormat.apply_value (c, this,
                             &values[index * valueFormat.get_len ()],
                             buffer->cur_pos());

    buffer->idx++;
    return_trace (true);
  }

  template<typename Iterator,
      typename SrcLookup,
      hb_requires (hb_is_iterator (Iterator))>
  void serialize (hb_serialize_context_t *c,
                  const SrcLookup *src,
                  Iterator it,
                  ValueFormat newFormat,
                  const hb_map_t *layout_variation_idx_map)
  {
    auto out = c->extend_min (this);
    if (unlikely (!out)) return;
    if (unlikely (!c->check_assign (valueFormat, newFormat, HB_SERIALIZE_ERROR_INT_OVERFLOW))) return;
    if (unlikely (!c->check_assign (valueCount, it.len (), HB_SERIALIZE_ERROR_ARRAY_OVERFLOW))) return;

    + it
    | hb_map (hb_second)
    | hb_apply ([&] (hb_array_t<const Value> _)
    { src->get_value_format ().copy_values (c, newFormat, src, &_, layout_variation_idx_map); })
    ;

    auto glyphs =
    + it
    | hb_map_retains_sorting (hb_first)
    ;

    coverage.serialize_serialize (c, glyphs);
  }

  bool subset (hb_subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const hb_set_t &glyphset = *c->plan->glyphset_gsub ();
    const hb_map_t &glyph_map = *c->plan->glyph_map;

    unsigned sub_length = valueFormat.get_len ();
    auto values_array = values.as_array (valueCount * sub_length);

    auto it =
    + hb_zip (this+coverage, hb_range ((unsigned) valueCount))
    | hb_filter (glyphset, hb_first)
    | hb_map_retains_sorting ([&] (const hb_pair_t<hb_codepoint_t, unsigned>& _)
                              {
                                return hb_pair (glyph_map[_.first],
                                                values_array.sub_array (_.second * sub_length,
                                                                        sub_length));
                              })
    ;

    bool ret = bool (it);
    SinglePos_serialize (c->serializer, this, it, c->plan->layout_variation_idx_map);
    return_trace (ret);
  }
};


}
}
}

#endif /* OT_LAYOUT_GPOS_SINGLEPOSFORMAT2_HH */
