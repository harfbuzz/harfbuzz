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

#include "graph.hh"
#include "../hb-ot-layout-gsubgpos.hh"
#include "../OT/Layout/GSUB/ExtensionSubst.hh"

#ifndef GRAPH_GSUBGPOS_GRAPH_HH
#define GRAPH_GSUBGPOS_GRAPH_HH

namespace graph {

struct Lookup;

struct make_extension_context_t
{
  hb_tag_t table_tag;
  graph_t& graph;
  hb_vector_t<char>& buffer;
  unsigned lookup_list_index;
  hb_hashmap_t<unsigned, graph::Lookup*> lookups;

  HB_INTERNAL make_extension_context_t (hb_tag_t table_tag_,
                                        graph_t& graph_,
                                        hb_vector_t<char>& buffer_);

  bool in_error () const
  {
    return buffer.in_error ();
  }

 private:
  HB_INTERNAL unsigned num_non_ext_subtables ();
};

template<typename T>
struct ExtensionFormat1 : public OT::ExtensionFormat1<T>
{
  void reset(unsigned type)
  {
    this->format = 1;
    this->extensionLookupType = type;
    this->extensionOffset = 0;
  }
};

struct Lookup : public OT::Lookup
{
  unsigned number_of_subtables () const
  {
    return subTable.len;
  }

  bool sanitize (graph_t::vertex_t& vertex) const
  {
    int64_t vertex_len = vertex.obj.tail - vertex.obj.head;
    if (vertex_len < OT::Lookup::min_size) return false;
    return vertex_len >= this->get_size ();
  }

  bool is_extension (hb_tag_t table_tag) const
  {
    return lookupType == extension_type (table_tag);
  }

  bool make_extension (make_extension_context_t& c,
                       unsigned this_index)
  {
    unsigned type = lookupType;
    unsigned ext_type = extension_type (c.table_tag);
    if (!ext_type || is_extension (c.table_tag))
    {
      // NOOP
      return true;
    }

    DEBUG_MSG (SUBSET_REPACK, nullptr,
               "Promoting lookup type %u (obj %u) to extension.",
               type,
               this_index);

    for (unsigned i = 0; i < subTable.len; i++)
    {
      unsigned subtable_index = c.graph.index_for_offset (this_index, &subTable[i]);
      if (!make_subtable_extension (c,
                                    this_index,
                                    subtable_index))
        return false;
    }

    lookupType = ext_type;
    return true;
  }

  bool make_subtable_extension (make_extension_context_t& c,
                                unsigned lookup_index,
                                unsigned subtable_index)
  {
    unsigned type = lookupType;
    unsigned extension_size = OT::ExtensionFormat1<OT::Layout::GSUB_impl::ExtensionSubst>::static_size;
    unsigned start = c.buffer.length;
    unsigned end = start + extension_size;
    if (!c.buffer.resize (c.buffer.length + extension_size))
      return false;

    ExtensionFormat1<OT::Layout::GSUB_impl::ExtensionSubst>* extension =
        (ExtensionFormat1<OT::Layout::GSUB_impl::ExtensionSubst>*) &c.buffer[start];
    extension->reset (type);

    unsigned ext_index = c.graph.new_node (&c.buffer.arrayZ[start],
                                           &c.buffer.arrayZ[end]);
    if (ext_index == (unsigned) -1) return false;

    auto& lookup_vertex = c.graph.vertices_[lookup_index];
    for (auto& l : lookup_vertex.obj.real_links.writer ())
    {
      if (l.objidx == subtable_index)
        // Change lookup to point at the extension.
        l.objidx = ext_index;
    }

    // Make extension point at the subtable.
    auto& ext_vertex = c.graph.vertices_[ext_index];
    auto& subtable_vertex = c.graph.vertices_[subtable_index];
    auto* l = ext_vertex.obj.real_links.push ();

    l->width = 4;
    l->objidx = subtable_index;
    l->is_signed = 0;
    l->whence = 0;
    l->position = 4;
    l->bias = 0;

    ext_vertex.parents.push (lookup_index);
    subtable_vertex.remap_parent (lookup_index, ext_index);

    return true;
  }

 private:
  unsigned extension_type (hb_tag_t table_tag) const
  {
    switch (table_tag)
    {
    case HB_OT_TAG_GPOS: return 9;
    case HB_OT_TAG_GSUB: return 7;
    default: return 0;
    }
  }
};

template <typename T>
struct LookupList : public OT::LookupList<T>
{
  bool sanitize (const graph_t::vertex_t& vertex) const
  {
    int64_t vertex_len = vertex.obj.tail - vertex.obj.head;
    if (vertex_len < OT::LookupList<T>::min_size) return false;
    return vertex_len >= OT::LookupList<T>::item_size * this->len;
  }
};

struct GSTAR : public OT::GSUBGPOS
{
  static GSTAR* graph_to_gstar (graph_t& graph)
  {
    const auto& r = graph.root ();

    GSTAR* gstar = (GSTAR*) r.obj.head;
    if (!gstar->sanitize (r))
      return nullptr;

    return gstar;
  }

  const void* get_lookup_list_field_offset () const
  {
    switch (u.version.major) {
    case 1: return u.version1.get_lookup_list_offset ();
#ifndef HB_NO_BORING_EXPANSION
    case 2: return u.version2.get_lookup_list_offset ();
#endif
    default: return 0;
    }
  }

  bool sanitize (const graph_t::vertex_t& vertex)
  {
    int64_t len = vertex.obj.tail - vertex.obj.head;
    if (len < OT::GSUBGPOS::min_size) return false;
    return len >= get_size ();
  }

  void find_lookups (graph_t& graph,
                     hb_hashmap_t<unsigned, Lookup*>& lookups /* OUT */)
  {
    switch (u.version.major) {
      case 1: find_lookups<SmallTypes> (graph, lookups); break;
#ifndef HB_NO_BORING_EXPANSION
      case 2: find_lookups<MediumTypes> (graph, lookups); break;
#endif
    }
  }

  unsigned get_lookup_list_index (graph_t& graph)
  {
    return graph.index_for_offset (graph.root_idx (),
                                   get_lookup_list_field_offset());
  }

  template<typename Types>
  void find_lookups (graph_t& graph,
                     hb_hashmap_t<unsigned, Lookup*>& lookups /* OUT */)
  {
    unsigned lookup_list_idx = get_lookup_list_index (graph);

    const LookupList<Types>* lookupList =
        (const LookupList<Types>*) graph.object (lookup_list_idx).head;
    if (!lookupList->sanitize (graph.vertices_[lookup_list_idx]))
      return;

    for (unsigned i = 0; i < lookupList->len; i++)
    {
      unsigned lookup_idx = graph.index_for_offset (lookup_list_idx, &(lookupList->arrayZ[i]));
      Lookup* lookup = (Lookup*) graph.object (lookup_idx).head;
      if (!lookup->sanitize (graph.vertices_[lookup_idx])) continue;
      lookups.set (lookup_idx, lookup);
    }
  }
};




}

#endif  /* GRAPH_GSUBGPOS_GRAPH_HH */
