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

  float xx = 1.f;
  float yx = 0.f;
  float xy = 0.f;
  float yy = 1.f;
  float x0 = 0.f;
  float y0 = 0.f;
} hb_transform_t;

typedef struct hb_extents_t
{
  float xmin, ymin, xmax, ymax;
} hb_extents_t;

typedef struct hb_bounds_t
{
  enum status_t {
    EMPTY,
    BOUNDED,
    UNBOUNDED,
  };

  hb_bounds_t (status_t status) : status (status) {}
  hb_bounds_t (const hb_extents_t &extents) :
    status (BOUNDED), extents (extents) {}

  status_t status;
  hb_extents_t extents = {0, 0, 0, 0};
} hb_bounds_t;

typedef struct  hb_paint_extents_context_t hb_paint_extents_context_t;

struct hb_paint_extents_context_t {

  hb_paint_extents_context_t ()
  {
    clips.push (hb_bounds_t{hb_bounds_t::status_t::UNBOUNDED});
    groups.push (hb_bounds_t{hb_bounds_t::status_t::EMPTY});
    transforms.push (hb_transform_t{});
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
    //hb_transform_t &r = transforms.tail ();

    hb_bounds_t b {extents};
    clips.push (b);
  }

  void pop_clip ()
  {
    clips.pop ();
  }

  void push_group ()
  {
    groups.push (hb_bounds_t{hb_bounds_t::status_t::EMPTY});
  }

  hb_bounds_t pop_group ()
  {
    return groups.pop ();
  }

  void paint ()
  {
    /* Union current clip bounds with current group bounds. */
    const hb_bounds_t &clip = clips.tail ();
    hb_bounds_t &group = groups.tail ();

    if (clip.status == hb_bounds_t::status_t::EMPTY)
      return; // Shouldn't happen

    if (group.status == hb_bounds_t::status_t::UNBOUNDED)
      return;

    if (group.status == hb_bounds_t::status_t::EMPTY)
    {
      group = clip;
      return;
    }

    /* Group is bounded now.  Clip is not empty. */

    if (clip.status == hb_bounds_t::status_t::UNBOUNDED)
    {
      group.status = hb_bounds_t::status_t::UNBOUNDED;
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
hb_paint_extents_push_clip_glyph (hb_paint_funcs_t *funcs HB_UNUSED,
				  void *paint_data,
				  hb_codepoint_t glyph,
				  hb_font_t *font,
				  void *user_data HB_UNUSED)
{
  hb_paint_extents_context_t *c = (hb_paint_extents_context_t *) paint_data;

  hb_glyph_extents_t glyph_extents;
  hb_font_get_glyph_extents (font, glyph, &glyph_extents);
  hb_extents_t extents = {(float) glyph_extents.x_bearing,
			  (float) glyph_extents.y_bearing + glyph_extents.height,
			  (float) glyph_extents.x_bearing + glyph_extents.width,
			  (float) glyph_extents.y_bearing};
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

  hb_bounds_t bounds = c->pop_group ();

  // TODO
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
