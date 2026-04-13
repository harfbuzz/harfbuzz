/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-font.h"

#include <hb-ot.h>

#include <map>

typedef std::map<unsigned int, glyph_info_t> glyph_cache_t;

struct demo_font_t {
  hb_face_t       *face;
  hb_font_t       *font;
  glyph_cache_t   *glyph_cache;
  demo_atlas_t    *atlas;
  hb_gpu_draw_t   *g;
  hb_gpu_paint_t  *p;

  unsigned int num_glyphs;
  unsigned int sum_bytes;
};

demo_font_t *
demo_font_create (hb_font_t    *hb_font,
		  demo_atlas_t *atlas)
{
  demo_font_t *font = (demo_font_t *) calloc (1, sizeof (demo_font_t));

  font->face = hb_face_reference (hb_font_get_face (hb_font));
  font->font = hb_font_reference (hb_font);
  font->glyph_cache = new glyph_cache_t ();
  font->atlas = demo_atlas_reference (atlas);
  font->g = hb_gpu_draw_create_or_fail ();
  font->p = hb_gpu_paint_create_or_fail ();

  return font;
}

void
demo_font_destroy (demo_font_t *font)
{
  if (!font)
    return;

  hb_gpu_paint_destroy (font->p);
  hb_gpu_draw_destroy (font->g);
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
  hb_glyph_extents_t hb_ext;
  hb_blob_t *blob = nullptr;
  bool is_paint = false;

  /* Try the paint path for color glyphs; fall back to draw on
   * failure (including the current stub state where encode always
   * returns NULL). */
  if (hb_ot_color_glyph_has_paint (font->face, glyph_index))
  {
    hb_gpu_paint_clear (font->p);
    if (hb_gpu_paint_glyph (font->p, font->font, glyph_index))
    {
      blob = hb_gpu_paint_encode (font->p, &hb_ext);
      is_paint = blob != nullptr;
    }
  }

  if (!blob)
  {
    hb_gpu_draw_clear (font->g);
    hb_gpu_draw_glyph (font->g, font->font, glyph_index);
    blob = hb_gpu_draw_encode (font->g, &hb_ext);
    if (!blob)
      die ("Failed encoding glyph");
  }

  unsigned int len = hb_blob_get_length (blob);

  glyph_info->extents.min_x = hb_ext.x_bearing;
  glyph_info->extents.max_x = hb_ext.x_bearing + hb_ext.width;
  glyph_info->extents.max_y = hb_ext.y_bearing;
  glyph_info->extents.min_y = hb_ext.y_bearing + hb_ext.height;
  glyph_info->advance = hb_font_get_glyph_h_advance (font->font, glyph_index);
  int x_scale, y_scale;
  hb_font_get_scale (font->font, &x_scale, &y_scale);
  glyph_info->upem = y_scale;
  glyph_info->is_empty = (len == 0);

  if (!glyph_info->is_empty)
    glyph_info->atlas_offset = demo_atlas_alloc (font->atlas,
						 hb_blob_get_data (blob, NULL),
						 len);

  font->num_glyphs++;
  font->sum_bytes += len;

  if (is_paint)
    hb_gpu_paint_recycle_blob (font->p, blob);
  else
    hb_gpu_draw_recycle_blob (font->g, blob);
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
demo_font_clear_cache (demo_font_t *font)
{
  font->glyph_cache->clear ();
  font->num_glyphs = 0;
  font->sum_bytes = 0;
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
