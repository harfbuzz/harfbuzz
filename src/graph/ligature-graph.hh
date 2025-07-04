/*
 * Copyright © 2025  Google, Inc.
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

#ifndef GRAPH_LIGATURE_GRAPH_HH
#define GRAPH_LIGATURE_GRAPH_HH

#include "OT/Layout/GSUB/LigatureSet.hh"
#include "OT/Layout/types.hh"
#include "hb-ot-layout-common.hh"

#include "graph.hh"
#include "../OT/Layout/GSUB/LigatureSubst.hh"
#include "../OT/Layout/GSUB/LigatureSubstFormat1.hh"
#include <algorithm>
#include <utility>

namespace graph {

struct LigatureSet : public OT::Layout::GSUB_impl::LigatureSet<SmallTypes>
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    int64_t vertex_len = vertex.obj.tail - vertex.obj.head;
    if (vertex_len < OT::Layout::GSUB_impl::LigatureSet<SmallTypes>::min_size) return false;
    hb_barrier ();

    int64_t total_len = ligature.get_size() + OT::Layout::GSUB_impl::LigatureSet<SmallTypes>::min_size - ligature.len.get_size();
    if (vertex_len < total_len) {
      return false;
    }
    return true;
  }
};

struct LigatureSubstFormat1 : public OT::Layout::GSUB_impl::LigatureSubstFormat1_2<SmallTypes>
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    int64_t vertex_len = vertex.obj.tail - vertex.obj.head;
    unsigned min_size = OT::Layout::GSUB_impl::LigatureSubstFormat1_2<SmallTypes>::min_size;
    if (vertex_len < min_size) return false;
    hb_barrier ();

    return vertex_len >=
        min_size + ligatureSet.get_size() - ligatureSet.len.get_size();
  }

  hb_vector_t<unsigned> split_subtables (gsubgpos_graph_context_t& c,
                                         unsigned parent_index,
                                         unsigned this_index)
  {
    auto split_points = compute_split_points(c, parent_index, this_index);
    split_context_t split_context {
      c,
      this,
      c.graph.duplicate_if_shared (parent_index, this_index),
    };
    return actuate_subtable_split<split_context_t> (split_context, split_points);
  }

 private:
  hb_vector_t<unsigned> compute_split_points(gsubgpos_graph_context_t& c,
                                             unsigned parent_index,
                                             unsigned this_index) const
  {
    // For ligature subst coverage is always packed last, and as a result is where an overflow
    // will happen if there is one, so we can check the estimate length of the
    // LigatureSubstFormat1 -> Coverage offset length which is the sum of all data in the
    // retained sub graph except for the coverage table itself.
    const unsigned base_size = OT::Layout::GSUB_impl::LigatureSubstFormat1_2<SmallTypes>::min_size;
    unsigned accumulated = base_size;

    unsigned ligature_index = 0;
    hb_vector_t<unsigned> split_points;
    for (unsigned i = 0; i < ligatureSet.len; i++)
    {
      accumulated += OT::HBUINT16::static_size; // for ligature set offset
      accumulated += OT::Layout::GSUB_impl::LigatureSet<SmallTypes>::min_size; // for ligature set table

      auto liga_set = c.graph.as_table<LigatureSet>(this_index, &ligatureSet[i]);
      if (!liga_set.table) {
        return hb_vector_t<unsigned> {};
      }

      for (unsigned j = 0; j < liga_set.table->ligature.len; j++)
      {
        const unsigned liga_id = c.graph.index_for_offset (liga_set.index, &liga_set.table->ligature[j]);
        const unsigned liga_size = c.graph.vertices_[liga_id].table_size ();

        accumulated += OT::HBUINT16::static_size; // for ligature offset
        accumulated += liga_size; // for the ligature table
        
        
        if (accumulated >= (1 << 16))
        {
          split_points.push(ligature_index);
          // We're going to split such that the current ligature will be in the new sub table.
          // That means we'll have one ligature subst (base_base), one ligature set, and one liga table
          accumulated = base_size + // for liga subst subtable
            (OT::HBUINT16::static_size * 2) + // for liga set and liga offset
            OT::Layout::GSUB_impl::LigatureSet<SmallTypes>::min_size + // for liga set subtable
            liga_size; // for liga sub table
        }

        ligature_index++;
      }
    }

    return split_points;
  }


  struct split_context_t
  {
    gsubgpos_graph_context_t& c;
    LigatureSubstFormat1* thiz;
    unsigned this_index;

    unsigned original_count ()
    {
      unsigned total = 0;
      for (unsigned i = 0; i < thiz->ligatureSet.len; i++)
      { 
        auto liga_set = c.graph.as_table<LigatureSet>(this_index, &thiz->ligatureSet[i]);
        if (!liga_set.table) {
          return 0;
        }
        total += liga_set.table->ligature.len;
      }
      return total;
    }

    unsigned clone_range (unsigned start, unsigned end)
    {
      return thiz->clone_range (c, this_index, start, end);
    }

    bool shrink (unsigned count)
    {
      return thiz->shrink (c, this_index, original_count(), count);
    }
  };

  std::pair<unsigned, LigatureSet*> new_liga_set(gsubgpos_graph_context_t& c, unsigned count) const {
    unsigned prime_size = OT::Layout::GSUB_impl::LigatureSet<SmallTypes>::min_size
                          + count * SmallTypes::size;

    unsigned prime_id = c.create_node (prime_size);
    if (prime_id == (unsigned) -1) return std::make_pair(-1, nullptr);

    LigatureSet* prime = (LigatureSet*) c.graph.object (prime_id).head;
    prime->ligature.len = count;
    return std::make_pair(prime_id, prime);
  }
   
  unsigned clone_range (gsubgpos_graph_context_t& c,
                        unsigned this_index,
                        unsigned start, unsigned end) const
  {
    DEBUG_MSG (SUBSET_REPACK, nullptr,
               "  Cloning LigatureSubstFormat1 (%u) range [%u, %u).", this_index, start, end);

    // Create an oversized new liga subst, we'll adjust the size down later. We don't know
    // the final size until we process it but we also need it to exist while we're processing
    // so that nodes can be moved to it as needed.
    unsigned prime_size = OT::Layout::GSUB_impl::LigatureSubstFormat1_2<SmallTypes>::min_size
                          + ligatureSet.get_size() - ligatureSet.len.get_size();
                           
    unsigned liga_subst_prime_id = c.create_node (prime_size);
    if (liga_subst_prime_id == (unsigned) -1) return -1;

    LigatureSubstFormat1* liga_subst_prime = (LigatureSubstFormat1*) c.graph.object (liga_subst_prime_id).head;
    liga_subst_prime->format = this->format;
    liga_subst_prime->ligatureSet.len = this->ligatureSet.len;

    // Locate all liga sets with ligas between start and end.
    // Clone or move them as needed.
    unsigned count = 0;
    unsigned liga_set_count = 0;
    unsigned liga_set_start = -1;
    unsigned liga_set_end = 0; // inclusive
    for (unsigned i = 0; i < ligatureSet.len; i++)
    {
      auto liga_set_index = c.graph.index_for_offset(this_index, &ligatureSet[i]);
      auto liga_set = c.graph.as_table<LigatureSet>(this_index, &ligatureSet[i]);
      if (!liga_set.table) {
        return -1;
      }
      unsigned num_ligas = liga_set.table->ligature.len;

      unsigned current_start = count;
      unsigned current_end = count + num_ligas;

      if (current_start >= start && current_end <= end) {
        // This liga set is fully contined within [start, end)
        // We can move the entire ligaset to the new liga subset object.
        liga_set_end = i;
        if (i < liga_set_start) liga_set_start = i;
        c.graph.move_child<> (this_index,
                              &ligatureSet[i],
                              liga_subst_prime_id,
                              &liga_subst_prime->ligatureSet[liga_set_count++]);
      }
      else if (current_start < end && start < current_end)
      {
        // This liga set partially overlaps [start, end). We'll need to create
        // a new liga set sub table and move the intersecting ligas to it.
        unsigned liga_count = std::min(end, current_end) - std::max(start, current_start);
        auto result = new_liga_set(c, liga_count);
        unsigned prime_id = result.first;
        LigatureSet* prime = result.second;
        if (prime_id == (unsigned) -1) return -1;

        unsigned new_index = 0;
        for (unsigned j = std::max(start, current_start) - count; j < std::min(end, current_end) - count; j++) {
          c.graph.move_child<> (liga_set_index,
                                &liga_set.table->ligature[j],
                                prime_id,
                                &prime->ligature[new_index++]);
        }

        liga_set_end = i;
        if (i < liga_set_start) liga_set_start = i;
        c.graph.add_link(&liga_subst_prime->ligatureSet[liga_set_count++], liga_subst_prime_id, prime_id);
      }
      
      count += num_ligas;
    }

    c.graph.vertices_[liga_subst_prime_id].obj.tail -= (liga_subst_prime->ligatureSet.len - liga_set_count) * SmallTypes::size;
    liga_subst_prime->ligatureSet.len = liga_set_count;

    unsigned coverage_id = c.graph.index_for_offset (this_index, &coverage);
    if (!Coverage::clone_coverage (c,
                                   coverage_id,
                                   liga_subst_prime_id,
                                   2,
                                   liga_set_start, liga_set_end))
      return -1;

    return liga_subst_prime_id;
  }

  bool shrink (gsubgpos_graph_context_t& c,
               unsigned this_index,
               unsigned old_count,
               unsigned count)
  {
    DEBUG_MSG (SUBSET_REPACK, nullptr,
               "  Shrinking LigatureSubstFormat1 (%u) to [0, %u).",
               this_index,
               count);
    if (count >= old_count)
      return true;                  

    unsigned new_liga_set_count = 0;
    for (unsigned i = 0; i < ligatureSet.len; i++)
    {
      auto liga_set = c.graph.as_table<LigatureSet>(this_index, &ligatureSet[i]);
      if (!liga_set.table) {
        return false;
      }

      unsigned num_ligas = liga_set.table->ligature.len;
      if (num_ligas <= count) {
        count -= num_ligas;
        continue;
      }

      if (count == 0) {
        // drop this liga set an all subsequent ones.
        new_liga_set_count = i;
        break;
      }

      // drop the trailing liga's from this set and all subsequent liga sets
      new_liga_set_count = i + 1;
      c.graph.vertices_[liga_set.index].obj.tail -= (num_ligas - count) * SmallTypes::size;
      liga_set.table->ligature.len = count;
      break;
    }

    // Adjust liga set array
    c.graph.vertices_[this_index].obj.tail -= (ligatureSet.len - new_liga_set_count) * SmallTypes::size;
    ligatureSet.len = new_liga_set_count;

    // Coverage matches the number of liga sets so rebuild as needed
    auto coverage = c.graph.as_mutable_table<Coverage> (this_index, &this->coverage);
    if (!coverage) return false;

    unsigned coverage_size = coverage.vertex->table_size ();
    auto new_coverage =
        + hb_zip (coverage.table->iter (), hb_range ())
        | hb_filter ([&] (hb_pair_t<unsigned, unsigned> p) {
          return p.second < new_liga_set_count;
        })
        | hb_map_retains_sorting (hb_first)
        ;

    return Coverage::make_coverage (c, new_coverage, coverage.index, coverage_size);
  }
};

struct LigatureSubst : public OT::Layout::GSUB_impl::LigatureSubst
{

  hb_vector_t<unsigned> split_subtables (gsubgpos_graph_context_t& c,
                                         unsigned parent_index,
                                         unsigned this_index)
  {
    switch (u.format) {
    case 1:
      return ((LigatureSubstFormat1*)(&u.format1))->split_subtables (c, parent_index, this_index);
#ifndef HB_NO_BEYOND_64K
    case 2: HB_FALLTHROUGH;
      // Don't split 24bit Ligature Subs
#endif
    default:
      return hb_vector_t<unsigned> ();
    }
  }

  bool sanitize (graph_t::vertex_t& vertex) const
  {
    int64_t vertex_len = vertex.obj.tail - vertex.obj.head;
    if (vertex_len < u.format.get_size ()) return false;
    hb_barrier ();

    switch (u.format) {
    case 1:
      return ((LigatureSubstFormat1*)(&u.format1))->sanitize (vertex);
#ifndef HB_NO_BEYOND_64K
    case 2:  HB_FALLTHROUGH;
#endif
    default:
      // We don't handle format 2 here.
      return false;
    }
  }
};

}

#endif  // GRAPH_LIGATURE_GRAPH_HH
