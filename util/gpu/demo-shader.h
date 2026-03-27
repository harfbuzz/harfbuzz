/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifndef DEMO_SHADERS_H
#define DEMO_SHADERS_H

#include "demo-common.h"
#include "demo-font.h"


struct glyph_vertex_t {
  /* Object-space position */
  GLfloat x;
  GLfloat y;
  /* Em-space texture coordinates */
  GLfloat tx;
  GLfloat ty;
  /* Object-space outward normal at this vertex */
  GLfloat nx;
  GLfloat ny;
  /* Em units per object-space unit (upem / font_size) */
  GLfloat emPerPos;
  /* Atlas offset (constant across glyph) */
  GLuint atlas_offset;
};

void
demo_shader_add_glyph_vertices (const demo_point_t              &p,
				double                           font_size,
				glyph_info_t                    *gi,
				std::vector<glyph_vertex_t>     *vertices,
				demo_extents_t                  *extents);


GLuint
demo_shader_create_program (void);


#endif /* DEMO_SHADERS_H */
