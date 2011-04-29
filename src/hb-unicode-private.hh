/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2011  Codethink Limited
 * Copyright © 2010,2011  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_UNICODE_PRIVATE_HH
#define HB_UNICODE_PRIVATE_HH

#include "hb-private.hh"

#include "hb-unicode.h"
#include "hb-object-private.hh"

HB_BEGIN_DECLS


/*
 * hb_unicode_funcs_t
 */

struct _hb_unicode_funcs_t {
  hb_object_header_t header;

  hb_unicode_funcs_t *parent;

  bool immutable;

#define IMPLEMENT(return_type, name) \
  inline return_type \
  get_##name (hb_codepoint_t unicode) \
  { return this->get.name (this, unicode, this->user_data.name); }

  IMPLEMENT (unsigned int, combining_class)
  IMPLEMENT (unsigned int, eastasian_width)
  IMPLEMENT (hb_unicode_general_category_t, general_category)
  IMPLEMENT (hb_codepoint_t, mirroring)
  IMPLEMENT (hb_script_t, script)

#undef IMPLEMENT

  /* Don't access these directly.  Call get_*() instead. */

  struct {
    hb_unicode_get_combining_class_func_t	combining_class;
    hb_unicode_get_eastasian_width_func_t	eastasian_width;
    hb_unicode_get_general_category_func_t	general_category;
    hb_unicode_get_mirroring_func_t		mirroring;
    hb_unicode_get_script_func_t		script;
  } get;

  struct {
    void 					*combining_class;
    void 					*eastasian_width;
    void 					*general_category;
    void 					*mirroring;
    void 					*script;
  } user_data;

  struct {
    hb_destroy_func_t				combining_class;
    hb_destroy_func_t				eastasian_width;
    hb_destroy_func_t				general_category;
    hb_destroy_func_t				mirroring;
    hb_destroy_func_t				script;
  } destroy;
};


#if HAVE_GLIB
extern HB_INTERNAL hb_unicode_funcs_t _hb_glib_unicode_funcs;
#define _hb_unicode_funcs_default _hb_glib_unicode_funcs
#elif HAVE_ICU
extern HB_INTERNAL hb_unicode_funcs_t _hb_icu_unicode_funcs;
#define _hb_unicode_funcs_default _hb_icu_unicode_funcs
#else
extern HB_INTERNAL hb_unicode_funcs_t _hb_unicode_funcs_nil;
#define _hb_unicode_funcs_default _hb_unicode_funcs_nil
#endif



HB_END_DECLS

#endif /* HB_UNICODE_PRIVATE_HH */
