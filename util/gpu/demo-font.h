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
		  demo_atlas_t *atlas);

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

/* Returns the face's palette (index 0) as premultiplied-alpha
 * RGBA floats.  Writes up to `capacity` entries to `out`; returns
 * the number of entries written. */
unsigned
demo_font_get_palette (demo_font_t *font,
		       float       *out,
		       unsigned     capacity);

void
demo_font_clear_cache (demo_font_t *font);

void
demo_font_print_stats (demo_font_t *font);


#endif /* DEMO_FONT_H */
