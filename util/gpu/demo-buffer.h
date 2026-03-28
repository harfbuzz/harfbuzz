/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef DEMO_BUFFER_H
#define DEMO_BUFFER_H

#include "demo-common.h"
#include "demo-font.h"
#include "demo-shader.h"

typedef struct demo_buffer_t demo_buffer_t;

demo_buffer_t *
demo_buffer_create (void);

void
demo_buffer_destroy (demo_buffer_t *buffer);


void
demo_buffer_clear (demo_buffer_t *buffer);

void
demo_buffer_extents (demo_buffer_t  *buffer,
		     demo_extents_t *ink_extents,
		     demo_extents_t *logical_extents);

void
demo_buffer_move_to (demo_buffer_t      *buffer,
		     const demo_point_t *p);

void
demo_buffer_add_text (demo_buffer_t *buffer,
		      const char    *utf8,
		      demo_font_t   *font,
		      double         font_size);

void
demo_buffer_add_glyph (demo_buffer_t      *buffer,
		       demo_font_t        *font,
		       double              font_size,
		       unsigned int        glyph_index,
		       double              x_offset,
		       double              y_offset,
		       double              x_advance);

void
demo_buffer_current_line (demo_buffer_t *buffer,
			  double         font_size);

void
demo_buffer_draw (demo_buffer_t *buffer);


#endif /* DEMO_BUFFER_H */
