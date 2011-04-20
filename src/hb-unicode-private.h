/*
 * Copyright (C) 2009  Red Hat, Inc.
 * Copyright © 2011 Codethink Limited
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
 * Codethink Author(s): Ryan Lortie
 */

#ifndef HB_UNICODE_PRIVATE_H
#define HB_UNICODE_PRIVATE_H

#include "hb-private.h"

#include "hb-unicode.h"

HB_BEGIN_DECLS


/*
 * hb_unicode_funcs_t
 */

struct _hb_unicode_funcs_t {
  hb_reference_count_t ref_count;
  hb_unicode_funcs_t *parent;

  hb_bool_t immutable;

  struct {
    hb_unicode_get_general_category_func_t	get_general_category;
    void                                       *get_general_category_data;
    hb_destroy_func_t                           get_general_category_destroy;

    hb_unicode_get_combining_class_func_t	get_combining_class;
    void                                       *get_combining_class_data;
    hb_destroy_func_t                           get_combining_class_destroy;

    hb_unicode_get_mirroring_func_t		get_mirroring;
    void                                       *get_mirroring_data;
    hb_destroy_func_t                           get_mirroring_destroy;

    hb_unicode_get_script_func_t		get_script;
    void                                       *get_script_data;
    hb_destroy_func_t                           get_script_destroy;

    hb_unicode_get_eastasian_width_func_t	get_eastasian_width;
    void                                       *get_eastasian_width_data;
    hb_destroy_func_t                           get_eastasian_width_destroy;
  } v;
};

extern HB_INTERNAL hb_unicode_funcs_t _hb_unicode_funcs_nil;


HB_END_DECLS

#endif /* HB_UNICODE_PRIVATE_H */
