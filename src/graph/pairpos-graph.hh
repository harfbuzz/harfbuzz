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
  unsigned get_size () const
  {
    return OT::Layout::GPOS_impl::PairPosFormat1_3<SmallTypes>::min_size
        + pairSet.get_size () - SmallTypes::size;
  }

  bool split_subtables (gsubgpos_graph_context_t& c, unsigned this_index)
  {
    printf("Checking if pair pos %u needs splits...\n", this_index);
    hb_set_t visited;
    const unsigned base_size = get_size ();
    printf("  base_size = %u\n", base_size);
    unsigned accumulated = base_size;
    // TODO: include coverage size
    unsigned num_pair_sets = pairSet.len;
    printf("  num_pair_sets = %u\n", num_pair_sets);
    for (unsigned i = 0; i < pairSet.len; i++)
    {
      unsigned pair_set_index = pair_set_graph_index (c, this_index, i);
      accumulated += c.graph.find_subgraph_size (pair_set_index, visited);

      if (accumulated > (1 << 16))
      {
        // TODO: do the split

        printf("  PairPos split needed %u/%u\n", i, num_pair_sets);
        accumulated = base_size;
      }
    }

    return true;
  }

 private:
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
