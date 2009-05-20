/*
 * Copyright (C) 2007,2008  Red Hat, Inc.
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
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

#ifndef HB_OT_LAYOUT_H
#define HB_OT_LAYOUT_H

#include "hb-common.h"
#include "harfbuzz-buffer.h"

HB_BEGIN_DECLS();

/*
 * hb_ot_layout_t
 */

typedef struct _hb_ot_layout_t hb_ot_layout_t;

hb_ot_layout_t *
hb_ot_layout_create (void);

hb_ot_layout_t *
hb_ot_layout_create_for_data (const char *font_data,
			      int         face_index);

void
hb_ot_layout_destroy (hb_ot_layout_t *layout);

/* XXX */
void
hb_ot_layout_set_direction (hb_ot_layout_t *layout,
			    hb_bool_t r2l);

void
hb_ot_layout_set_hinting (hb_ot_layout_t *layout,
			  hb_bool_t hinted);

void
hb_ot_layout_set_scale (hb_ot_layout_t *layout,
			hb_16dot16_t x_scale, hb_16dot16_t y_scale);

void
hb_ot_layout_set_ppem (hb_ot_layout_t *layout,
		       unsigned int x_ppem, unsigned int y_ppem);

/* TODO sanitizing API/constructor (make_writable_func_t) */
/* TODO get_table_func_t constructor */

/*
 * GDEF
 */

typedef enum {
  HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED	= 0x0000,
  HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH	= 0x0002,
  HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE	= 0x0004,
  HB_OT_LAYOUT_GLYPH_CLASS_MARK		= 0x0008,
  HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT	= 0x0010
} hb_ot_layout_glyph_class_t;

hb_bool_t
hb_ot_layout_has_font_glyph_classes (hb_ot_layout_t *layout);

hb_ot_layout_glyph_class_t
hb_ot_layout_get_glyph_class (hb_ot_layout_t *layout,
			      hb_codepoint_t  glyph);

void
hb_ot_layout_set_glyph_class (hb_ot_layout_t            *layout,
			      hb_codepoint_t             glyph,
			      hb_ot_layout_glyph_class_t klass);

void
hb_ot_layout_build_glyph_classes (hb_ot_layout_t *layout,
				  uint16_t        num_total_glyphs,
				  hb_codepoint_t *glyphs,
				  unsigned char  *klasses,
				  uint16_t        count);

/*
 * GSUB/GPOS
 */

typedef enum {
  HB_OT_LAYOUT_TABLE_TYPE_GSUB,
  HB_OT_LAYOUT_TABLE_TYPE_GPOS,
  HB_OT_LAYOUT_TABLE_TYPE_NONE
} hb_ot_layout_table_type_t;

typedef uint32_t hb_ot_layout_feature_mask_t;

#define HB_OT_LAYOUT_NO_SCRIPT_INDEX		((unsigned int) 0xFFFF)
#define HB_OT_LAYOUT_NO_FEATURE_INDEX		((unsigned int) 0xFFFF)
#define HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX	((unsigned int) 0xFFFF)
#define HB_OT_LAYOUT_TAG_DEFAULT_SCRIPT		HB_TAG ('D', 'F', 'L', 'T')
#define HB_OT_LAYOUT_TAG_DEFAULT_LANGUAGE	HB_TAG ('d', 'f', 'l', 't')

unsigned int
hb_ot_layout_table_get_script_count (hb_ot_layout_t            *layout,
				     hb_ot_layout_table_type_t  table_type);

hb_tag_t
hb_ot_layout_table_get_script_tag (hb_ot_layout_t            *layout,
				   hb_ot_layout_table_type_t  table_type,
				   unsigned int               script_index);

hb_bool_t
hb_ot_layout_table_find_script (hb_ot_layout_t            *layout,
				hb_ot_layout_table_type_t  table_type,
				hb_tag_t                   script_tag,
				unsigned int              *script_index);

