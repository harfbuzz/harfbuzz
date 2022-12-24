/*
 * Copyright © 2022 Behdad Esfahbod
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
 */

#ifndef HB_PAINT_EXTENTS_HH
#define HB_PAINT_EXTENTS_HH

#include "hb.hh"
#include "hb-paint.hh"
#include "hb-draw.h"


typedef struct hb_extents_t
{
  hb_extents_t () {}
  hb_extents_t (float xmin, float ymin, float xmax, float ymax) :
    xmin (xmin), ymin (ymin), xmax (xmax), ymax (ymax) {}

  bool is_empty () const { return xmin > xmax; }

  float xmin = 0.f;
  float ymin = 0.f;
  float xmax = -1.f;
  float ymax = -1.f;
} hb_extents_t;

typedef struct hb_transform_t
{
  hb_transform_t () {}
  hb_transform_t (float xx, float yx,
		  float xy, float yy,
		  float x0, float y0) :
    xx (xx), yx (yx), xy (xy), yy (yy), x0 (x0), y0 (y0) {}

  void multiply (const hb_transform_t &o)
  {
    /* Copied from cairo, with "o" being "a" there and "this" being "b" there. */
    hb_transform_t r;

    r.xx = o.xx * xx + o.yx * xy;
    r.yx = o.xx * yx + o.yx * yy;

    r.xy = o.xy * xx + o.yy * xy;
    r.yy = o.xy * yx + o.yy * yy;

    r.x0 = o.x0 * xx + o.y0 * xy + x0;
    r.y0 = o.x0 * yx + o.y0 * yy + y0;

    *this = r;
  }

  void transform_distance (float &dx, float &dy)
  {
    float new_x = xx * dx + xy * dy;
    float new_y = yx * dx + yy * dy;
    dx = new_x;
    dy = new_y;
  }

  void transform_point (float &x, float &y)
  {
    transform_distance (x, y);
    x += x0;
    y += y0;
  }

  void transform_extents (hb_extents_t extents)
  {
    float quad_x[4], quad_y[4];

    quad_x[0] = extents.xmin;
    quad_y[0] = extents.ymin;
    quad_x[1] = extents.xmin;
    quad_y[1] = extents.ymax;
    quad_x[2] = extents.xmax;
    quad_y[2] = extents.ymin;
    quad_x[3] = extents.xmax;
    quad_y[3] = extents.ymax;

    for (unsigned i = 0; i < 4; i++)
      transform_point (quad_x[i], quad_y[i]);

    extents.xmin = extents.xmax = quad_x[0];
    extents.ymin = extents.ymax = quad_y[0];

    for (unsigned i = 1; i < 4; i++)
    {
      extents.xmin = hb_min (extents.xmin, quad_x[i]);
      extents.ymin = hb_min (extents.ymin, quad_y[i]);
      extents.xmax = hb_max (extents.xmax, quad_x[i]);
      extents.ymax = hb_max (extents.ymax, quad_y[i]);
    }
  }

  float xx = 1.f;
  float yx = 0.f;
  float xy = 0.f;
  float yy = 1.f;
  float x0 = 0.f;
  float y0 = 0.f;
} hb_transform_t;

typedef struct hb_bounds_t
{
  enum status_t {
    EMPTY,
    BOUNDED,
    UNBOUNDED,
  };

  hb_bounds_t (status_t status) : status (status) {}
  hb_bounds_t (const hb_extents_t &extents) :
    status (extents.is_empty () ? EMPTY : BOUNDED), extents (extents) {}

  status_t status;
  hb_extents_t extents;
} hb_bounds_t;

typedef struct  hb_paint_extents_context_t hb_paint_extents_context_t;

struct hb_paint_extents_context_t {

  hb_paint_extents_context_t ()
  {
    clips.push (hb_bounds_t{hb_bounds_t::UNBOUNDED});
    groups.push (hb_bounds_t{hb_bounds_t::EMPTY});
    transforms.push (hb_transform_t{});
  }

  hb_extents_t get_extents ()
  {
    return groups.tail().extents;
  }

  bool is_bounded ()
  {
    return groups.tail().status != hb_bounds_t::UNBOUNDED;
  }

  void push_transform (const hb_transform_t &trans)
  {
    hb_transform_t r = transforms.tail ();
    r.multiply (trans);
    transforms.push (r);
  }

