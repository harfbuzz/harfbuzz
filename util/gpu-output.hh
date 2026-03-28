/*
 * Copyright (C) 2026  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Author(s): Behdad Esfahbod
 */

#ifndef GPU_OUTPUT_HH
#define GPU_OUTPUT_HH

#include "gpu/demo-common.h"
#include "gpu/demo-buffer.h"
#include "gpu/demo-font.h"
#include "gpu/demo-glstate.h"
#include "gpu/demo-view.h"

#include "options.hh"
#include "font-options.hh"

#define WINDOW_W 700
#define WINDOW_H 700

struct gpu_output_t
{
  static constexpr bool repeat_shape = false;

  void add_options (option_parser_t *parser HB_UNUSED) {}

  template <typename app_t>
  void init (hb_buffer_t *buffer_ HB_UNUSED, const app_t *app)
  {
    font = app->font;
    face = hb_font_get_face (font);

    int x_scale, y_scale;
    hb_font_get_scale (font, &x_scale, &y_scale);
    font_size = (double) y_scale;

    /* Setup GL */
    glfwSetErrorCallback ([] (int error, const char *desc) {
      fprintf (stderr, "GLFW error %d: %s\n", error, desc);
    });
    if (!glfwInit ())
      fail (false, "Failed to initialize GLFW");

    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint (GLFW_SRGB_CAPABLE, GLFW_TRUE);

    window = glfwCreateWindow (WINDOW_W, WINDOW_H, "HarfBuzz GPU Demo", NULL, NULL);
    if (!window) {
      glfwTerminate ();
      fail (false, "Failed to create GLFW window");
    }
    glfwMakeContextCurrent (window);

    glewExperimental = GL_TRUE;
    glewInit ();
    while (glGetError () != GL_NO_ERROR)
      ;
    if (!glewIsSupported ("GL_VERSION_3_3"))
      fail (false, "OpenGL 3.3 not supported");

    {
      int fb_width, fb_height;
      glfwGetFramebufferSize (window, &fb_width, &fb_height);
      glViewport (0, 0, fb_width, fb_height);
    }

    st = demo_glstate_create ();
    vu = demo_view_create (st, window);

    glfwSetWindowUserPointer (window, this);
    glfwSetFramebufferSizeCallback (window, [] (GLFWwindow *w, int width, int height) {
      auto *self = (gpu_output_t *) glfwGetWindowUserPointer (w);
      demo_view_reshape_func (self->vu, width, height);
    });
    glfwSetKeyCallback (window, [] (GLFWwindow *w, int key, int scancode, int action, int mods) {
      auto *self = (gpu_output_t *) glfwGetWindowUserPointer (w);
      demo_view_key_func (self->vu, key, scancode, action, mods);
    });
    glfwSetCharCallback (window, [] (GLFWwindow *w, unsigned int cp) {
      auto *self = (gpu_output_t *) glfwGetWindowUserPointer (w);
      demo_view_char_func (self->vu, cp);
    });
    glfwSetMouseButtonCallback (window, [] (GLFWwindow *w, int button, int action, int mods) {
      auto *self = (gpu_output_t *) glfwGetWindowUserPointer (w);
      demo_view_mouse_func (self->vu, button, action, mods);
    });
    glfwSetScrollCallback (window, [] (GLFWwindow *w, double xoff, double yoff) {
      auto *self = (gpu_output_t *) glfwGetWindowUserPointer (w);
      demo_view_scroll_func (self->vu, xoff, yoff);
    });
    glfwSetCursorPosCallback (window, [] (GLFWwindow *w, double x, double y) {
      auto *self = (gpu_output_t *) glfwGetWindowUserPointer (w);
      demo_view_motion_func (self->vu, x, y);
    });

    demo_font_ = demo_font_create (font, demo_glstate_get_atlas (st));
    buf = demo_buffer_create ();

    demo_point_t top_left = {0, 0};
    demo_buffer_move_to (buf, &top_left);
    demo_buffer_current_line (buf, font_size);
  }

  void new_line ()
  {
    demo_buffer_current_line (buf, font_size);
  }

  void consume_text (hb_buffer_t *buffer HB_UNUSED,
		     const char *text HB_UNUSED,
		     unsigned text_len HB_UNUSED,
		     hb_bool_t utf8_clusters HB_UNUSED) {}

  void consume_glyphs (hb_buffer_t *buffer,
		       const char *text HB_UNUSED,
		       unsigned text_len HB_UNUSED,
		       hb_bool_t utf8_clusters HB_UNUSED)
  {
    unsigned vert_start = demo_buffer_get_vertex_count (buf);

    unsigned count = hb_buffer_get_length (buffer);
    hb_glyph_info_t *infos = hb_buffer_get_glyph_infos (buffer, nullptr);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (buffer, nullptr);

    for (unsigned i = 0; i < count; i++)
      demo_buffer_add_glyph (buf, demo_font_,
			     font_size,
			     infos[i].codepoint,
			     pos[i].x_offset,
			     pos[i].y_offset,
			     pos[i].x_advance);

    unsigned vert_end = demo_buffer_get_vertex_count (buf);

    if (hb_buffer_get_direction (buffer) == HB_DIRECTION_RTL && vert_end > vert_start)
      rtl_lines.push_back ({vert_start, vert_end});
  }

  void error (const char *message)
  {
    fprintf (stderr, "error: %s\n", message);
  }

  template <typename app_t>
  void finish (hb_buffer_t *buffer_ HB_UNUSED, const app_t *app HB_UNUSED)
  {
    /* Right-align RTL lines to the logical extents width. */
    if (!rtl_lines.empty ())
    {
      /* Compute average line width across RTL lines. */
      float sum_max_x = 0;
      for (auto &lr : rtl_lines)
      {
	float line_max_x = 0;
	for (unsigned i = lr.start; i < lr.end; i++)
	{
	  float x = demo_buffer_get_vertex_x (buf, i);
	  if (x > line_max_x)
	    line_max_x = x;
	}
	lr.max_x = line_max_x;
	sum_max_x += line_max_x;
      }
      float avg_width = sum_max_x / rtl_lines.size ();

      /* Right-align each RTL line to the average width. */
      for (auto &lr : rtl_lines)
	demo_buffer_shift_vertices (buf, lr.start, lr.end,
				    avg_width - lr.max_x);
    }

    demo_font_print_stats (demo_font_);
    demo_view_print_help (vu);
    demo_view_setup (vu);

    demo_view_display (vu, buf);
    glfwPollEvents ();
    demo_view_display (vu, buf);

    while (!glfwWindowShouldClose (window))
    {
      glfwPollEvents ();
      if (demo_view_should_redraw (vu))
	demo_view_display (vu, buf);
      else
	glfwWaitEvents ();
    }

    demo_buffer_destroy (buf);
    demo_font_destroy (demo_font_);
    demo_view_destroy (vu);
    demo_glstate_destroy (st);

    glfwDestroyWindow (window);
    glfwTerminate ();
  }

  private:

  struct line_range_t { unsigned start, end; float max_x; };

  double font_size = 1;
  hb_font_t *font = nullptr;
  hb_face_t *face = nullptr;
  std::vector<line_range_t> rtl_lines;

  GLFWwindow *window = nullptr;
  demo_glstate_t *st = nullptr;
  demo_view_t *vu = nullptr;
  demo_font_t *demo_font_ = nullptr;
  demo_buffer_t *buf = nullptr;
};

#endif /* GPU_OUTPUT_HH */
