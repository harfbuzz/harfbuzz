/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#ifndef DEMO_RENDERER_H
#define DEMO_RENDERER_H

#include "demo-common.h"
#include "demo-shader.h"

typedef struct demo_atlas_t demo_atlas_t;


struct demo_renderer_t
{
  virtual ~demo_renderer_t () {}

  /* Atlas for glyph data upload. */
  virtual demo_atlas_t *get_atlas () = 0;

  /* One-time setup after atlas is populated. */
  virtual void setup () = 0;

  /* Rendering state. */
  virtual void set_gamma (float gamma) = 0;
  virtual void set_foreground (float r, float g, float b, float a) = 0;
  virtual void set_background (float r, float g, float b, float a) = 0;
  virtual void set_debug (bool enabled) = 0;
  virtual void set_stem_darkening (bool enabled) { (void) enabled; }

  /* Vsync toggle. */
  virtual void toggle_vsync (bool &vsync) = 0;

  /* Render a frame.  generation changes when buffer content is rebuilt. */
  virtual void display (glyph_vertex_t *vertices, unsigned int count,
			unsigned generation,
			int width, int height, float mat[16]) = 0;
};


#ifndef HB_GPU_NO_GLFW
demo_renderer_t *
demo_renderer_create_gl (GLFWwindow *window, bool draw_only);

#ifdef __APPLE__
demo_renderer_t *
demo_renderer_create_metal (GLFWwindow *window, bool draw_only);
#endif

#ifdef _WIN32
demo_renderer_t *
demo_renderer_create_d3d11 (GLFWwindow *window, bool draw_only);
#endif
#endif


#endif /* DEMO_RENDERER_H */
