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

  unsigned clone (gsubgpos_graph_context_t& c,
                  unsigned this_index,
                  const hb_hashmap_t<unsigned, unsigned>& pos_to_index,
                  unsigned start,
                  unsigned end)
  {
    // TODO
    return -1;
  }
};

struct MarkArray : public OT::Layout::GPOS_impl::MarkArray
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    // TODO
    return false;
  }

  unsigned clone (gsubgpos_graph_context_t& c,
                  unsigned this_index,
                  const hb_hashmap_t<unsigned, unsigned>& pos_to_index,
                  hb_set_t& marks)
  {
    // TODO
    return -1;
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
      partial_coverage_size += OT::HBUINT16::static_size * info.marks.get_population ();
      unsigned accumulated_delta = OT::Layout::GPOS_impl::MarkRecord::static_size * info.marks.get_population ();
      // TODO this doesn't count up null offsets.
      accumulated_delta += OT::Offset16::static_size * info.child_indices.length;
      for (unsigned objidx : info.child_indices)
        accumulated_delta += c.graph.find_subgraph_size (objidx, visited);

      accumulated += accumulated_delta;
      unsigned total = accumulated + partial_coverage_size;

      if (total >= (1 << 16))
      {
        split_points.push (klass);
        accumulated = base_size + accumulated_delta;
        partial_coverage_size = 4 + OT::HBUINT16::static_size * info.marks.get_population ();
        visited.clear (); // node sharing isn't allowed between splits.
      }
    }


    const unsigned mark_array_id = c.graph.index_for_offset (this_index, &markArray);
    const unsigned base_array_id = c.graph.index_for_offset (this_index, &baseArray);


    split_context_t split_context {
      c,
      this,
      this_index,
      std::move (class_to_info),
      c.graph.vertices_[mark_array_id].position_to_index_map (),
      c.graph.vertices_[base_array_id].position_to_index_map ()
    };

    return actuate_subtable_split<split_context_t> (split_context, split_points);
  }

 private:

  struct class_info_t {
    hb_set_t marks;
    hb_vector_t<unsigned> child_indices;
  };

  struct split_context_t {
    gsubgpos_graph_context_t& c;
    MarkBasePosFormat1* thiz;
    unsigned this_index;
    hb_vector_t<class_info_t> class_to_info;
    hb_hashmap_t<unsigned, unsigned> mark_array_links;
    hb_hashmap_t<unsigned, unsigned> base_array_links;

    unsigned original_count ()
    {
      return thiz->classCount;
    }

    unsigned clone_range (unsigned start, unsigned end)
    {
      return thiz->clone_range (*this, this->this_index, start, end);
    }

    bool shrink (unsigned count)
    {
      return thiz->shrink (*this, this->this_index, count);
    }
  };

  hb_vector_t<class_info_t> get_class_info (gsubgpos_graph_context_t& c,
                                            unsigned this_index)
  {
    hb_vector_t<class_info_t> class_to_info;

    unsigned class_count= classCount;
    class_to_info.resize (class_count);

    unsigned mark_array_id =
        c.graph.index_for_offset (this_index, &markArray);
    auto& mark_array_v = c.graph.vertices_[mark_array_id];
    MarkArray* mark_array = (MarkArray*) mark_array_v.obj.head;
    // TODO sanitize

    unsigned mark_count = mark_array->len;
    for (unsigned mark = 0; mark < mark_count; mark++)
    {
      unsigned klass = (*mark_array)[mark].get_class ();
      class_to_info[klass].marks.add (mark);
    }

    for (const auto& link : mark_array_v.obj.real_links)
    {
      unsigned mark = (link.position - 2) /
                     OT::Layout::GPOS_impl::MarkRecord::static_size;
      unsigned klass = (*mark_array)[mark].get_class ();
      class_to_info[klass].child_indices.push (link.objidx);
    }

    unsigned base_array_id =
        c.graph.index_for_offset (this_index, &baseArray);
    auto& base_array_v = c.graph.vertices_[base_array_id];

    for (const auto& link : base_array_v.obj.real_links)
    {
      unsigned index = (link.position - 2) / OT::Offset16::static_size;
      unsigned klass = index % class_count;
      class_to_info[klass].child_indices.push (link.objidx);
    }

    return class_to_info;
  }

  bool shrink (split_context_t& c,
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
    return -1; // TODO
  }

  // Create a new PairPos including PairSet's from start (inclusive) to end (exclusive).
  // Returns object id of the new object.
  unsigned clone_range (split_context_t& sc,
                        unsigned this_index,
                        unsigned start, unsigned end) const
  {
    DEBUG_MSG (SUBSET_REPACK, nullptr,
               "  Cloning MarkBasePosFormat1 (%u) range [%u, %u).", this_index, start, end);

    // TODO

    graph_t& graph = sc.c.graph;
    unsigned prime_size = OT::Layout::GPOS_impl::MarkBasePosFormat1_2<SmallTypes>::static_size;

    unsigned prime_id = sc.c.create_node (prime_size);
    if (prime_id == (unsigned) -1) return -1;

    MarkBasePosFormat1* prime = (MarkBasePosFormat1*) graph.object (prime_id).head;
    prime->format = this->format;
    prime->classCount = this->classCount;

    unsigned base_coverage_id =
        graph.index_for_offset (sc.this_index, &baseCoverage);
    auto* base_coverage_link = graph.vertices_[prime_id].obj.real_links.push ();
    base_coverage_link->width = SmallTypes::size;
    base_coverage_link->objidx = base_coverage_id;
    base_coverage_link->position = 4;
    graph.vertices_[base_coverage_id].parents.push (prime_id);
    graph.duplicate (prime_id, base_coverage_id);

    hb_set_t marks;
    for (unsigned klass = start; klass < end; klass++)
    {
      + sc.class_to_info[klass].marks.iter ()
      | hb_sink (marks)
      ;
    }

    if (!Coverage::add_coverage (sc.c,
                                 prime_id,
                                 2,
                                 + marks.iter (),
                                 marks.get_population () * 2 + 4))
      return -1;

    unsigned mark_array_id =
        graph.index_for_offset (sc.this_index, &markArray);
    auto& mark_array_v = graph.vertices_[mark_array_id];
    MarkArray* mark_array = (MarkArray*) mark_array_v.obj.head;
    // TODO sanitize
    unsigned new_mark_array = mark_array->clone (sc.c,
                                                 mark_array_id,
                                                 sc.mark_array_links,
                                                 marks);

    auto* mark_array_link = graph.vertices_[prime_id].obj.real_links.push ();
    mark_array_link->width = SmallTypes::size;
    mark_array_link->objidx = new_mark_array;
    mark_array_link->position = 8;
    graph.vertices_[new_mark_array].parents.push (prime_id);

    unsigned base_array_id =
        graph.index_for_offset (sc.this_index, &baseArray);
    auto& base_array_v = graph.vertices_[base_array_id];
    AnchorMatrix* base_array = (AnchorMatrix*) base_array_v.obj.head;
    // TODO sanitize
    unsigned new_base_array = base_array->clone (sc.c,
                                                 mark_array_id,
                                                 sc.base_array_links,
                                                 start, end);
    auto* base_array_link = graph.vertices_[prime_id].obj.real_links.push ();
    base_array_link->width = SmallTypes::size;
    base_array_link->objidx = new_base_array;
    base_array_link->position = 10;
    graph.vertices_[new_base_array].parents.push (prime_id);

    return prime_id;
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
