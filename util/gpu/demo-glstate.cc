/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-glstate.h"

struct demo_glstate_t {
  GLuint program;
  GLuint program_msaa;
  bool msaa;
  demo_atlas_t *atlas;
  /* Cached uniform values for program switching. */
  float gamma;
  float fg[4];
  float debug;
  float stem_darkening;
};

demo_glstate_t *
demo_glstate_create (void)
{
  demo_glstate_t *st = (demo_glstate_t *) calloc (1, sizeof (demo_glstate_t));

  st->program = demo_shader_create_program (false);
  st->program_msaa = demo_shader_create_program (true);
  st->msaa = false;
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
  glDeleteProgram (st->program_msaa);

  free (st);
}

static GLuint
demo_glstate_active_program (demo_glstate_t *st)
{
  return st->msaa ? st->program_msaa : st->program;
}

static void
demo_glstate_apply_uniforms (demo_glstate_t *st)
{
  GLuint p = demo_glstate_active_program (st);
  demo_atlas_set_uniforms (st->atlas);
  glUniform1f (glGetUniformLocation (p, "u_gamma"), st->gamma);
  glUniform4fv (glGetUniformLocation (p, "u_foreground"), 1, st->fg);
  glUniform1f (glGetUniformLocation (p, "u_debug"), st->debug);
  glUniform1f (glGetUniformLocation (p, "u_stem_darkening"), st->stem_darkening);
}

void
demo_glstate_set_msaa (demo_glstate_t *st, bool enabled)
{
  st->msaa = enabled;
  glUseProgram (demo_glstate_active_program (st));
  demo_glstate_apply_uniforms (st);
}

void
demo_glstate_setup (demo_glstate_t *st)
{
  glUseProgram (demo_glstate_active_program (st));

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
  glUniformMatrix4fv (glGetUniformLocation (demo_glstate_active_program (st), "u_matViewProjection"), 1, GL_FALSE, mat);

  GLint viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  glUniform2f (glGetUniformLocation (demo_glstate_active_program (st), "u_viewport"),
	       (float) viewport[2], (float) viewport[3]);
}

void
demo_glstate_set_gamma (demo_glstate_t *st, float gamma)
{
  st->gamma = gamma;
  glUniform1f (glGetUniformLocation (demo_glstate_active_program (st), "u_gamma"), gamma);
}

void
demo_glstate_set_foreground (demo_glstate_t *st,
			     float r, float g, float b, float a)
{
  st->fg[0] = r; st->fg[1] = g; st->fg[2] = b; st->fg[3] = a;
  glUniform4f (glGetUniformLocation (demo_glstate_active_program (st), "u_foreground"), r, g, b, a);
}

void
demo_glstate_set_debug (demo_glstate_t *st, bool enabled)
{
  st->debug = enabled ? 1.f : 0.f;
  glUniform1f (glGetUniformLocation (demo_glstate_active_program (st), "u_debug"), st->debug);
}

void
demo_glstate_set_stem_darkening (demo_glstate_t *st, bool enabled)
{
  st->stem_darkening = enabled ? 1.f : 0.f;
  glUniform1f (glGetUniformLocation (demo_glstate_active_program (st), "u_stem_darkening"), st->stem_darkening);
}
