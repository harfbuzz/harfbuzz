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

#ifndef GRAPH_PAIRPOS_GRAPH_HH
#define GRAPH_PAIRPOS_GRAPH_HH

#include "../OT/Layout/GPOS/PairPos.hh"
#include "../OT/Layout/GPOS/PosLookupSubTable.hh"

namespace graph {

struct PairPosFormat1 : public OT::Layout::GPOS_impl::PairPosFormat1_3<SmallTypes>
{
  bool split_subtables (gsubgpos_graph_context_t& c, unsigned this_index)
  {
    printf("Checking if pair pos %u needs splits...\n", this_index);
    hb_set_t visited;
    const unsigned base_size = OT::Layout::GPOS_impl::PairPosFormat1_3<SmallTypes>::min_size;
    printf("  base_size = %u\n", base_size);
    unsigned accumulated = base_size;
    // TODO: include coverage size
    unsigned num_pair_sets = pairSet.len;
    printf("  num_pair_sets = %u\n", num_pair_sets);
    hb_vector_t<unsigned> split_points;
    for (unsigned i = 0; i < pairSet.len; i++)
    {
      unsigned pair_set_index = pair_set_graph_index (c, this_index, i);
      accumulated += c.graph.find_subgraph_size (pair_set_index, visited);
      accumulated += SmallTypes::size; // for PairSet offset.

      if (accumulated > (1 << 15)) // TODO (1 << 16)
      {
        printf("  PairPos split needed %u/%u\n", i, num_pair_sets);
        split_points.push (i);
        accumulated = base_size;
      }
    }

    // TODO: do the split
    do_split (c, this_index, split_points);

    return true;
  }

 private:

  // Split this PairPos into two or more PairPos's. split_points defines
  // the indices (first index to include in the new table) to split at.
  // Returns the object id's of the newly created PairPos subtables.
  hb_vector_t<unsigned> do_split (gsubgpos_graph_context_t& c,
                                  unsigned this_index,
                                  const hb_vector_t<unsigned> split_points)
  {
    hb_vector_t<unsigned> new_objects;
    if (!split_points)
      return new_objects;

    for (unsigned i = 0; i < split_points.length; i++)
    {
      unsigned start = split_points[i];
      unsigned end = (i < split_points.length - 1) ? split_points[i + 1] : pairSet.len;
      new_objects.push (clone_range (c, this_index, start, end));
      // TODO error checking.
    }

    shrink (split_points[0]);

    return new_objects;
  }

  void shrink (unsigned size)
  {
    printf("  shrink to [0, %u).\n", size);
    // TODO
  }

  // Create a new PairPos including PairSet's from start (inclusive) to end (exclusive).
  // Returns object id of the new object.
  unsigned clone_range (gsubgpos_graph_context_t& c,
                        unsigned this_index,
                        unsigned start, unsigned end) const
  {
    printf("  cloning range [%u, %u).\n", start, end);

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


    // TODO: serialize a new coverage table.
    return pair_pos_prime_id;
  }

  unsigned pair_set_graph_index (gsubgpos_graph_context_t& c, unsigned this_index, unsigned i) const
  {
    return c.graph.index_for_offset (this_index, &pairSet[i]);
  }
};

struct PairPosFormat2 : public OT::Layout::GPOS_impl::PairPosFormat2_4<SmallTypes>
{
  bool split_subtables (gsubgpos_graph_context_t& c, unsigned this_index)
  {
    // TODO
    return true;
  }
};

struct PairPos : public OT::Layout::GPOS_impl::PairPos
{
  bool split_subtables (gsubgpos_graph_context_t& c, unsigned this_index)
  {
    unsigned format = u.format;
    printf("PairPos::format = %u\n", format);
    switch (u.format) {
    case 1:
      return ((PairPosFormat1*)(&u.format1))->split_subtables (c, this_index);
    case 2:
      return ((PairPosFormat2*)(&u.format2))->split_subtables (c, this_index);
#ifndef HB_NO_BORING_EXPANSION
    case 3:
    case 4:
      // Don't split 24bit PairPos's.
#endif
    default:
      return true;
    }
  }
};

}

#endif  // GRAPH_PAIRPOS_GRAPH_HH
