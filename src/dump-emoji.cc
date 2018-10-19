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
#include "hb-ot-color-cbdt-table.hh"
#include "hb-ot-color-colr-table.hh"
#include "hb-ot-color-cpal-table.hh"
#include "hb-ot-color-sbix-table.hh"
#include "hb-ot-color-svg-table.hh"

#include "hb-ft.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-svg.h>

#ifdef HAVE_GLIB
#include <glib.h>
#endif
#include <stdlib.h>
#include <stdio.h>

static void
cbdt_callback (const uint8_t* data, unsigned int length,
	       unsigned int group, unsigned int gid)
{
  char output_path[255];
  sprintf (output_path, "out/cbdt-%d-%d.png", group, gid);
  FILE *f = fopen (output_path, "wb");
  fwrite (data, 1, length, f);
  fclose (f);
}

static void
sbix_callback (const uint8_t* data, unsigned int length,
	       unsigned int group, unsigned int gid)
{
  char output_path[255];
  sprintf (output_path, "out/sbix-%d-%d.png", group, gid);
  FILE *f = fopen (output_path, "wb");
  fwrite (data, 1, length, f);
  fclose (f);
}

static void
svg_callback (const uint8_t* data, unsigned int length,
	      unsigned int start_glyph, unsigned int end_glyph)
{
  char output_path[255];
  if (start_glyph == end_glyph)
    sprintf (output_path, "out/svg-%d.svg", start_glyph);
  else
    sprintf (output_path, "out/svg-%d-%d.svg", start_glyph, end_glyph);

  // append "z" if the content is gzipped
  if ((data[0] == 0x1F) && (data[1] == 0x8B))
    strcat (output_path, "z");

  FILE *f = fopen (output_path, "wb");
  fwrite (data, 1, length, f);
  fclose (f);
}

static void
colr_cpal_rendering (hb_face_t *face, cairo_font_face_t *cairo_face)
{
  unsigned int upem = hb_face_get_upem (face);

  for (hb_codepoint_t gid = 0; gid < hb_face_get_glyph_count (face); ++gid)
  {
    unsigned int num_layers = hb_ot_color_get_color_layers (face, gid, 0, nullptr, nullptr, nullptr);
    if (!num_layers)
      continue;

    hb_codepoint_t *layer_gids = (hb_codepoint_t*) calloc (num_layers, sizeof (hb_codepoint_t));
    unsigned int *color_indices = (unsigned int*) calloc (num_layers, sizeof (unsigned int));

    hb_ot_color_get_color_layers (face, gid, 0, &num_layers, layer_gids, color_indices);
    if (num_layers)
    {
      // Measure
      cairo_text_extents_t extents;
      {
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cairo_t *cr = cairo_create (surface);
	cairo_set_font_face (cr, cairo_face);
	cairo_set_font_size (cr, upem);

	cairo_glyph_t *glyphs = (cairo_glyph_t *) calloc (num_layers, sizeof (cairo_glyph_t));
	for (unsigned int j = 0; j < num_layers; ++j)
	  glyphs[j].index = layer_gids[j];
	cairo_glyph_extents (cr, glyphs, num_layers, &extents);
	free (glyphs);
	cairo_surface_destroy (surface);
	cairo_destroy (cr);
      }

      // Add a slight margin
      extents.width += extents.width / 10;
      extents.height += extents.height / 10;
      extents.x_bearing -= extents.width / 20;
      extents.y_bearing -= extents.height / 20;

      // Render
      unsigned int pallet_count = hb_ot_color_get_palette_count (face);
      for (unsigned int pallet = 0; pallet < pallet_count; ++pallet) {
	char output_path[255];

        unsigned int num_colors = hb_ot_color_get_palette_colors (face, pallet, 0, nullptr, nullptr);
        if (!num_colors)
          continue;

        hb_ot_color_t *colors = (hb_ot_color_t*) calloc (num_colors, sizeof (hb_ot_color_t));
        hb_ot_color_get_palette_colors (face, pallet, 0, &num_colors, colors);
        if (num_colors)
        {
	  // If we have more than one pallet, use a better namin
	  if (pallet_count == 1)
	    sprintf (output_path, "out/colr-%d.svg", gid);
	  else
	    sprintf (output_path, "out/colr-%d-%d.svg", gid, pallet);

	  cairo_surface_t *surface = cairo_svg_surface_create (output_path, extents.width, extents.height);
	  cairo_t *cr = cairo_create (surface);
	  cairo_set_font_face (cr, cairo_face);
	  cairo_set_font_size (cr, upem);

	  for (unsigned int layer = 0; layer < num_layers; ++layer)
	  {
	    uint32_t color = 0xFF;
            if (color_indices[layer] != 0xFFFF)
	      color = colors[color_indices[layer]];
	    int alpha = hb_ot_color_get_alpha (color);
	    int r = hb_ot_color_get_red (color);
	    int g = hb_ot_color_get_green (color);
	    int b = hb_ot_color_get_blue (color);
	    cairo_set_source_rgba (cr, r / 255., g / 255., b / 255., alpha);

	    cairo_glyph_t glyph;
	    glyph.index = layer_gids[layer];
	    glyph.x = -extents.x_bearing;
	    glyph.y = -extents.y_bearing;
	    cairo_show_glyphs (cr, &glyph, 1);
	  }

	  cairo_surface_destroy (surface);
	  cairo_destroy (cr);
        }
        free (colors);
      }
    }

    free (layer_gids);
    free (color_indices);
  }
}

