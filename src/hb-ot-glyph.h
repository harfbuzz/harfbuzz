/*
 * Copyright © 2019  Ebrahim Byagowi
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

typedef struct hb_ot_glyph_path_t hb_ot_glyph_path_t;

HB_EXTERN hb_ot_glyph_path_t *
hb_ot_glyph_path_empty (void);

HB_EXTERN hb_ot_glyph_path_t *
hb_ot_glyph_create_path (hb_position_t     *coords,
			 unsigned int       coords_count,
			 uint8_t           *commands,
			 unsigned int       commands_count,
			 void              *user_data,
			 hb_destroy_func_t  destroy);

HB_EXTERN const hb_position_t *
hb_ot_glyph_path_get_coords (hb_ot_glyph_path_t *path, unsigned int *count);

HB_EXTERN const uint8_t *
hb_ot_glyph_path_get_commands (hb_ot_glyph_path_t *path, unsigned int *count);

HB_EXTERN hb_ot_glyph_path_t *
hb_ot_glyph_path_reference (hb_ot_glyph_path_t *path);

HB_EXTERN void
hb_ot_glyph_path_destroy (hb_ot_glyph_path_t *path);

HB_EXTERN hb_ot_glyph_path_t *
hb_ot_glyph_create_path_from_font (hb_font_t *font, hb_codepoint_t glyph);

HB_END_DECLS

#endif /* HB_OT_GLYPH_H */
