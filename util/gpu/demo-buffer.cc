/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-buffer.h"

static hb_feature_t g_features[64];
static unsigned g_feature_count = 0;

void
demo_buffer_set_features (const hb_feature_t *features, unsigned count)
{
  g_feature_count = count < 64 ? count : 64;
  memcpy (g_features, features, g_feature_count * sizeof (hb_feature_t));
}

struct demo_buffer_t {
  demo_point_t cursor;
  std::vector<glyph_vertex_t> *vertices;
  demo_extents_t ink_extents;
  demo_extents_t logical_extents;
  unsigned generation;
};

demo_buffer_t *
demo_buffer_create (void)
{
  demo_buffer_t *buffer = (demo_buffer_t *) calloc (1, sizeof (demo_buffer_t));

  buffer->vertices = new std::vector<glyph_vertex_t>;

  demo_buffer_clear (buffer);

  return buffer;
}

void
demo_buffer_destroy (demo_buffer_t *buffer)
{
  if (!buffer)
    return;

  delete buffer->vertices;
  free (buffer);
}


void
demo_buffer_clear (demo_buffer_t *buffer)
{
  buffer->vertices->clear ();
  demo_extents_clear (&buffer->ink_extents);
  demo_extents_clear (&buffer->logical_extents);
  buffer->generation++;
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
    hb_shape (hb_font, hb_buffer, g_features, g_feature_count);

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
}

void
demo_buffer_current_line (demo_buffer_t *buffer,
			  double         font_size)
{
  buffer->cursor.x = 0;
  buffer->cursor.y += font_size;
}

void
demo_buffer_add_glyph (demo_buffer_t      *buffer,
		       demo_font_t        *font,
		       double              font_size,
		       unsigned int        glyph_index,
		       double              x_offset,
		       double              y_offset,
		       double              x_advance)
{
  glyph_info_t gi;
  demo_font_lookup_glyph (font, glyph_index, &gi);

  double scale = font_size / gi.upem;

  demo_extents_t ink_extents;
  demo_point_t position = buffer->cursor;
  position.x += scale * x_offset;
  position.y -= scale * y_offset;
  demo_shader_add_glyph_vertices (position, font_size, &gi, buffer->vertices, &ink_extents);
  demo_extents_extend (&buffer->ink_extents, &ink_extents);

  demo_point_t corner;
  corner.x = buffer->cursor.x;
  corner.y = buffer->cursor.y - font_size;
  demo_extents_add (&buffer->logical_extents, &corner);
  corner.x = buffer->cursor.x + scale * gi.advance;
  corner.y = buffer->cursor.y;
  demo_extents_add (&buffer->logical_extents, &corner);

  buffer->cursor.x += scale * x_advance;
}

glyph_vertex_t *
demo_buffer_get_vertices (demo_buffer_t *buffer,
			  unsigned int  *count)
{
  if (count)
    *count = buffer->vertices->size ();
  if (buffer->vertices->empty ())
    return nullptr;
  return &(*buffer->vertices)[0];
}

unsigned
demo_buffer_get_generation (demo_buffer_t *buffer)
{
  return buffer->generation;
}
