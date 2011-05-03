/*
 * Copyright © 2009  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#ifndef HB_FONT_H
#define HB_FONT_H

#include "hb-common.h"
#include "hb-blob.h"

HB_BEGIN_DECLS


typedef struct _hb_face_t hb_face_t;
typedef struct _hb_font_t hb_font_t;

/*
 * hb_face_t
 */

hb_face_t *
hb_face_create_for_data (hb_blob_t    *blob,
			 unsigned int  index);

typedef hb_blob_t * (*hb_get_table_func_t)  (hb_tag_t tag, void *user_data);

/* calls destroy() when not needing user_data anymore */
hb_face_t *
hb_face_create_for_tables (hb_get_table_func_t  get_table,
			   void                *user_data,
			   hb_destroy_func_t    destroy);

hb_face_t *
hb_face_reference (hb_face_t *face);

void
hb_face_destroy (hb_face_t *face);

hb_bool_t
hb_face_set_user_data (hb_face_t          *face,
		       hb_user_data_key_t *key,
		       void *              data,
		       hb_destroy_func_t   destroy);


void *
hb_face_get_user_data (hb_face_t          *face,
		       hb_user_data_key_t *key);


hb_blob_t *
hb_face_reference_table (hb_face_t *face,
			 hb_tag_t   tag);

unsigned int
hb_face_get_upem (hb_face_t *face);


/*
 * hb_font_funcs_t
 */

typedef struct _hb_font_funcs_t hb_font_funcs_t;

hb_font_funcs_t *
hb_font_funcs_create (void);

hb_font_funcs_t *
hb_font_funcs_reference (hb_font_funcs_t *ffuncs);

void
hb_font_funcs_destroy (hb_font_funcs_t *ffuncs);

hb_bool_t
hb_font_funcs_set_user_data (hb_font_funcs_t    *ffuncs,
			     hb_user_data_key_t *key,
			     void *              data,
			     hb_destroy_func_t   destroy);


void *
hb_font_funcs_get_user_data (hb_font_funcs_t    *ffuncs,
			     hb_user_data_key_t *key);


void
hb_font_funcs_make_immutable (hb_font_funcs_t *ffuncs);

hb_bool_t
hb_font_funcs_is_immutable (hb_font_funcs_t *ffuncs);

/* funcs */

typedef struct _hb_glyph_extents_t
{
    hb_position_t x_bearing;
    hb_position_t y_bearing;
    hb_position_t width;
    hb_position_t height;
} hb_glyph_extents_t;

typedef hb_codepoint_t (*hb_font_get_glyph_func_t) (hb_font_t *font, const void *user_data,
						    hb_codepoint_t unicode, hb_codepoint_t variation_selector);
typedef void (*hb_font_get_glyph_advance_func_t) (hb_font_t *font, const void *user_data,
						  hb_codepoint_t glyph,
						  hb_position_t *x_advance, hb_position_t *y_advance);
typedef void (*hb_font_get_glyph_extents_func_t) (hb_font_t *font, const void *user_data,
						  hb_codepoint_t glyph,
						  hb_glyph_extents_t *extents);
typedef hb_bool_t (*hb_font_get_contour_point_func_t) (hb_font_t *font, const void *user_data,
						       unsigned int point_index, hb_codepoint_t glyph,
						       hb_position_t *x, hb_position_t *y);
typedef hb_position_t (*hb_font_get_kerning_func_t) (hb_font_t *font, const void *user_data,
						     hb_codepoint_t first_glyph, hb_codepoint_t second_glyph);


void
hb_font_funcs_set_glyph_func (hb_font_funcs_t *ffuncs,
			      hb_font_get_glyph_func_t glyph_func);

void
hb_font_funcs_set_glyph_advance_func (hb_font_funcs_t *ffuncs,
				      hb_font_get_glyph_advance_func_t glyph_advance_func);