  void pop_transform ()
  {
    transforms.pop ();
  }

  void push_clip (hb_extents_t extents)
  {
    /* Transform extents and push a new clip. */
    hb_transform_t &r = transforms.tail ();
    r.transform_extents (extents);

    hb_bounds_t b {extents};
    clips.push (b);
  }

  void pop_clip ()
  {
    clips.pop ();
  }

  void push_group ()
  {
    groups.push (hb_bounds_t{hb_bounds_t::EMPTY});
  }

  void pop_group (hb_paint_composite_mode_t mode)
  {
    const hb_bounds_t src_bounds = groups.pop ();
    hb_bounds_t &backdrop_bounds = groups.tail ();

    switch ((int) mode)
    {
      case HB_PAINT_COMPOSITE_MODE_CLEAR:
	backdrop_bounds.status = hb_bounds_t::EMPTY;
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC:
      case HB_PAINT_COMPOSITE_MODE_SRC_OUT:
	backdrop_bounds = src_bounds;
	break;
      case HB_PAINT_COMPOSITE_MODE_DEST:
      case HB_PAINT_COMPOSITE_MODE_DEST_OUT:
	break;
      case HB_PAINT_COMPOSITE_MODE_SRC_IN:
      case HB_PAINT_COMPOSITE_MODE_DEST_IN:
	// Intersect
	if (src_bounds.status == hb_bounds_t::EMPTY)
	  backdrop_bounds.status = hb_bounds_t::EMPTY;
	else if (src_bounds.status == hb_bounds_t::BOUNDED)
	{
	  if (backdrop_bounds.status == hb_bounds_t::UNBOUNDED)
	    backdrop_bounds = src_bounds;
	  else if (backdrop_bounds.status == hb_bounds_t::BOUNDED)
	  {
	    backdrop_bounds.extents.xmin = hb_max (backdrop_bounds.extents.xmin, src_bounds.extents.xmin);
	    backdrop_bounds.extents.ymin = hb_max (backdrop_bounds.extents.ymin, src_bounds.extents.ymin);
	    backdrop_bounds.extents.xmax = hb_min (backdrop_bounds.extents.xmax, src_bounds.extents.xmax);
	    backdrop_bounds.extents.ymax = hb_min (backdrop_bounds.extents.ymax, src_bounds.extents.ymax);
	    if (backdrop_bounds.extents.xmin >= backdrop_bounds.extents.xmax ||
	        backdrop_bounds.extents.ymin >= backdrop_bounds.extents.ymax)
	      backdrop_bounds.status = hb_bounds_t::EMPTY;
	  }
	}
	break;
      default:
	// Union
	if (src_bounds.status == hb_bounds_t::UNBOUNDED)
	  backdrop_bounds.status = hb_bounds_t::UNBOUNDED;
	else if (src_bounds.status == hb_bounds_t::BOUNDED)
	{
	  if (backdrop_bounds.status == hb_bounds_t::EMPTY)
	    backdrop_bounds = src_bounds;
	  else if (backdrop_bounds.status == hb_bounds_t::BOUNDED)
	  {
	    backdrop_bounds.extents.xmin = hb_min (backdrop_bounds.extents.xmin, src_bounds.extents.xmin);
	    backdrop_bounds.extents.ymin = hb_min (backdrop_bounds.extents.ymin, src_bounds.extents.ymin);
	    backdrop_bounds.extents.xmax = hb_max (backdrop_bounds.extents.xmax, src_bounds.extents.xmax);
	    backdrop_bounds.extents.ymax = hb_max (backdrop_bounds.extents.ymax, src_bounds.extents.ymax);
	  }
	}
	break;
     }
  }

  void paint ()
  {
    /* Union current clip bounds with current group bounds. */
    const hb_bounds_t &clip = clips.tail ();
    hb_bounds_t &group = groups.tail ();

    if (clip.status == hb_bounds_t::EMPTY)
      return; // Shouldn't happen

    if (group.status == hb_bounds_t::UNBOUNDED)
      return;

    if (group.status == hb_bounds_t::EMPTY)
    {
      group = clip;
      return;
    }

    /* Group is bounded now.  Clip is not empty. */

    if (clip.status == hb_bounds_t::UNBOUNDED)
    {
      group.status = hb_bounds_t::UNBOUNDED;
      return;
    }

    /* Both are bounded. Union. */
    group.extents.xmin = hb_min (group.extents.xmin, clip.extents.xmin);
    group.extents.ymin = hb_min (group.extents.ymin, clip.extents.ymin);
    group.extents.xmax = hb_max (group.extents.xmax, clip.extents.xmax);
    group.extents.ymax = hb_max (group.extents.ymax, clip.extents.ymax);
  }

