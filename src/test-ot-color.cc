/*
 * Copyright © 2018  Ebrahim Byagowi
 * Copyright © 2018  Khaled Hosny
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

#include "hb.hh"

#ifdef HB_NO_OPEN
#define hb_blob_create_from_file(x)  hb_blob_get_empty ()
#endif

#if !defined(HB_NO_COLOR)

#include "hb-ot.h"

#include <stdlib.h>
#include <stdio.h>

static void
svg_dump (hb_face_t *face, unsigned int face_index)
{
  unsigned glyph_count = hb_face_get_glyph_count (face);

  for (unsigned int glyph_id = 0; glyph_id < glyph_count; glyph_id++)
  {
    hb_blob_t *blob = hb_ot_color_glyph_reference_svg (face, glyph_id);

    if (hb_blob_get_length (blob) == 0) continue;

    unsigned int length;
    const char *data = hb_blob_get_data (blob, &length);

    char output_path[255];
    sprintf (output_path, "out/svg-%u-%u.svg%s",
	     glyph_id,
	     face_index,
	     // append "z" if the content is gzipped, https://stackoverflow.com/a/6059405
	     (length > 2 && (data[0] == '\x1F') && (data[1] == '\x8B')) ? "z" : "");

    FILE *f = fopen (output_path, "wb");
    fwrite (data, 1, length, f);
    fclose (f);

    hb_blob_destroy (blob);
  }
}

/* _png API is so easy to use unlike the below code, don't get confused */
static void
png_dump (hb_face_t *face, unsigned int face_index)
{
  unsigned glyph_count = hb_face_get_glyph_count (face);
  hb_font_t *font = hb_font_create (face);

  /* scans the font for strikes */
  unsigned int sample_glyph_id;
  /* we don't care about different strikes for different glyphs at this point */
  for (sample_glyph_id = 0; sample_glyph_id < glyph_count; sample_glyph_id++)
  {
    hb_blob_t *blob = hb_ot_color_glyph_reference_png (font, sample_glyph_id);
    unsigned int blob_length = hb_blob_get_length (blob);
    hb_blob_destroy (blob);
    if (blob_length != 0)
      break;
  }

  unsigned int upem = hb_face_get_upem (face);
  unsigned int blob_length = 0;
  unsigned int strike = 0;
  for (unsigned int ppem = 1; ppem < upem; ppem++)
  {
    hb_font_set_ppem (font, ppem, ppem);
    hb_blob_t *blob = hb_ot_color_glyph_reference_png (font, sample_glyph_id);
    unsigned int new_blob_length = hb_blob_get_length (blob);
    hb_blob_destroy (blob);
    if (new_blob_length != blob_length)
    {
      for (unsigned int glyph_id = 0; glyph_id < glyph_count; glyph_id++)
      {
	hb_blob_t *blob = hb_ot_color_glyph_reference_png (font, glyph_id);

	if (hb_blob_get_length (blob) == 0) continue;

	unsigned int length;
	const char *data = hb_blob_get_data (blob, &length);

	char output_path[255];
	sprintf (output_path, "out/png-%u-%u-%u.png", glyph_id, strike, face_index);

	FILE *f = fopen (output_path, "wb");
	fwrite (data, 1, length, f);
	fclose (f);

	hb_blob_destroy (blob);
      }

      strike++;
      blob_length = new_blob_length;
    }
  }

  hb_font_destroy (font);
}

struct user_data_t
{
  FILE *f;
  hb_position_t ascender;
};

static void
move_to (hb_position_t to_x, hb_position_t to_y, user_data_t &user_data)
{
  fprintf (user_data.f, "M%d,%d", to_x, user_data.ascender - to_y);
}

static void
line_to (hb_position_t to_x, hb_position_t to_y, user_data_t &user_data)
{
  fprintf (user_data.f, "L%d,%d", to_x, user_data.ascender - to_y);
}

static void
conic_to (hb_position_t control_x, hb_position_t control_y,
	  hb_position_t to_x, hb_position_t to_y,
	  user_data_t &user_data)
{
  fprintf (user_data.f, "Q%d,%d %d,%d", control_x, user_data.ascender - control_y,
					to_x, user_data.ascender - to_y);
}

