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

  demo_renderer_gl_t (GLFWwindow *window_) : window (window_)
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
		int width, int height, float mat[16]) override
  {
    glViewport (0, 0, width, height);
    demo_glstate_set_matrix (st, mat);

    glClear (GL_COLOR_BUFFER_BIT);

    if (count > 0)
    {
      GLint program;
      glGetIntegerv (GL_CURRENT_PROGRAM, &program);

      glBindVertexArray (vao_name);
      glBindBuffer (GL_ARRAY_BUFFER, buf_name);
      glBufferData (GL_ARRAY_BUFFER,
		    sizeof (glyph_vertex_t) * count,
		    (const char *) vertices, GL_STATIC_DRAW);

      GLsizei stride = sizeof (glyph_vertex_t);

      GLint loc_pos = glGetAttribLocation (program, "a_position");
      glEnableVertexAttribArray (loc_pos);
      glVertexAttribPointer (loc_pos, 2, GL_FLOAT, GL_FALSE, stride,
			     (const void *) offsetof (glyph_vertex_t, x));

      GLint loc_tex = glGetAttribLocation (program, "a_texcoord");
      glEnableVertexAttribArray (loc_tex);
      glVertexAttribPointer (loc_tex, 2, GL_FLOAT, GL_FALSE, stride,
			     (const void *) offsetof (glyph_vertex_t, tx));

      GLint loc_normal = glGetAttribLocation (program, "a_normal");
      glEnableVertexAttribArray (loc_normal);
      glVertexAttribPointer (loc_normal, 2, GL_FLOAT, GL_FALSE, stride,
			     (const void *) offsetof (glyph_vertex_t, nx));

      GLint loc_epp = glGetAttribLocation (program, "a_emPerPos");
      glEnableVertexAttribArray (loc_epp);
      glVertexAttribPointer (loc_epp, 1, GL_FLOAT, GL_FALSE, stride,
			     (const void *) offsetof (glyph_vertex_t, emPerPos));

      GLint loc_glyph = glGetAttribLocation (program, "a_glyphLoc");
      glEnableVertexAttribArray (loc_glyph);
      glVertexAttribIPointer (loc_glyph, 1, GL_UNSIGNED_INT, stride,
			      (const void *) offsetof (glyph_vertex_t, atlas_offset));

      glDrawArrays (GL_TRIANGLES, 0, count);

      glDisableVertexAttribArray (loc_pos);
      glDisableVertexAttribArray (loc_tex);
      glDisableVertexAttribArray (loc_normal);
      glDisableVertexAttribArray (loc_epp);
      glDisableVertexAttribArray (loc_glyph);
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
