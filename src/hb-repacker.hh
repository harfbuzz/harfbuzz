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
#include "hb-serialize.hh"
#include "hb-vector.hh"


struct graph_t
{
  /*
   * A topological sorting of an object graph. Ordered
   * in reverse serialization order (first object in the
   * serialization is at the end of the graph). This matches
   * the 'packed' object stack used internally in the
   * serializer
   */
  graph_t (const hb_vector_t<hb_serialize_context_t::object_t *>& objects)
      : objects_ (objects)
  {}

  /*
   * serialize graph into the provided serialization buffer.
   */
  void serialize (hb_serialize_context_t* c)
  {
    c->start_serialize<void> ();
    for (unsigned i = 0; i < objects_.length; i++) {
      if (!objects_[i]) continue;

      c->push ();

      size_t size = objects_[i]->tail - objects_[i]->head;
      char* start = c->allocate_size <char> (size);
      if (!start) return;

      memcpy (start, objects_[i]->head, size);

      for (const auto& link : objects_[i]->links)
        serialize_link (link, start, c);

      c->pop_pack (false);
    }
    c->end_serialize ();
  }

  /*
   * Generates a new topological sorting of graph using BFS.
   */
  void sort_bfs ()
  {
    hb_vector_t<int> queue;
    hb_vector_t<hb_serialize_context_t::object_t *> sorted_graph;

    // Object graphs are in reverse order, the first object is at the end
    // of the vector.
    queue.push (objects_.length - 1);

    hb_set_t visited;
    while (queue.length)
    {
      int next_id = queue[0];
      queue.remove(0);
      visited.add(next_id);

      hb_serialize_context_t::object_t* next = objects_[next_id];
      sorted_graph.push (next);

      for (const auto& link : next->links) {
        if (!visited.has (link.objidx))
          queue.push (link.objidx);
      }
    }

    sorted_graph.as_array ().reverse ();
    objects_ = sorted_graph;
    // TODO(garretrieger): remap object id's on the links.
    // TODO(garretrieger): what order should graphs be in (first object at the end? or the beginning)

    // TODO(garretrieger): check that all objects made it over into the sorted copy
    //                     (ie. all objects are connected in the original graph).
  }

  /*
   * Will any offsets overflow on graph when it's serialized?
   */
  bool will_overflow()
  {
    // TODO(garretrieger): implement me.
    return false;
  }

 private:

  template <typename O> void
  serialize_link_of_type (const hb_serialize_context_t::object_t::link_t& link,
                          char* head,
                          hb_serialize_context_t* c)
  {
    OT::Offset<O>* offset = reinterpret_cast<OT::Offset<O>*> (head + link.position);
    *offset = 0;
    c->add_link (*offset,
                 link.objidx,
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

  hb_vector_t<hb_serialize_context_t::object_t *> objects_;
};


/*
 * Re-serialize the provided object graph into the serialization context
 * using BFS (Breadth First Search) to produce the topological ordering.
 */
inline void
hb_resolve_overflows (const hb_vector_t<hb_serialize_context_t::object_t *>& packed,
                      hb_serialize_context_t* c) {
  graph_t sorted_graph (packed);
  sorted_graph.sort_bfs ();
  if (sorted_graph.will_overflow ()) {
    // TODO(garretrieger): additional offset resolution strategies
    // - Promotion to extension lookups.
    // - Table duplication.
    // - Table splitting.
  }

  sorted_graph.serialize (c);
}


#endif /* HB_REPACKER_HH */