unsigned int
hb_ot_layout_table_get_feature_count (hb_ot_layout_t            *layout,
				      hb_ot_layout_table_type_t  table_type);

hb_tag_t
hb_ot_layout_table_get_feature_tag (hb_ot_layout_t            *layout,
				    hb_ot_layout_table_type_t  table_type,
				    unsigned int               feature_index);

hb_bool_t
hb_ot_layout_table_find_feature (hb_ot_layout_t            *layout,
				 hb_ot_layout_table_type_t  table_type,
				 hb_tag_t                   feature_tag,
				 unsigned int              *feature_index);

unsigned int
hb_ot_layout_table_get_lookup_count (hb_ot_layout_t            *layout,
				     hb_ot_layout_table_type_t  table_type);

unsigned int
hb_ot_layout_script_get_language_count (hb_ot_layout_t            *layout,
					hb_ot_layout_table_type_t  table_type,
					unsigned int               script_index);

hb_tag_t
hb_ot_layout_script_get_language_tag (hb_ot_layout_t            *layout,
				      hb_ot_layout_table_type_t  table_type,
				      unsigned int               script_index,
				      unsigned int               language_index);

hb_bool_t
hb_ot_layout_script_find_language (hb_ot_layout_t            *layout,
				   hb_ot_layout_table_type_t  table_type,
				   unsigned int               script_index,
				   hb_tag_t                   language_tag,
				   unsigned int              *language_index);

hb_bool_t
hb_ot_layout_language_get_required_feature_index (hb_ot_layout_t            *layout,
						  hb_ot_layout_table_type_t  table_type,
						  unsigned int               script_index,
						  unsigned int               language_index,
						  unsigned int              *feature_index);

unsigned int
hb_ot_layout_language_get_feature_count (hb_ot_layout_t            *layout,
					 hb_ot_layout_table_type_t  table_type,
					 unsigned int               script_index,
					 unsigned int               language_index);

unsigned int
hb_ot_layout_language_get_feature_index (hb_ot_layout_t            *layout,
				         hb_ot_layout_table_type_t  table_type,
				         unsigned int               script_index,
				         unsigned int               language_index,
				         unsigned int               num_feature);

hb_tag_t
hb_ot_layout_language_get_feature_tag (hb_ot_layout_t            *layout,
				       hb_ot_layout_table_type_t  table_type,
				       unsigned int               script_index,
				       unsigned int               language_index,
				       unsigned int               num_feature);

hb_bool_t
hb_ot_layout_language_find_feature (hb_ot_layout_t            *layout,
				    hb_ot_layout_table_type_t  table_type,
				    unsigned int               script_index,
				    unsigned int               language_index,
				    hb_tag_t                   feature_tag,
				    unsigned int              *feature_index);

unsigned int
hb_ot_layout_feature_get_lookup_count (hb_ot_layout_t            *layout,
				       hb_ot_layout_table_type_t  table_type,
				       unsigned int               feature_index);

unsigned int
hb_ot_layout_feature_get_lookup_index (hb_ot_layout_t            *layout,
				       hb_ot_layout_table_type_t  table_type,
				       unsigned int               feature_index,
				       unsigned int               num_lookup);

/*
 * GSUB
 */

hb_bool_t
hb_ot_layout_substitute_lookup (hb_ot_layout_t              *layout,
				hb_buffer_t                 *buffer,
			        unsigned int                 lookup_index,
				hb_ot_layout_feature_mask_t  mask);

hb_bool_t
hb_ot_layout_position_lookup   (hb_ot_layout_t              *layout,
				hb_buffer_t                 *buffer,
			        unsigned int                 lookup_index,
				hb_ot_layout_feature_mask_t  mask);










/*
#define PANGO_OT_ALL_GLYPHS			((guint) 0xFFFF)

*/

HB_END_DECLS();

#endif /* HB_OT_LAYOUT_H */
