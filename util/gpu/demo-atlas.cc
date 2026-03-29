/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-atlas.h"

#define TEXEL_SIZE 8 /* sizeof RGBA16I texel */

#ifdef HB_GPU_ATLAS_2D
#define ATLAS_TEX_WIDTH 4096
#endif

struct demo_atlas_t {
  unsigned int refcount;

  GLuint tex_unit;
  GLuint tex_name;
#ifndef HB_GPU_ATLAS_2D
  GLuint buf_name;
#endif
  GLuint capacity; /* in texels */
  GLuint cursor;   /* in texels */
};

static void
demo_atlas_bind_texture (demo_atlas_t *at);


demo_atlas_t *
demo_atlas_create (unsigned int capacity)
{
  demo_atlas_t *at = (demo_atlas_t *) calloc (1, sizeof (demo_atlas_t));
  at->refcount = 1;

  glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *) &at->tex_unit);
  glGenTextures (1, &at->tex_name);
  at->capacity = capacity;
  at->cursor = 0;

#ifdef HB_GPU_ATLAS_2D
  demo_atlas_bind_texture (at);
  unsigned height = (capacity + ATLAS_TEX_WIDTH - 1) / ATLAS_TEX_WIDTH;
  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA16I,
		  ATLAS_TEX_WIDTH, height, 0,
		  GL_RGBA_INTEGER, GL_SHORT, NULL);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
  glGenBuffers (1, &at->buf_name);
  glBindBuffer (GL_TEXTURE_BUFFER, at->buf_name);
  glBufferData (GL_TEXTURE_BUFFER, capacity * TEXEL_SIZE, NULL, GL_STATIC_DRAW);
  demo_atlas_bind_texture (at);
  gl(TexBuffer) (GL_TEXTURE_BUFFER, GL_RGBA16I, at->buf_name);
#endif

  return at;
}

demo_atlas_t *
demo_atlas_reference (demo_atlas_t *at)
{
  if (at) at->refcount++;
  return at;
}

void
demo_atlas_destroy (demo_atlas_t *at)
{
  if (!at || --at->refcount)
    return;

  glDeleteTextures (1, &at->tex_name);
#ifndef HB_GPU_ATLAS_2D
  glDeleteBuffers (1, &at->buf_name);
#endif
  free (at);
}

static void
demo_atlas_bind_texture (demo_atlas_t *at)
{
  glActiveTexture (at->tex_unit);
#ifdef HB_GPU_ATLAS_2D
  glBindTexture (GL_TEXTURE_2D, at->tex_name);
#else
  glBindTexture (GL_TEXTURE_BUFFER, at->tex_name);
#endif
}

void
demo_atlas_set_uniforms (demo_atlas_t *at)
{
  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);

  glUniform1i (glGetUniformLocation (program, "hb_gpu_atlas"), at->tex_unit - GL_TEXTURE0);
#ifdef HB_GPU_ATLAS_2D
  glUniform1i (glGetUniformLocation (program, "hb_gpu_atlas_width"), ATLAS_TEX_WIDTH);
#endif
}

unsigned int
demo_atlas_alloc (demo_atlas_t *at,
		  const char   *data,
		  unsigned int  len_bytes)
{
  unsigned int len_texels = len_bytes / TEXEL_SIZE;

  if (at->cursor + len_texels > at->capacity)
    die ("Ran out of atlas memory");

  unsigned int offset = at->cursor;
  at->cursor += len_texels;

#ifdef HB_GPU_ATLAS_2D
  demo_atlas_bind_texture (at);
  /* Upload row by row since data may span multiple rows. */
  unsigned int remaining = len_texels;
  unsigned int src_offset = 0;
  unsigned int dst_offset = offset;
  while (remaining)
  {
    unsigned int x = dst_offset % ATLAS_TEX_WIDTH;
    unsigned int y = dst_offset / ATLAS_TEX_WIDTH;
    unsigned int row_count = ATLAS_TEX_WIDTH - x;
    if (row_count > remaining)
      row_count = remaining;
    gl(TexSubImage2D) (GL_TEXTURE_2D, 0,
		       x, y, row_count, 1,
		       GL_RGBA_INTEGER, GL_SHORT,
		       data + src_offset * TEXEL_SIZE);
    src_offset += row_count;
    dst_offset += row_count;
    remaining -= row_count;
  }
#else
  glBindBuffer (GL_TEXTURE_BUFFER, at->buf_name);
  glBufferSubData (GL_TEXTURE_BUFFER,
		   offset * TEXEL_SIZE,
		   len_bytes,
		   data);
#endif

  return offset;
}

unsigned int
demo_atlas_get_used (demo_atlas_t *at)
{
  return at->cursor;
}

void
demo_atlas_clear (demo_atlas_t *at)
{
  at->cursor = 0;
}
