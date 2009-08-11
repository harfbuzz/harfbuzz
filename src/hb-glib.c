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

#include "hb-private.h"

#include "hb-glib.h"

#include "hb-unicode-private.h"

static hb_unicode_funcs_t *glib_ufuncs;

static hb_codepoint_t hb_glib_get_mirroring_nil (hb_codepoint_t unicode) { g_unichar_get_mirror_char (unicode, &unicode); return unicode; }
static hb_category_t hb_glib_get_general_category_nil (hb_codepoint_t unicode) { return g_unichar_type (unicode); }
static hb_script_t hb_glib_get_script_nil (hb_codepoint_t unicode) { return g_unichar_get_script (unicode); }
static unsigned int hb_glib_get_combining_class_nil (hb_codepoint_t unicode) { return g_unichar_combining_class (unicode); }
static unsigned int hb_glib_get_eastasian_width_nil (hb_codepoint_t unicode) { return g_unichar_iswide (unicode); }


hb_unicode_funcs_t *
hb_glib_get_unicode_funcs (void)
{
  if (HB_UNLIKELY (!glib_ufuncs)) {
    glib_ufuncs = hb_unicode_funcs_create ();

    hb_unicode_funcs_set_mirroring_func (glib_ufuncs, hb_glib_get_mirroring_nil);
    hb_unicode_funcs_set_general_category_func (glib_ufuncs, hb_glib_get_general_category_nil);
    hb_unicode_funcs_set_script_func (glib_ufuncs, hb_glib_get_script_nil);
    hb_unicode_funcs_set_combining_class_func (glib_ufuncs, hb_glib_get_combining_class_nil);
    hb_unicode_funcs_set_eastasian_width_func (glib_ufuncs, hb_glib_get_eastasian_width_nil);

    hb_unicode_funcs_make_immutable (glib_ufuncs);
  }

  return hb_unicode_funcs_reference (glib_ufuncs);
}
