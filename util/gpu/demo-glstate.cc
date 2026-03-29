/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-glstate.h"

struct demo_glstate_t {
  GLuint program;
  demo_atlas_t *atlas;
};

demo_glstate_t *
demo_glstate_create (void)
{
  demo_glstate_t *st = (demo_glstate_t *) calloc (1, sizeof (demo_glstate_t));

  st->program = demo_shader_create_program ();
  st->atlas = demo_atlas_create (1024 * 1024);

  return st;
}

void
demo_glstate_destroy (demo_glstate_t *st)
{
  if (!st)
    return;

  demo_atlas_destroy (st->atlas);
  glDeleteProgram (st->program);

  free (st);
}

void
demo_glstate_setup (demo_glstate_t *st)
{
  glUseProgram (st->program);

  demo_atlas_set_uniforms (st->atlas);
  demo_glstate_set_gamma (st, 1.f);
  demo_glstate_set_foreground (st, 0.f, 0.f, 0.f, 1.f);

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

demo_atlas_t *
demo_glstate_get_atlas (demo_glstate_t *st)
{
  return st->atlas;
}

void
demo_glstate_set_matrix (demo_glstate_t *st, float mat[16])
{
  glUniformMatrix4fv (glGetUniformLocation (st->program, "u_matViewProjection"), 1, GL_FALSE, mat);

  GLint viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  glUniform2f (glGetUniformLocation (st->program, "u_viewport"),
	       (float) viewport[2], (float) viewport[3]);
}

void
demo_glstate_set_gamma (demo_glstate_t *st, float gamma)
{
  glUniform1f (glGetUniformLocation (st->program, "u_gamma"), gamma);
}

void
demo_glstate_set_foreground (demo_glstate_t *st,
			     float r, float g, float b, float a)
{
  glUniform4f (glGetUniformLocation (st->program, "u_foreground"), r, g, b, a);
}

void
demo_glstate_set_debug (demo_glstate_t *st, bool enabled)
{
  glUniform1f (glGetUniformLocation (st->program, "u_debug"), enabled ? 1.f : 0.f);
}
