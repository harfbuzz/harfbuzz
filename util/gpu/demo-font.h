/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifndef DEMO_FONT_H
#define DEMO_FONT_H

#include "demo-common.h"
#include "demo-atlas.h"

typedef struct {
  demo_extents_t extents;
  double         advance;
  hb_bool_t      is_empty;
  unsigned int   upem;
  unsigned int   atlas_offset;
} glyph_info_t;


typedef struct demo_font_t demo_font_t;

demo_font_t *
demo_font_create (hb_font_t    *font,
		  demo_atlas_t *atlas,
		  hb_bool_t     draw_only);

void
demo_font_destroy (demo_font_t *font);


hb_face_t *
demo_font_get_face (demo_font_t *font);

hb_font_t *
demo_font_get_font (demo_font_t *font);


void
demo_font_lookup_glyph (demo_font_t  *font,
			unsigned int  glyph_index,
			glyph_info_t *glyph_info);

/* Forward paint-encoder configuration to the underlying
 * hb_gpu_paint_t and clear the glyph cache (colors are baked into
 * encoded blobs, so palette / override changes invalidate them). */
void
demo_font_set_palette (demo_font_t *font,
		       unsigned     palette_index);

void
demo_font_set_foreground (demo_font_t *font,
			  hb_color_t   foreground);

void
demo_font_clear_custom_palette_colors (demo_font_t *font);

hb_bool_t
demo_font_set_custom_palette_color (demo_font_t *font,
				    unsigned int color_index,
				    hb_color_t   color);

void
demo_font_clear_cache (demo_font_t *font);

void
demo_font_print_stats (demo_font_t *font);


#endif /* DEMO_FONT_H */
