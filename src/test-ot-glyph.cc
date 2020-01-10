/*
 * Copyright © 2019  Ebrahim Byagowi
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

#include "hb-ot.h"

#ifdef HB_NO_OPEN
#define hb_blob_create_from_file(x)  hb_blob_get_empty ()
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct user_data_t
{
  FILE *f;
  hb_position_t ascender;
};

static void
move_to (hb_position_t to_x, hb_position_t to_y, void *user_data)
{
  user_data_t u = *((user_data_t *) user_data);
  fprintf (u.f, "M%d,%d", to_x, u.ascender - to_y);
}

static void
line_to (hb_position_t to_x, hb_position_t to_y, void *user_data)
{
  user_data_t u = *((user_data_t *) user_data);
  fprintf (u.f, "L%d,%d", to_x, u.ascender - to_y);
}

static void
conic_to (hb_position_t control_x, hb_position_t control_y,
	  hb_position_t to_x, hb_position_t to_y,
	  void *user_data)
{
  user_data_t u = *((user_data_t *) user_data);
  fprintf (u.f, "Q%d,%d %d,%d", control_x, u.ascender - control_y,
				to_x, u.ascender - to_y);
}

static void
cubic_to (hb_position_t control1_x, hb_position_t control1_y,
	  hb_position_t control2_x, hb_position_t control2_y,
	  hb_position_t to_x, hb_position_t to_y,
	  void *user_data)
{
  user_data_t u = *((user_data_t *) user_data);
  fprintf (u.f, "C%d,%d %d,%d %d,%d", control1_x, u.ascender - control1_y,
				      control2_x, u.ascender - control2_y,
				      to_x, u.ascender - to_y);
}

int
main (int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf (stderr, "usage: %s font-file.ttf\n", argv[0]);
    exit (1);
  }

  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  unsigned int num_faces = hb_face_count (blob);
  if (num_faces == 0)
  {
    fprintf (stderr, "error: The file (%s) was corrupted, empty or not found", argv[1]);
    exit (1);
  }

  hb_ot_glyph_decompose_funcs_t funcs;
  funcs.move_to = (hb_ot_glyph_decompose_move_to_func_t) move_to;
  funcs.line_to = (hb_ot_glyph_decompose_line_to_func_t) line_to;
  funcs.conic_to = (hb_ot_glyph_decompose_conic_to_func_t) conic_to;
  funcs.cubic_to = (hb_ot_glyph_decompose_cubic_to_func_t) cubic_to;

  for (unsigned int face_index = 0; face_index < hb_face_count (blob); face_index++)
  {
    hb_face_t *face = hb_face_create (blob, face_index);
    hb_font_t *font = hb_font_create (face);
    unsigned int glyph_count = hb_face_get_glyph_count (face);
    for (unsigned int gid = 0; gid < glyph_count; ++gid)
    {
      hb_font_extents_t font_extents;
      hb_font_get_extents_for_direction (font, HB_DIRECTION_LTR, &font_extents);
      hb_glyph_extents_t extents = {0};
      if (!hb_font_get_glyph_extents (font, gid, &extents))
      {
        printf ("Skip gid: %d\n", gid);
	continue;
      }

      char name[100];
      sprintf (name, "%d.svg", gid);
      FILE *f = fopen (name, "wb");
      fprintf (f, "<svg xmlns=\"http://www.w3.org/2000/svg\""
		  " viewBox=\"%d %d %d %d\"><path d=\"",
		  extents.x_bearing, 0,
		  extents.x_bearing + extents.width, font_extents.ascender - font_extents.descender); //-extents.height);
      user_data_t user_data;
      user_data.ascender = font_extents.ascender;
      user_data.f = f;
      if (!hb_ot_glyph_decompose (font, gid, &funcs, &user_data))
        printf ("Failed to decompose gid: %d\n", gid);
      fprintf (f, "\"/></svg>");
      fclose (f);
    }
    hb_font_destroy (font);
    hb_face_destroy (face);
  }

  hb_blob_destroy (blob);

  return 0;
}