  hb_vector_t<hb_bounds_t> clips;
  hb_vector_t<hb_bounds_t> groups;
  hb_vector_t<hb_transform_t> transforms;
};

static void
hb_paint_extents_push_transform (hb_paint_funcs_t *funcs HB_UNUSED,
				 void *paint_data,
				 float xx, float yx,
				 float xy, float yy,
				 float dx, float dy,
				 void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->push_transform (hb_transform_t {xx, yx, xy, yy, dx, dy});
}

static void
hb_paint_extents_pop_transform (hb_paint_funcs_t *funcs HB_UNUSED,
			        void *paint_data,
				void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->pop_transform ();
}

static void
add_point (hb_extents_t *extents,
           float x, float y)
{
  if (extents->xmax < extents->xmin)
  {
    extents->xmin = extents->xmax = x;
    extents->ymin = extents->ymax = y;
  }
  else
  {
    extents->xmin = hb_min (extents->xmin, x);
    extents->ymin = hb_min (extents->ymin, y);
    extents->xmax = hb_max (extents->xmax, x);
    extents->ymax = hb_max (extents->ymax, y);
  }
}

static void
move_to (hb_draw_funcs_t *dfuncs,
         void *data,
         hb_draw_state_t *st,
         float to_x, float to_y,
         void *)
{
  hb_extents_t *extents = (hb_extents_t *)data;
  add_point (extents, to_x, to_y);
}

static void
line_to (hb_draw_funcs_t *dfuncs,
         void *data,
         hb_draw_state_t *st,
         float to_x, float to_y,
         void *)
{
  hb_extents_t *extents = (hb_extents_t *)data;
  add_point (extents, to_x, to_y);
}

static void
cubic_to (hb_draw_funcs_t *dfuncs,
          void *data,
          hb_draw_state_t *st,
          float control1_x, float control1_y,
          float control2_x, float control2_y,
          float to_x, float to_y,
          void *)
{
  hb_extents_t *extents = (hb_extents_t *)data;
  add_point (extents, control1_x, control1_y);
  add_point (extents, control2_x, control2_y);
  add_point (extents, to_x, to_y);
}

static void
close_path (hb_draw_funcs_t *dfuncs,
            void *data,
            hb_draw_state_t *st,
            void *)
{
}

static hb_draw_funcs_t *
hb_draw_extent_get_funcs ()
{
  hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, move_to, nullptr, nullptr);
  hb_draw_funcs_set_line_to_func (funcs, line_to, nullptr, nullptr);
  hb_draw_funcs_set_cubic_to_func (funcs, cubic_to, nullptr, nullptr);
  hb_draw_funcs_set_close_path_func (funcs, close_path, nullptr, nullptr);
  return funcs;
}

static void
hb_paint_extents_push_clip_glyph (hb_paint_funcs_t *funcs HB_UNUSED,
				  void *paint_data,
				  hb_codepoint_t glyph,
				  hb_font_t *font,
				  void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  hb_extents_t extents;
  hb_draw_funcs_t *draw_extent_funcs = hb_draw_extent_get_funcs ();
  hb_font_draw_glyph (font, glyph, draw_extent_funcs, &extents);
  hb_draw_funcs_destroy (draw_extent_funcs);
  c->push_clip (extents);
}

static void
hb_paint_extents_push_clip_rectangle (hb_paint_funcs_t *funcs HB_UNUSED,
				      void *paint_data,
				      float xmin, float ymin, float xmax, float ymax,
				      void *user_data)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  hb_extents_t extents = {xmin, ymin, xmax, ymax};
  c->push_clip (extents);
}

static void
hb_paint_extents_pop_clip (hb_paint_funcs_t *funcs HB_UNUSED,
			   void *paint_data,
			   void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->pop_clip ();
}

static void
hb_paint_extents_push_group (hb_paint_funcs_t *funcs HB_UNUSED,
			     void *paint_data,
			     void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->push_group ();
}

