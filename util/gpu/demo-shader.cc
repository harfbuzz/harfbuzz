/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-shader.h"

static const char *demo_vertex_glsl = R"glsl(
uniform mat4 u_matViewProjection;
uniform vec2 u_viewport;

in vec2 a_position;
in vec2 a_texcoord;
in vec2 a_normal;
in float a_emPerPos;
in uint a_glyphLoc;

out vec2 v_texcoord;
flat out uint v_glyphLoc;

void main ()
{
  vec2 pos = a_position;
  vec2 tex = a_texcoord;

  vec4 jac = vec4 (a_emPerPos, 0.0, 0.0, -a_emPerPos);

  hb_gpu_dilate (pos, tex, a_normal, jac,
		 u_matViewProjection, u_viewport);

  gl_Position = u_matViewProjection * vec4 (pos, 0.0, 1.0);
  v_texcoord = tex;
  v_glyphLoc = a_glyphLoc;
}
)glsl";

static const char *demo_fragment_glsl = R"glsl(
uniform float u_gamma;
uniform float u_debug;
uniform float u_stem_darkening;
uniform vec4 u_foreground;

in vec2 v_texcoord;
flat in uint v_glyphLoc;

out vec4 fragColor;

void main ()
{
  float coverage = hb_gpu_render (v_texcoord, v_glyphLoc);

  /* Stem darkening / thinning at small sizes.
   * Light text on dark: stems get too fat → thin them (exponent > 1).
   * Dark text on light: stems get too thin → darken them (exponent < 1).
   * The foreground brightness tells us which mode we're in. */
  if (u_stem_darkening > 0.0)
    coverage = hb_gpu_darken (coverage,
      dot (u_foreground.rgb, vec3 (1.0 / 3.0)),
      hb_gpu_ppem (v_texcoord, v_glyphLoc));

  if (u_gamma != 1.0)
    coverage = pow (coverage, u_gamma);

  if (u_debug > 0.0)
  {
    ivec2 counts = _hb_gpu_curve_counts (v_texcoord, v_glyphLoc);
    float r = clamp (float (counts.x) / 8.0, 0.0, 1.0);
    float g = clamp (float (counts.y) / 8.0, 0.0, 1.0);
    fragColor = vec4 (r, g, coverage, max (max (r, g), coverage));
    return;
  }

  fragColor = vec4 (u_foreground.rgb, u_foreground.a * coverage);
}
)glsl";



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
demo_shader_create_program (void)
{
  GLuint vertex_shader, fragment_shader, program;

#ifdef HB_GPU_ATLAS_2D
  const GLchar *preamble = "#version 300 es\nprecision highp float;\nprecision highp int;\n#define HB_GPU_ATLAS_2D\n";
#else
  const GLchar *preamble = "#version 330\n";
#endif

  const GLchar *vert_sources[] = {preamble,
				  hb_gpu_shader_vertex_source (HB_GPU_SHADER_LANG_GLSL),
				  demo_vertex_glsl};
  vertex_shader = compile_shader (GL_VERTEX_SHADER,
				  ARRAY_LEN (vert_sources),
				  vert_sources);

  const GLchar *frag_sources[] = {preamble,
				  hb_gpu_shader_fragment_source (HB_GPU_SHADER_LANG_GLSL),
				  demo_fragment_glsl};
  fragment_shader = compile_shader (GL_FRAGMENT_SHADER,
				    ARRAY_LEN (frag_sources),
				    frag_sources);

  program = link_program (vertex_shader, fragment_shader);
  return program;
}
