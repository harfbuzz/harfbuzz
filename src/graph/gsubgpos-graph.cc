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

#include "gsubgpos-graph.hh"

namespace graph {

make_extension_context_t::make_extension_context_t (hb_tag_t table_tag_,
                                                    graph_t& graph_,
                                                    hb_vector_t<char>& buffer_)
    : table_tag (table_tag_),
      graph (graph_),
      buffer (buffer_),
      lookup_list_index (0),
      lookups ()
{
  GSTAR* gstar = graph::GSTAR::graph_to_gstar (graph_);
  if (gstar) {
    gstar->find_lookups (graph, lookups);
    lookup_list_index = gstar->get_lookup_list_index (graph_);
  }

  unsigned extension_size = OT::ExtensionFormat1<OT::Layout::GSUB_impl::ExtensionSubst>::static_size;
  buffer.alloc (num_non_ext_subtables () * extension_size);
}

unsigned make_extension_context_t::num_non_ext_subtables ()  {
  unsigned count = 0;
  for (auto l : lookups.values ())
  {
    if (l->is_extension (table_tag)) continue;
    count += l->number_of_subtables ();
  }
  return count;
}

}