static void
hb_paint_extents_pop_group (hb_paint_funcs_t *funcs HB_UNUSED,
			    void *paint_data,
			    hb_paint_composite_mode_t mode,
			    void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->pop_group (mode);
}

static void
hb_paint_extents_paint_image (hb_paint_funcs_t *funcs HB_UNUSED,
			      void *paint_data,
			      hb_blob_t *blob HB_UNUSED,
			      unsigned int width HB_UNUSED,
			      unsigned int height HB_UNUSED,
			      hb_tag_t format HB_UNUSED,
			      float slant HB_UNUSED,
			      hb_glyph_extents_t *glyph_extents,
			      void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  hb_extents_t extents = {(float) glyph_extents->x_bearing,
			  (float) glyph_extents->y_bearing + glyph_extents->height,
			  (float) glyph_extents->x_bearing + glyph_extents->width,
			  (float) glyph_extents->y_bearing};
  c->push_clip (extents);
  c->paint ();
  c->pop_clip ();
}

static void
hb_paint_extents_paint_color (hb_paint_funcs_t *funcs HB_UNUSED,
			      void *paint_data,
			      hb_bool_t use_foreground HB_UNUSED,
			      hb_color_t color HB_UNUSED,
			      void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->paint ();
}

static void
hb_paint_extents_paint_linear_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
				        void *paint_data,
				        hb_color_line_t *color_line HB_UNUSED,
				        float x0 HB_UNUSED, float y0 HB_UNUSED,
				        float x1 HB_UNUSED, float y1 HB_UNUSED,
				        float x2 HB_UNUSED, float y2 HB_UNUSED,
				        void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->paint ();
}

static void
hb_paint_extents_paint_radial_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
				        void *paint_data,
				        hb_color_line_t *color_line HB_UNUSED,
				        float x0 HB_UNUSED, float y0 HB_UNUSED, float r0 HB_UNUSED,
				        float x1 HB_UNUSED, float y1 HB_UNUSED, float r1 HB_UNUSED,
				        void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->paint ();
}

static void
hb_paint_extents_paint_sweep_gradient (hb_paint_funcs_t *funcs HB_UNUSED,
				       void *paint_data,
				       hb_color_line_t *color_line HB_UNUSED,
				       float cx HB_UNUSED, float cy HB_UNUSED,
				       float start_angle HB_UNUSED,
				       float end_angle HB_UNUSED,
				       void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  c->paint ();
}

static inline hb_paint_funcs_t *
hb_paint_extents_get_funcs ()
{
  hb_paint_funcs_t *funcs = hb_paint_funcs_create ();

  hb_paint_funcs_set_push_transform_func (funcs, hb_paint_extents_push_transform, nullptr, nullptr);
  hb_paint_funcs_set_pop_transform_func (funcs, hb_paint_extents_pop_transform, nullptr, nullptr);
  hb_paint_funcs_set_push_clip_glyph_func (funcs, hb_paint_extents_push_clip_glyph, nullptr, nullptr);
  hb_paint_funcs_set_push_clip_rectangle_func (funcs, hb_paint_extents_push_clip_rectangle, nullptr, nullptr);
  hb_paint_funcs_set_pop_clip_func (funcs, hb_paint_extents_pop_clip, nullptr, nullptr);
  hb_paint_funcs_set_push_group_func (funcs, hb_paint_extents_push_group, nullptr, nullptr);
  hb_paint_funcs_set_pop_group_func (funcs, hb_paint_extents_pop_group, nullptr, nullptr);
  hb_paint_funcs_set_color_func (funcs, hb_paint_extents_paint_color, nullptr, nullptr);
  hb_paint_funcs_set_image_func (funcs, hb_paint_extents_paint_image, nullptr, nullptr);
  hb_paint_funcs_set_linear_gradient_func (funcs, hb_paint_extents_paint_linear_gradient, nullptr, nullptr);
  hb_paint_funcs_set_radial_gradient_func (funcs, hb_paint_extents_paint_radial_gradient, nullptr, nullptr);
  hb_paint_funcs_set_sweep_gradient_func (funcs, hb_paint_extents_paint_sweep_gradient, nullptr, nullptr);

  return funcs;
}

#endif /* HB_PAINT_EXTENTS_HH */
