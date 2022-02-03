/*
 * Copyright © 2019-2020  Ebrahim Byagowi
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

#ifndef HB_NO_DRAW

#include "hb-draw.hh"

static void
hb_draw_move_to_nil (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data HB_UNUSED,
		     float to_x HB_UNUSED, float to_y HB_UNUSED,
		     void *user_data HB_UNUSED) {}

static void
hb_draw_line_to_nil (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data HB_UNUSED,
		     float to_x HB_UNUSED, float to_y HB_UNUSED,
		     void *user_data HB_UNUSED) {}

static void
hb_draw_quadratic_to_nil (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data HB_UNUSED,
			  float control_x HB_UNUSED, float control_y HB_UNUSED,
			  float to_x HB_UNUSED, float to_y HB_UNUSED,
			  void *user_data HB_UNUSED) {}

static void
hb_draw_cubic_to_nil (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data HB_UNUSED,
		      float control1_x HB_UNUSED, float control1_y HB_UNUSED,
		      float control2_x HB_UNUSED, float control2_y HB_UNUSED,
		      float to_x HB_UNUSED, float to_y HB_UNUSED,
		      void *user_data HB_UNUSED) {}

static void
hb_draw_close_path_nil (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data HB_UNUSED,
			void *user_data HB_UNUSED) {}


#define HB_DRAW_FUNC_IMPLEMENT(name)						\
										\
void										\
hb_draw_funcs_set_##name##_func (hb_draw_funcs_t	 *dfuncs,		\
				 hb_draw_##name##_func_t  func,			\
				 void			 *user_data,		\
				 hb_destroy_func_t	  destroy)		\
{										\
  if (hb_object_is_immutable (dfuncs))						\
    return;									\
										\
  if (dfuncs->destroy.name)							\
    dfuncs->destroy.name (dfuncs->user_data.name);				\
										\
  if (func) {									\
    dfuncs->func.name = func;							\
    dfuncs->user_data.name = user_data;						\
    dfuncs->destroy.name = destroy;						\
  } else {									\
    dfuncs->func.name = hb_draw_##name##_nil;					\
    dfuncs->user_data.name = nullptr;						\
    dfuncs->destroy.name = nullptr;						\
  }										\
}

HB_DRAW_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_DRAW_FUNC_IMPLEMENT


/**
 * hb_draw_funcs_set_move_to_func:
 * @funcs: draw functions object
 * @move_to: move-to callback
 *
 * Sets move-to callback to the draw functions object.
 *
 * Since: REPLACEME
 **/

/**
 * hb_draw_funcs_set_line_to_func:
 * @funcs: draw functions object
 * @line_to: line-to callback
 *
 * Sets line-to callback to the draw functions object.
 *
 * Since: REPLACEME
 **/

/**
 * hb_draw_funcs_set_quadratic_to_func:
 * @funcs: draw functions object
 * @move_to: quadratic-to callback
 *
 * Sets quadratic-to callback to the draw functions object.
 *
 * Since: REPLACEME
 **/

/**
 * hb_draw_funcs_set_cubic_to_func:
 * @funcs: draw functions
 * @cubic_to: cubic-to callback
 *
 * Sets cubic-to callback to the draw functions object.
 *
 * Since: REPLACEME
 **/

/**
 * hb_draw_funcs_set_close_path_func:
 * @funcs: draw functions object
 * @close_path: close-path callback
 *
 * Sets close-path callback to the draw functions object.
 *
 * Since: REPLACEME
 **/

/**
 * hb_draw_funcs_create:
 *
 * Creates a new draw callbacks object.
 *
 * Since: REPLACEME
 **/
hb_draw_funcs_t *
hb_draw_funcs_create ()
{
  hb_draw_funcs_t *funcs;
  if (unlikely (!(funcs = hb_object_create<hb_draw_funcs_t> ())))
    return const_cast<hb_draw_funcs_t *> (&Null (hb_draw_funcs_t));

  /* XXX Clean up. Use nil object. */
  funcs->func.move_to =  hb_draw_move_to_nil;
  funcs->func.line_to = hb_draw_line_to_nil;
  funcs->func.quadratic_to = hb_draw_quadratic_to_nil;
  funcs->func.cubic_to = hb_draw_cubic_to_nil;
  funcs->func.close_path = hb_draw_close_path_nil;
  return funcs;
}

DEFINE_NULL_INSTANCE (hb_draw_funcs_t) =
{
  HB_OBJECT_HEADER_STATIC,

  {
#define HB_DRAW_FUNC_IMPLEMENT(name) hb_draw_##name##_nil,
    HB_DRAW_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_DRAW_FUNC_IMPLEMENT
  }
};

/* XXX Remove. */

bool
hb_draw_funcs_t::quadratic_to_is_set () { return func.quadratic_to != hb_draw_quadratic_to_nil; }

/**
 * hb_draw_funcs_reference:
 * @funcs: draw functions
 *
 * Add to callbacks object refcount.
 *
 * Returns: The same object.
 * Since: REPLACEME
 **/
hb_draw_funcs_t *
hb_draw_funcs_reference (hb_draw_funcs_t *funcs)
{
  return hb_object_reference (funcs);
}

/**
 * hb_draw_funcs_destroy:
 * @funcs: draw functions
 *
 * Decreases refcount of callbacks object and deletes the object if it reaches
 * to zero.
 *
 * Since: REPLACEME
 **/
void
hb_draw_funcs_destroy (hb_draw_funcs_t *funcs)
{
  if (!hb_object_destroy (funcs)) return;

  hb_free (funcs);
}

/**
 * hb_draw_funcs_make_immutable:
 * @funcs: draw functions
 *
 * Makes funcs object immutable.
 *
 * Since: REPLACEME
 **/
void
hb_draw_funcs_make_immutable (hb_draw_funcs_t *funcs)
{
  if (hb_object_is_immutable (funcs))
    return;

  hb_object_make_immutable (funcs);
}

/**
 * hb_draw_funcs_is_immutable:
 * @funcs: draw functions
 *
 * Checks whether funcs is immutable.
 *
 * Returns: If is immutable.
 * Since: REPLACEME
 **/
hb_bool_t
hb_draw_funcs_is_immutable (hb_draw_funcs_t *funcs)
{
  return hb_object_is_immutable (funcs);
}

#endif
