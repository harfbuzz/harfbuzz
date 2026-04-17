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
#include "gpu/demo-renderer.h"
#include "gpu/demo-view.h"

#include "options.hh"
#include "font-options.hh"
#include "view-options.hh"

#include <string>
#include <vector>

#define WINDOW_W 700
#define WINDOW_H 700

struct gpu_output_t
{
  static constexpr bool repeat_shape = false;

  ~gpu_output_t ()
  {
    g_free (output_file);
  }

  hb_bool_t use_metal = false;
  hb_bool_t use_d3d11 = false;
  hb_bool_t demo = false;
  hb_bool_t bench = false;
  hb_bool_t show_extents = false; /* --show-extents */
  /* Initial state for interactive toggles; each corresponds to
   * a keyboard shortcut in demo_view_t. */
  hb_bool_t dark             = false;  /* --dark              -> b */
  hb_bool_t no_vsync         = false;  /* --no-vsync          -> v */
  hb_bool_t fullscreen       = false;  /* --fullscreen        -> f */
  hb_bool_t no_stem_darken   = false;  /* --no-stem-darken    -> s */
  hb_bool_t debug            = false;  /* --debug             -> d */
  char *type_text = nullptr;
  char *output_file = nullptr;  /* PPM only; "-" for stdout */
  view_options_t view;

  static gboolean
  parse_bench (const char *name G_GNUC_UNUSED,
	       const char *arg G_GNUC_UNUSED,
	       gpointer    data,
	       GError    **error G_GNUC_UNUSED)
  {
    auto *self = (gpu_output_t *) data;
    self->demo = true;
    self->bench = true;
    if (!self->type_text)
      self->type_text = g_strdup ("v30=f ");
    return true;
  }

  void add_options (option_parser_t *parser)
  {
    GOptionEntry entries[] =
    {
      {"demo",		0, 0, G_OPTION_ARG_NONE,	&this->demo,		"Use built-in demo font and text",	nullptr},
      {"bench",		0, G_OPTION_FLAG_NO_ARG,
				G_OPTION_ARG_CALLBACK,	(gpointer) &parse_bench,"Demo in fullscreen benchmark mode",	nullptr},
      {"type",		'T', 0, G_OPTION_ARG_STRING,	&this->type_text,	"Type these keystrokes on start",	"keys"},
      {"draw",		0, 0, G_OPTION_ARG_NONE,	&this->view.force_draw,	"Force monochrome draw path",		nullptr},
      {"paint",		0, 0, G_OPTION_ARG_NONE,	&this->view.force_paint,	"Force color paint path",		nullptr},
      {"output-file",	'o', 0, G_OPTION_ARG_STRING,	&this->output_file,	"Render one frame to PPM file (\"-\" for stdout) and exit","filename"},
      {"show-extents",	0, 0, G_OPTION_ARG_NONE,	&this->show_extents,	"Draw a frame around each glyph's ink extents",	nullptr},
      /* Interactive-toggle defaults.  Each mirrors a keybinding
       * in the running demo. */
      {"dark",		0, 0, G_OPTION_ARG_NONE,	&this->dark,		"Start in dark mode (toggle: b)",	nullptr},
      {"no-vsync",	0, 0, G_OPTION_ARG_NONE,	&this->no_vsync,	"Start with vsync off (toggle: v)",	nullptr},
      {"fullscreen",	0, 0, G_OPTION_ARG_NONE,	&this->fullscreen,	"Start fullscreen (toggle: f)",		nullptr},
      {"no-stem-darken",0, 0, G_OPTION_ARG_NONE,	&this->no_stem_darken,	"Start with stem darkening off (toggle: s)",nullptr},
      {"debug",		0, 0, G_OPTION_ARG_NONE,	&this->debug,		"Start with debug heatmap on (toggle: d)",nullptr},
      /* Reuse the storage in `view` for the four options hb-gpu
       * actually honors.  view_options_t::post_parse() does the
       * parsing for us; gpu_output_t::post_parse() below forwards
       * to it. */
      {"background",	0, 0, G_OPTION_ARG_STRING,	&this->view.back,		"Set background color (default: " DEFAULT_BACK ")","rrggbb/rrggbbaa"},
      {"foreground",	0, 0, G_OPTION_ARG_STRING,	&this->view.fore,		"Set foreground color (default: " DEFAULT_FORE ")","rrggbb/rrggbbaa"},
      {"font-palette",	0, 0, G_OPTION_ARG_INT,		&this->view.palette,		"Set font palette (default: 0)",	"index"},
      {"custom-palette",0, 0, G_OPTION_ARG_STRING,	&this->view.custom_palette,	"Custom palette",			"comma-separated colors"},
#ifdef __APPLE__
      {"metal",		0, 0, G_OPTION_ARG_NONE,	&this->use_metal,	"Use Metal renderer",			nullptr},
#endif
#ifdef _WIN32
      {"direct3d",	0, 0, G_OPTION_ARG_NONE,	&this->use_d3d11,	"Use Direct3D 11 renderer",		nullptr},
#endif
      {nullptr}
    };
    parser->add_group (entries,
		       "gpu",
		       "GPU options:",
		       "Options for GPU rendering",
		       this);
  }

