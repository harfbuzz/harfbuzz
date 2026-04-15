/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-font.h"
#include "demo-atlas.h"

#include <hb-ot.h>

#include <map>

typedef std::map<unsigned int, glyph_info_t> glyph_cache_t;

struct demo_font_t {
  hb_face_t       *face;
  hb_font_t       *font;
  glyph_cache_t   *glyph_cache;
  demo_atlas_t    *atlas;
  hb_gpu_paint_t  *p;
  hb_gpu_draw_t   *d;
  bool             draw_only;
  bool             show_extents;

  unsigned int num_glyphs;
  unsigned int sum_bytes;
};

demo_font_t *
demo_font_create (hb_font_t    *hb_font,
		  demo_atlas_t *atlas,
		  hb_bool_t     draw_only)
{
  demo_font_t *font = (demo_font_t *) calloc (1, sizeof (demo_font_t));

  font->face = hb_face_reference (hb_font_get_face (hb_font));
  font->font = hb_font_reference (hb_font);
  font->glyph_cache = new glyph_cache_t ();
  font->atlas = demo_atlas_reference (atlas);
  font->draw_only = draw_only;
  if (draw_only)
    font->d = hb_gpu_draw_create_or_fail ();
  else
    font->p = hb_gpu_paint_create_or_fail ();

  return font;
}

void
demo_font_destroy (demo_font_t *font)
{
  if (!font)
    return;

  hb_gpu_paint_destroy (font->p);
  hb_gpu_draw_destroy (font->d);
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

void
demo_font_set_show_extents (demo_font_t *font, hb_bool_t show)
{
  bool b = !!show;
  if (font->show_extents == b) return;
  font->show_extents = b;
  demo_font_clear_cache (font);
}

void
demo_font_set_palette (demo_font_t *font, unsigned palette_index)
{
  if (font->draw_only)
    return;
  hb_gpu_paint_set_palette (font->p, palette_index);
  demo_font_clear_cache (font);
}

void
demo_font_clear_custom_palette_colors (demo_font_t *font)
{
  if (font->draw_only)
    return;
  hb_gpu_paint_clear_custom_palette_colors (font->p);
  demo_font_clear_cache (font);
}

hb_bool_t
demo_font_set_custom_palette_color (demo_font_t *font,
				    unsigned int color_index,
				    hb_color_t   color)
{
  if (font->draw_only)
    return false;
  hb_bool_t ok = hb_gpu_paint_set_custom_palette_color (font->p, color_index, color);
  demo_font_clear_cache (font);
  return ok;
}


static void
_demo_font_upload_glyph (demo_font_t  *font,
			 unsigned int  glyph_index,
			 glyph_info_t *glyph_info)
{
  hb_glyph_extents_t hb_ext = {};
  hb_blob_t *blob;
  if (font->draw_only)
  {
    hb_gpu_draw_clear (font->d);
    hb_gpu_draw_glyph (font->d, font->font, glyph_index);
    if (font->show_extents)
    {
      /* Embed a stroked rectangle around the glyph's ink
       * extents in the same blob.  Pad the nominal rect
       * outward by stroke_width / 2 so the stroke sits
       * entirely outside the ink box (otherwise the inner
       * half of the stroke would knock out against the
       * glyph shape, since both fill in the same blob). */
      hb_glyph_extents_t g = {};
      if (hb_font_get_glyph_extents (font->font, glyph_index, &g) &&
	  (g.width != 0 || g.height != 0))
      {
	int x_scale, y_scale;
	hb_font_get_scale (font->font, &x_scale, &y_scale);
	float sw   = (float) (x_scale * 0.01);
	float half = 0.5f * sw;
	float x = (float) g.x_bearing - half;
	float y = (float) (g.y_bearing + g.height) - half;  /* min_y */
	float w = (float) g.width + sw;
	float h = (float) -g.height + sw;                   /* positive */
	hb_draw_funcs_t *dfuncs = hb_gpu_draw_get_funcs ();
	hb_draw_state_t st = HB_DRAW_STATE_DEFAULT;
	hb_draw_rect (dfuncs, font->d, &st, x, y, w, h, sw);
      }
    }
    blob = hb_gpu_draw_encode (font->d, &hb_ext);
  }
  else
  {
    hb_gpu_paint_clear (font->p);
    hb_gpu_paint_glyph (font->p, font->font, glyph_index);
    blob = hb_gpu_paint_encode (font->p, &hb_ext);
  }
  unsigned int len = blob ? hb_blob_get_length (blob) : 0;

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

  if (blob)
  {
    if (font->draw_only)
      hb_gpu_draw_recycle_blob (font->d, blob);
    else
      hb_gpu_paint_recycle_blob (font->p, blob);
  }
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
