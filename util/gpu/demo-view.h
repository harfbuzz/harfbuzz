/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef DEMO_VIEW_H
#define DEMO_VIEW_H

#include "demo-common.h"
#include "demo-buffer.h"
#include "demo-glstate.h"

typedef struct demo_metal_t demo_metal_t;
typedef struct demo_view_t demo_view_t;

demo_view_t *
demo_view_create (demo_glstate_t *st, GLFWwindow *window);

demo_view_t *
demo_view_create_metal (demo_metal_t *mt, GLFWwindow *window);

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
demo_view_scroll_func (demo_view_t *vu, double xoffset, double yoffset);

void
demo_view_motion_func (demo_view_t *vu, double x, double y);

void
demo_view_print_help (demo_view_t *vu);

void
demo_view_display (demo_view_t *vu, demo_buffer_t *buffer);

void
demo_view_setup (demo_view_t *vu);

bool
demo_view_should_redraw (demo_view_t *vu);


#endif /* DEMO_VIEW_H */
