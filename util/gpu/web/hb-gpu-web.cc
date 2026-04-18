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

/*
 * Web (Emscripten) build of the hb-gpu demo.
 *
 * Reuses the desktop demo code with two changes:
 *   1. emscripten_set_main_loop replaces the blocking while loop
 *   2. HB_GPU_ATLAS_2D is defined (WebGL2 lacks texture buffers)
 *
 * Build:
 *   source emsdk/emsdk_env.sh
 *   ./util/gpu/web/build.sh
 *
 * Serve:
 *   python3 -m http.server -d util/gpu/web/out
 */

#include <emscripten.h>
#include <emscripten/html5.h>

/* Pull in the demo code — all headers are self-contained */
#include "../demo-common.h"
#include "../demo-buffer.h"
#include "../demo-font.h"
#include "../demo-renderer.h"
#include "../demo-view.h"

#include "../default-text-combined.hh"
#include "../default-text-en.hh"
#include "../default-font.hh"

static demo_renderer_t *renderer;
static demo_view_t *vu;
static demo_buffer_t *buffer;
static demo_font_t *current_demo_font;
static hb_blob_t *current_blob;
static hb_face_t *current_face;
static hb_font_t *current_font;
static char *current_text;
static bool custom_text;

static void
rebuild_buffer (const char *text)
{
  /* strdup first -- text may alias current_text. */
  char *new_text = strdup (text);
  free (current_text);
  current_text = new_text;

  demo_font_clear_cache (current_demo_font);
  demo_atlas_clear (renderer->get_atlas ());

  demo_buffer_clear (buffer);
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, text, current_demo_font, 1);
  demo_view_reset (vu);
  /* Don't call demo_view_display here: it may run from a
   * microtask (e.g. the font-fetch .then() callback) where the
   * canvas back-buffer won't swap to the front.  Leave
   * needs_redraw=true (set by demo_view_reset) so the next
   * main_loop_iter renders inside a proper rAF tick. */
}

extern "C" {

EMSCRIPTEN_KEEPALIVE void
web_load_font (const char *data, int len)
{
  hb_blob_t *blob = hb_blob_create (data, len,
				     HB_MEMORY_MODE_DUPLICATE,
				     NULL, NULL);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  hb_blob_destroy (current_blob);
  hb_face_destroy (current_face);
  hb_font_destroy (current_font);

  current_blob = blob;
  current_face = face;
  current_font = font;

  /* Flush old font state */
  demo_font_destroy (current_demo_font);
  current_demo_font = demo_font_create (font, renderer->get_atlas (), false);

  rebuild_buffer (custom_text ? current_text : default_text_en);
  demo_font_print_stats (current_demo_font);
}

EMSCRIPTEN_KEEPALIVE void
web_set_text (const char *utf8)
{
  custom_text = true;
  rebuild_buffer (utf8);
}

/* Apply a comma-separated list of variation settings
 * ("wght=400,wdth=100") to the current font, then rebuild
 * the buffer so the new axis values take effect.  Cheap
 * to call repeatedly: suitable for live slider updates. */
EMSCRIPTEN_KEEPALIVE void
web_set_variations (const char *settings)
{
  if (!current_font) return;

  /* Caps the axis count: no font ships close to this. */
  hb_variation_t vars[32];
  unsigned n = 0;
  const char *p = settings;
  while (p && *p && n < (sizeof vars / sizeof vars[0]))
  {
    const char *end = strchr (p, ',');
    int len = end ? (int) (end - p) : (int) strlen (p);
    if (hb_variation_from_string (p, len, &vars[n]))
      n++;
    p = end ? end + 1 : nullptr;
  }
  hb_font_set_variations (current_font, vars, n);

  rebuild_buffer (custom_text ? current_text : default_text_en);
}

EMSCRIPTEN_KEEPALIVE void
web_set_features (const char *settings)
{
  hb_feature_t feats[64];
  unsigned n = 0;
  const char *p = settings;
  while (p && *p && n < 64)
  {
    const char *end = strchr (p, ',');
    int len = end ? (int) (end - p) : (int) strlen (p);
    if (hb_feature_from_string (p, len, &feats[n]))
      n++;
    p = end ? end + 1 : nullptr;
  }
  demo_buffer_set_features (feats, n);
  rebuild_buffer (custom_text ? current_text : default_text_en);
}

EMSCRIPTEN_KEEPALIVE void
web_set_palette (unsigned palette_index)
{
  if (!current_demo_font) return;
  demo_font_set_palette (current_demo_font, palette_index);
  rebuild_buffer (custom_text ? current_text : default_text_en);
}

EMSCRIPTEN_KEEPALIVE const char *
web_get_text ()
{
  return current_text;
}

EMSCRIPTEN_KEEPALIVE void
web_toggle_animation ()
{
  demo_view_key_func (vu, 32 /* GLFW_KEY_SPACE */, 0, 1, 0);
}

EMSCRIPTEN_KEEPALIVE void
web_set_dark (int dark)
{
  /* Toggle dark mode via the key handler if the current
   * state doesn't match the request.  We can't check
   * vu->dark_mode directly (opaque type), so track it
   * locally. */
  static bool is_dark = false;
  if ((!is_dark) == (!dark)) return;
  is_dark = !is_dark;
  demo_view_key_func (vu, 66 /* GLFW_KEY_B */, 0, 1, 0);
}

EMSCRIPTEN_KEEPALIVE void
web_reset ()
{
  demo_view_reset (vu);
}

EMSCRIPTEN_KEEPALIVE void
web_request_redraw ()
{
  demo_view_request_redraw (vu);
}

EMSCRIPTEN_KEEPALIVE void
web_cancel_gesture ()
{
  demo_view_cancel_gesture (vu);
}

EMSCRIPTEN_KEEPALIVE void
web_pinch (float pan_dx, float pan_dy,
	   float zoom_factor, float angle_delta,
	   float cx, float cy, int w, int h)
{
  demo_view_pinch (vu, pan_dx, pan_dy, zoom_factor, angle_delta, cx, cy, w, h);
}

} /* extern "C" */

