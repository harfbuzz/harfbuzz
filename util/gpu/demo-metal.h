/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifndef DEMO_METAL_H
#define DEMO_METAL_H

#include "demo-common.h"
#include "demo-shader.h"  /* for glyph_vertex_t */

typedef struct demo_metal_t demo_metal_t;

demo_metal_t *
demo_metal_create (GLFWwindow *window, unsigned int atlas_capacity);

void
demo_metal_destroy (demo_metal_t *mt);

/* Atlas allocation -- same interface as demo_atlas_t */
unsigned int
demo_metal_atlas_alloc (demo_metal_t *mt,
			const char   *data,
			unsigned int  len_bytes);

unsigned int
demo_metal_atlas_get_used (demo_metal_t *mt);

void
demo_metal_atlas_clear (demo_metal_t *mt);

/* Rendering state */
void
demo_metal_set_gamma (demo_metal_t *mt, float gamma);

void
demo_metal_set_foreground (demo_metal_t *mt,
			   float r, float g, float b, float a);

/* Frame rendering */
void
demo_metal_display (demo_metal_t    *mt,
		    glyph_vertex_t  *vertices,
		    unsigned int     vertex_count,
		    int              width,
		    int              height,
		    float             mat[16]);

#endif /* DEMO_METAL_H */
