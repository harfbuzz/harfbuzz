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

#include "hb-repacker.hh"
#include "hb-open-type.hh"

static void start_object(const char* tag,
                         unsigned len,
                         hb_serialize_context_t* c)
{
  c->push ();
  char* obj = c->allocate_size<char> (len);
  strncpy (obj, tag, len);
}


static unsigned add_object(const char* tag,
                           unsigned len,
                           hb_serialize_context_t* c)
{
  start_object (tag, len, c);
  return c->pop_pack (false);
}


static void add_offset (unsigned id,
                        hb_serialize_context_t* c)
{
  OT::Offset16* offset = c->start_embed<OT::Offset16> ();
  c->extend_min (offset);
  c->add_link (*offset, id);
}

static void
populate_serializer_simple (hb_serialize_context_t* c)
{
  c->start_serialize<char> ();

  unsigned obj_1 = add_object ("ghi", 3, c);
  unsigned obj_2 = add_object ("def", 3, c);

  start_object ("abc", 3, c);
  add_offset (obj_2, c);
  add_offset (obj_1, c);
  c->pop_pack ();

  c->end_serialize();
}

static void
populate_serializer_complex (hb_serialize_context_t* c)
{
  c->start_serialize<char> ();

  unsigned obj_4 = add_object ("jkl", 3, c);
  unsigned obj_3 = add_object ("ghi", 3, c);

  start_object ("def", 3, c);
  add_offset (obj_3, c);
  unsigned obj_2 = c->pop_pack (false);

  start_object ("abc", 3, c);
  add_offset (obj_2, c);
  add_offset (obj_4, c);
  c->pop_pack ();

  c->end_serialize();
}

static void test_sort_bfs ()
{
  size_t buffer_size = 100;
  void* buffer = malloc (buffer_size);
  hb_serialize_context_t c (buffer, buffer_size);
  populate_serializer_complex (&c);

  graph_t graph (c.object_graph ());
  graph.sort_bfs ();

  assert(strncmp (graph.objects_[3].head, "abc", 3) == 0);
  assert(graph.objects_[3].links.length == 2);
  assert(graph.objects_[3].links[0].objidx == 2);
  assert(graph.objects_[3].links[1].objidx == 1);

  assert(strncmp (graph.objects_[2].head, "def", 3) == 0);
  assert(graph.objects_[2].links.length == 1);
  assert(graph.objects_[2].links[0].objidx == 0);

  assert(strncmp (graph.objects_[1].head, "jkl", 3) == 0);
  assert(graph.objects_[1].links.length == 0);

  assert(strncmp (graph.objects_[0].head, "ghi", 3) == 0);
  assert(graph.objects_[0].links.length == 0);
}

static void
test_serialize ()
{
  size_t buffer_size = 100;
  void* buffer_1 = malloc (buffer_size);
  hb_serialize_context_t c1 (buffer_1, buffer_size);
  populate_serializer_simple (&c1);
  hb_bytes_t expected = c1.copy_bytes ();

  void* buffer_2 = malloc (buffer_size);
  hb_serialize_context_t c2 (buffer_2, buffer_size);

  graph_t graph (c1.object_graph ());
  graph.serialize (&c2);
  hb_bytes_t actual = c2.copy_bytes ();

  assert (actual == expected);

  free (buffer_1);
  free (buffer_2);
}

int
main (int argc, char **argv)
{
  test_serialize ();
  test_sort_bfs ();
}