static void
main_loop_iter ()
{
  glfwPollEvents ();

  if (demo_view_should_redraw (vu))
    demo_view_display (vu, buffer);
}

static void
framebuffer_size_func (GLFWwindow *window, int width, int height)
{ demo_view_reshape_func (vu, width, height); }

static void
key_func (GLFWwindow *window, int key, int scancode, int action, int mods)
{ demo_view_key_func (vu, key, scancode, action, mods); }

static void
char_func (GLFWwindow *window, unsigned int codepoint)
{ demo_view_char_func (vu, codepoint); }

static void
mouse_func (GLFWwindow *window, int button, int action, int mods)
{ demo_view_mouse_func (vu, button, action, mods); }

static void
scroll_func (GLFWwindow *window, double xoffset, double yoffset)
{ demo_view_scroll_func (vu, xoffset, yoffset); }

static void
cursor_func (GLFWwindow *window, double x, double y)
{ demo_view_motion_func (vu, x, y); }

static const char *arg_text = nullptr;
static const char *arg_font = nullptr;

int
main (int argc, char **argv)
{
  /* Accept --text=... and --font=... so the JS shell can forward
   * URL parameters.  Everything else is ignored. */
  for (int i = 1; i < argc; i++)
  {
    if (!strncmp (argv[i], "--text=", 7))
      arg_text = argv[i] + 7;
    else if (!strncmp (argv[i], "--font=", 7))
      arg_font = argv[i] + 7;
  }

  if (!glfwInit ())
  {
    fprintf (stderr, "Failed to initialize GLFW\n");
    return 1;
  }

  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

  /* Query actual canvas size from the browser. */
  int canvas_w = 700, canvas_h = 700;
  emscripten_get_canvas_element_size ("#canvas", &canvas_w, &canvas_h);

  GLFWwindow *window = glfwCreateWindow (canvas_w, canvas_h, "HarfBuzz GPU Demo", NULL, NULL);
  if (!window)
  {
    glfwTerminate ();
    fprintf (stderr, "Failed to create GLFW window\n");
    return 1;
  }
  glfwMakeContextCurrent (window);

  glfwSetFramebufferSizeCallback (window, framebuffer_size_func);
  glfwSetKeyCallback (window, key_func);
  glfwSetCharCallback (window, char_func);
  glfwSetMouseButtonCallback (window, mouse_func);
  glfwSetScrollCallback (window, scroll_func);
  glfwSetCursorPosCallback (window, cursor_func);

  {
    int fb_width, fb_height;
    glfwGetFramebufferSize (window, &fb_width, &fb_height);
    glViewport (0, 0, fb_width, fb_height);
  }

  renderer = demo_renderer_create_gl (window, false);
  vu = demo_view_create (renderer, window);

  current_blob = hb_blob_create ((const char *) default_font,
				 sizeof (default_font),
				 HB_MEMORY_MODE_READONLY,
				 NULL, NULL);
  current_face = hb_face_create (current_blob, 0);
  current_font = hb_font_create (current_face);
  current_demo_font = demo_font_create (current_font, renderer->get_atlas (), false);

  current_text = strdup (arg_text ? arg_text : default_text_combined);
  if (arg_text)
    custom_text = true;

  buffer = demo_buffer_create ();
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  /* When ?font= is pending, skip initial glyph upload -- the
   * async font load will replace the font anyway, and we'd
   * upload defaults-into-atlas just to throw them away.  Buffer
   * stays empty until web_load_font's rebuild_buffer runs. */
  if (!arg_font)
    demo_buffer_add_text (buffer, current_text, current_demo_font, 1);

  demo_font_print_stats (current_demo_font);

  demo_view_setup (vu);

  emscripten_set_main_loop (main_loop_iter, 0, 1);

  /* Not reached */
  return 0;
}