static void
cubic_to (hb_position_t control1_x, hb_position_t control1_y,
	  hb_position_t control2_x, hb_position_t control2_y,
	  hb_position_t to_x, hb_position_t to_y,
	  user_data_t &user_data)
{
  fprintf (user_data.f, "C%d,%d %d,%d %d,%d", control1_x, user_data.ascender - control1_y,
					       control2_x, user_data.ascender - control2_y,
					       to_x, user_data.ascender - to_y);
}

static void
close_path (user_data_t &user_data)
{
  fprintf (user_data.f, "Z");
}

static void
layered_glyph_dump (hb_font_t *font, hb_ot_glyph_decompose_funcs_t *funcs, unsigned int face_index)
{
  hb_face_t *face = hb_font_get_face (font);
  unsigned num_glyphs = hb_face_get_glyph_count (face);
  for (hb_codepoint_t gid = 0; gid < num_glyphs; ++gid)
  {
    unsigned int num_layers = hb_ot_color_glyph_get_layers (face, gid, 0, nullptr, nullptr);
    if (!num_layers) continue;

    hb_ot_color_layer_t *layers = (hb_ot_color_layer_t*) malloc (num_layers * sizeof (hb_ot_color_layer_t));

    hb_ot_color_glyph_get_layers (face, gid, 0, &num_layers, layers);
    if (num_layers)
    {
      hb_font_extents_t font_extents;
      hb_font_get_extents_for_direction (font, HB_DIRECTION_LTR, &font_extents);
      hb_glyph_extents_t extents = {0};
      if (!hb_font_get_glyph_extents (font, gid, &extents))
      {
	printf ("Skip gid: %d\n", gid);
	continue;
      }

      unsigned int palette_count = hb_ot_color_palette_get_count (face);
      for (unsigned int palette = 0; palette < palette_count; ++palette)
      {
	unsigned int num_colors = hb_ot_color_palette_get_colors (face, palette, 0, nullptr, nullptr);
	if (!num_colors)
	  continue;

	char output_path[255];
	sprintf (output_path, "out/colr-%u-%u-%u.svg", gid, palette, face_index);
        FILE *f = fopen (output_path, "wb");
        fprintf (f, "<svg xmlns=\"http://www.w3.org/2000/svg\""
		    " viewBox=\"%d %d %d %d\">\n",
		    extents.x_bearing, 0,
		    extents.x_bearing + extents.width, -extents.height);
        user_data_t user_data;
        user_data.ascender = extents.y_bearing;
        user_data.f = f;

	hb_color_t *colors = (hb_color_t*) calloc (num_colors, sizeof (hb_color_t));
	hb_ot_color_palette_get_colors (face, palette, 0, &num_colors, colors);
	if (num_colors)
	{
	  for (unsigned int layer = 0; layer < num_layers; ++layer)
	  {
	    hb_color_t color = 0x000000FF;
	    if (layers[layer].color_index != 0xFFFF)
	      color = colors[layers[layer].color_index];
	    fprintf (f, "<path fill=\"#%02X%02X%02X\" ",
		     hb_color_get_red (color), hb_color_get_green (color), hb_color_get_green (color));
	    if (hb_color_get_alpha (color) != 255)
	      fprintf (f, "fill-opacity=\"%.3f\"", (double) hb_color_get_alpha (color) / 255.);
	    fprintf (f, "d=\"");
	    if (!hb_ot_glyph_decompose (font, layers[layer].glyph, funcs, &user_data))
	      printf ("Failed to decompose layer %d while %d\n", layers[layer].glyph, gid);
	    fprintf (f, "\"/>\n");
	  }
	}
	free (colors);

	fprintf (f, "</svg>");
	fclose (f);
      }
    }


    free (layers);
  }
}

