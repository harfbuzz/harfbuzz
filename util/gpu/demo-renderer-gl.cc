/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-renderer.h"
#include "demo-atlas.h"
#include "demo-glstate.h"


struct demo_renderer_gl_t : demo_renderer_t
{
  GLFWwindow *window;
  demo_glstate_t *st;
  GLuint vao_name;
  GLuint buf_name;
  bool vao_ready;
  unsigned int uploaded_count;
  unsigned uploaded_generation;

  demo_renderer_gl_t (GLFWwindow *window_)
    : window (window_), vao_ready (false),
      uploaded_count (0), uploaded_generation (0)
  {
    st = demo_glstate_create ();
    glGenVertexArrays (1, &vao_name);
    glGenBuffers (1, &buf_name);
  }

  ~demo_renderer_gl_t () override
  {
    glDeleteVertexArrays (1, &vao_name);
    glDeleteBuffers (1, &buf_name);
    demo_glstate_destroy (st);
  }

  void setup_vao ()
  {
    if (vao_ready)
      return;
    vao_ready = true;

    GLint program;
    glGetIntegerv (GL_CURRENT_PROGRAM, &program);

    glBindVertexArray (vao_name);
    glBindBuffer (GL_ARRAY_BUFFER, buf_name);

    GLsizei stride = sizeof (glyph_vertex_t);

    GLint loc;
    loc = glGetAttribLocation (program, "a_position");
    glEnableVertexAttribArray (loc);
    glVertexAttribPointer (loc, 2, GL_FLOAT, GL_FALSE, stride,
			   (const void *) offsetof (glyph_vertex_t, x));

    loc = glGetAttribLocation (program, "a_texcoord");
    glEnableVertexAttribArray (loc);
    glVertexAttribPointer (loc, 2, GL_FLOAT, GL_FALSE, stride,
			   (const void *) offsetof (glyph_vertex_t, tx));

    loc = glGetAttribLocation (program, "a_normal");
    glEnableVertexAttribArray (loc);
    glVertexAttribPointer (loc, 2, GL_FLOAT, GL_FALSE, stride,
			   (const void *) offsetof (glyph_vertex_t, nx));

    loc = glGetAttribLocation (program, "a_emPerPos");
    glEnableVertexAttribArray (loc);
    glVertexAttribPointer (loc, 1, GL_FLOAT, GL_FALSE, stride,
			   (const void *) offsetof (glyph_vertex_t, emPerPos));

    loc = glGetAttribLocation (program, "a_glyphLoc");
    glEnableVertexAttribArray (loc);
    glVertexAttribIPointer (loc, 1, GL_UNSIGNED_INT, stride,
			    (const void *) offsetof (glyph_vertex_t, atlas_offset));

    glBindVertexArray (0);
  }

  demo_atlas_t *get_atlas () override
  {
    return demo_glstate_get_atlas (st);
  }

  void setup () override
  {
    demo_glstate_setup (st);
  }

  void set_gamma (float gamma) override
  {
    demo_glstate_set_gamma (st, gamma);
  }

  void set_foreground (float r, float g, float b, float a) override
  {
    demo_glstate_set_foreground (st, r, g, b, a);
  }

  void set_background (float r, float g, float b, float a) override
  {
    glClearColor (r, g, b, a);
  }

  void set_debug (bool enabled) override
  {
    demo_glstate_set_debug (st, enabled);
  }

  void set_stem_darkening (bool enabled) override
  {
    demo_glstate_set_stem_darkening (st, enabled);
  }

  void set_msaa (bool enabled) override
  {
    demo_glstate_set_msaa (st, enabled);
  }

  bool set_srgb (bool enabled) override
  {
#if defined(GL_FRAMEBUFFER_SRGB)
    while (glGetError () != GL_NO_ERROR)
      ;
    if (enabled)
    {
      glEnable (GL_FRAMEBUFFER_SRGB);
      if (glGetError () != GL_NO_ERROR)
	return false;
    }
    else
      glDisable (GL_FRAMEBUFFER_SRGB);
    return true;
#else
    return false;
#endif
  }

  void toggle_vsync (bool &vsync) override
  {
    vsync = !vsync;
    glfwSwapInterval (vsync ? 1 : 0);
  }

  void display (glyph_vertex_t *vertices, unsigned int count,
		unsigned generation,
		int width, int height, float mat[16]) override
  {
    glViewport (0, 0, width, height);
    demo_glstate_set_matrix (st, mat);

    glClear (GL_COLOR_BUFFER_BIT);

    if (count > 0)
    {
      setup_vao ();

      glBindVertexArray (vao_name);

      if (count != uploaded_count || generation != uploaded_generation)
      {
	glBindBuffer (GL_ARRAY_BUFFER, buf_name);
	glBufferData (GL_ARRAY_BUFFER,
		      sizeof (glyph_vertex_t) * count,
		      (const char *) vertices, GL_STATIC_DRAW);
	uploaded_count = count;
	uploaded_generation = generation;
      }

      glDrawArrays (GL_TRIANGLES, 0, count);

      glBindVertexArray (0);
    }

    glfwSwapBuffers (window);
  }
};


demo_renderer_t *
demo_renderer_create_gl (GLFWwindow *window)
{
  return new demo_renderer_gl_t (window);
}
