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

#include "hb-private.h"

#include "hb-unicode-private.h"

HB_BEGIN_DECLS


/*
 * hb_unicode_funcs_t
 */

static hb_codepoint_t
hb_unicode_get_mirroring_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
                              hb_codepoint_t      unicode   HB_UNUSED,
                              void               *user_data HB_UNUSED)
{
  return unicode;
}

static hb_unicode_general_category_t
hb_unicode_get_general_category_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
                                     hb_codepoint_t      unicode   HB_UNUSED,
                                     void               *user_data HB_UNUSED)
{
  return HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER;
}

static hb_script_t
hb_unicode_get_script_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
                           hb_codepoint_t      unicode   HB_UNUSED,
                           void               *user_data HB_UNUSED)
{
  return HB_SCRIPT_UNKNOWN;
}

static unsigned int
hb_unicode_get_combining_class_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
                                    hb_codepoint_t      unicode   HB_UNUSED,
                                    void               *user_data HB_UNUSED)
{
  return 0;
}

static unsigned int
hb_unicode_get_eastasian_width_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
                                    hb_codepoint_t      unicode   HB_UNUSED,
                                    void               *user_data HB_UNUSED)
{
  return 1;
}

hb_unicode_funcs_t _hb_unicode_funcs_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */
  NULL, /* parent */
  TRUE, /* immutable */
  {
    hb_unicode_get_general_category_nil, NULL, NULL,
    hb_unicode_get_combining_class_nil, NULL, NULL,
    hb_unicode_get_mirroring_nil, NULL, NULL,
    hb_unicode_get_script_nil, NULL, NULL,
    hb_unicode_get_eastasian_width_nil, NULL, NULL
  }
};

hb_unicode_funcs_t *
hb_unicode_funcs_create (hb_unicode_funcs_t *parent)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  if (parent != NULL) {
    ufuncs->parent = hb_unicode_funcs_reference (parent);
    hb_unicode_funcs_make_immutable (parent);
    ufuncs->v = parent->v;

    /* Clear out the destroy notifies from our parent.
     *
     * We don't want to destroy the user_data twice and since we hold a
     * reference on our parent then we know that the user_data will
     * survive for at least as long as we do anyway.
     */
    ufuncs->v.get_general_category_destroy = NULL;
    ufuncs->v.get_combining_class_destroy = NULL;
    ufuncs->v.get_mirroring_destroy = NULL;
    ufuncs->v.get_script_destroy = NULL;
    ufuncs->v.get_eastasian_width_destroy = NULL;
  } else {
    ufuncs->v = _hb_unicode_funcs_nil.v;
  }

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

  if (ufuncs->parent != NULL)
    hb_unicode_funcs_destroy (ufuncs->parent);

  if (ufuncs->v.get_general_category_destroy != NULL)
    ufuncs->v.get_general_category_destroy (ufuncs->v.get_general_category_data);

  if (ufuncs->v.get_combining_class_destroy != NULL)
    ufuncs->v.get_combining_class_destroy (ufuncs->v.get_combining_class_data);

  if (ufuncs->v.get_mirroring_destroy != NULL)
    ufuncs->v.get_mirroring_destroy (ufuncs->v.get_mirroring_data);

  if (ufuncs->v.get_script_destroy != NULL)
    ufuncs->v.get_script_destroy (ufuncs->v.get_script_data);

  if (ufuncs->v.get_eastasian_width_destroy != NULL)
    ufuncs->v.get_eastasian_width_destroy (ufuncs->v.get_eastasian_width_data);

  free (ufuncs);
}

hb_unicode_funcs_t *
hb_unicode_funcs_get_parent (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->parent;
}

void
hb_unicode_funcs_make_immutable (hb_unicode_funcs_t *ufuncs)
{
  if (HB_OBJECT_IS_INERT (ufuncs))
    return;

  ufuncs->immutable = TRUE;
}

hb_bool_t
hb_unicode_funcs_is_immutable (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->immutable;
}


#define SETTER(name) \
void                                                                           \
hb_unicode_funcs_set_##name##_func (hb_unicode_funcs_t             *ufuncs,    \
                                    hb_unicode_get_##name##_func_t  func,      \
                                    void                           *user_data, \
                                    hb_destroy_func_t               destroy)   \
{                                                                              \
  if (ufuncs->immutable)                                                       \
    return;                                                                    \
                                                                               \
  if (func != NULL) {                                                          \
    ufuncs->v.get_##name = func;                                               \
    ufuncs->v.get_##name##_data = user_data;                                   \
    ufuncs->v.get_##name##_destroy = destroy;                                  \
  } else if (ufuncs->parent != NULL) {                                         \
    ufuncs->v.get_##name = ufuncs->parent->v.get_##name;                       \
    ufuncs->v.get_##name##_data = ufuncs->parent->v.get_##name##_data;;        \
    ufuncs->v.get_##name##_destroy = NULL;                                     \
  } else {                                                                     \
    ufuncs->v.get_##name = hb_unicode_get_##name##_nil;                        \
    ufuncs->v.get_##name##_data = NULL;                                        \
    ufuncs->v.get_##name##_destroy = NULL;                                     \
  }                                                                            \
}

SETTER(mirroring)
SETTER(general_category)
SETTER(script)
SETTER(combining_class)
SETTER(eastasian_width)

hb_codepoint_t
hb_unicode_get_mirroring (hb_unicode_funcs_t *ufuncs,
			  hb_codepoint_t unicode)
{
  return ufuncs->v.get_mirroring (ufuncs, unicode,
				  ufuncs->v.get_mirroring_data);
}

hb_unicode_general_category_t
hb_unicode_get_general_category (hb_unicode_funcs_t *ufuncs,
				 hb_codepoint_t unicode)
{
  return ufuncs->v.get_general_category (ufuncs, unicode,
					 ufuncs->v.get_general_category_data);
}

hb_script_t
hb_unicode_get_script (hb_unicode_funcs_t *ufuncs,
		       hb_codepoint_t unicode)
{
  return ufuncs->v.get_script (ufuncs, unicode,
			       ufuncs->v.get_script_data);
}

unsigned int
hb_unicode_get_combining_class (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode)
{
  return ufuncs->v.get_combining_class (ufuncs, unicode,
					ufuncs->v.get_combining_class_data);
}

unsigned int
hb_unicode_get_eastasian_width (hb_unicode_funcs_t *ufuncs,
				hb_codepoint_t unicode)
{
  return ufuncs->v.get_eastasian_width (ufuncs, unicode,
					ufuncs->v.get_eastasian_width_data);
}


HB_END_DECLS
