/*
 * Copyright © 2020  Ebrahim Byagowi
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

#ifndef HB_DRAW_HH
#define HB_DRAW_HH

#include "hb.hh"


/*
 * hb_draw_funcs_t
 */

#define HB_DRAW_FUNCS_IMPLEMENT_CALLBACKS \
  HB_DRAW_FUNC_IMPLEMENT (move_to) \
  HB_DRAW_FUNC_IMPLEMENT (line_to) \
  HB_DRAW_FUNC_IMPLEMENT (quadratic_to) \
  HB_DRAW_FUNC_IMPLEMENT (cubic_to) \
  HB_DRAW_FUNC_IMPLEMENT (close_path) \
  /* ^--- Add new callbacks here */

struct hb_draw_funcs_t
{
  hb_object_header_t header;

  struct {
#define HB_DRAW_FUNC_IMPLEMENT(name) hb_draw_##name##_func_t name;
    HB_DRAW_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_DRAW_FUNC_IMPLEMENT
  } func;

  struct {
#define HB_DRAW_FUNC_IMPLEMENT(name) void *name;
    HB_DRAW_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_DRAW_FUNC_IMPLEMENT
  } user_data;

  struct {
#define HB_DRAW_FUNC_IMPLEMENT(name) hb_destroy_func_t name;
    HB_DRAW_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_DRAW_FUNC_IMPLEMENT
  } destroy;

  void move_to (void *draw_data,
		float to_x, float to_y)
  { func.move_to (this, draw_data,
		  to_x, to_y,
		  user_data.move_to); }
  void line_to (void *draw_data,
		float to_x, float to_y)
  { func.line_to (this, draw_data,
		  to_x, to_y,
		  user_data.line_to); }
  void quadratic_to (void *draw_data,
		     float control_x, float control_y,
		     float to_x, float to_y)
  { func.quadratic_to (this, draw_data,
		       control_x, control_y,
		       to_x, to_y,
		       user_data.quadratic_to); }
  void cubic_to (void *draw_data,
		 float control1_x, float control1_y,
		 float control2_x, float control2_y,
		 float to_x, float to_y)
  { func.cubic_to (this, draw_data,
		   control1_x, control1_y,
		   control2_x, control2_y,
		   to_x, to_y,
		   user_data.cubic_to); }
  void close_path (void *draw_data)
  { func.close_path (this, draw_data,
		     user_data.close_path); }

  /* XXX Remove */
  HB_INTERNAL bool quadratic_to_is_set ();
};
DECLARE_NULL_INSTANCE (hb_draw_funcs_t);

struct draw_helper_t
{
  draw_helper_t (hb_draw_funcs_t *funcs_, void *draw_data_)
    : funcs {funcs_}, draw_data {draw_data_}, st HB_DRAW_STATE_DEFAULT {}

  ~draw_helper_t () { end_path (); }

  void move_to (float x, float y)
  {
    if (st.path_open) end_path ();
    st.current_x = st.path_start_x = x;
    st.current_y = st.path_start_y = y;
  }

  void line_to (float x, float y)
  {
    if (!st.path_open) start_path ();
    funcs->line_to (draw_data, x, y);
    st.current_x = x;
    st.current_y = y;
  }

  void
  quadratic_to (float control_x, float control_y,
		float to_x, float to_y)
  {
    if (!st.path_open) start_path ();
    if (funcs->quadratic_to_is_set ())
      funcs->quadratic_to (draw_data, control_x, control_y, to_x, to_y);
    else
      funcs->cubic_to (draw_data,
		       (st.current_x + 2.f * control_x) / 3.f,
		       (st.current_y + 2.f * control_y) / 3.f,
		       (to_x + 2.f * control_x) / 3.f,
		       (to_y + 2.f * control_y) / 3.f,
		       to_x, to_y);
    st.current_x = to_x;
    st.current_y = to_y;
  }

  void
  cubic_to (float control1_x, float control1_y,
	    float control2_x, float control2_y,
	    float to_x, float to_y)
  {
    if (!st.path_open) start_path ();
    funcs->cubic_to (draw_data, control1_x, control1_y, control2_x, control2_y, to_x, to_y);
    st.current_x = to_x;
    st.current_y = to_y;
  }

  void end_path ()
  {
    if (st.path_open)
    {
      if ((st.path_start_x != st.current_x) || (st.path_start_y != st.current_y))
	funcs->line_to (draw_data, st.path_start_x, st.path_start_y);
      funcs->close_path (draw_data);
    }
    st.path_open = false;
    st.path_start_x = st.current_x = st.path_start_y = st.current_y = 0;
  }

  protected:

  void start_path ()
  {
    if (st.path_open) end_path ();
    st.path_open = true;
    funcs->move_to (draw_data, st.path_start_x, st.path_start_y);
  }

  hb_draw_funcs_t *funcs;
  void *draw_data;
  hb_draw_state_t st;
};

#endif /* HB_DRAW_HH */
