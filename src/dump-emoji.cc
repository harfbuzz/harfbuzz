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

#include "hb.h"
#include "hb-private.hh"
#include "hb-ot-color-cbdt-table.hh"
#include "hb-ot-color-sbix-table.hh"
#include "hb-ot-color-svg-table.hh"

#ifdef HAVE_GLIB
#include <glib.h>
#endif
#include <stdlib.h>
#include <stdio.h>

#ifndef HB_NO_VISIBILITY
const void * const OT::_hb_NullPool[HB_NULL_POOL_SIZE / sizeof (void *)] = {};
#endif

void cbdt_callback (const uint8_t* data, unsigned int length,
                    unsigned int group, unsigned int gid)
{
  char outName[255];
  sprintf (outName, "out/cbdt-%d-%d.png", group, gid);
  FILE *f = fopen (outName, "wb");
  fwrite (data, 1, length, f);
  fclose (f);
}

void sbix_callback (const uint8_t* data, unsigned int length,
                    unsigned int group, unsigned int gid)
{
  char outName[255];
  sprintf (outName, "out/sbix-%d-%d.png", group, gid);
  FILE *f = fopen (outName, "wb");
  fwrite (data, 1, length, f);
  fclose (f);
}

void svg_callback (const uint8_t* data, unsigned int length,
                   unsigned int start_glyph, unsigned int end_glyph)
{
  char outName[255];
  if (start_glyph == end_glyph)
    sprintf (outName, "out/svg-%d.svg", start_glyph);
  else
    sprintf (outName, "out/svg-%d-%d.svg", start_glyph, end_glyph);

  // append "z" if the content is gzipped
  if ((data[0] == 0x1F) && (data[1] == 0x8B))
    strcat (outName, "z");

  FILE *f = fopen (outName, "wb");
  fwrite (data, 1, length, f);
  fclose (f);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s font-file.ttf\n", argv[0]);
    exit (1);
  }

  hb_blob_t *blob = nullptr;
  {
    const char *font_data;
    unsigned int len;
    hb_destroy_func_t destroy;
    void *user_data;
    hb_memory_mode_t mm;

#ifdef HAVE_GLIB
    GMappedFile *mf = g_mapped_file_new (argv[1], false, nullptr);
    font_data = g_mapped_file_get_contents (mf);
    len = g_mapped_file_get_length (mf);
    destroy = (hb_destroy_func_t) g_mapped_file_unref;
    user_data = (void *) mf;
    mm = HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE;
#else
    FILE *f = fopen (argv[1], "rb");
    fseek (f, 0, SEEK_END);
    len = ftell (f);
    fseek (f, 0, SEEK_SET);
    font_data = (const char *) malloc (len);
    if (!font_data) len = 0;
    len = fread ((char *) font_data, 1, len, f);
    destroy = free;
    user_data = (void *) font_data;
    fclose (f);
    mm = HB_MEMORY_MODE_WRITABLE;
#endif

    blob = hb_blob_create (font_data, len, mm, user_data, destroy);
  }

  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  OT::CBDT::accelerator_t cbdt;
  cbdt.init (face);
  cbdt.dump (cbdt_callback);
  cbdt.fini ();

  OT::sbix::accelerator_t sbix;
  sbix.init (face);
  sbix.dump (sbix_callback);
  sbix.fini ();

  OT::SVG::accelerator_t svg;
  svg.init (face);
  svg.dump (svg_callback);
  svg.fini ();

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
