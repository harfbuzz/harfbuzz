/*
 * Copyright © 2016  Google, Inc.
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

HB_BEGIN_DECLS

/*
 * CPAL -- Color Palette
 * https://docs.microsoft.com/en-us/typography/opentype/spec/cpal
 */
#define HB_OT_TAG_CPAL HB_TAG('C','P','A','L')

/**
 * hb_ot_color_t:
 * ARGB data type for holding color values.
 *
 * Since: REPLACEME
 */
typedef uint32_t hb_ot_color_t;

#define hb_ot_color_get_alpha(color) (color & 0xFF)
#define hb_ot_color_get_red(color)   ((color >> 8) & 0xFF)
#define hb_ot_color_get_green(color) ((color >> 16) & 0xFF)
#define hb_ot_color_get_blue(color)  ((color >> 24) & 0xFF)

HB_EXTERN hb_bool_t
hb_ot_color_has_cpal_data (hb_face_t *face);

HB_EXTERN hb_bool_t
hb_ot_color_has_colr_data (hb_face_t *face);

HB_EXTERN unsigned int
hb_ot_color_get_palette_count (hb_face_t *face);

// HB_EXTERN unsigned int
// hb_ot_color_get_palette_name_id (hb_face_t *face, unsigned int palette);

// HB_EXTERN hb_ot_color_palette_flags_t
// hb_ot_color_get_palette_flags (hb_face_t *face, unsigned int palette);

HB_EXTERN unsigned int
hb_ot_color_get_palette_colors (hb_face_t      *face,
				unsigned int    palette, /* default=0 */
				unsigned int    start_offset,
				unsigned int   *color_count /* IN/OUT */,
				hb_ot_color_t  *colors /* OUT */);


HB_EXTERN unsigned int
hb_ot_color_get_color_layers (hb_face_t       *face,
			      hb_codepoint_t   gid,
			      unsigned int     offset,
			      unsigned int    *count, /* IN/OUT */
			      hb_codepoint_t  *gids, /* OUT */
			      unsigned int    *color_indices /* OUT */);

HB_END_DECLS

#endif /* HB_OT_COLOR_H */
