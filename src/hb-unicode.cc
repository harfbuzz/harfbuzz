/*
 * Copyright (C) 2009  Red Hat, Inc.
 * Copyright © 2011 Codethink Limited
 * Copyright (C) 2010  Google, Inc.
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

#include "hb-private.h"

#include "hb-unicode-private.hh"

HB_BEGIN_DECLS


/*
 * hb_unicode_funcs_t
 */

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

static hb_unicode_general_category_t
hb_unicode_get_general_category_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
				     hb_codepoint_t      unicode   HB_UNUSED,
				     void               *user_data HB_UNUSED)
{
  return HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER;
}

static hb_codepoint_t
hb_unicode_get_mirroring_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
			      hb_codepoint_t      unicode   HB_UNUSED,
			      void               *user_data HB_UNUSED)
{
  return unicode;
}

static hb_script_t
hb_unicode_get_script_nil (hb_unicode_funcs_t *ufuncs    HB_UNUSED,
			   hb_codepoint_t      unicode   HB_UNUSED,
			   void               *user_data HB_UNUSED)
{
  return HB_SCRIPT_UNKNOWN;
}


hb_unicode_funcs_t _hb_unicode_funcs_nil = {
  HB_REFERENCE_COUNT_INVALID, /* ref_count */
  NULL, /* parent */
  TRUE, /* immutable */
  {
    hb_unicode_get_combining_class_nil,
    hb_unicode_get_eastasian_width_nil,
    hb_unicode_get_general_category_nil,
    hb_unicode_get_mirroring_nil,
    hb_unicode_get_script_nil,
  }
};


hb_unicode_funcs_t *
hb_unicode_funcs_create (hb_unicode_funcs_t *parent)
{
  hb_unicode_funcs_t *ufuncs;

  if (!HB_OBJECT_DO_CREATE (hb_unicode_funcs_t, ufuncs))
    return &_hb_unicode_funcs_nil;

  if (parent != NULL)
  {
    hb_unicode_funcs_make_immutable (parent);
    ufuncs->parent = hb_unicode_funcs_reference (parent);

    ufuncs->get = parent->get;

    /* We can safely copy user_data from parent since we hold a reference
     * onto it and it's immutable.  We should not copy the destroy notifiers
     * though. */
    ufuncs->user_data = parent->user_data;
  }
  else
  {
    ufuncs->get = _hb_unicode_funcs_nil.get;
  }

  return ufuncs;
}

hb_unicode_funcs_t *
hb_unicode_funcs_reference (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_REFERENCE (ufuncs);
}

void
hb_unicode_funcs_destroy (hb_unicode_funcs_t *ufuncs)
{
  HB_OBJECT_DO_DESTROY (ufuncs);

#define DESTROY(name) if (ufuncs->destroy.name) ufuncs->destroy.name (ufuncs->user_data.name)
  DESTROY (combining_class);
  DESTROY (eastasian_width);
  DESTROY (general_category);
  DESTROY (mirroring);
  DESTROY (script);
#undef DESTROY

  if (ufuncs->parent != NULL)
    hb_unicode_funcs_destroy (ufuncs->parent);

  free (ufuncs);
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

hb_unicode_funcs_t *
hb_unicode_funcs_get_parent (hb_unicode_funcs_t *ufuncs)
{
  return ufuncs->parent;
}


#define IMPLEMENT(return_type, name)                                           \
                                                                               \
void                                                                           \
hb_unicode_funcs_set_##name##_func (hb_unicode_funcs_t             *ufuncs,    \
                                    hb_unicode_get_##name##_func_t  func,      \
                                    void                           *user_data, \
                                    hb_destroy_func_t               destroy)   \
{                                                                              \
  if (ufuncs->immutable)                                                       \
    return;                                                                    \
                                                                               \
  if (ufuncs->destroy.name)                                                    \
    ufuncs->destroy.name (ufuncs->user_data.name);                             \
                                                                               \
  if (func) {                                                                  \
    ufuncs->get.name = func;                                                   \
    ufuncs->user_data.name = user_data;                                        \
    ufuncs->destroy.name = destroy;                                            \
  } else if (ufuncs->parent != NULL) {                                         \
    ufuncs->get.name = ufuncs->parent->get.name;                               \
    ufuncs->user_data.name = ufuncs->parent->user_data.name;                   \
    ufuncs->destroy.name = NULL;                                               \
  } else {                                                                     \
    ufuncs->get.name = hb_unicode_get_##name##_nil;                            \
    ufuncs->user_data.name = NULL;                                             \
    ufuncs->destroy.name = NULL;                                               \
  }                                                                            \
}                                                                              \
                                                                               \
return_type                                                                    \
hb_unicode_get_##name (hb_unicode_funcs_t *ufuncs,                             \
		       hb_codepoint_t      unicode)                            \
{                                                                              \
  return ufuncs->get.name (ufuncs, unicode, ufuncs->user_data.name);           \
}

IMPLEMENT (unsigned int, combining_class)
IMPLEMENT (unsigned int, eastasian_width)
IMPLEMENT (hb_unicode_general_category_t, general_category)
IMPLEMENT (hb_codepoint_t, mirroring)
IMPLEMENT (hb_script_t, script)

#undef IMPLEMENT


HB_END_DECLS
