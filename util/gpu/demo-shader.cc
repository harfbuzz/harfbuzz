/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-shader.h"

#include "demo-vertex-glsl.hh"
#include "demo-fragment-glsl.hh"



static GLuint
compile_shader (GLenum         type,
		GLsizei        count,
		const GLchar** sources)
{
  GLuint shader;
  GLint compiled;

  if (!(shader = glCreateShader (type)))
    return shader;

  glShaderSource (shader, count, sources, 0);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint info_len = 0;
    LOGW ("%s shader failed to compile\n",
	     type == GL_VERTEX_SHADER ? "Vertex" : "Fragment");
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetShaderInfoLog (shader, info_len, NULL, info_log);

      LOGW ("%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return shader;
}

static GLuint
link_program (GLuint vertex_shader,
	      GLuint fragment_shader)
{
  GLuint program;
  GLint linked;

  program = glCreateProgram ();
  glAttachShader (program, vertex_shader);
  glAttachShader (program, fragment_shader);
  glLinkProgram (program);
  glDeleteShader (vertex_shader);
  glDeleteShader (fragment_shader);

  glGetProgramiv (program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint info_len = 0;
    LOGW ("Program failed to link\n");
    glGetProgramiv (program, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetProgramInfoLog (program, info_len, NULL, info_log);

      LOGW ("%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return program;
}

GLuint
demo_shader_create_program (bool draw_only)
{
  GLuint vertex_shader, fragment_shader, program;

#ifdef HB_GPU_ATLAS_2D
  const GLchar *version_es = "#version 300 es\nprecision highp float;\nprecision highp int;\n#define HB_GPU_ATLAS_2D\n";
  const GLchar *version = version_es;
#else
  const GLchar *version = "#version 330\n";
#endif
  const GLchar *preamble = draw_only ? "#define HB_GPU_DEMO_DRAW\n" : "";

  const GLchar *vert_sources[] = {version, preamble,
				  hb_gpu_shader_source      (HB_GPU_SHADER_STAGE_VERTEX, HB_GPU_SHADER_LANG_GLSL),
				  hb_gpu_draw_shader_source (HB_GPU_SHADER_STAGE_VERTEX, HB_GPU_SHADER_LANG_GLSL),
				  demo_vertex_glsl};
  vertex_shader = compile_shader (GL_VERTEX_SHADER,
				  ARRAY_LEN (vert_sources),
				  vert_sources);

  /* Mode-specific fragment shader: each path pulls in only the
   * helper source it actually calls, keeping the compiled program
   * small. */
  GLuint sc = 0;
  const GLchar *frag_sources[5];
  frag_sources[sc++] = version;
  frag_sources[sc++] = preamble;
  frag_sources[sc++] = hb_gpu_shader_source      (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL);
  frag_sources[sc++] = draw_only
    ? hb_gpu_draw_shader_source  (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL)
    : hb_gpu_paint_shader_source (HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL);
  frag_sources[sc++] = demo_fragment_glsl;
  fragment_shader = compile_shader (GL_FRAGMENT_SHADER, sc, frag_sources);

  program = link_program (vertex_shader, fragment_shader);
  return program;
}
