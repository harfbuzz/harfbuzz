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

static inline void
demo_shader_add_glyph_vertices (const demo_point_t              &p,
				double                           font_size,
				glyph_info_t                    *gi,
				std::vector<glyph_vertex_t>     *vertices,
				demo_extents_t                  *extents)
{
  if (gi->is_empty)
    return;

  double scale = font_size / gi->upem;

  glyph_vertex_t v[4];

  for (int ci = 0; ci < 4; ci++) {
    int cx = (ci >> 1) & 1;
    int cy = ci & 1;

    double ex = (1 - cx) * gi->extents.min_x + cx * gi->extents.max_x;
    double ey = (1 - cy) * gi->extents.min_y + cy * gi->extents.max_y;

    v[ci].x = (float) (p.x + scale * ex);
    v[ci].y = (float) (p.y - scale * ey);
    v[ci].tx = (float) ex;
    v[ci].ty = (float) ey;
    v[ci].nx = cx ? 1.f : -1.f;
    v[ci].ny = cy ? -1.f : 1.f;
    v[ci].emPerPos = (float) (1.0 / scale);
    v[ci].atlas_offset = gi->atlas_offset;
  }

  vertices->push_back (v[0]);
  vertices->push_back (v[1]);
  vertices->push_back (v[2]);

  vertices->push_back (v[1]);
  vertices->push_back (v[2]);
  vertices->push_back (v[3]);

  if (extents) {
    demo_extents_clear (extents);
    for (int i = 0; i < 4; i++) {
      demo_point_t pt = {(double) v[i].x, (double) v[i].y};
      demo_extents_add (extents, &pt);
    }
  }
}


GLuint
demo_shader_create_program (bool draw_only);


#endif /* DEMO_SHADERS_H */
