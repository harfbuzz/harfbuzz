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

#include "hb-unicode-private.h"

/*
 * hb_unicode_funcs_t
 */

static hb_unicode_funcs_t _hb_unicode_funcs_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */

  NULL, /*get_general_category */
  NULL, /*get_combining_class */
  NULL, /*get_mirroring_char */
  NULL, /*get_script */
  NULL  /*get_eastasian_width */
};

hb_unicode_funcs_t *
hb_unicode_funcs_create (void)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  return ufuncs;
}

hb_unicode_funcs_t *
hb_unicode_funcs_reference (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_REFERENCE (ufuncs);
}

unsigned int
hb_unicode_funcs_get_reference_count (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_GET_REFERENCE_COUNT (ufuncs);
}

void
hb_unicode_funcs_destroy (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_DESTROY (ufuncs);

  free (ufuncs);
}

hb_unicode_funcs_t *
hb_unicode_funcs_copy (hb_unicode_funcs_t *other_ufuncs)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  *ufuncs = *other_ufuncs;
  HB_OBJECT_DO_INIT (ufuncs);

  return ufuncs;
}


void
hb_unicode_funcs_set_mirroring_char_func (hb_unicode_funcs_t *ufuncs,
					  hb_unicode_get_mirroring_char_func_t mirroring_char_func)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->get_mirroring_char = mirroring_char_func;
}

void
hb_unicode_funcs_set_general_category_func (hb_unicode_funcs_t *ufuncs,
					    hb_unicode_get_general_category_func_t general_category_func)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->get_general_category = general_category_func;
}

void
hb_unicode_funcs_set_script_func (hb_unicode_funcs_t *ufuncs,
				  hb_unicode_get_script_func_t script_func)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->get_script = script_func;
}

void
hb_unicode_funcs_set_combining_class_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_combining_class_func_t combining_class_func)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->get_combining_class = combining_class_func;
}

void
hb_unicode_funcs_set_eastasian_width_func (hb_unicode_funcs_t *ufuncs,
					   hb_unicode_get_eastasian_width_func_t eastasian_width_func)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->get_eastasian_width = eastasian_width_func;
}

