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

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_GLYPH_H
#define HB_OT_GLYPH_H

#include "hb.h"

HB_BEGIN_DECLS

typedef void (*hb_ot_glyph_decompose_move_to_func_t) (hb_position_t to_x, hb_position_t to_y, void *user_data);
typedef void (*hb_ot_glyph_decompose_line_to_func_t) (hb_position_t to_x, hb_position_t to_y, void *user_data);
typedef void (*hb_ot_glyph_decompose_conic_to_func_t) (hb_position_t control_x, hb_position_t control_y,
						       hb_position_t to_x, hb_position_t to_y,
						       void *user_data);
typedef void (*hb_ot_glyph_decompose_cubic_to_func_t) (hb_position_t control1_x, hb_position_t control1_y,
						       hb_position_t control2_x, hb_position_t control2_y,
						       hb_position_t to_x, hb_position_t to_y,
						       void *user_data);
typedef void (*hb_ot_glyph_decompose_close_path_func_t) (void *user_data);

/**
 * hb_ot_glyph_decompose_funcs_t:
 *
 * Glyph decompose callbacks.
 *
 * Since: REPLACEME
 **/
typedef struct hb_ot_glyph_decompose_funcs_t hb_ot_glyph_decompose_funcs_t;

HB_EXTERN void
hb_ot_glyph_decompose_funcs_set_move_to_func (hb_ot_glyph_decompose_funcs_t        *funcs,
					      hb_ot_glyph_decompose_move_to_func_t  move_to);

HB_EXTERN void
hb_ot_glyph_decompose_funcs_set_line_to_func (hb_ot_glyph_decompose_funcs_t        *funcs,
					      hb_ot_glyph_decompose_move_to_func_t  line_to);

HB_EXTERN void
hb_ot_glyph_decompose_funcs_set_conic_to_func (hb_ot_glyph_decompose_funcs_t         *funcs,
					       hb_ot_glyph_decompose_conic_to_func_t  conic_to);

HB_EXTERN void
hb_ot_glyph_decompose_funcs_set_cubic_to_func (hb_ot_glyph_decompose_funcs_t         *funcs,
					       hb_ot_glyph_decompose_cubic_to_func_t  cubic_to);

HB_EXTERN void
hb_ot_glyph_decompose_funcs_set_close_path_func (hb_ot_glyph_decompose_funcs_t           *funcs,
						 hb_ot_glyph_decompose_close_path_func_t  close_path);

HB_EXTERN hb_ot_glyph_decompose_funcs_t *
hb_ot_glyph_decompose_funcs_create (void);

HB_EXTERN hb_ot_glyph_decompose_funcs_t *
hb_ot_glyph_decompose_funcs_reference (hb_ot_glyph_decompose_funcs_t *funcs);

HB_EXTERN void
hb_ot_glyph_decompose_funcs_destroy (hb_ot_glyph_decompose_funcs_t *funcs);

HB_EXTERN hb_bool_t
hb_ot_glyph_decompose (hb_font_t *font, hb_codepoint_t glyph,
		       const hb_ot_glyph_decompose_funcs_t *funcs,
		       void *user_data);

HB_END_DECLS

#endif /* HB_OT_GLYPH_H */