  void post_parse (GError **error)
  {
    /* Delegate to view_options_t: parses foreground / background
     * / custom-palette strings into rgba_color_t / GArray. */
    view.post_parse (error);
  }

  template <typename app_t>
  void init (hb_buffer_t *buffer_ HB_UNUSED, const app_t *app)
  {
    font = app->font;
    face = hb_font_get_face (font);

    int x_scale, y_scale;
    hb_font_get_scale (font, &x_scale, &y_scale);
    font_size = (double) y_scale;

    glfwSetErrorCallback ([] (int error, const char *desc) {
      fprintf (stderr, "GLFW error %d: %s\n", error, desc);
    });
    if (!glfwInit ())
      fail (false, "Failed to initialize GLFW");

    /* Pick draw vs paint mode.  Explicit --draw / --paint wins;
     * otherwise auto: fonts without color paint get the draw path
     * (smaller shader, better perf).  A mismatched pair of flags
     * is a user error; prefer paint. */
    draw_only = view.force_paint ? false
	      : view.force_draw  ? true
	      : !(hb_ot_color_has_paint (face) ||
		  hb_ot_color_has_layers (face));

    if (use_metal)
      init_metal ();
#ifdef _WIN32
    else if (use_d3d11)
      init_d3d11 ();
#endif
    else
      init_gl ();

    vu = demo_view_create (renderer, window);

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

    demo_font_ = demo_font_create (font, renderer->get_atlas (), draw_only);
    demo_font_set_palette (demo_font_, view.palette);
    demo_font_set_show_extents (demo_font_, show_extents);
    if (view.custom_palette_entries)
    {
      for (unsigned i = 0; i < view.custom_palette_entries->len; i++)
      {
	auto &e = g_array_index (view.custom_palette_entries,
				 view_options_t::custom_palette_entry_t, i);
	hb_color_t c = HB_COLOR (e.color.b, e.color.g, e.color.r, e.color.a);
	demo_font_set_custom_palette_color (demo_font_, e.index, c);
      }
    }
    buf = demo_buffer_create ();

    demo_point_t top_left = {0, 0};
    demo_buffer_move_to (buf, &top_left);
    demo_buffer_current_line (buf, font_size);
  }

