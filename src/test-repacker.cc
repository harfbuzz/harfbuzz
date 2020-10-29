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

static void
populate_serializer (hb_serialize_context_t* c)
{
  c->start_serialize<char> ();
  c->push ();
  char* obj = c->allocate_size<char> (3);
  strncpy (obj, "ghi", 3);
  unsigned obj_3 = c->pop_pack ();

  c->push ();
  obj = c->allocate_size<char> (3);
  strncpy (obj, "def", 3);
  unsigned obj_2 = c->pop_pack ();

  c->push ();
  obj = c->allocate_size<char> (3);
  strncpy (obj, "abc", 3);

  OT::Offset16* offset = c->start_embed<OT::Offset16> ();
  c->extend_min (offset);
  c->add_link (*offset, obj_2);

  offset = c->start_embed<OT::Offset16> ();
  c->extend_min (offset);
  c->add_link (*offset, obj_3);

  c->pop_pack ();

  c->end_serialize();
}

static void
test_serialize ()
{
  size_t buffer_size = 100;
  void* buffer_1 = malloc (buffer_size);
  hb_serialize_context_t c1 (buffer_1, buffer_size);
  populate_serializer (&c1);
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
}
