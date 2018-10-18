/*
 * Copyright © 2013  Google, Inc.
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

#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_DEPRECATED_H
#define HB_DEPRECATED_H

#include "hb-common.h"
#include "hb-unicode.h"
#include "hb-font.h"
#include "hb-set.h"

HB_BEGIN_DECLS

#ifndef HB_DISABLE_DEPRECATED

#define HB_SCRIPT_CANADIAN_ABORIGINAL		HB_SCRIPT_CANADIAN_SYLLABICS

#define HB_BUFFER_FLAGS_DEFAULT			HB_BUFFER_FLAG_DEFAULT
#define HB_BUFFER_SERIALIZE_FLAGS_DEFAULT	HB_BUFFER_SERIALIZE_FLAG_DEFAULT

typedef hb_bool_t (*hb_font_get_glyph_func_t) (hb_font_t *font, void *font_data,
					       hb_codepoint_t unicode, hb_codepoint_t variation_selector,
					       hb_codepoint_t *glyph,
					       void *user_data);

HB_EXTERN HB_DEPRECATED_FOR(hb_font_funcs_set_nominal_glyph_func or hb_font_funcs_set_variation_glyph_func) void
hb_font_funcs_set_glyph_func (hb_font_funcs_t *ffuncs,
			      hb_font_get_glyph_func_t func,
			      void *user_data, hb_destroy_func_t destroy);

HB_EXTERN HB_DEPRECATED void
hb_set_invert (hb_set_t *set);

/**
 * hb_unicode_eastasian_width_func_t:
 *
 * Deprecated: 2.0.0
 */
typedef unsigned int			(*hb_unicode_eastasian_width_func_t)	(hb_unicode_funcs_t *ufuncs,
										 hb_codepoint_t      unicode,
										 void               *user_data);

/**
 * hb_unicode_funcs_set_eastasian_width_func:
 * @ufuncs: a Unicode function structure
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy:
 *
 * 
 *
 * Since: 0.9.2
 * Deprecated: 2.0.0
 **/
HB_EXTERN HB_DEPRECATED void
hb_unicode_funcs_set_eastasian_width_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_eastasian_width_func_t func,
					   void *user_data, hb_destroy_func_t destroy);

/**
 * hb_unicode_eastasian_width:
 *
 * Since: 0.9.2
 * Deprecated: 2.0.0
 **/
HB_EXTERN HB_DEPRECATED unsigned int
hb_unicode_eastasian_width (hb_unicode_funcs_t *ufuncs,
			    hb_codepoint_t unicode);


/**
 * hb_unicode_decompose_compatibility_func_t:
 * @ufuncs: a Unicode function structure
 * @u: codepoint to decompose
 * @decomposed: address of codepoint array (of length %HB_UNICODE_MAX_DECOMPOSITION_LEN) to write decomposition into
 * @user_data: user data pointer as passed to hb_unicode_funcs_set_decompose_compatibility_func()
 *
 * Fully decompose @u to its Unicode compatibility decomposition. The codepoints of the decomposition will be written to @decomposed.
 * The complete length of the decomposition will be returned.
 *
 * If @u has no compatibility decomposition, zero should be returned.
 *
 * The Unicode standard guarantees that a buffer of length %HB_UNICODE_MAX_DECOMPOSITION_LEN codepoints will always be sufficient for any
 * compatibility decomposition plus an terminating value of 0.  Consequently, @decompose must be allocated by the caller to be at least this length.  Implementations
 * of this function type must ensure that they do not write past the provided array.
 *
 * Return value: number of codepoints in the full compatibility decomposition of @u, or 0 if no decomposition available.
 *
 * Deprecated: 2.0.0
 */
typedef unsigned int			(*hb_unicode_decompose_compatibility_func_t)	(hb_unicode_funcs_t *ufuncs,
											 hb_codepoint_t      u,
											 hb_codepoint_t     *decomposed,
											 void               *user_data);

/**
 * HB_UNICODE_MAX_DECOMPOSITION_LEN:
 *
 * See Unicode 6.1 for details on the maximum decomposition length.
 *
 * Deprecated: 2.0.0
 */
#define HB_UNICODE_MAX_DECOMPOSITION_LEN (18+1) /* codepoints */

