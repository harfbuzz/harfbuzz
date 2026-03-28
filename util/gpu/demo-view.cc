/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 */

#include "demo-view.h"

extern "C" {
#include "trackball.h"
#include "matrix4x4.h"
}

struct demo_view_t {
  demo_glstate_t *st;
  GLFWwindow *window;

  /* Output */
  GLint vsync;
  hb_bool_t srgb;
  hb_bool_t fullscreen;

  /* Mouse handling */
  int buttons;
  int modifiers;
  bool dragged;
  bool click_handled;
  double cursorx, cursory;
  double beginx, beginy;
  double lastx, lasty, lastt;
  double dx,dy, dt;

  /* Transformation */
  float quat[4];
  double scalex;
  double scaley;
  demo_point_t translate;
  double perspective;

  /* Animation */
  float rot_axis[3];
  double rot_speed;
  bool animate;
  int num_frames;
  double fps_start_time;
  double last_frame_time;
  bool has_fps_timer;
  double fps_timer_interval;
  double fps_timer_last;

  /* Vim-style repeat count */
  unsigned int repeat_count;

  /* Dirty flag for redraw */
  bool needs_redraw;

  /* Window geometry just before going fullscreen */
  int x;
  int y;
  int width;
  int height;
};

demo_view_t *
demo_view_create (demo_glstate_t *st, GLFWwindow *window)
{
  demo_view_t *vu = (demo_view_t *) calloc (1, sizeof (demo_view_t));

  vu->st = st;
  vu->window = window;
  vu->needs_redraw = true;
  demo_view_reset (vu);

  return vu;
}

void
demo_view_destroy (demo_view_t *vu)
{
  if (!vu)
    return;

  free (vu);
}


#define ANIMATION_SPEED 1.
void
demo_view_reset (demo_view_t *vu)
{
  vu->perspective = 16;
  vu->scalex = vu->scaley = 1;
  vu->translate.x = vu->translate.y = 0;
  trackball (vu->quat , 0.0, 0.0, 0.0, 0.0);
  vset (vu->rot_axis, 0., 0., 1.);
  vu->rot_speed = ANIMATION_SPEED;
  vu->needs_redraw = true;
}


static void
demo_view_scale_perspective (demo_view_t *vu, double factor)
{
  vu->perspective = clamp (vu->perspective * factor, .01, 100.);
}

static void
demo_view_scalex (demo_view_t *vu, double factor)
{
  vu->scalex *= factor;
}

static void
demo_view_scaley (demo_view_t *vu, double factor)
{
  vu->scaley *= factor;
}


static void
demo_view_scale (demo_view_t *vu, double sx, double sy)
{
  vu->scalex *= sx;
  vu->scaley *= sy;
}

static void
demo_view_translate (demo_view_t *vu, double dx, double dy)
{
  vu->translate.x += dx / vu->scalex;
  vu->translate.y += dy / vu->scaley;
}

static void
demo_view_apply_transform (demo_view_t *vu, float *mat)
{
  int viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  GLint width  = viewport[2];
  GLint height = viewport[3];

  m4Scale (mat, vu->scalex, vu->scaley, 1);
  m4Translate (mat, vu->translate.x, vu->translate.y, 0);

  {
    double d = std::max (width, height);
    double near = d / vu->perspective;
    double far = near + d;
    double factor = near / (2 * near + d);
    m4Frustum (mat, -width * factor, width * factor, -height * factor, height * factor, near, far);
    m4Translate (mat, 0, 0, -(near + d * .5));
  }

  float m[4][4];
  build_rotmatrix (m, vu->quat);
  m4MultMatrix(mat, &m[0][0]);

  m4Scale (mat, 1, -1, 1);
}


static double
current_time (void)
{
  return glfwGetTime ();
}

static void
start_animation (demo_view_t *vu)
{
  vu->num_frames = 0;
  vu->last_frame_time = vu->fps_start_time = current_time ();
  vu->fps_timer_interval = 5.0;
  vu->fps_timer_last = current_time ();
  vu->has_fps_timer = true;
}

static void
demo_view_toggle_animation (demo_view_t *vu)
{
  vu->animate = !vu->animate;
  LOGI ("Setting animation %s.\n", vu->animate ? "on" : "off");
  if (vu->animate)
    start_animation (vu);
}


