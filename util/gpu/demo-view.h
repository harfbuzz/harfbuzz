/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef DEMO_VIEW_H
#define DEMO_VIEW_H

#include "demo-common.h"
#include "demo-buffer.h"
#include "demo-renderer.h"

typedef struct demo_view_t demo_view_t;

#ifndef HB_GPU_NO_GLFW
demo_view_t *
demo_view_create (demo_renderer_t *renderer, GLFWwindow *window);
#endif

demo_view_t *
demo_view_create_headless (demo_renderer_t *renderer,
			   int fb_width, int fb_height,
			   int win_width, int win_height);

void
demo_view_destroy (demo_view_t *vu);


void
demo_view_reset (demo_view_t *vu);

void
demo_view_reshape_func (demo_view_t *vu, int width, int height);

void
demo_view_key_func (demo_view_t *vu, int key, int scancode, int action, int mods);

void
demo_view_char_func (demo_view_t *vu, unsigned int codepoint);

void
demo_view_mouse_func (demo_view_t *vu, int button, int action, int mods);

void
demo_view_cancel_gesture (demo_view_t *vu);

void
demo_view_scroll_func (demo_view_t *vu, double xoffset, double yoffset);

void
demo_view_zoom_around (demo_view_t *vu, double factor,
		       double cx, double cy,
		       int width, int height);

void
demo_view_rotate_z (demo_view_t *vu, double angle);

void
demo_view_rotate_z_around (demo_view_t *vu, double angle,
			   double cx, double cy,
			   int width, int height);

void
demo_view_pinch (demo_view_t *vu,
		 double pan_dx, double pan_dy,
		 double zoom_factor,
		 double angle_delta,
		 double cx, double cy,
		 int width, int height);

void
demo_view_motion_func (demo_view_t *vu, double x, double y);

void
demo_view_print_help (demo_view_t *vu);

void
demo_view_display (demo_view_t *vu, demo_buffer_t *buffer);

void
demo_view_setup (demo_view_t *vu);

void
demo_view_type (demo_view_t *vu, const char *keys);

void
demo_view_set_fps_quit (demo_view_t *vu, int count);

void
demo_view_request_redraw (demo_view_t *vu);

bool
demo_view_should_redraw (demo_view_t *vu);


#endif /* DEMO_VIEW_H */