/**
 * hb_unicode_funcs_set_decompose_compatibility_func:
 * @ufuncs: a Unicode function structure
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy:
 *
 * 
 *
 * Since: 0.9.2
 * Deprecated: 2.0.0
 **/
HB_EXTERN HB_DEPRECATED void
hb_unicode_funcs_set_decompose_compatibility_func (hb_unicode_funcs_t *ufuncs,
						   hb_unicode_decompose_compatibility_func_t func,
						   void *user_data, hb_destroy_func_t destroy);

/**
 * hb_unicode_decompose_compatibility:
 * 
 *
 * Deprecated: 2.0.0
 **/
HB_EXTERN HB_DEPRECATED unsigned int
hb_unicode_decompose_compatibility (hb_unicode_funcs_t *ufuncs,
				    hb_codepoint_t      u,
				    hb_codepoint_t     *decomposed);


typedef hb_position_t (*hb_font_get_glyph_kerning_func_t) (hb_font_t *font, void *font_data,
							   hb_codepoint_t first_glyph, hb_codepoint_t second_glyph,
							   void *user_data);
typedef hb_font_get_glyph_kerning_func_t hb_font_get_glyph_h_kerning_func_t;
typedef hb_font_get_glyph_kerning_func_t hb_font_get_glyph_v_kerning_func_t;

/**
 * hb_font_funcs_set_glyph_h_kerning_func:
 * @ffuncs: font functions.
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy:
 *
 * 
 *
 * Since: 0.9.2
 * Deprecated: 2.0.0
 **/
HB_EXTERN void
hb_font_funcs_set_glyph_h_kerning_func (hb_font_funcs_t *ffuncs,
					hb_font_get_glyph_h_kerning_func_t func,
					void *user_data, hb_destroy_func_t destroy);

/**
 * hb_font_funcs_set_glyph_v_kerning_func:
 * @ffuncs: font functions.
 * @func: (closure user_data) (destroy destroy) (scope notified):
 * @user_data:
 * @destroy:
 *
 * 
 *
 * Since: 0.9.2
 * Deprecated: 2.0.0
 **/
HB_EXTERN void
hb_font_funcs_set_glyph_v_kerning_func (hb_font_funcs_t *ffuncs,
					hb_font_get_glyph_v_kerning_func_t func,
					void *user_data, hb_destroy_func_t destroy);

HB_EXTERN hb_position_t
hb_font_get_glyph_h_kerning (hb_font_t *font,
			     hb_codepoint_t left_glyph, hb_codepoint_t right_glyph);
HB_EXTERN hb_position_t
hb_font_get_glyph_v_kerning (hb_font_t *font,
			     hb_codepoint_t top_glyph, hb_codepoint_t bottom_glyph);

HB_EXTERN void
hb_font_get_glyph_kerning_for_direction (hb_font_t *font,
					 hb_codepoint_t first_glyph, hb_codepoint_t second_glyph,
					 hb_direction_t direction,
					 hb_position_t *x, hb_position_t *y);

/* Like hb_ot_layout_table_find_script, but takes zero-terminated array of scripts to test */
HB_EXTERN HB_DEPRECATED_FOR (hb_ot_layout_table_select_script) hb_bool_t
hb_ot_layout_table_choose_script (hb_face_t      *face,
				  hb_tag_t        table_tag,
				  const hb_tag_t *script_tags,
				  unsigned int   *script_index,
				  hb_tag_t       *chosen_script);

HB_EXTERN HB_DEPRECATED_FOR (hb_ot_layout_script_select_language) hb_bool_t
hb_ot_layout_script_find_language (hb_face_t    *face,
				   hb_tag_t      table_tag,
				   unsigned int  script_index,
				   hb_tag_t      language_tag,
				   unsigned int *language_index);

HB_EXTERN HB_DEPRECATED_FOR (hb_ot_tags_from_script_and_language) void
hb_ot_tags_from_script (hb_script_t  script,
			hb_tag_t    *script_tag_1,
			hb_tag_t    *script_tag_2);

HB_EXTERN HB_DEPRECATED_FOR (hb_ot_tags_from_script_and_language) hb_tag_t
hb_ot_tag_from_language (hb_language_t language);


#endif

HB_END_DECLS

#endif /* HB_DEPRECATED_H */
