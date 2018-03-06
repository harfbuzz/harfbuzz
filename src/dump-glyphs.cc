/*
 * Copyright © 2018  Ebrahim Byagowi
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
 */

#include "hb-static.cc"
#include "hb-ot-glyf-table.hh"

int
main (int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf (stderr, "usage: %s font-file.ttf glyph-id\n", argv[0]);
    exit (1);
  }

  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  unsigned int num_faces = hb_face_count (blob);
  if (num_faces == 0)
  {
    fprintf (stderr, "error: The file (%s) was corrupted, empty or not found", argv[1]);
    exit (1);
  }
  hb_face_t *face = hb_face_create (blob, 0);

  OT::glyf::accelerator_t glyf;
  glyf.init (face);
  glyf.glyph_coordinates (atoi (argv[2]));
  glyf.fini ();

  hb_face_destroy (face);
}