static void
demo_view_toggle_vsync (demo_view_t *vu)
{
  GLint vsync = !vu->vsync;
  glfwSwapInterval (vsync);
  vu->vsync = vsync;
  LOGI ("Setting vsync %s.\n", vu->vsync ? "on" : "off");
}

static void
demo_view_toggle_srgb (demo_view_t *vu)
{
  hb_bool_t srgb = !vu->srgb;
#if defined(GL_FRAMEBUFFER_SRGB)
  while (glGetError () != GL_NO_ERROR)
    ;

  if (srgb)
    glEnable (GL_FRAMEBUFFER_SRGB);
  else
    glDisable (GL_FRAMEBUFFER_SRGB);

  if (glGetError () == GL_NO_ERROR) {
    vu->srgb = srgb;
    LOGI ("Setting sRGB framebuffer %s.\n", vu->srgb ? "on" : "off");
  } else {
    if (vu->srgb)
      glEnable (GL_FRAMEBUFFER_SRGB);
    else
      glDisable (GL_FRAMEBUFFER_SRGB);
    while (glGetError () != GL_NO_ERROR)
      ;
    LOGW ("Failed to set sRGB framebuffer state\n");
  }
#else
  LOGW ("No sRGB framebuffer extension found; failed to set sRGB framebuffer\n");
#endif
}

