/*
 * Copyright © 2022  Google, Inc.
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
 * Google Author(s): Garret Rieger
 */

#ifndef GRAPH_MARKBASEPOS_GRAPH_HH
#define GRAPH_MARKBASEPOS_GRAPH_HH

#include "split-helpers.hh"
#include "coverage-graph.hh"
#include "../OT/Layout/GPOS/MarkBasePos.hh"
#include "../OT/Layout/GPOS/PosLookupSubTable.hh"

namespace graph {

struct AnchorMatrix : public OT::Layout::GPOS_impl::AnchorMatrix
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    // TODO
    return false;
  }
};

struct MarkArray : public OT::Layout::GPOS_impl::MarkArray
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    // TODO
    return false;
  }
};

struct MarkBasePosFormat1 : public OT::Layout::GPOS_impl::MarkBasePosFormat1_2<SmallTypes>
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    int64_t vertex_len = vertex.obj.tail - vertex.obj.head;
    unsigned min_size = OT::Layout::GPOS_impl::MarkBasePosFormat1_2<SmallTypes>::min_size;
    if (vertex_len < min_size) return false;

    // TODO
    return true;
  }

  hb_vector_t<unsigned> split_subtables (gsubgpos_graph_context_t& c, unsigned this_index)
  {
    hb_set_t visited;

    const unsigned coverage_id = c.graph.index_for_offset (this_index, &markCoverage);
    const unsigned coverage_size = c.graph.vertices_[coverage_id].table_size ();

    const unsigned base_coverage_id = c.graph.index_for_offset (this_index, &baseCoverage);
    const unsigned base_size =
        OT::Layout::GPOS_impl::PairPosFormat1_3<SmallTypes>::min_size +
        c.graph.vertices_[base_coverage_id].table_size ();

    hb_vector_t<class_info_t> class_to_info = get_class_info (c, this_index);

    unsigned partial_coverage_size = 4;
    unsigned accumulated = base_size;
    hb_vector_t<unsigned> split_points;
    unsigned class_count = classCount;
    for (unsigned klass = 0; klass < class_count; klass++)
    {
      class_info_t& info = class_to_info[klass];
      partial_coverage_size += OT::HBUINT16::static_size * info.num_marks;
      unsigned accumulated_delta = OT::Layout::GPOS_impl::MarkRecord::static_size * info.num_marks;
      accumulated_delta += OT::Offset16::static_size * info.child_indices.length;
      for (unsigned objidx : info.child_indices)
        accumulated_delta += c.graph.find_subgraph_size (objidx, visited);

      accumulated += accumulated_delta;
      unsigned total = accumulated + partial_coverage_size;

      if (total >= (1 << 16))
      {
        split_points.push (klass);
        accumulated = base_size + accumulated_delta;
        partial_coverage_size = 4 + OT::HBUINT16::static_size * info.num_marks;
        visited.clear (); // node sharing isn't allowed between splits.
      }
    }

    split_context_t split_context {
      c,
      this,
      this_index,
    };

    return actuate_subtable_split<split_context_t> (split_context, split_points);
  }

 private:

  struct split_context_t {
    gsubgpos_graph_context_t& c;
    MarkBasePosFormat1* thiz;
    unsigned this_index;

    unsigned original_count ()
    {
      return thiz->pairSet.len;
    }

    unsigned clone_range (unsigned start, unsigned end)
    {
      return thiz->clone_range (this->c, this->this_index, start, end);
    }

    bool shrink (unsigned count)
    {
      return thiz->shrink (this->c, this->this_index, count);
    }
  };

  struct class_info_t {
    unsigned num_marks;
    hb_vector_t<unsigned> child_indices;
  };

  hb_vector_t<class_info_t> get_class_info (gsubgpos_graph_context_t& c,
                                            unsigned this_index)
  {
    hb_vector_t<class_info_t> class_to_info;

    unsigned class_count= classCount;
    class_to_size.resize (class_count);

    unsigned mark_array_id =
        c.graph.index_for_offset (this_index, &markArray);
    auto& mark_array_v = graph.vertices_[coverage_id];
    MarkArray* mark_array = (MarkArray*) mark_array_v.head;
    // TODO sanitize

    unsigned mark_count = mark_array->length;
    for (unsigned mark = 0; mark < mark_count; mark++)
    {
      unsigned klass = (*mark_array)[mark].klass;
      class_to_size[klass].num_marks++;
    }

    for (const auto* link : mark_array_v.obj.real_links)
    {
      unsiged mark = (link->position - 2) /
                     OT::Layout::GPOS_impl::MarkReecord::static_size;
      unsigned klass = (*mark_array)[mark].klass;
      class_to_info[klass].child_indices.push (link.objidx);
    }

    unsigned base_array_id =
        c.graph.index_for_offset (this_index, &baseArray);
    auto& base_array_v = graph.vertices_[coverage_id];
    AnchorMatrix* base_array = (AnchorMatrix*) base_array_v.head;
    // TODO sanitize

    unsigned base_count = base_array->rows;
    for (const auto* link : base_array_v.obj.real_links)
    {
      unsigned index = (link->position - 2) / OT::Offset16::static_size;
      unsigned klass = index % class_count;
      class_to_info[klass].child_indices.push (link->objidx);
    }

    return class_to_info;
  }

  bool shrink (gsubgpos_graph_context_t& c,
               unsigned this_index,
               unsigned count)
  {
    /*
    DEBUG_MSG (SUBSET_REPACK, nullptr,
               "  Shrinking PairPosFormat1 (%u) to [0, %u).",
               this_index,
               count);
    unsigned old_count = pairSet.len;
    if (count >= old_count)
      return true;

    pairSet.len = count;
    c.graph.vertices_[this_index].obj.tail -= (old_count - count) * SmallTypes::size;

    unsigned coverage_id = c.graph.mutable_index_for_offset (this_index, &coverage);
    unsigned coverage_size = c.graph.vertices_[coverage_id].table_size ();
    auto& coverage_v = c.graph.vertices_[coverage_id];

    Coverage* coverage_table = (Coverage*) coverage_v.obj.head;
    if (!coverage_table || !coverage_table->sanitize (coverage_v))
      return false;

    auto new_coverage =
        + hb_zip (coverage_table->iter (), hb_range ())
        | hb_filter ([&] (hb_pair_t<unsigned, unsigned> p) {
          return p.second < count;
        })
        | hb_map_retains_sorting (hb_first)
        ;

    return Coverage::make_coverage (c, new_coverage, coverage_id, coverage_size);
    */
    return false; // TODO
  }

  // Create a new PairPos including PairSet's from start (inclusive) to end (exclusive).
  // Returns object id of the new object.
  unsigned clone_range (gsubgpos_graph_context_t& c,
                        unsigned this_index,
                        unsigned start, unsigned end) const
  {
    /*
    DEBUG_MSG (SUBSET_REPACK, nullptr,
               "  Cloning PairPosFormat1 (%u) range [%u, %u).", this_index, start, end);

    unsigned num_pair_sets = end - start;
    unsigned prime_size = OT::Layout::GPOS_impl::PairPosFormat1_3<SmallTypes>::min_size
                          + num_pair_sets * SmallTypes::size;

    unsigned pair_pos_prime_id = c.create_node (prime_size);
    if (pair_pos_prime_id == (unsigned) -1) return -1;

    PairPosFormat1* pair_pos_prime = (PairPosFormat1*) c.graph.object (pair_pos_prime_id).head;
    pair_pos_prime->format = this->format;
    pair_pos_prime->valueFormat[0] = this->valueFormat[0];
    pair_pos_prime->valueFormat[1] = this->valueFormat[1];
    pair_pos_prime->pairSet.len = num_pair_sets;

    for (unsigned i = start; i < end; i++)
    {
      c.graph.move_child<> (this_index,
                            &pairSet[i],
                            pair_pos_prime_id,
                            &pair_pos_prime->pairSet[i - start]);
    }

    unsigned coverage_id = c.graph.index_for_offset (this_index, &coverage);
    if (!Coverage::clone_coverage (c,
                                   coverage_id,
                                   pair_pos_prime_id,
                                   2,
                                   start, end))
      return -1;

    return pair_pos_prime_id;
    */
    return false; // TODO
  }
};


struct MarkBasePos : public OT::Layout::GPOS_impl::MarkBasePos
{
  hb_vector_t<unsigned> split_subtables (gsubgpos_graph_context_t& c,
                                         unsigned this_index)
  {
    switch (u.format) {
    case 1:
      return ((MarkBasePosFormat1*)(&u.format1))->split_subtables (c, this_index);
#ifndef HB_NO_BORING_EXPANSION
    case 2: HB_FALLTHROUGH;
      // Don't split 24bit PairPos's.
#endif
    default:
      return hb_vector_t<unsigned> ();
    }
  }

  bool sanitize (graph_t::vertex_t& vertex) const
  {
    int64_t vertex_len = vertex.obj.tail - vertex.obj.head;
    if (vertex_len < u.format.get_size ()) return false;

    switch (u.format) {
    case 1:
      return ((PairPosFormat1*)(&u.format1))->sanitize (vertex);
#ifndef HB_NO_BORING_EXPANSION
    case 2: HB_FALLTHROUGH;
#endif
    default:
      // We don't handle format 3 and 4 here.
      return false;
    }
  }
};


}

#endif  // GRAPH_MARKBASEPOS_GRAPH_HH
