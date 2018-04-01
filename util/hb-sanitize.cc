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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf (stderr, "usage: %s font-file.ttf\n", argv[0]);
    return 1;
  }

  if (strcmp (argv[1], "--help") == 0)
  {
    printf ("Sanitizes a font against HarfBuzz tables sanitizers");
    return 0;
  }

  if (strcmp (argv[1], "--version") == 0)
  {
    printf ("%s (%s) %s\n", g_get_prgname (), PACKAGE_NAME, PACKAGE_VERSION);
    if (strcmp (HB_VERSION_STRING, hb_version_string ()))
      printf ("Linked HarfBuzz library has a different version: %s\n", hb_version_string ());

    return 0;
  }

  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  unsigned int faces = 1; // hb_face_count (blob); #1002

  unsigned int fails = 0;
  for (unsigned int face_index = 0; face_index < faces; ++face_index)
  {
    printf ("Face index: %d\n", face_index);
    hb_face_t *face = hb_face_create (blob, face_index);

    hb_tag_t tags[32];
    unsigned int table_count = sizeof (tags) / sizeof (hb_tag_t);
    hb_face_get_table_tags (face, 0, &table_count, tags);

    for (unsigned int table_index = 0; table_index < table_count; ++table_index)
    {
      char buf[4];
      hb_tag_to_string (tags[table_index], buf);
      hb_blob_t *sanitized = hb_sanitize (face, tags[table_index]);

      if (!sanitized)
	printf ("%.*s: %s\n", 4, buf, "SKIPPED!");
      else if (hb_blob_get_length (sanitized))
	printf ("%.*s: %s\n", 4, buf, "PASSED!");
      else
      {
	++fails;
	printf ("%.*s: %s\n", 4, buf, "FAILED!");
      }
    }

    hb_face_destroy (face);
  }

  hb_blob_destroy (blob);

  if (fails == 0)
    printf ("All the tables passed sanitization successfully.\n");
  else
    printf ("%d table(s) has failed on sanitization.\n", fails);

  return fails;
}
