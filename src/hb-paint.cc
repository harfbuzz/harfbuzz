/*
 * Copyright © 2022 Matthias Clasen
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

#include "hb.hh"

#ifndef HB_NO_PAINT

#include "hb-paint.hh"

static void
hb_paint_push_transform_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                             float xx HB_UNUSED, float xy HB_UNUSED,
                             float yx HB_UNUSED, float yy HB_UNUSED,
                             float x0 HB_UNUSED, float y0 HB_UNUSED,
                             void *user_data HB_UNUSED) {}

static void
hb_paint_pop_transform_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                            void *user_data HB_UNUSED) {}

static void
hb_paint_push_clip_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                        hb_codepoint_t glyph HB_UNUSED,
                        void *user_data HB_UNUSED) {}

static void
hb_paint_pop_clip_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                       void *user_data HB_UNUSED) {}

static void
hb_paint_solid_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                    unsigned int color_index HB_UNUSED,
                    void *user_data HB_UNUSED) {}

static void
hb_paint_linear_gradient_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                              hb_color_line_t *color_line HB_UNUSED,
                              hb_position_t x0, hb_position_t y0 HB_UNUSED,
                              hb_position_t x1, hb_position_t y1 HB_UNUSED,
                              hb_position_t x2, hb_position_t y2 HB_UNUSED,
                              void *user_data HB_UNUSED) {}

static void
hb_paint_radial_gradient_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                              hb_color_line_t *color_line HB_UNUSED,
                              hb_position_t x0, hb_position_t y0 HB_UNUSED,
                              float r0 HB_UNUSED,
                              hb_position_t x1 HB_UNUSED, hb_position_t y1 HB_UNUSED,
                              float r1 HB_UNUSED,
                              void *user_data HB_UNUSED) {}

static void
hb_paint_sweep_gradient_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                             hb_color_line_t *color_line HB_UNUSED,
                             hb_position_t x0, hb_position_t y0 HB_UNUSED,
                             float start_angle HB_UNUSED,
                             float end_angle HB_UNUSED,
                             void *user_data HB_UNUSED) {}

static void
hb_paint_push_group_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                     void *user_data HB_UNUSED) {}

static void
hb_paint_pop_group_and_composite_nil (hb_paint_funcs_t *funcs HB_UNUSED, void *paint_data HB_UNUSED,
                    hb_paint_composite_mode_t mode,
                    void *user_data HB_UNUSED) {}

static bool
_hb_paint_funcs_set_preamble (hb_paint_funcs_t  *funcs,
                             bool                func_is_null,
                             void              **user_data,
                             hb_destroy_func_t  *destroy)
{
  if (hb_object_is_immutable (funcs))
  {
    if (*destroy)
      (*destroy) (*user_data);
    return false;
  }

  if (func_is_null)
  {
    if (*destroy)
      (*destroy) (*user_data);
    *destroy = nullptr;
    *user_data = nullptr;
  }

  return true;
}

static bool
_hb_paint_funcs_set_middle (hb_paint_funcs_t  *funcs,
                            void              *user_data,
                            hb_destroy_func_t  destroy)
{
  if (user_data && !funcs->user_data)
  {
    funcs->user_data = (decltype (funcs->user_data)) hb_calloc (1, sizeof (*funcs->user_data));
    if (unlikely (!funcs->user_data))
      goto fail;
  }
  if (destroy && !funcs->destroy)
  {
    funcs->destroy = (decltype (funcs->destroy)) hb_calloc (1, sizeof (*funcs->destroy));
    if (unlikely (!funcs->destroy))
      goto fail;
  }

  return true;

fail:
  if (destroy)
    (destroy) (user_data);
  return false;
}

#define HB_PAINT_FUNC_IMPLEMENT(name)                                           \
                                                                                \
void                                                                            \
hb_paint_funcs_set_##name##_func (hb_paint_funcs_t         *funcs,              \
                                  hb_paint_##name##_func_t  func,               \
                                  void                     *user_data,          \
                                  hb_destroy_func_t         destroy)            \
{                                                                               \
  if (!_hb_paint_funcs_set_preamble (funcs, !func, &user_data, &destroy))       \
      return;                                                                   \
                                                                                \
  if (funcs->destroy && funcs->destroy->name)                                   \
    funcs->destroy->name (!funcs->user_data ? nullptr : funcs->user_data->name);\
                                                                                \
  if (!_hb_paint_funcs_set_middle (funcs, user_data, destroy))                  \
      return;                                                                   \
                                                                                \
  if (func)                                                                     \
    funcs->func.name = func;                                                    \
  else                                                                          \
    funcs->func.name = hb_paint_##name##_nil;                                   \
                                                                                \
  if (funcs->user_data)                                                         \
    funcs->user_data->name = user_data;                                         \
  if (funcs->destroy)                                                           \
    funcs->destroy->name = destroy;                                             \
}

HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_PAINT_FUNC_IMPLEMENT

hb_paint_funcs_t *
hb_paint_funcs_create ()
{
  hb_paint_funcs_t *funcs;
  if (unlikely (!(funcs = hb_object_create<hb_paint_funcs_t> ())))
    return const_cast<hb_paint_funcs_t *> (&Null (hb_paint_funcs_t));

  funcs->func =  Null (hb_paint_funcs_t).func;

  return funcs;
}

DEFINE_NULL_INSTANCE (hb_paint_funcs_t) =
{
  HB_OBJECT_HEADER_STATIC,

  {
#define HB_PAINT_FUNC_IMPLEMENT(name) hb_paint_##name##_nil,
    HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_PAINT_FUNC_IMPLEMENT
  }
};

hb_paint_funcs_t *
hb_paint_funcs_reference (hb_paint_funcs_t *funcs)
{
  return hb_object_reference (funcs);
}

void
hb_paint_funcs_destroy (hb_paint_funcs_t *funcs)
{
  if (!hb_object_destroy (funcs)) return;

  if (funcs->destroy)
  {
#define HB_PAINT_FUNC_IMPLEMENT(name) \
    if (funcs->destroy->name) funcs->destroy->name (!funcs->user_data ? nullptr : funcs->user_data->name);
      HB_PAINT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_PAINT_FUNC_IMPLEMENT
  }

  hb_free (funcs->destroy);
  hb_free (funcs->user_data);
  hb_free (funcs);
}

void
hb_paint_funcs_make_immutable (hb_paint_funcs_t *funcs)
{
  if (hb_object_is_immutable (funcs))
    return;

  hb_object_make_immutable (funcs);
}

hb_bool_t
hb_paint_funcs_is_immutable (hb_paint_funcs_t *funcs)
{
  return hb_object_is_immutable (funcs);
}

void
hb_paint_push_transform (hb_paint_funcs_t *funcs, void *paint_data,
                         float xx, float xy,
                         float yx, float yy,
                         float x0, float y0)
{
  funcs->push_transform (paint_data, xx, xy, yx, yy, x0, y0);
}

void
hb_paint_pop_transform (hb_paint_funcs_t *funcs, void *paint_data)
{
  funcs->pop_transform (paint_data);
}

void
hb_paint_push_clip (hb_paint_funcs_t *funcs, void *paint_data,
                    hb_codepoint_t glyph)
{
  funcs->push_clip (paint_data, glyph);
}

void
hb_paint_pop_clip (hb_paint_funcs_t *funcs, void *paint_data)
{
  funcs->pop_clip (paint_data);
}

void
hb_paint_solid (hb_paint_funcs_t *funcs, void *paint_data,
                unsigned int color_index)
{
  funcs->solid (paint_data, color_index);
}

void
hb_paint_linear_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_color_line_t *color_line,
                          hb_position_t x0, hb_position_t y0,
                          hb_position_t x1, hb_position_t y1,
                          hb_position_t x2, hb_position_t y2)
{
  funcs->linear_gradient (paint_data, color_line, x0, y0, x1, y1, x2, y2);
}

void
hb_paint_radial_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                          hb_color_line_t *color_line,
                          hb_position_t x0, hb_position_t y0,
                          float r0,
                          hb_position_t x1, hb_position_t y1,
                          float r1)
{
  funcs->radial_gradient (paint_data, color_line, x0, y0, r0, y1, x1, r1);
}

void
hb_paint_sweep_gradient (hb_paint_funcs_t *funcs, void *paint_data,
                         hb_color_line_t *color_line,
                         hb_position_t x0, hb_position_t y0,
                         float start_angle, float end_angle)
{
  funcs->sweep_gradient (paint_data, color_line, x0, y0, start_angle, end_angle);
}

void
hb_paint_push_group (hb_paint_funcs_t *funcs, void *paint_data)
{
  funcs->push_group (paint_data);
}

void
hb_paint_pop_group_and_composite (hb_paint_funcs_t *funcs, void *paint_data,
                                  hb_paint_composite_mode_t mode)
{
  funcs->pop_group_and_composite (paint_data, mode);
}

#endif
