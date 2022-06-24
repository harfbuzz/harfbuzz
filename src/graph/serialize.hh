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

#ifndef GRAPH_SERIALIZE_HH
#define GRAPH_SERIALIZE_HH

namespace graph {

template <typename O> inline void
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

inline
void serialize_link (const hb_serialize_context_t::object_t::link_t& link,
                     char* head,
                     hb_serialize_context_t* c)
{
  switch (link.width)
  {
    case 0:
      // Virtual links aren't serialized.
      return;
    case 4:
      if (link.is_signed)
      {
        serialize_link_of_type<OT::HBINT32> (link, head, c);
      } else {
        serialize_link_of_type<OT::HBUINT32> (link, head, c);
      }
      return;
    case 2:
      if (link.is_signed)
      {
        serialize_link_of_type<OT::HBINT16> (link, head, c);
      } else {
        serialize_link_of_type<OT::HBUINT16> (link, head, c);
      }
      return;
    case 3:
      serialize_link_of_type<OT::HBUINT24> (link, head, c);
      return;
    default:
      // Unexpected link width.
      assert (0);
  }
}

/*
 * serialize graph into the provided serialization buffer.
 */
inline hb_blob_t* serialize (const graph_t& graph)
{
  hb_vector_t<char> buffer;
  size_t size = graph.total_size_in_bytes ();
  if (!buffer.alloc (size)) {
    DEBUG_MSG (SUBSET_REPACK, nullptr, "Unable to allocate output buffer.");
    return nullptr;
  }
  hb_serialize_context_t c((void *) buffer, size);

  c.start_serialize<void> ();
  const auto& vertices = graph.vertices_;
  for (unsigned i = 0; i < vertices.length; i++) {
    c.push ();

    size_t size = vertices[i].obj.tail - vertices[i].obj.head;
    char* start = c.allocate_size <char> (size);
    if (!start) {
      DEBUG_MSG (SUBSET_REPACK, nullptr, "Buffer out of space.");
      return nullptr;
    }

    memcpy (start, vertices[i].obj.head, size);

    // Only real links needs to be serialized.
    for (const auto& link : vertices[i].obj.real_links)
      serialize_link (link, start, &c);

    // All duplications are already encoded in the graph, so don't
    // enable sharing during packing.
    c.pop_pack (false);
  }
  c.end_serialize ();

  if (c.in_error ()) {
    DEBUG_MSG (SUBSET_REPACK, nullptr, "Error during serialization. Err flag: %d",
               c.errors);
    return nullptr;
  }

  return c.copy_blob ();
}

} // namespace graph

#endif // GRAPH_SERIALIZE_HH