  void init_gl ()
  {
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    if (output_file)
      glfwWindowHint (GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow (WINDOW_W, WINDOW_H, "HarfBuzz GPU", NULL, NULL);
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

    renderer = demo_renderer_create_gl (window, draw_only);
  }

  void init_metal ()
  {
#ifdef __APPLE__
    glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow (WINDOW_W, WINDOW_H, "HarfBuzz GPU (Metal)", NULL, NULL);
    if (!window) {
      glfwTerminate ();
      fail (false, "Failed to create GLFW window");
    }

    renderer = demo_renderer_create_metal (window, draw_only);
    if (!renderer) {
      glfwTerminate ();
      fail (false, "Failed to initialize Metal");
    }
#else
    fail (false, "Metal is only available on macOS");
#endif
  }

  void init_d3d11 ()
  {
#ifdef _WIN32
    glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow (WINDOW_W, WINDOW_H, "HarfBuzz GPU (Direct3D)", NULL, NULL);
    if (!window) {
      glfwTerminate ();
      fail (false, "Failed to create GLFW window");
    }

    renderer = demo_renderer_create_d3d11 (window, draw_only);
    if (!renderer) {
      glfwTerminate ();
      fail (false, "Failed to initialize Direct3D 11");
    }
#else
    fail (false, "Direct3D is only available on Windows");
#endif
  }

  void write_output_file ()
  {
    int fb_w = 0, fb_h = 0;
    glfwGetFramebufferSize (window, &fb_w, &fb_h);
    if (fb_w <= 0 || fb_h <= 0)
      fail (false, "Framebuffer has zero size");

    FILE *fp;
    bool use_stdout = (0 == strcmp (output_file, "-"));
    if (use_stdout)
    {
#if defined(_WIN32) || defined(__CYGWIN__)
      setmode (fileno (stdout), O_BINARY);
#endif
      fp = stdout;
    }
    else
    {
      fp = fopen (output_file, "wb");
      if (!fp)
	fail (false, "Cannot open output file `%s': %s",
	      output_file, strerror (errno));
    }

    /* glReadPixels delivers rows bottom-up; PPM writes top-down,
     * so we flip while assembling. */
    std::vector<uint8_t> pixels ((size_t) fb_w * (size_t) fb_h * 3);
    glPixelStorei (GL_PACK_ALIGNMENT, 1);
    glReadPixels (0, 0, fb_w, fb_h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data ());

    fprintf (fp, "P6\n%d %d\n255\n", fb_w, fb_h);
    const unsigned row_bytes = (unsigned) fb_w * 3u;
    for (int y = fb_h - 1; y >= 0; y--)
      fwrite (pixels.data () + (size_t) y * row_bytes, 1, row_bytes, fp);

    if (!use_stdout)
      fclose (fp);
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
  }

  void error (const char *message)
  {
    fprintf (stderr, "error: %s\n", message);
  }

  template <typename app_t>
  void finish (hb_buffer_t *buffer_ HB_UNUSED, const app_t *app HB_UNUSED)
  {
    hb_gpu_demo_quiet = output_file ? 1 : 0;
    demo_font_print_stats (demo_font_);
    demo_view_print_help (vu);
    demo_view_setup (vu);

    /* Apply view-options foreground/background.  These default to
     * #000000 / #FFFFFF which match the demo's default LIGHT mode,
     * so always setting them is a no-op unless the user passed a
     * flag.  The dark-mode toggle (b key) mutates renderer state
     * afterwards and stays in effect. */
    renderer->set_foreground (view.foreground_color.r / 255.f,
			      view.foreground_color.g / 255.f,
			      view.foreground_color.b / 255.f,
			      view.foreground_color.a / 255.f);
    renderer->set_background (view.background_color.r / 255.f,
			      view.background_color.g / 255.f,
			      view.background_color.b / 255.f,
			      view.background_color.a / 255.f);

    /* Apply cmdline-driven toggle defaults via the same keystroke
     * path the user would use interactively. */
    std::string toggles;
    if (dark)            toggles += 'b';
    if (no_vsync)        toggles += 'v';
    if (fullscreen)      toggles += 'f';
    if (no_stem_darken)  toggles += 's';
    if (debug)           toggles += 'd';
    if (!toggles.empty ())
      demo_view_type (vu, toggles.c_str ());

    if (type_text)
      demo_view_type (vu, type_text);
    if (bench)
      demo_view_set_fps_quit (vu, 3);

    demo_view_display (vu, buf);
    glfwPollEvents ();
    demo_view_display (vu, buf);

    if (output_file)
      write_output_file ();
    else
    {
      while (!glfwWindowShouldClose (window))
      {
	glfwPollEvents ();
	if (demo_view_should_redraw (vu))
	  demo_view_display (vu, buf);
	else
	  glfwWaitEvents ();
      }
    }

    demo_buffer_destroy (buf);
    demo_font_destroy (demo_font_);
    demo_view_destroy (vu);
    delete renderer;

    glfwDestroyWindow (window);
    glfwTerminate ();
  }

  private:

  bool   draw_only = false;
  double font_size = 1;
  hb_font_t *font = nullptr;
  hb_face_t *face = nullptr;

  GLFWwindow *window = nullptr;
  demo_renderer_t *renderer = nullptr;
  demo_view_t *vu = nullptr;
  demo_font_t *demo_font_ = nullptr;
  demo_buffer_t *buf = nullptr;
};

#endif /* GPU_OUTPUT_HH */
