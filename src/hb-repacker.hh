/*
 * Copyright © 2020  Google, Inc.
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

#ifndef HB_REPACKER_HH
#define HB_REPACKER_HH

#include "hb-open-type.hh"
#include "hb-map.hh"
#include "hb-serialize.hh"
#include "hb-vector.hh"


struct graph_t
{
  // TODO(garretrieger): add an error tracking system similar to what serialize_context_t
  //                     does.

  /*
   * A topological sorting of an object graph. Ordered
   * in reverse serialization order (first object in the
   * serialization is at the end of the list). This matches
   * the 'packed' object stack used internally in the
   * serializer
   */
  graph_t (const hb_vector_t<hb_serialize_context_t::object_t *>& objects)
  {
    bool removed_nil = false;
    for (unsigned i = 0; i < objects.length; i++)
    {
      // If this graph came from a serialization buffer object 0 is the
      // nil object. We don't need it for our purposes here so drop it.
      if (i == 0 && !objects[i])
      {
        removed_nil = true;
        continue;
      }

      auto* copy = objects_.push (*objects[i]);
      if (!removed_nil) continue;
      for (unsigned i = 0; i < copy->links.length; i++)
        // Fix indices to account for removed nil object.
        copy->links[i].objidx--;
    }
  }

  ~graph_t ()
  {
    objects_.fini_deep ();
  }

  /*
   * serialize graph into the provided serialization buffer.
   */
  void serialize (hb_serialize_context_t* c)
  {
    c->start_serialize<void> ();
    for (unsigned i = 0; i < objects_.length; i++) {
      c->push ();

      size_t size = objects_[i].tail - objects_[i].head;
      char* start = c->allocate_size <char> (size);
      if (!start) return;

      memcpy (start, objects_[i].head, size);

      for (const auto& link : objects_[i].links)
        serialize_link (link, start, c);

      c->pop_pack (false);
    }
    c->end_serialize ();
  }

  /*
   * Generates a new topological sorting of graph using Kahn's
   * algorithm: https://en.wikipedia.org/wiki/Topological_sorting#Algorithms
   */
  void sort_kahn ()
  {
    if (objects_.length <= 1) {
      // Graph of 1 or less doesn't need sorting.
      return;
    }

    hb_vector_t<unsigned> queue;
    hb_vector_t<hb_serialize_context_t::object_t> sorted_graph;
    hb_map_t id_map;
    hb_map_t edge_count;
    incoming_edge_count (&edge_count);

    // Object graphs are in reverse order, the first object is at the end
    // of the vector. Since the graph is topologically sorted it's safe to
    // assume the first object has no incoming edges.
    queue.push (objects_.length - 1);
    int new_id = objects_.length - 1;

    while (queue.length)
    {
      unsigned next_id = queue[0];
      queue.remove(0);

      hb_serialize_context_t::object_t& next = objects_[next_id];
      sorted_graph.push (next);
      id_map.set (next_id, new_id--);

      for (const auto& link : next.links) {
        // TODO(garretrieger): sort children from smallest to largest
        edge_count.set (link.objidx, edge_count.get (link.objidx) - 1);
        if (!edge_count.get (link.objidx))
          queue.push (link.objidx);
      }
    }

    if (new_id != -1)
    {
      // Graph is not fully connected, there are unsorted objects.
      // TODO(garretrieger): handle this.
      assert (false);
    }

    remap_obj_indices (id_map, &sorted_graph);

    sorted_graph.as_array ().reverse ();
    objects_ = sorted_graph;
    sorted_graph.fini_deep ();
  }

  /*
   * Will any offsets overflow on graph when it's serialized?
   */
  bool will_overflow()
  {
    // TODO(garretrieger): implement me.
    // - Check for offsets that exceed their width or;
    // - are negative if using a non-signed link.
    return false;
  }

 private:

  /*
   * Updates all objidx's in all links using the provided mapping.
   */
  void remap_obj_indices (const hb_map_t& id_map,
                          hb_vector_t<hb_serialize_context_t::object_t>* sorted_graph)
  {
    for (unsigned i = 0; i < sorted_graph->length; i++)
    {
      for (unsigned j = 0; j < (*sorted_graph)[i].links.length; j++)
      {
        auto& link = (*sorted_graph)[i].links[j];
        if (!id_map.has (link.objidx))
          // TODO(garretrieger): handle this.
          assert (false);
        link.objidx = id_map.get (link.objidx);
      }
    }
  }

  /*
   * Creates a map from objid to # of incoming edges.
   */
  void incoming_edge_count (hb_map_t* out)
  {
    for (unsigned i = 0; i < objects_.length; i++)
    {
      if (!out->has (i))
        out->set (i, 0);

      for (const auto& l : objects_[i].links)
      {
        unsigned id = l.objidx;
        if (out->has (id))
          out->set (id, out->get (id) + 1);
        else
          out->set (id, 1);
      }
    }
  }

  template <typename O> void
  serialize_link_of_type (const hb_serialize_context_t::object_t::link_t& link,
                          char* head,
                          hb_serialize_context_t* c)
  {
    OT::Offset<O>* offset = reinterpret_cast<OT::Offset<O>*> (head + link.position);
    *offset = 0;
    c->add_link (*offset,
                 // serializer has an extra nil object at the start of the
                 // object array. So all id's are +1 of what our id's are.
                 link.objidx + 1,
                 (hb_serialize_context_t::whence_t) link.whence,
                 link.bias);
  }

  void serialize_link (const hb_serialize_context_t::object_t::link_t& link,
                 char* head,
                 hb_serialize_context_t* c)
  {
    if (link.is_wide)
    {
      if (link.is_signed)
      {
        serialize_link_of_type<OT::HBINT32> (link, head, c);
      } else {
        serialize_link_of_type<OT::HBUINT32> (link, head, c);
      }
    } else {
      if (link.is_signed)
      {
        serialize_link_of_type<OT::HBINT16> (link, head, c);
      } else {
        serialize_link_of_type<OT::HBUINT16> (link, head, c);
      }
    }
  }

 public:
  hb_vector_t<hb_serialize_context_t::object_t> objects_;
};


/*
 * Re-serialize the provided object graph into the serialization context
 * using BFS (Breadth First Search) to produce the topological ordering.
 */
inline void
hb_resolve_overflows (const hb_vector_t<hb_serialize_context_t::object_t *>& packed,
                      hb_serialize_context_t* c) {
  graph_t sorted_graph (packed);
  sorted_graph.sort_kahn ();
  if (sorted_graph.will_overflow ()) {
    // TODO(garretrieger): try additional offset resolution strategies
    // - Dijkstra sort of weighted graph.
    // - Promotion to extension lookups.
    // - Table duplication.
    // - Table splitting.
  }

  sorted_graph.serialize (c);
}


#endif /* HB_REPACKER_HH */
