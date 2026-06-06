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
#include "../hb-ot-layout-common.hh"

#ifndef GRAPH_CLASSDEF_GRAPH_HH
#define GRAPH_CLASSDEF_GRAPH_HH

namespace graph {

template <typename Types>
struct ClassDefFormat1_3 : public OT::ClassDefFormat1_3<Types>
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    size_t vertex_len = vertex.obj.tail - vertex.obj.head;
    constexpr unsigned min_size = OT::ClassDefFormat1_3<Types>::min_size;
    if (vertex_len < min_size) return false;
    hb_barrier ();
    return vertex_len >= min_size + this->classValue.get_size () - this->classValue.len.get_size ();
  }
};

template <typename Types>
struct ClassDefFormat2_4 : public OT::ClassDefFormat2_4<Types>
{
  bool sanitize (graph_t::vertex_t& vertex) const
  {
    size_t vertex_len = vertex.obj.tail - vertex.obj.head;
    constexpr unsigned min_size = OT::ClassDefFormat2_4<Types>::min_size;
    if (vertex_len < min_size) return false;
    hb_barrier ();
    return vertex_len >= min_size + this->rangeRecord.get_size () - this->rangeRecord.len.get_size ();
  }
};

struct ClassDef : public OT::ClassDef
{
  template<typename It>
  static bool add_class_def (gsubgpos_graph_context_t& c,
                             unsigned parent_id,
                             unsigned link_position,
                             It glyph_and_class,
                             unsigned max_size)
  {
    unsigned class_def_prime_id = c.graph.new_node (nullptr, nullptr);
    auto& class_def_prime_vertex = c.graph.vertices_[class_def_prime_id];
    if (!make_class_def (c, glyph_and_class, class_def_prime_id, max_size))
      return false;

    auto* class_def_link = c.graph.vertices_[parent_id].obj.real_links.push ();
    class_def_link->width = SmallTypes::size;
    class_def_link->objidx = class_def_prime_id;
    class_def_link->position = link_position;
    class_def_prime_vertex.add_parent (parent_id, false);

    return true;
  }

  template<typename It>
  static bool make_class_def (gsubgpos_graph_context_t& c,
                              It glyph_and_class,
                              unsigned dest_obj,
                              unsigned max_size)
  {
    char* buffer = (char*) hb_calloc (1, max_size);
    hb_serialize_context_t serializer (buffer, max_size);
    OT::ClassDef_serialize (&serializer, glyph_and_class);
    serializer.end_serialize ();
    if (serializer.in_error ())
    {
      hb_free (buffer);
      return false;
    }

    hb_bytes_t class_def_copy = serializer.copy_bytes ();
    if (!class_def_copy.arrayZ) return false;
    // Give ownership to the context, it will cleanup the buffer.
    if (!c.add_buffer ((char *) class_def_copy.arrayZ))
    {
      hb_free ((char *) class_def_copy.arrayZ);
      return false;
    }

    auto& obj = c.graph.vertices_[dest_obj].obj;
    obj.head = (char *) class_def_copy.arrayZ;
    obj.tail = obj.head + class_def_copy.length;

    hb_free (buffer);
    return true;
  }

  bool sanitize (graph_t::vertex_t& vertex) const
  {
    size_t vertex_len = vertex.obj.tail - vertex.obj.head;
    if (vertex_len < OT::ClassDef::min_size) return false;
    hb_barrier ();
    switch (u.format.v)
    {
    case 1: return ((ClassDefFormat1_3<SmallTypes>*)this)->sanitize (vertex);
    case 2: return ((ClassDefFormat2_4<SmallTypes>*)this)->sanitize (vertex);
#ifndef HB_NO_BEYOND_64K
    case 3: return ((ClassDefFormat1_3<MediumTypes>*)this)->sanitize (vertex);
    case 4: return ((ClassDefFormat2_4<MediumTypes>*)this)->sanitize (vertex);
#endif
    default: return false;
    }
  }
};


struct class_def_size_estimator_t
{
  template<typename It>
  class_def_size_estimator_t (It glyph_and_class)
      : num_ranges_per_class (), glyphs_per_class ()
  {
    reset();
    for (auto p : + glyph_and_class)
    {
      unsigned gid = p.first;
      unsigned klass = p.second;

      hb_set_t* glyphs;
      if (glyphs_per_class.has (klass, &glyphs) && glyphs) {
        glyphs->add (gid);
        continue;
      }

      hb_set_t new_glyphs;
      new_glyphs.add (gid);
      glyphs_per_class.set (klass, std::move (new_glyphs));
    }

    if (in_error ()) return;

    for (unsigned klass : glyphs_per_class.keys ())
    {
      if (!klass) continue; // class 0 doesn't get encoded.

      const hb_set_t& glyphs = glyphs_per_class.get (klass);
      hb_codepoint_t start = HB_SET_VALUE_INVALID;
      hb_codepoint_t end = HB_SET_VALUE_INVALID;

      unsigned count = 0;
      while (glyphs.next_range (&start, &end))
        count++;

      num_ranges_per_class.set (klass, count);
    }
  }