static void
dump_glyphs (hb_font_t *font, hb_ot_glyph_decompose_funcs_t *funcs, unsigned int face_index)
{
  unsigned num_glyphs = hb_face_get_glyph_count (hb_font_get_face (font));
  for (unsigned int gid = 0; gid < num_glyphs; ++gid)
  {
    hb_font_extents_t font_extents;
    hb_font_get_extents_for_direction (font, HB_DIRECTION_LTR, &font_extents);
    hb_glyph_extents_t extents = {0};
    if (!hb_font_get_glyph_extents (font, gid, &extents))
    {
      printf ("Skip gid: %d\n", gid);
      continue;
    }

    char output_path[255];
    sprintf (output_path, "out/%u-%u.svg", face_index, gid);
    FILE *f = fopen (output_path, "wb");
    fprintf (f, "<svg xmlns=\"http://www.w3.org/2000/svg\""
		" viewBox=\"%d %d %d %d\"><path d=\"",
		extents.x_bearing, 0,
		extents.x_bearing + extents.width, font_extents.ascender - font_extents.descender);
    user_data_t user_data;
    user_data.ascender = font_extents.ascender;
    user_data.f = f;
    if (!hb_ot_glyph_decompose (font, gid, funcs, &user_data))
      printf ("Failed to decompose gid: %d\n", gid);
    fprintf (f, "\"/></svg>");
    fclose (f);
  }
}

int
main (int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s font-file.ttf\n"
		     "run it like `rm -rf out && mkdir out && %s font-file.ttf`\n",
		     argv[0], argv[0]);
    exit (1);
  }


  FILE *font_name_file = fopen ("out/.dumped_font_name", "r");
  if (font_name_file != nullptr)
  {
    fprintf (stderr, "Purge or move ./out folder in order to run a new dump\n");
    exit (1);
  }

  font_name_file = fopen ("out/.dumped_font_name", "w");
  if (font_name_file == nullptr)
  {
    fprintf (stderr, "./out is not accessible as a folder, create it please\n");
    exit (1);
  }
  fwrite (argv[1], 1, strlen (argv[1]), font_name_file);
  fclose (font_name_file);

  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  unsigned int num_faces = hb_face_count (blob);
  if (num_faces == 0)
  {
    fprintf (stderr, "error: The file (%s) was corrupted, empty or not found", argv[1]);
    exit (1);
  }

  hb_ot_glyph_decompose_funcs_t *funcs = hb_ot_glyph_decompose_funcs_create ();
  hb_ot_glyph_decompose_funcs_set_move_to_func (funcs, (hb_ot_glyph_decompose_move_to_func_t) move_to);
  hb_ot_glyph_decompose_funcs_set_line_to_func (funcs, (hb_ot_glyph_decompose_line_to_func_t) line_to);
  hb_ot_glyph_decompose_funcs_set_conic_to_func (funcs, (hb_ot_glyph_decompose_conic_to_func_t) conic_to);
  hb_ot_glyph_decompose_funcs_set_cubic_to_func (funcs, (hb_ot_glyph_decompose_cubic_to_func_t) cubic_to);
  hb_ot_glyph_decompose_funcs_set_close_path_func (funcs, (hb_ot_glyph_decompose_close_path_func_t) close_path);

  for (unsigned int face_index = 0; face_index < hb_face_count (blob); face_index++)
  {
    hb_face_t *face = hb_face_create (blob, face_index);
    hb_font_t *font = hb_font_create (face);

    if (hb_ot_color_has_png (face))
      printf ("Dumping png (CBDT/sbix)...\n");
    png_dump (face, face_index);

    if (hb_ot_color_has_svg (face))
      printf ("Dumping svg (SVG )...\n");
    svg_dump (face, face_index);

    if (hb_ot_color_has_layers (face) && hb_ot_color_has_palettes (face))
      printf ("Dumping layered color glyphs (COLR/CPAL)...\n");
    layered_glyph_dump (font, funcs, face_index);

    dump_glyphs (font, funcs, face_index);

    hb_font_destroy (font);
    hb_face_destroy (face);
  }

  hb_ot_glyph_decompose_funcs_destroy (funcs);
  hb_blob_destroy (blob);

  return 0;
}

#else
int main (int argc, char **argv) { return 0; }
#endif
