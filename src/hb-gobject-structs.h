/*
 * Copyright (C) 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_GOBJECT_H_IN
#error "Include <hb-gobject.h> instead."
#endif

#ifndef HB_GOBJECT_STRUCTS_H
#define HB_GOBJECT_STRUCTS_H

#include "hb.h"

#include <glib-object.h>

HB_BEGIN_DECLS


/* Object types */

/**
 * hb_gobject_blob_get_type:
 *
 * Since: 0.9.2
 **/
HB_EXTERN GType
hb_blob_get_gtype (void);
#define HB_GOBJECT_TYPE_BLOB (hb_blob_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_blob_get_gtype) GType
hb_gobject_blob_get_type (void);

/**
 * hb_gobject_buffer_get_type:
 *
 * Since: 0.9.2
 **/
HB_EXTERN GType
hb_buffer_get_gtype (void);
#define HB_GOBJECT_TYPE_BUFFER (hb_buffer_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_buffer_get_gtype) GType
hb_gobject_buffer_get_type (void);

/**
 * hb_gobject_face_get_type:
 *
 * Since: 0.9.2
 **/
HB_EXTERN GType
hb_face_get_gtype (void);
#define HB_GOBJECT_TYPE_FACE (hb_face_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_face_get_gtype) GType
hb_gobject_face_get_type (void);

/**
 * hb_gobject_font_get_type:
 *
 * Since: 0.9.2
 **/
HB_EXTERN GType
hb_font_get_gtype (void);
#define HB_GOBJECT_TYPE_FONT (hb_font_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_font_get_gtype) GType
hb_gobject_font_get_type (void);

/**
 * hb_gobject_font_funcs_get_type:
 *
 * Since: 0.9.2
 **/
HB_EXTERN GType
hb_font_funcs_get_gtype (void);
#define HB_GOBJECT_TYPE_FONT_FUNCS (hb_font_funcs_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_font_funcs_get_gtype) GType
hb_gobject_font_funcs_get_type (void);

HB_EXTERN GType
hb_set_get_gtype (void);
#define HB_GOBJECT_TYPE_SET (hb_set_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_set_get_gtype) GType
hb_gobject_set_get_type (void);

HB_EXTERN GType
hb_map_get_gtype (void);
#define HB_GOBJECT_TYPE_MAP (hb_map_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_map_get_gtype) GType
hb_gobject_map_get_type (void);

HB_EXTERN GType
hb_shape_plan_get_gtype (void);
#define HB_GOBJECT_TYPE_SHAPE_PLAN (hb_shape_plan_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_shape_plan_get_gtype) GType
hb_gobject_shape_plan_get_type (void);

/**
 * hb_gobject_unicode_funcs_get_type:
 *
 * Since: 0.9.2
 **/
HB_EXTERN GType
hb_unicode_funcs_get_gtype (void);
#define HB_GOBJECT_TYPE_UNICODE_FUNCS (hb_unicode_funcs_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_unicode_funcs_get_gtype) GType
hb_gobject_unicode_funcs_get_type (void);

/* Value types */

HB_EXTERN GType
hb_feature_get_gtype (void);
#define HB_GOBJECT_TYPE_FEATURE (hb_feature_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_feature_get_gtype) GType
hb_gobject_feature_get_type (void);

HB_EXTERN GType
hb_glyph_info_get_gtype (void);
#define HB_GOBJECT_TYPE_GLYPH_INFO (hb_glyph_info_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_glyph_info_get_gtype) GType
hb_gobject_glyph_info_get_type (void);

HB_EXTERN GType
hb_glyph_position_get_gtype (void);
#define HB_GOBJECT_TYPE_GLYPH_POSITION (hb_glyph_position_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_glyph_position_get_gtype) GType
hb_gobject_glyph_position_get_type (void);

HB_EXTERN GType
hb_segment_properties_get_gtype (void);
#define HB_GOBJECT_TYPE_SEGMENT_PROPERTIES (hb_segment_properties_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_segment_properties_get_gtype) GType
hb_gobject_segment_properties_get_type (void);

HB_EXTERN GType
hb_user_data_key_get_gtype (void);
#define HB_GOBJECT_TYPE_USER_DATA_KEY (hb_user_data_key_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_user_data_key_get_gtype) GType
hb_gobject_user_data_key_get_type (void);

HB_EXTERN GType
hb_ot_math_glyph_variant_get_gtype (void);
#define HB_GOBJECT_TYPE_OT_MATH_GLYPH_VARIANT (hb_ot_math_glyph_variant_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_ot_math_glyph_variant_get_gtype) GType
hb_gobject_ot_math_glyph_variant_get_type (void);

HB_EXTERN GType
hb_ot_math_glyph_part_get_gtype (void);
#define HB_GOBJECT_TYPE_OT_MATH_GLYPH_PART (hb_ot_math_glyph_part_get_gtype ())

HB_EXTERN HB_DEPRECATED_FOR (hb_ot_math_glyph_part_get_gtype) GType
hb_gobject_ot_math_glyph_part_get_type (void);

HB_END_DECLS

#endif /* HB_GOBJECT_H */
