/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju, Wojciech Baranowski
 */

#ifndef _WIN32
#include <libgen.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#include "demo-buffer.h"
#include "demo-font.h"
#include "demo-view.h"

static demo_glstate_t *st;
static demo_view_t *vu;
static demo_buffer_t *buffer;

#define WINDOW_W 700
#define WINDOW_H 700

#ifdef _WIN32

static int opterr = 1;
static int optind = 1;
static int optopt;
static char *optarg;

static int getopt(int argc, char *argv[], char *opts)
{
    static int sp = 1;
    int c;
    char *cp;

    if (sp == 1) {
        if (optind >= argc ||
            argv[optind][0] != '-' || argv[optind][1] == '\0')
            return EOF;
        else if (!strcmp(argv[optind], "--")) {
            optind++;
            return EOF;
        }
    }
    optopt = c = argv[optind][sp];
    if (c == ':' || !(cp = strchr(opts, c))) {
        fprintf(stderr, ": illegal option -- %c\n", c);
        if (argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return '?';
    }
    if (*++cp == ':') {
        if (argv[optind][sp+1] != '\0')
            optarg = &argv[optind++][sp+1];
        else if(++optind >= argc) {
            fprintf(stderr, ": option requires an argument -- %c\n", c);
            sp = 1;
            return '?';
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if (argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }

    return c;
}

#endif

static void
framebuffer_size_func (GLFWwindow *window, int width, int height)
{
  demo_view_reshape_func (vu, width, height);
}

static void
key_func (GLFWwindow *window, int key, int scancode, int action, int mods)
{
  demo_view_key_func (vu, key, scancode, action, mods);
}

static void
char_func (GLFWwindow *window, unsigned int codepoint)
{
  demo_view_char_func (vu, codepoint);
}

static void
mouse_func (GLFWwindow *window, int button, int action, int mods)
{
  demo_view_mouse_func (vu, button, action, mods);
}

static void
scroll_func (GLFWwindow *window, double xoffset, double yoffset)
{
  demo_view_scroll_func (vu, xoffset, yoffset);
}

static void
cursor_func (GLFWwindow *window, double x, double y)
{
  demo_view_motion_func (vu, x, y);
}

static void
show_usage(const char *name)
{
  printf("Usage:\n"
	 "  %s [fontfile [text]]\n"
	 "or:\n"
	 "  %s [-h] [-f fontfile] [-t text]\n"
	 "\n"
	 "  -h             show this help message and exit;\n"
	 "  -t text        the text string to be rendered;\n"
	 "  -f fontfile    the font file\n"
	 "\n", name, name);

  demo_view_print_help (NULL);
}

static bool
contains_case_insensitive (const char *haystack, const char *needle)
{
  if (!haystack || !needle)
    return false;

  size_t needle_len = strlen (needle);
  if (!needle_len)
    return true;

  for (const char *h = haystack; *h; h++) {
    size_t i = 0;
    while (i < needle_len &&
	   h[i] &&
	   tolower ((unsigned char) h[i]) == tolower ((unsigned char) needle[i]))
      i++;
    if (i == needle_len)
      return true;
  }

  return false;
}

static bool
running_on_gnome_desktop (void)
{
  const char *desktops[] = {
    getenv ("XDG_CURRENT_DESKTOP"),
    getenv ("XDG_SESSION_DESKTOP"),
    getenv ("DESKTOP_SESSION"),
  };

  for (unsigned int i = 0; i < ARRAY_LEN (desktops); i++) {
    if (contains_case_insensitive (desktops[i], "gnome"))
      return true;
  }

  return false;
}

static bool
using_mesa_gl (void)
{
  const char *strings[] = {
    (const char *) glGetString (GL_VENDOR),
    (const char *) glGetString (GL_RENDERER),
    (const char *) glGetString (GL_VERSION),
  };

  for (unsigned int i = 0; i < ARRAY_LEN (strings); i++) {
    if (contains_case_insensitive (strings[i], "mesa"))
      return true;
  }

  return false;
}

static void
warn_about_vsync_override (void)
{
  const char *vblank_mode = getenv ("vblank_mode");
  if (vblank_mode) {
    LOGW ("Using vblank_mode=%s from the environment.\n", vblank_mode);
    return;
  }

  if (!running_on_gnome_desktop () || !using_mesa_gl ())
    return;

  LOGW ("GNOME/Mesa detected with vblank_mode unset.\n");
  LOGW ("If toggling vsync with `v` has no effect, restart with `vblank_mode=0`.\n");
}

static void
glfw_error_callback (int error, const char *description)
{
  LOGW ("GLFW error %d: %s\n", error, description);
}

int
main (int argc, char** argv)
{
# include "default-text.h"
  const char *text = NULL;
  const char *font_path = NULL;
  char arg;
  while ((arg = getopt(argc, argv, (char *)"t:f:h")) != -1) {
    switch (arg) {
    case 't':
      text = optarg;
      break;
    case 'f':
      font_path = optarg;
      break;
    case 'h':
      show_usage(argv[0]);
      return 0;
    default:
      return 1;
    }
  }
  if (!font_path)
  {
    if (optind < argc)
      font_path = argv[optind++];
  }
  if (!text)
  {
    if (optind < argc)
      text = argv[optind++];
    else
      text = default_text;
  }
  if (!text)
  {
    show_usage(argv[0]);
    return 1;
  }

  /* Setup GLFW */
  glfwSetErrorCallback (glfw_error_callback);
  if (!glfwInit ())
    die ("Failed to initialize GLFW");

  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint (GLFW_SRGB_CAPABLE, GLFW_TRUE);

  GLFWwindow *window = glfwCreateWindow (WINDOW_W, WINDOW_H, "HarfBuzz GPU Demo", NULL, NULL);
  if (!window) {
    glfwTerminate ();
    die ("Failed to create GLFW window");
  }
  glfwMakeContextCurrent (window);

  glfwSetFramebufferSizeCallback (window, framebuffer_size_func);
  glfwSetKeyCallback (window, key_func);
  glfwSetCharCallback (window, char_func);
  glfwSetMouseButtonCallback (window, mouse_func);
  glfwSetScrollCallback (window, scroll_func);
  glfwSetCursorPosCallback (window, cursor_func);

  /* Setup GLEW */
  glewExperimental = GL_TRUE;
  glewInit ();
  while (glGetError () != GL_NO_ERROR)
    ;
  if (!glewIsSupported ("GL_VERSION_3_3"))
    die ("OpenGL 3.3 not supported");

  {
    int fb_width, fb_height;
    glfwGetFramebufferSize (window, &fb_width, &fb_height);
    glViewport (0, 0, fb_width, fb_height);
  }

  st = demo_glstate_create ();
  vu = demo_view_create (st, window);
  demo_view_print_help (vu);
  warn_about_vsync_override ();

  hb_blob_t *blob = NULL;
  if (font_path)
  {
    blob = hb_blob_create_from_file_or_fail (font_path);
  }
  else
  {
#ifdef _WIN32
    blob = hb_blob_create_from_file_or_fail ("C:\\Windows\\Fonts\\calibri.ttf");
#else
    #include "default-font.h"
    blob = hb_blob_create ((const char *) default_font, sizeof (default_font),
			   HB_MEMORY_MODE_READONLY, NULL, NULL);
#endif
  }
  if (!blob)
    die ("Failed to open font file");

  hb_face_t *hb_face = hb_face_create (blob, 0);
  demo_font_t *font = demo_font_create (hb_face, demo_glstate_get_atlas (st));

  buffer = demo_buffer_create ();
  demo_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, text, font, 1);

  demo_font_print_stats (font);

  demo_view_setup (vu);

  demo_view_display (vu, buffer);
  glfwPollEvents ();
  demo_view_display (vu, buffer);

  /* Main loop */
  while (!glfwWindowShouldClose (window))
  {
    glfwPollEvents ();

    if (demo_view_should_redraw (vu))
      demo_view_display (vu, buffer);
    else
      glfwWaitEvents ();
  }

  demo_buffer_destroy (buffer);
  demo_font_destroy (font);

  hb_face_destroy (hb_face);
  hb_blob_destroy (blob);

  demo_view_destroy (vu);
  demo_glstate_destroy (st);

  glfwDestroyWindow (window);
  glfwTerminate ();

  return 0;
}
