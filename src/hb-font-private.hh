/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_FONT_PRIVATE_HH
#define HB_FONT_PRIVATE_HH

#include "hb-private.hh"

#include "hb-font.h"
#include "hb-object-private.hh"

HB_BEGIN_DECLS


/*
 * hb_font_funcs_t
 */

struct _hb_font_funcs_t {
  hb_object_header_t header;

  hb_bool_t immutable;

  struct {
    hb_font_get_glyph_func_t		get_glyph;
    hb_font_get_glyph_advance_func_t	get_glyph_advance;
    hb_font_get_glyph_extents_func_t	get_glyph_extents;
    hb_font_get_contour_point_func_t	get_contour_point;
    hb_font_get_kerning_func_t		get_kerning;
  } v;
};

extern HB_INTERNAL hb_font_funcs_t _hb_font_funcs_nil;


/*
 * hb_face_t
 */

struct _hb_face_t {
  hb_object_header_t header;

  hb_get_table_func_t  get_table;
  void                *user_data;
  hb_destroy_func_t    destroy;

  hb_blob_t *head_blob;
  const struct head *head_table;

  struct hb_ot_layout_t *ot_layout;
};


/*
 * hb_font_t
 */

struct _hb_font_t {
  hb_object_header_t header;

  hb_face_t *face;

  unsigned int x_scale;
  unsigned int y_scale;

  unsigned int x_ppem;
  unsigned int y_ppem;

  hb_font_funcs_t   *klass;
  void              *user_data;
  hb_destroy_func_t  destroy;
};


HB_END_DECLS

#endif /* HB_FONT_PRIVATE_HH */
