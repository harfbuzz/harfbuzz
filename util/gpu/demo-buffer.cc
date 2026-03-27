/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-buffer.h"

struct demo_buffer_t {
  demo_point_t cursor;
  std::vector<glyph_vertex_t> *vertices;
  demo_extents_t ink_extents;
  demo_extents_t logical_extents;
  bool dirty;
  GLuint vao_name;
  GLuint buf_name;
};

demo_buffer_t *
demo_buffer_create (void)
{
  demo_buffer_t *buffer = (demo_buffer_t *) calloc (1, sizeof (demo_buffer_t));

  buffer->vertices = new std::vector<glyph_vertex_t>;
  glGenVertexArrays (1, &buffer->vao_name);
  glGenBuffers (1, &buffer->buf_name);

  demo_buffer_clear (buffer);

  return buffer;
}

void
demo_buffer_destroy (demo_buffer_t *buffer)
{
  if (!buffer)
    return;

  glDeleteVertexArrays (1, &buffer->vao_name);
  glDeleteBuffers (1, &buffer->buf_name);
  delete buffer->vertices;
  free (buffer);
}


void
demo_buffer_clear (demo_buffer_t *buffer)
{
  buffer->vertices->clear ();
  demo_extents_clear (&buffer->ink_extents);
  demo_extents_clear (&buffer->logical_extents);
  buffer->dirty = true;
}

void
demo_buffer_extents (demo_buffer_t  *buffer,
		     demo_extents_t *ink_extents,
		     demo_extents_t *logical_extents)
{
  if (ink_extents)
    *ink_extents = buffer->ink_extents;
  if (logical_extents)
    *logical_extents = buffer->logical_extents;
}

void
demo_buffer_move_to (demo_buffer_t      *buffer,
		     const demo_point_t *p)
{
  buffer->cursor = *p;
}

void
demo_buffer_add_text (demo_buffer_t *buffer,
		      const char    *utf8,
		      demo_font_t   *font,
		      double         font_size)
{
  hb_face_t *hb_face = demo_font_get_face (font);
  hb_font_t *hb_font = demo_font_get_font (font);
  hb_buffer_t *hb_buffer = hb_buffer_create ();

  demo_point_t top_left = buffer->cursor;
  buffer->cursor.y += font_size;
  double scale = font_size / hb_face_get_upem (hb_face);

  while (utf8)
  {
    const char *end = strchr (utf8, '\n');

    hb_buffer_clear_contents (hb_buffer);
    hb_buffer_add_utf8 (hb_buffer, utf8, end ? end - utf8 : -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);
    hb_shape (hb_font, hb_buffer, NULL, 0);

    unsigned count;
    hb_glyph_info_t *infos = hb_buffer_get_glyph_infos (hb_buffer, &count);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);
    for (unsigned i = 0; i < count; i++)
    {
      unsigned int glyph_index = infos[i].codepoint;
      glyph_info_t gi;
      demo_font_lookup_glyph (font, glyph_index, &gi);

      demo_extents_t ink_extents;
      demo_point_t position = buffer->cursor;
      position.x += scale * pos[i].x_offset;
      position.y -= scale * pos[i].y_offset;
      demo_shader_add_glyph_vertices (position, font_size, &gi, buffer->vertices, &ink_extents);
      demo_extents_extend (&buffer->ink_extents, &ink_extents);

      demo_point_t corner;
      corner.x = buffer->cursor.x;
      corner.y = buffer->cursor.y - font_size;
      demo_extents_add (&buffer->logical_extents, &corner);
      corner.x = buffer->cursor.x + scale * gi.advance;
      corner.y = buffer->cursor.y;
      demo_extents_add (&buffer->logical_extents, &corner);

      buffer->cursor.x += scale * pos[i].x_advance;
    }

    if (end) {
      buffer->cursor.y += font_size;
      buffer->cursor.x = top_left.x;
      utf8 = end + 1;
    }
      else
      utf8 = NULL;
  }

  hb_buffer_destroy (hb_buffer);
  buffer->dirty = true;
}

void
demo_buffer_draw (demo_buffer_t *buffer)
{
  GLint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, &program);

  glBindVertexArray (buffer->vao_name);
  glBindBuffer (GL_ARRAY_BUFFER, buffer->buf_name);
  if (buffer->dirty) {
    glBufferData (GL_ARRAY_BUFFER,
		  sizeof (glyph_vertex_t) * buffer->vertices->size (),
		  (const char *) &(*buffer->vertices)[0], GL_STATIC_DRAW);
    buffer->dirty = false;
  }

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

  glDrawArrays (GL_TRIANGLES, 0, buffer->vertices->size ());

  glDisableVertexAttribArray (loc_pos);
  glDisableVertexAttribArray (loc_tex);
  glDisableVertexAttribArray (loc_normal);
  glDisableVertexAttribArray (loc_epp);
  glDisableVertexAttribArray (loc_glyph);
  glBindVertexArray (0);
}