void
hb_font_funcs_set_glyph_extents_func (hb_font_funcs_t *ffuncs,
				      hb_font_get_glyph_extents_func_t glyph_extents_func);

void
hb_font_funcs_set_contour_point_func (hb_font_funcs_t *ffuncs,
				      hb_font_get_contour_point_func_t contour_point_func);

void
hb_font_funcs_set_kerning_func (hb_font_funcs_t *ffuncs,
				hb_font_get_kerning_func_t kerning_func);


/* These never return NULL.  Return fallback defaults instead. */

hb_font_get_glyph_func_t
hb_font_funcs_get_glyph_func (hb_font_funcs_t *ffuncs);

hb_font_get_glyph_advance_func_t
hb_font_funcs_get_glyph_advance_func (hb_font_funcs_t *ffuncs);

hb_font_get_glyph_extents_func_t
hb_font_funcs_get_glyph_extents_func (hb_font_funcs_t *ffuncs);

hb_font_get_contour_point_func_t
hb_font_funcs_get_contour_point_func (hb_font_funcs_t *ffuncs);

hb_font_get_kerning_func_t
hb_font_funcs_get_kerning_func (hb_font_funcs_t *ffuncs);


hb_codepoint_t
hb_font_get_glyph (hb_font_t *font,
		   hb_codepoint_t unicode, hb_codepoint_t variation_selector);

void
hb_font_get_glyph_advance (hb_font_t *font,
			   hb_codepoint_t glyph,
			   hb_position_t *x_advance, hb_position_t *y_advance);

void
hb_font_get_glyph_extents (hb_font_t *font,
			   hb_codepoint_t glyph,
			   hb_glyph_extents_t *extents);

hb_bool_t
hb_font_get_contour_point (hb_font_t *font,
			   unsigned int point_index, hb_codepoint_t glyph,
			   hb_position_t *x, hb_position_t *y);

hb_position_t
hb_font_get_kerning (hb_font_t *font,
		     hb_codepoint_t first_glyph, hb_codepoint_t second_glyph);


/*
 * hb_font_t
 */

/* Fonts are very light-weight objects */

hb_font_t *
hb_font_create (hb_face_t *face);

hb_font_t *
hb_font_reference (hb_font_t *font);

void
hb_font_destroy (hb_font_t *font);

hb_bool_t
hb_font_set_user_data (hb_font_t          *font,
		       hb_user_data_key_t *key,
		       void *              data,
		       hb_destroy_func_t   destroy);


void *
hb_font_get_user_data (hb_font_t          *font,
		       hb_user_data_key_t *key);


hb_face_t *
hb_font_get_face (hb_font_t *font);


void
hb_font_set_funcs (hb_font_t         *font,
		   hb_font_funcs_t   *klass,
		   void              *user_data,
		   hb_destroy_func_t  destroy);

/* Returns what was set and unsets it, but doesn't destroy(user_data).
 * This is useful for wrapping / chaining font_funcs_t's.
 *
 * The client is responsible for:
 *
 *   - Take ownership of the reference on the returned klass,
 *
 *   - Calling "destroy(user_data)" exactly once if returned destroy func
 *     is not NULL and the returned info is not needed anymore.
 */
void
hb_font_unset_funcs (hb_font_t          *font,
		     hb_font_funcs_t   **klass,
		     void              **user_data,
		     hb_destroy_func_t  *destroy);


/*
 * We should add support for full matrices.
 */
void
hb_font_set_scale (hb_font_t *font,
		   int x_scale,
		   int y_scale);

void
hb_font_get_scale (hb_font_t *font,
		   int *x_scale,
		   int *y_scale);

/*
 * A zero value means "no hinting in that direction"
 */
void
hb_font_set_ppem (hb_font_t *font,
		  unsigned int x_ppem,
		  unsigned int y_ppem);

void
hb_font_get_ppem (hb_font_t *font,
		  unsigned int *x_ppem,
		  unsigned int *y_ppem);


HB_END_DECLS

#endif /* HB_FONT_H */
