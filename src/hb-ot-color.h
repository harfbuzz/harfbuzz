/*
 * Copyright © 2016  Google, Inc.
 * Copyright © 2018  Khaled Hosny
 * Copyright © 2018  Ebrahim Byagowi
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
 *
 * Google Author(s): Sascha Brawer
 */

#ifndef HB_OT_H_IN
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_COLOR_H
#define HB_OT_COLOR_H

#include "hb.h"
#include "hb-ot-name.h"

HB_BEGIN_DECLS

HB_EXTERN hb_bool_t
hb_ot_color_has_cpal_data (hb_face_t *face);

HB_EXTERN hb_bool_t
hb_ot_color_has_colr_data (hb_face_t *face);

HB_EXTERN unsigned int
hb_ot_color_get_palette_count (hb_face_t *face);

HB_EXTERN hb_name_id_t
hb_ot_color_get_palette_name_id (hb_face_t *face, unsigned int palette);

HB_EXTERN unsigned int
hb_ot_color_get_palette_entry_count (hb_face_t *face);

HB_EXTERN hb_name_id_t
hb_ot_color_get_palette_entry_name_id (hb_face_t *face, unsigned int palette_entry);

HB_EXTERN unsigned int
hb_ot_color_get_palette_colors (hb_face_t      *face,
				unsigned int    palette,      /* default=0 */
				unsigned int    start_offset,
				unsigned int   *color_count   /* IN/OUT */,
				hb_color_t     *colors        /* OUT */);

HB_EXTERN unsigned int
hb_ot_color_get_color_layers (hb_face_t       *face,
			      hb_codepoint_t   gid,
			      unsigned int     offset,
			      unsigned int    *count,        /* IN/OUT */
			      hb_codepoint_t  *gids,         /* OUT */
			      unsigned int    *color_indices /* OUT */);

/**
 * hb_ot_color_palette_flags_t:
 * @HB_OT_COLOR_PALETTE_FLAG_DEFAULT: default indicating that there is nothing special to note about a color palette.
 * @HB_OT_COLOR_PALETTE_FLAG_FOR_LIGHT_BACKGROUND: flag indicating that the color palette is suitable for rendering text on light background.
 * @HB_OT_COLOR_PALETTE_FLAG_FOR_DARK_BACKGROUND: flag indicating that the color palette is suitable for rendering text on dark background.
 *
 * Since: REPLACEME
 */
typedef enum { /*< flags >*/
  HB_OT_COLOR_PALETTE_FLAG_DEFAULT = 0x00000000u,
  HB_OT_COLOR_PALETTE_FLAG_FOR_LIGHT_BACKGROUND = 0x00000001u,
  HB_OT_COLOR_PALETTE_FLAG_FOR_DARK_BACKGROUND = 0x00000002u,
} hb_ot_color_palette_flags_t;

HB_EXTERN hb_ot_color_palette_flags_t
hb_ot_color_get_palette_flags (hb_face_t *face, unsigned int palette);

HB_END_DECLS

#endif /* HB_OT_COLOR_H */
