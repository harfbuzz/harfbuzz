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

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_DRAW_H
#define HB_DRAW_H

#include "hb.h"

HB_BEGIN_DECLS

/**
 * hb_draw_funcs_t:
 *
 * Glyph draw callbacks.
 *
 * _move_to, _line_to and _cubic_to calls are necessary to be defined but we
 * translate _quadratic_to calls to _cubic_to if the callback isn't defined.
 *
 * Since: REPLACEME
 **/

typedef struct hb_draw_funcs_t hb_draw_funcs_t;

typedef void (*hb_draw_move_to_func_t) (hb_draw_funcs_t *dfuncs, void *draw_data,
					float to_x, float to_y,
					void *user_data);
typedef void (*hb_draw_line_to_func_t) (hb_draw_funcs_t *dfuncs, void *draw_data,
					float to_x, float to_y,
					void *user_data);
typedef void (*hb_draw_quadratic_to_func_t) (hb_draw_funcs_t *dfuncs, void *draw_data,
					     float control_x, float control_y,
					     float to_x, float to_y,
					     void *user_data);
typedef void (*hb_draw_cubic_to_func_t) (hb_draw_funcs_t *dfuncs, void *draw_data,
					 float control1_x, float control1_y,
					 float control2_x, float control2_y,
					 float to_x, float to_y,
					 void *user_data);
typedef void (*hb_draw_close_path_func_t) (hb_draw_funcs_t *dfuncs, void *draw_data,
					   void *user_data);

HB_EXTERN void
hb_draw_funcs_set_move_to_func (hb_draw_funcs_t        *funcs,
				hb_draw_move_to_func_t  move_to,
				void *user_data, hb_destroy_func_t destroy);

HB_EXTERN void
hb_draw_funcs_set_line_to_func (hb_draw_funcs_t        *funcs,
				hb_draw_line_to_func_t  line_to,
				void *user_data, hb_destroy_func_t destroy);

HB_EXTERN void
hb_draw_funcs_set_quadratic_to_func (hb_draw_funcs_t             *funcs,
				     hb_draw_quadratic_to_func_t  quadratic_to,
				     void *user_data, hb_destroy_func_t destroy);

HB_EXTERN void
hb_draw_funcs_set_cubic_to_func (hb_draw_funcs_t         *funcs,
				 hb_draw_cubic_to_func_t  cubic_to,
				 void *user_data, hb_destroy_func_t destroy);

HB_EXTERN void
hb_draw_funcs_set_close_path_func (hb_draw_funcs_t           *funcs,
				   hb_draw_close_path_func_t  close_path,
				   void *user_data, hb_destroy_func_t destroy);

HB_EXTERN hb_draw_funcs_t *
hb_draw_funcs_create (void);

HB_EXTERN hb_draw_funcs_t *
hb_draw_funcs_reference (hb_draw_funcs_t *funcs);

HB_EXTERN void
hb_draw_funcs_destroy (hb_draw_funcs_t *funcs);

HB_EXTERN void
hb_draw_funcs_make_immutable (hb_draw_funcs_t *funcs);

HB_EXTERN hb_bool_t
hb_draw_funcs_is_immutable (hb_draw_funcs_t *funcs);

HB_END_DECLS

#endif /* HB_DRAW_H */
