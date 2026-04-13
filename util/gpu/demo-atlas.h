/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifndef DEMO_ATLAS_H
#define DEMO_ATLAS_H

#include "demo-common.h"


typedef struct demo_atlas_t demo_atlas_t;

demo_atlas_t *
demo_atlas_create (unsigned int capacity);

demo_atlas_t *
demo_atlas_reference (demo_atlas_t *at);

void
demo_atlas_destroy (demo_atlas_t *at);


/* Returns the 1D offset (in texels) where the data was placed. */
unsigned int
demo_atlas_alloc (demo_atlas_t *at,
		  const char   *data,
		  unsigned int  len_bytes);

unsigned int
demo_atlas_get_used (demo_atlas_t *at);

void
demo_atlas_clear (demo_atlas_t *at);

void
demo_atlas_set_uniforms (demo_atlas_t *at);


/* Create an atlas backed by external callbacks (for Metal, etc.) */
typedef struct {
  void *ctx;
  unsigned int (*alloc) (void *ctx, const char *data, unsigned int len_bytes);
  unsigned int (*get_used) (void *ctx);
  void (*clear) (void *ctx);
} demo_atlas_backend_t;

demo_atlas_t *
demo_atlas_create_external (const demo_atlas_backend_t *backend);


#endif /* DEMO_ATLAS_H */