static void
demo_view_toggle_fullscreen (demo_view_t *vu)
{
  vu->fullscreen = !vu->fullscreen;
  if (vu->fullscreen) {
    glfwGetWindowPos (vu->window, &vu->x, &vu->y);
    glfwGetWindowSize (vu->window, &vu->width, &vu->height);
    GLFWmonitor *monitor = glfwGetPrimaryMonitor ();
    const GLFWvidmode *mode = glfwGetVideoMode (monitor);
    glfwSetWindowMonitor (vu->window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  } else {
    glfwSetWindowMonitor (vu->window, NULL, vu->x, vu->y, vu->width, vu->height, 0);
  }
}



void
demo_view_reshape_func (demo_view_t *vu, int width, int height)
{
  vu->needs_redraw = true;
}

#define STEP 1.05
void
demo_view_char_func (demo_view_t *vu, unsigned int codepoint)
{
  if (codepoint >= '0' && codepoint <= '9') {
    vu->repeat_count = vu->repeat_count * 10 + (codepoint - '0');
    return;
  }

  unsigned int count = vu->repeat_count ? vu->repeat_count : 1;
  vu->repeat_count = 0;

  for (unsigned int i = 0; i < count; i++) {
    switch (codepoint)
    {
      case '=':
	demo_view_scale (vu, STEP, STEP);
	break;
      case '-':
	demo_view_scale (vu, 1. / STEP, 1. / STEP);
	break;

      case '[':
	demo_view_scalex (vu, STEP);
	break;
      case ']':
	demo_view_scalex (vu, 1. / STEP);
	break;

      case '{':
	demo_view_scaley (vu, STEP);
	break;
      case '}':
	demo_view_scaley (vu, 1. / STEP);
	break;

      case 'k':
	demo_view_translate (vu, 0, -.1);
	break;
      case 'j':
	demo_view_translate (vu, 0, +.1);
	break;
      case 'h':
	demo_view_translate (vu, +.1, 0);
	break;
      case 'l':
	demo_view_translate (vu, -.1, 0);
	break;

      default:
	return;
    }
  }
  vu->needs_redraw = true;
}

void
demo_view_key_func (demo_view_t *vu, int key, int scancode, int action, int mods)
{
  if (action != GLFW_PRESS)
    return;

  switch (key)
  {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
      glfwSetWindowShouldClose (vu->window, GLFW_TRUE);
      break;

    case GLFW_KEY_SPACE:
      demo_view_toggle_animation (vu);
      break;
    case GLFW_KEY_SLASH:
      if (mods & GLFW_MOD_SHIFT)
	demo_view_print_help (vu);
      break;
    case GLFW_KEY_S:
      demo_view_toggle_srgb (vu);
      break;
    case GLFW_KEY_V:
      demo_view_toggle_vsync (vu);
      break;

    case GLFW_KEY_F:
      demo_view_toggle_fullscreen (vu);
      break;

    case GLFW_KEY_R:
      demo_view_reset (vu);
      break;

    case GLFW_KEY_UP:
      demo_view_translate (vu, 0, -.1);
      break;
    case GLFW_KEY_DOWN:
      demo_view_translate (vu, 0, +.1);
      break;
    case GLFW_KEY_LEFT:
      demo_view_translate (vu, +.1, 0);
      break;
    case GLFW_KEY_RIGHT:
      demo_view_translate (vu, -.1, 0);
      break;

    default:
      return;
  }
  vu->needs_redraw = true;
}

void
demo_view_mouse_func (demo_view_t *vu, int button, int action, int mods)
{
  if (action == GLFW_PRESS) {
    vu->buttons |= (1 << button);
    vu->click_handled = false;
  } else
    vu->buttons &= ~(1 << button);
  vu->modifiers = mods;

  glfwGetCursorPos (vu->window, &vu->cursorx, &vu->cursory);
  double x = vu->cursorx, y = vu->cursory;

  switch (button)
  {
    case GLFW_MOUSE_BUTTON_LEFT:
      if (action == GLFW_RELEASE && !vu->dragged && !vu->click_handled)
	demo_view_toggle_animation (vu);
      break;
    case GLFW_MOUSE_BUTTON_RIGHT:
      switch (action) {
	case GLFW_PRESS:
	  if (vu->animate) {
	    demo_view_toggle_animation (vu);
	    vu->click_handled = true;
	  }
	  break;
	case GLFW_RELEASE:
	  if (!vu->animate)
	    {
	      if (!vu->dragged && !vu->click_handled)
		demo_view_toggle_animation (vu);
	      else if (vu->dt) {
		double speed = hypot (vu->dx, vu->dy) / vu->dt;
		if (speed > 0.1)
		  demo_view_toggle_animation (vu);
	      }
	      vu->dx = vu->dy = vu->dt = 0;
	    }
	  break;
      }
      break;
  }

  vu->beginx = vu->lastx = x;
  vu->beginy = vu->lasty = y;
  vu->dragged = false;

  vu->needs_redraw = true;
}

void
demo_view_scroll_func (demo_view_t *vu, double xoffset, double yoffset)
{
  if (yoffset > 0)
    demo_view_scale (vu, STEP, STEP);
  else if (yoffset < 0)
    demo_view_scale (vu, 1. / STEP, 1. / STEP);
  vu->needs_redraw = true;
}

void
demo_view_motion_func (demo_view_t *vu, double x, double y)
{
  vu->cursorx = x;
  vu->cursory = y;

  if (!vu->buttons)
    return;

  /* Only count as dragged if moved more than a few pixels (helps touch). */
  if (!vu->dragged)
  {
    double dx = x - vu->beginx;
    double dy = y - vu->beginy;
    if (dx * dx + dy * dy > 25)
      vu->dragged = true;
    else
      return;
  }

  int width, height;
  glfwGetWindowSize (vu->window, &width, &height);

  if (vu->buttons & (1 << GLFW_MOUSE_BUTTON_LEFT))
  {
    demo_view_translate (vu,
			 +2 * (x - vu->lastx) / width,
			 -2 * (y - vu->lasty) / height);
  }

  if (vu->buttons & (1 << GLFW_MOUSE_BUTTON_RIGHT))
  {
    if (vu->modifiers & GLFW_MOD_SHIFT) {
      demo_view_scale_perspective (vu, 1 - ((y - vu->lasty) / height) * 5);
    } else {
      float dquat[4];
      trackball (dquat,
		 (2.0*vu->lastx -         width) / width,
		 (       height - 2.0*vu->lasty) / height,
		 (        2.0*x -         width) / width,
		 (       height -         2.0*y) / height );

      vu->dx = x - vu->lastx;
      vu->dy = y - vu->lasty;
      vu->dt = current_time () - vu->lastt;

      add_quats (dquat, vu->quat, vu->quat);

      if (vu->dt) {
	vcopy (dquat, vu->rot_axis);
	vnormal (vu->rot_axis);
	vu->rot_speed = 2 * acos (dquat[3]) / vu->dt;
      }
    }
  }

  if (vu->buttons & (1 << GLFW_MOUSE_BUTTON_MIDDLE))
  {
    double factor = 1 - ((y - vu->lasty) / height) * 5;
    demo_view_scale (vu, factor, factor);
    demo_view_translate (vu,
			 +(2. * vu->beginx / width  - 1) * (1 - factor),
			 -(2. * vu->beginy / height - 1) * (1 - factor));
  }

  vu->lastx = x;
  vu->lasty = y;
  vu->lastt = current_time ();

  vu->needs_redraw = true;
}

void
demo_view_print_help (demo_view_t *vu)
{
  (void) vu;

  LOGI ("hb-gpu-demo controls\n");
  LOGI ("Keyboard:\n");
  LOGI ("  Esc, q                    Quit\n");
  LOGI ("  ?                         This help\n");
  LOGI ("  Space                     Toggle animation\n");
  LOGI ("  f                         Toggle fullscreen\n");
  LOGI ("  s                         Toggle sRGB framebuffer\n");
  LOGI ("  v                         Toggle vsync\n");
  LOGI ("  =, -                      Zoom in/out\n");
  LOGI ("  [, ]                      Stretch/shrink horizontally\n");
  LOGI ("  {, }                      Stretch/shrink vertically\n");
  LOGI ("  h, j, k, l               Pan (vim-style)\n");
  LOGI ("  Arrow keys                Pan\n");
  LOGI ("  r                         Reset view\n");
  LOGI ("  <N><key>                  Repeat key N times (e.g. 30=)\n");
  LOGI ("Mouse:\n");
  LOGI ("  Left drag                 Pan\n");
  LOGI ("  Middle drag / wheel       Zoom\n");
  LOGI ("  Right drag                Rotate\n");
  LOGI ("  Shift + right drag        Adjust perspective\n");
  LOGI ("  Right drag and release    Spin animation\n");
  LOGI ("  Click / tap               Toggle animation\n");
  LOGI ("\n");
}


static void
advance_frame (demo_view_t *vu, double dtime)
{
  if (vu->animate) {
    float dquat[4];
    axis_to_quat (vu->rot_axis, vu->rot_speed * dtime, dquat);
    add_quats (dquat, vu->quat, vu->quat);
    vu->num_frames++;
  }
}

void
demo_view_display (demo_view_t *vu, demo_buffer_t *buffer)
{
  double new_time = current_time ();
  advance_frame (vu, new_time - vu->last_frame_time);
  vu->last_frame_time = new_time;

  if (vu->animate && vu->has_fps_timer) {
    double now = current_time ();
    if (now - vu->fps_timer_last >= vu->fps_timer_interval) {
      LOGI ("%gfps\n", vu->num_frames / (now - vu->fps_start_time));
      vu->num_frames = 0;
      vu->fps_start_time = now;
      vu->fps_timer_last = now;
    }
  }

  int width, height;
  glfwGetFramebufferSize (vu->window, &width, &height);
  glViewport (0, 0, width, height);


  float mat[16];

  m4LoadIdentity (mat);

  demo_view_apply_transform (vu, mat);

  demo_extents_t extents;
  demo_buffer_extents (buffer, NULL, &extents);
  double content_scale = .9 * std::min (width  / (extents.max_x - extents.min_x),
					height / (extents.max_y - extents.min_y));
  m4Scale (mat, content_scale, content_scale, 1);
  m4Translate (mat,
	       -(extents.max_x + extents.min_x) / 2.,
	       -(extents.max_y + extents.min_y) / 2., 0);

  demo_glstate_set_matrix (vu->st, mat);

  glClearColor (1, 1, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  demo_buffer_draw (buffer);

  glfwSwapBuffers (vu->window);
  vu->needs_redraw = false;
}

void
demo_view_setup (demo_view_t *vu)
{
  if (!vu->vsync)
    demo_view_toggle_vsync (vu);
  if (!vu->srgb)
    demo_view_toggle_srgb (vu);
  demo_glstate_setup (vu->st);
}

bool
demo_view_should_redraw (demo_view_t *vu)
{
  return vu->needs_redraw || vu->animate;
}
