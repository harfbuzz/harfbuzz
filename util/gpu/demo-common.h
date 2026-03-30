/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#ifndef DEMO_COMMON_H
#define DEMO_COMMON_H

#include <hb.h>
#include <hb-gpu.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <algorithm>
#include <vector>

#ifndef HB_GPU_NO_GLFW
#include <GL/glew.h>

#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#  include <OpenGL/OpenGL.h>
#endif

#include <GLFW/glfw3.h>
#endif


#define LOGI(...) ((void) fprintf (stdout, __VA_ARGS__))
#define LOGW(...) ((void) fprintf (stderr, __VA_ARGS__))
#define LOGE(...) ((void) fprintf (stderr, __VA_ARGS__), abort ())


#define ARRAY_LEN(Array) (sizeof (Array) / sizeof (*Array))


#define gl(name) \
	for (GLint __ee, __ii = 0; \
	     __ii < 1; \
	     (__ii++, \
	      (__ee = glGetError()) && \
	      (fprintf (stderr, "gl" #name " failed with error %04X on line %d\n", __ee, __LINE__), abort (), 0))) \
	  gl##name


static inline void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

template <typename T>
T clamp (T v, T m, T M)
{
  return v < m ? m : v > M ? M : v;
}


/* Simple extents/point types for the demo, matching glyphy's layout */

typedef struct {
  double x;
  double y;
} demo_point_t;

typedef struct {
  double min_x;
  double min_y;
  double max_x;
  double max_y;
} demo_extents_t;

static inline void
demo_extents_clear (demo_extents_t *extents)
{
  extents->min_x = extents->min_y =  1e308;
  extents->max_x = extents->max_y = -1e308;
}

static inline int
demo_extents_is_empty (const demo_extents_t *extents)
{
  return extents->min_x > extents->max_x;
}

static inline void
demo_extents_add (demo_extents_t *extents, const demo_point_t *p)
{
  if (demo_extents_is_empty (extents)) {
    extents->min_x = extents->max_x = p->x;
    extents->min_y = extents->max_y = p->y;
    return;
  }
  extents->min_x = std::min (extents->min_x, p->x);
  extents->min_y = std::min (extents->min_y, p->y);
  extents->max_x = std::max (extents->max_x, p->x);
  extents->max_y = std::max (extents->max_y, p->y);
}

static inline void
demo_extents_extend (demo_extents_t *extents, const demo_extents_t *other)
{
  if (demo_extents_is_empty (other))
    return;
  if (demo_extents_is_empty (extents)) {
    *extents = *other;
    return;
  }
  extents->min_x = std::min (extents->min_x, other->min_x);
  extents->min_y = std::min (extents->min_y, other->min_y);
  extents->max_x = std::max (extents->max_x, other->max_x);
  extents->max_y = std::max (extents->max_y, other->max_y);
}


#endif /* DEMO_COMMON_H */