static void
dump_glyphs (cairo_font_face_t *cairo_face, unsigned int upem,
	     unsigned int num_glyphs)
{
  // Dump every glyph available on the font
  return; // disabled for now
  for (unsigned int i = 0; i < num_glyphs; ++i)
  {
    cairo_text_extents_t extents;
    cairo_glyph_t glyph = {0};
    glyph.index = i;

    // Measure
    {
      cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
      cairo_t *cr = cairo_create (surface);
      cairo_set_font_face (cr, cairo_face);
      cairo_set_font_size (cr, upem);

      cairo_glyph_extents (cr, &glyph, 1, &extents);
      cairo_surface_destroy (surface);
      cairo_destroy (cr);
    }

    // Add a slight margin
    extents.width += extents.width / 10;
    extents.height += extents.height / 10;
    extents.x_bearing -= extents.width / 20;
    extents.y_bearing -= extents.height / 20;

    // Render
    {
      char output_path[255];
      sprintf (output_path, "out/%d.svg", i);
      cairo_surface_t *surface = cairo_svg_surface_create (output_path, extents.width, extents.height);
      cairo_t *cr = cairo_create (surface);
      cairo_set_font_face (cr, cairo_face);
      cairo_set_font_size (cr, upem);
      glyph.x = -extents.x_bearing;
      glyph.y = -extents.y_bearing;
      cairo_show_glyphs (cr, &glyph, 1);
      cairo_surface_destroy (surface);
      cairo_destroy (cr);
    }
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


  FILE *font_name_file = fopen ("out/_font_name_file.txt", "r");
  if (font_name_file != nullptr)
  {
    fprintf (stderr, "Purge or move ./out folder in order to run a new dump\n");
    exit (1);
  }

  font_name_file = fopen ("out/_font_name_file.txt", "w");
  if (font_name_file == nullptr)
  {
    fprintf (stderr, "./out is not accessible as a folder, create it please\n");
    exit (1);
  }
  fwrite (argv[1], 1, strlen (argv[1]), font_name_file);
  fclose (font_name_file);

  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
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

  cairo_font_face_t *cairo_face;
  {
    FT_Library library;
    FT_Init_FreeType (&library);
    FT_Face ftface;
    FT_New_Face (library, argv[1], 0, &ftface);
    cairo_face = cairo_ft_font_face_create_for_ft_face (ftface, 0);
  }
  unsigned int num_glyphs = hb_face_get_glyph_count (face);
  unsigned int upem = hb_face_get_upem (face);
  colr_cpal_rendering (face, cairo_face);
  dump_glyphs (cairo_face, upem, num_glyphs);


  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
