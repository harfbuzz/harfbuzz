/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifndef DEMO_GLSTATE_H
#define DEMO_GLSTATE_H

#include "demo-common.h"
#include "demo-buffer.h"

#include "demo-atlas.h"
#include "demo-shader.h"

typedef struct demo_glstate_t demo_glstate_t;

demo_glstate_t *
demo_glstate_create (void);

void
demo_glstate_destroy (demo_glstate_t *st);


void
demo_glstate_setup (demo_glstate_t *st);

demo_atlas_t *
demo_glstate_get_atlas (demo_glstate_t *st);

void
demo_glstate_set_matrix (demo_glstate_t *st, float mat[16]);

void
demo_glstate_set_gamma (demo_glstate_t *st, float gamma);

void
demo_glstate_set_foreground (demo_glstate_t *st,
			     float r, float g, float b, float a);

void
demo_glstate_set_debug (demo_glstate_t *st, bool enabled);


#endif /* DEMO_GLSTATE_H */
