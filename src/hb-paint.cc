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

/**
 * SECTION: hb-paint
 * @title: hb-paint
 * @short_description: Glyph painting
 * @include: hb.h
 *
 * Functions for painting glyphs.
 *
 * The main purpose of these functions is to paint
 * (extract) color glyph layers from the COLRv1 table,
 * but the API works for drawing ordinary outlines
 * and images as well.
 **/

static void
hb_paint_push_transform_nil (hb_paint_funcs_t *funcs, void *paint_data,
                             float xx, float yx,
                             float xy, float yy,
                             float dx, float dy,
                             void *user_data) {}

static void
hb_paint_pop_transform_nil (hb_paint_funcs_t *funcs, void *paint_data,
                            void *user_data) {}

static void
hb_paint_push_clip_glyph_nil (hb_paint_funcs_t *funcs, void *paint_data,
                              hb_codepoint_t glyph,
                              hb_font_t *font,
                              void *user_data) {}

static void
hb_paint_push_clip_rectangle_nil (hb_paint_funcs_t *funcs, void *paint_data,
                                  float xmin, float ymin, float xmax, float ymax,
                                  void *user_data) {}

static void
hb_paint_pop_clip_nil (hb_paint_funcs_t *funcs, void *paint_data,
                       void *user_data) {}

static void
hb_paint_color_nil (hb_paint_funcs_t *funcs, void *paint_data,
                    hb_bool_t is_foreground,
                    hb_color_t color,
                    void *user_data) {}

static void
hb_paint_image_nil (hb_paint_funcs_t *funcs, void *paint_data,
                    hb_blob_t *image,
                    unsigned int width,
                    unsigned int height,
                    hb_tag_t format,
                    float slant_xy,
                    hb_glyph_extents_t *extents,
                    void *user_data) {}

static void
hb_paint_linear_gradient_nil (hb_paint_funcs_t *funcs, void *paint_data,
                              hb_color_line_t *color_line,
                              float x0, float y0,
                              float x1, float y1,
                              float x2, float y2,
                              void *user_data) {}

static void
hb_paint_radial_gradient_nil (hb_paint_funcs_t *funcs, void *paint_data,
                              hb_color_line_t *color_line,
                              float x0, float y0, float r0,
                              float x1, float y1, float r1,
                              void *user_data) {}

static void
hb_paint_sweep_gradient_nil (hb_paint_funcs_t *funcs, void *paint_data,
                             hb_color_line_t *color_line,
                             float x0, float y0,
                             float start_angle,
                             float end_angle,
                             void *user_data) {}

static void
hb_paint_push_group_nil (hb_paint_funcs_t *funcs, void *paint_data,
                         void *user_data) {}

static void
hb_paint_pop_group_nil (hb_paint_funcs_t *funcs, void *paint_data,
                        hb_paint_composite_mode_t mode,
                        void *user_data) {}

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

/**
 * hb_paint_funcs_create:
 *
 * Creates a new #hb_paint_funcs_t structure of paint functions.
 *
 * The initial reference count of 1 should be released with hb_paint_funcs_destroy()
 * when you are done using the #hb_paint_funcs_t. This function never returns
 * `NULL`. If memory cannot be allocated, a special singleton #hb_paint_funcs_t
 * object will be returned.
 *
 * Returns value: (transfer full): the paint-functions structure
 *
 * Since: REPLACEME
 */
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

/**
 * hb_paint_funcs_get_empty:
 *
 * Fetches the singleton empty paint-functions structure.
 *
 * Return value: (transfer full): The empty paint-functions structure
 *
 * Since: REPLACEME
 **/
hb_paint_funcs_t *
hb_paint_funcs_get_empty ()
{
  return const_cast<hb_paint_funcs_t *> (&Null (hb_paint_funcs_t));
}

/**
 * hb_paint_funcs_reference: (skip)
 * @funcs: The paint-functions structure
 *
 * Increases the reference count on a paint-functions structure.
 *
 * This prevents @funcs from being destroyed until a matching
 * call to hb_paint_funcs_destroy() is made.
 *
 * Return value: The paint-functions structure
 *
 * Since: REPLACEME
 */
hb_paint_funcs_t *
hb_paint_funcs_reference (hb_paint_funcs_t *funcs)
{
  return hb_object_reference (funcs);
}

/**
 * hb_paint_funcs_destroy: (skip)
 * @funcs: The paint-functions structure
 *
 * Decreases the reference count on a paint-functions structure.
 *
 * When the reference count reaches zero, the structure
 * is destroyed, freeing all memory.
 *
 * Since: REPLACEME
 */
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

/**
 * hb_paint_funcs_set_user_data: (skip)
 * @funcs: The paint-functions structure
 * @key: The user-data key
 * @data: A pointer to the user data
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified paint-functions structure.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * Since: REPLACEME
 **/
hb_bool_t
hb_paint_funcs_set_user_data (hb_paint_funcs_t *funcs,
			     hb_user_data_key_t *key,
			     void *              data,
			     hb_destroy_func_t   destroy,
			     hb_bool_t           replace)
{
  return hb_object_set_user_data (funcs, key, data, destroy, replace);
}

/**
 * hb_paint_funcs_get_user_data: (skip)
 * @funcs: The paint-functions structure
 * @key: The user-data key to query
 *
 * Fetches the user-data associated with the specified key,
 * attached to the specified paint-functions structure.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: REPLACEME
 **/
void *
hb_paint_funcs_get_user_data (const hb_paint_funcs_t *funcs,
			     hb_user_data_key_t       *key)
{
  return hb_object_get_user_data (funcs, key);
}

/**
 * hb_paint_funcs_make_immutable:
 * @funcs: The paint-functions structure
 *
 * Makes a paint-functions structure immutable.
 *
 * After this call, all attempts to set one of the callbacks
 * on @funcs will fail.
 *
 * Since: REPLACEME
 */
void
hb_paint_funcs_make_immutable (hb_paint_funcs_t *funcs)
{
  if (hb_object_is_immutable (funcs))
    return;

  hb_object_make_immutable (funcs);
}

/**
 * hb_paint_funcs_is_immutable:
 * @funcs: The paint-functions structure
 *
 * Tests whether a paint-functions structure is immutable.
 *
 * Return value: `true` if @funcs is immutable, `false` otherwise
 *
 * Since: REPLACEME
 */
hb_bool_t
hb_paint_funcs_is_immutable (hb_paint_funcs_t *funcs)
{
  return hb_object_is_immutable (funcs);
}

#endif
