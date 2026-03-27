/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-font.h"

#include <map>

typedef std::map<unsigned int, glyph_info_t> glyph_cache_t;

struct demo_font_t {
  hb_face_t       *face;
  hb_font_t       *font;
  glyph_cache_t   *glyph_cache;
  demo_atlas_t    *atlas;
  hb_gpu_glyph_t  *g;

  unsigned int num_glyphs;
  unsigned int sum_bytes;
};

demo_font_t *
demo_font_create (hb_face_t    *face,
		  demo_atlas_t *atlas)
{
  demo_font_t *font = (demo_font_t *) calloc (1, sizeof (demo_font_t));

  font->face = hb_face_reference (face);
  font->font = hb_font_create (face);
  font->glyph_cache = new glyph_cache_t ();
  font->atlas = demo_atlas_reference (atlas);
  font->g = hb_gpu_glyph_create_or_fail ();

  return font;
}

void
demo_font_destroy (demo_font_t *font)
{
  if (!font)
    return;

  hb_gpu_glyph_destroy (font->g);
  demo_atlas_destroy (font->atlas);
  delete font->glyph_cache;
  hb_font_destroy (font->font);
  hb_face_destroy (font->face);
  free (font);
}


hb_face_t *
demo_font_get_face (demo_font_t *font)
{
  return font->face;
}

hb_font_t *
demo_font_get_font (demo_font_t *font)
{
  return font->font;
}


static void
_demo_font_upload_glyph (demo_font_t  *font,
			 unsigned int  glyph_index,
			 glyph_info_t *glyph_info)
{
  hb_gpu_glyph_reset (font->g);

  hb_gpu_glyph_draw_glyph (font->g, font->font, glyph_index);

  hb_blob_t *blob = hb_gpu_glyph_encode (font->g);
  if (!blob)
    die ("Failed encoding glyph");

  unsigned int len = hb_blob_get_length (blob);

  /* Get extents in font design units */
  hb_glyph_extents_t hb_ext;
  hb_gpu_glyph_get_extents (font->g, &hb_ext);

  glyph_info->extents.min_x = hb_ext.x_bearing;
  glyph_info->extents.max_x = hb_ext.x_bearing + hb_ext.width;
  glyph_info->extents.max_y = hb_ext.y_bearing;
  glyph_info->extents.min_y = hb_ext.y_bearing + hb_ext.height;
  glyph_info->advance = hb_font_get_glyph_h_advance (font->font, glyph_index);
  glyph_info->upem = hb_face_get_upem (font->face);
  glyph_info->is_empty = (len == 0);

  if (!glyph_info->is_empty)
    glyph_info->atlas_offset = demo_atlas_alloc (font->atlas,
						 hb_blob_get_data (blob, NULL),
						 len);

  font->num_glyphs++;
  font->sum_bytes += len;

  hb_blob_destroy (blob);
}

void
demo_font_lookup_glyph (demo_font_t  *font,
			unsigned int  glyph_index,
			glyph_info_t *glyph_info)
{
  if (font->glyph_cache->find (glyph_index) == font->glyph_cache->end ()) {
    _demo_font_upload_glyph (font, glyph_index, glyph_info);
    (*font->glyph_cache)[glyph_index] = *glyph_info;
  } else
    *glyph_info = (*font->glyph_cache)[glyph_index];
}

void
demo_font_print_stats (demo_font_t *font)
{
  double atlas_used_kb = demo_atlas_get_used (font->atlas) * 8 / 1024.;

  if (!font->num_glyphs)
    return;

  LOGI ("%3d glyphs; avg %5.2fkb per glyph; atlas used %5.2fkb\n",
	font->num_glyphs,
	font->sum_bytes / 1024. / font->num_glyphs,
	atlas_used_kb);
}