  void reset() {
    included_glyphs.clear();
    included_class_def_glyphs.clear();
    included_classes.clear();
    class_def_range_count = 0;
  }

  // Compute the size of coverage for all glyphs added via 'add_class_def_size'.
  unsigned coverage_size () const
  {
    unsigned glyph_size = glyph_id_size (included_glyphs);
    unsigned coverage_base_size = OT::HBUINT16::static_size + glyph_size;
    unsigned range_size = 3 * glyph_size;
    unsigned format1_size = coverage_base_size + glyph_size * included_glyphs.get_population();
    unsigned format2_size = coverage_base_size + range_size * num_glyph_ranges (included_glyphs);
    return hb_min(format1_size, format2_size);
  }

  // Compute the new size of the ClassDef table if all glyphs associated with 'klass' were added.
  unsigned add_class_def_size (unsigned klass)
  {
    if (!included_classes.has(klass)) {
      hb_set_t* glyphs = nullptr;
      if (glyphs_per_class.has(klass, &glyphs)) {
        included_glyphs.union_(*glyphs);
        if (klass)
          included_class_def_glyphs.union_(*glyphs);
      }

      if (klass)
        class_def_range_count += num_ranges_per_class.get (klass);
      included_classes.add(klass);
    }

    return class_def_uses_format1 () ? class_def_format1_size () : class_def_format2_size ();
  }

  unsigned num_glyph_ranges (const hb_set_t &glyphs) const {
    hb_codepoint_t start = HB_SET_VALUE_INVALID;
    hb_codepoint_t end = HB_SET_VALUE_INVALID;

    unsigned count = 0;
    while (glyphs.next_range (&start, &end)) {
        count++;
    }
    return count;
  }

  bool in_error ()
  {
    if (num_ranges_per_class.in_error ()) return true;
    if (glyphs_per_class.in_error ()) return true;
    if (included_glyphs.in_error ()) return true;
    if (included_class_def_glyphs.in_error ()) return true;

    for (const hb_set_t& s : glyphs_per_class.values ())
    {
      if (s.in_error ()) return true;
    }
    return false;
  }

 private:
  unsigned glyph_id_size (const hb_set_t &glyphs) const
  {
#ifndef HB_NO_BEYOND_64K
    if (!glyphs.is_empty () && glyphs.get_max () > 0xFFFFu)
      return OT::Layout::MediumTypes::size;
#endif
    return OT::Layout::SmallTypes::size;
  }

  unsigned class_def_format1_size () const
  {
    unsigned glyph_size = glyph_id_size (included_class_def_glyphs);
    unsigned size = OT::HBUINT16::static_size + glyph_size + glyph_size;
    if (!included_class_def_glyphs.is_empty ())
      size += glyph_size * (included_class_def_glyphs.get_max () -
			    included_class_def_glyphs.get_min () + 1);
    return size;
  }

  unsigned class_def_format2_size () const
  {
    unsigned glyph_size = glyph_id_size (included_class_def_glyphs);
    unsigned range_size = 2 * glyph_size + OT::HBUINT16::static_size;
    unsigned size = OT::HBUINT16::static_size + glyph_size;
    if (included_class_def_glyphs.is_empty ())
      return OT::HBUINT16::static_size + OT::Layout::SmallTypes::size;
    return size + range_size * class_def_range_count;
  }

  bool class_def_uses_format1 () const
  {
    if (included_class_def_glyphs.is_empty ()) return false;
    return 1 + (included_class_def_glyphs.get_max () -
		included_class_def_glyphs.get_min () + 1) <=
	   class_def_range_count * 3;
  }

  hb_hashmap_t<unsigned, unsigned> num_ranges_per_class;
  hb_hashmap_t<unsigned, hb_set_t> glyphs_per_class;
  hb_set_t included_classes;
  hb_set_t included_glyphs;
  hb_set_t included_class_def_glyphs;
  unsigned class_def_range_count;
};


}

#endif  // GRAPH_CLASSDEF_GRAPH_HH
