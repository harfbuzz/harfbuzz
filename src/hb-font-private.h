/*
 * Copyright (C) 2009  Red Hat, Inc.
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

#ifndef HB_FONT_PRIVATE_H
#define HB_FONT_PRIVATE_H

#include "hb-private.h"

#include "hb-font.h"

#include "hb-unicode-private.h"
#include "hb-ot-layout-private.h"

HB_BEGIN_DECLS

/*
 * hb_font_funcs_t
 */

struct _hb_font_funcs_t {
  hb_reference_count_t ref_count;

  hb_bool_t immutable;

  hb_font_get_glyph_func_t glyph_func;
  hb_font_get_contour_point_func_t contour_point_func;
  hb_font_get_glyph_metrics_func_t glyph_metrics_func;
  hb_font_get_kerning_func_t kerning_func;
};

HB_INTERNAL hb_font_funcs_t
_hb_font_funcs_nil;

/*
 * hb_face_t
 */

struct _hb_face_t {
  hb_reference_count_t ref_count;

  hb_blob_t *blob;
  unsigned int  index;

  hb_get_table_func_t  get_table;
  hb_destroy_func_t    destroy;
  void                *user_data;

  hb_unicode_funcs_t *unicode;

  hb_ot_layout_t ot_layout;
};


/*
 * hb_font_t
 */

struct _hb_font_t {
  hb_reference_count_t ref_count;

  hb_16dot16_t x_scale;
  hb_16dot16_t y_scale;

  unsigned int x_ppem;
  unsigned int y_ppem;

  hb_font_funcs_t *klass;
};


HB_END_DECLS

#endif /* HB_FONT_PRIVATE_H */
